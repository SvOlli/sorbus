/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements Apple Computer I emulation
 * for the Sorbus Computer
 */

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "rp2040_purple.h"
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/binary_info.h>

bi_decl(bi_program_name("Sorbus Computer Native Core"))
bi_decl(bi_program_description("implement an own home computer flavor"))
bi_decl(bi_program_url("https://xayax.net/sorbus/"))

#include "bus.h"
#include "gpio_oc.h"

#include "native_rom.h"

// set this to 1 to run for 5 million cycles while keeping time
#define SPEED_TEST 0
// the delay between characters printed, the Apple 1 can only add
// a single characters per frame displayed at 60Hz
#define WRITE_DELAY_US 16666
// this is where the write protected area starts
#define ROM_START (0xE000)

uint32_t write_delay_us = WRITE_DELAY_US;

queue_t queue_uart_read;
queue_t queue_uart_write;

uint8_t memory[0x10000]; // 64k of RAM/ROM and I/O
uint32_t state;
uint32_t address;

volatile bool enable_print = false;

volatile uint32_t cycles_left_reset = 5;
volatile uint32_t cycles_left_nmi   = 0;
volatile uint32_t cycles_left_irq   = 0;


void run_console()
{
   int in = PICO_ERROR_TIMEOUT, out;

   for(;;)
   {
      if( in == PICO_ERROR_TIMEOUT )
      {
         in = getchar_timeout_us(1000);
      }
      if( in != PICO_ERROR_TIMEOUT )
      {
      }

      if( queue_try_remove( &queue_uart_write, &out ) )
      {
         putchar( out );
      }

      tight_loop_contents();
   }
}


static inline void write_data_bus( uint8_t data )
{
   gpio_oc_set_by_mask( bus_config.mask_data, ((uint32_t)data) << bus_config.shift_data );
}


static inline uint8_t read_data_bus()
{
   return (gpio_get_all() >> bus_config.shift_data);
}


static inline void handle_ram()
{
   // assume that data bus direction is already set
   if( state & bus_config.mask_rw )
   {
      // read from memory and write to bus
      write_data_bus( memory[address] );
   }
   else
   {
      // read from bus and write to memory write
      // WozMon (and Krusader) is in ROM, so only accept writes below romstart
      if( address < ROM_START )
      {
         memory[address] = (gpio_get_all() >> bus_config.shift_data); // truncate is intended
      }
   }
}


static inline void handle_io()
{
   uint8_t data;
   bool success;
   switch( address & 0xFF )
   {
      case 0x00: /* console UART read */
         success = queue_try_remove( &queue_uart_read, &data );
         write_data_bus( success ? data : 0x00 );
         break;
      case 0x01: /* console UART read queue */
         write_data_bus( queue_get_level( &queue_uart_read )  );
         break;
      case 0x02: /* console UART write */
         data = state >> bus_config.shift_data;
         queue_try_add( &queue_uart_write, &data );
         break;
      case 0x03: /* console UART write queue */
         write_data_bus( queue_get_level( &queue_uart_write )  );
         break;
#if 0
      /* maybe use $80-$F9 as mirror from ROM for bankswitching routines? */
      case 0xFE: /* set bankswitch register for $E000-$EFFF */
         rom_bank_e = read_data_bus();
         break;
      case 0xFF: /* set bankswitch register for $F000-$FFFF */
         rom_bank_f = read_data_bus();
         break;
#endif
      default:
         break;
   }
}


void run_bus()
{
#if SPEED_TEST
   uint64_t time_start, time_end;
   double time_exec;
   double time_mhz;
   uint32_t cyc;

   time_start = time_us_64();
   for(cyc = 0; cyc < 5000000; ++cyc)
#else
   // when not running as speed test run main loop forever
   for(;;)
#endif
   {
      if( cycles_left_reset )
      {
         int dummy;
         --cycles_left_reset;
         gpio_clr_mask( bus_config.mask_reset );
         while( queue_try_remove( &queue_uart_read, &dummy ) )
         {
            // just loop until queue is empty
         }
         while( queue_try_remove( &queue_uart_write, &dummy ) )
         {
            // just loop until queue is empty
         }
      }
      else
      {
         gpio_set_mask( bus_config.mask_reset );
      }

      if( cycles_left_nmi )
      {
         --cycles_left_nmi;
         gpio_clr_mask( bus_config.mask_nmi );
      }
      else
      {
         gpio_set_mask( bus_config.mask_nmi );
      }

      if( cycles_left_irq )
      {
         --cycles_left_irq;
         gpio_clr_mask( bus_config.mask_irq );
      }
      else
      {
         gpio_set_mask( bus_config.mask_irq );
      }

      // done: set clock to high
      gpio_set_mask( bus_config.mask_clock );

      // bus should be still valid from clock low
      state = gpio_get_all();

      // setup bus direction so I/O can settle
      if( state & bus_config.mask_rw )
      {
         // read from memory and write to bus
         gpio_set_dir_out_masked( bus_config.mask_data );
      }
      else
      {
         // read from bus and write to memory write
         gpio_set_dir_in_masked( bus_config.mask_data );
      }

      address = ((state & bus_config.mask_address) >> bus_config.shift_address);

      // setup data
      if( (address & 0xFF00) == 0xDF00 )
      {
         /* internal i/o */
         handle_io();
      }
      if( (address == 0x0000) || ((address & 0xF000) == 0xD000) )
      {
         /* external i/o: do nothing */
      }
      else
      {
         handle_ram();
      }

      // done: set clock to low
      gpio_clr_mask( bus_config.mask_clock );
   }

#if SPEED_TEST
   time_end = time_us_64();
   time_exec = (double)(time_end - time_start) / CLOCKS_PER_SEC / 10000;
   time_mhz = (double)cyc / time_exec;
   for(;;)
   {
      printf( "\rbus has terminated after %d cycles in %.06f seconds: %.0fHz ", cyc, time_exec, time_mhz );
      sleep_ms(2000);
   }
#endif
}


int main()
{
   // setup UART
   stdio_init_all();
   uart_set_translate_crlf( uart0, true );

   // setup between UART core and bus core
   queue_init( &queue_uart_read,  sizeof(int), 128 );
   queue_init( &queue_uart_write, sizeof(int), 128 );

   // clean out memory
   // yes, this is not original, but more userfriendly and also easier
   // to implement a real DRAM initstate profile
   // also we've got 64k addressable, which is not original as well
   memset( &memory[0x0000], 0x00, sizeof(memory) );

   // and we need also setup the ROM
   // it's implemented as part of RAM with write disabled
   memcpy( &memory[ROM_START], &native_rom[0], sizeof(native_rom) );

   // setup the bus and run the bus core
   bus_init();
   multicore_launch_core1( run_bus );

   // run interactive console -> should never return
   run_console();

   return 0;
}
