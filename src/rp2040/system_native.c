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
// this is where the write protected area starts
#define ROM_START (0xE000)

queue_t queue_uart_read;
queue_t queue_uart_write;

uint8_t memory[0x10000]; // 64k of RAM/ROM and I/O
uint32_t state;
uint32_t address;

volatile bool enable_print = false;

volatile uint32_t cycles_left_reset       = 8;
volatile uint32_t nmi_timer_cycles_left   = 0;
volatile uint32_t irq_timer_cycles_left   = 0;

// no need to volatile these, as they are only used within run_bus loop
uint32_t nmi_timer_restart_value = 0;
uint32_t irq_timer_restart_value = 0;


void set_bank( uint8_t bank )
{
   // as of now only one bank exists
   memcpy( &memory[ROM_START], &native_rom[0], sizeof(native_rom) );
}


void system_reboot()
{
   cycles_left_reset       = 8;
}

void system_reset()
{
   int dummy;
   nmi_timer_cycles_left   = 0;
   nmi_timer_restart_value = 0;
   irq_timer_cycles_left   = 0;
   irq_timer_restart_value = 0;
   set_bank( 0 );
   while( queue_try_remove( &queue_uart_read, &dummy ) )
   {
      // just loop until queue is empty
   }
   while( queue_try_remove( &queue_uart_write, &dummy ) )
   {
      // just loop until queue is empty
   }
}


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
         if( queue_try_add( &queue_uart_read, &in ) )
         {
            // need to handle overflow?
         }
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


static inline void setup_timer( uint8_t value, uint8_t config )
{
   // config bits (decoded from address)
   //           0        1
   // bit 0: lowbyte  highbyte
   // bit 1: repeat   oneshot
   // bit 2: irq      nmi

   // check timer to use
   if( config & (1<<2) )
   {
      // nmi
      if( config & (1<<0) )
      {
         // highbyte: stop timer and store
         nmi_timer_cycles_left   = 0;
         nmi_timer_restart_value = value << 8;
      }
      else
      {
         // lowbyte
         nmi_timer_restart_value |= value;
         nmi_timer_cycles_left   = nmi_timer_restart_value + 8; // duration of interrupt signal
         if( config & (1<<1) )
         {
            // oneshot
            nmi_timer_restart_value = 0;
         }
      }
   }
   else
   {
      // irq
      if( config & (1<<0) )
      {
         // highbyte: stop timer and store
         irq_timer_cycles_left   = 0;
         irq_timer_restart_value = value << 8;
      }
      else
      {
         // lowbyte
         irq_timer_restart_value |= value;
         irq_timer_cycles_left   = irq_timer_restart_value + 8; // duration of interrupt signal
         if( config & (1<<1) )
         {
            // oneshot
            irq_timer_restart_value = 0;
         }
      }
   }
}


static inline void handle_io()
{
   // TODO: split this up in reads and writes
   uint8_t data = state >> bus_config.shift_data;
   bool success;
   switch( address & 0xFF )
   {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
         setup_timer( data, address & 0x07 );
      case 0xFA: /* console UART read */
         success = queue_try_remove( &queue_uart_read, &data );
         write_data_bus( success ? data : 0x00 );
         break;
      case 0xFB: /* console UART read queue */
         write_data_bus( queue_get_level( &queue_uart_read )  );
         break;
      case 0xFC: /* console UART write */
         queue_try_add( &queue_uart_write, &data );
         break;
      case 0xFD: /* console UART write queue */
         write_data_bus( queue_get_level( &queue_uart_write )  );
         break;
#if 0
      case 0xFF: /* set bankswitch register for $F000-$FFFF */
         rom_bank_f = read_data_bus();
         break;
#endif
      default:
         /* everything else is handled like RAM by design */
         handle_ram();
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
      if( gpio_get_all() & bus_config.mask_reset )
      {
         system_reset();
      }

      if( cycles_left_reset )
      {
         --cycles_left_reset;
         gpio_clr_mask( bus_config.mask_reset );
      }
      else
      {
         gpio_set_mask( bus_config.mask_reset );
      }

      if( nmi_timer_cycles_left )
      {
         if( --nmi_timer_cycles_left < 8 )
         {
            gpio_clr_mask( bus_config.mask_nmi );
         }
         if( !nmi_timer_cycles_left )
         {
            nmi_timer_cycles_left = nmi_timer_restart_value;
         }
      }
      else
      {
         gpio_set_mask( bus_config.mask_nmi );
      }

      if( irq_timer_cycles_left )
      {
         if( --irq_timer_cycles_left < 8 )
         {
            gpio_clr_mask( bus_config.mask_irq );
         }
         if( !irq_timer_cycles_left )
         {
            irq_timer_cycles_left = irq_timer_restart_value;
         }
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
         // note to future self: check if there's a better way for
         // external i/o then to set the GPIOs to write for a brief time
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
      if( (address <= 0x0003) || ((address & 0xF000) == 0xD000) )
      {
         /* external i/o: keep hands off the bus */
         gpio_set_dir_in_masked( bus_config.mask_data );
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
   memset( &memory[0x0000], 0x00, sizeof(memory) );

   // and we need also setup the system
   system_reboot();

   // setup the bus and run the bus core
   bus_init();
   multicore_launch_core1( run_bus );

   // run interactive console -> should never return
   run_console();

   return 0;
}
