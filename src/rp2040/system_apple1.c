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

#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>

#include "bus.h"
#include "cpudetect_apple1.h"
#include "krusader.h"

// set this to 1 to run for 2 million cycles while keeping time
#define SPEED_TEST 0
#define WRITE_DELAY_US 16666

uint32_t write_delay_us = WRITE_DELAY_US;

queue_t keyboard_queue;
queue_t display_queue;

uint8_t memory[0x10000]; // 64k of RAM/ROM
uint32_t state;
uint32_t address;

volatile bool enable_print = false;

volatile uint32_t cycles_left_reset = 5;
volatile uint32_t cycles_left_nmi   = 0;
volatile uint32_t cycles_left_irq   = 0;


// small relocateable 6502 routine to print all characters to display
const uint8_t printloop[] =
{
   0xa2,0x00,0x8a,0x20,0xef,0xff,0xe8,0x18,
   0x90,0xf8
};


int apple2output( int c )
{
   c &= 0x7f;
   if( c == 0x0d )
   {
      // don't modify return below
   }
   else if( c <= 0x1f )
   {
      // bytes below space need to be adjusted
      c = 0;
   }
   else if( c >= 0x60 )
   {
      // bytes above underscore will be trimmed down
      c &= 0x5f;
   }
   return c;
}


int input2apple( int c )
{
   // Apple 1 only has upper charset
   if( (c >= 'a') && (c <= 'z') )
   {
      c &= 0xdf;
   }
   else if( (c == 0x08) || (c == 0x7f) )
   {
      return 0xdf;
   }
   return c | 0x80;
}


void run_console()
{
   int in = PICO_ERROR_TIMEOUT, out;
   int col = 0;
   uint64_t next_write = time_us_64();

   for(;;)
   {
      if( in == PICO_ERROR_TIMEOUT )
      {
         in = getchar_timeout_us(1000);
      }
      if( in != PICO_ERROR_TIMEOUT )
      {
         switch( in )
         {
            case '`':
               cycles_left_reset = 8;
               enable_print = false;
               in = PICO_ERROR_TIMEOUT;
               break;
            case '{':
               write_delay_us = 0;
               in = PICO_ERROR_TIMEOUT;
               break;
            case '}':
               write_delay_us = WRITE_DELAY_US;
               in = PICO_ERROR_TIMEOUT;
               break;
            default:
               if( queue_try_add( &keyboard_queue, &in ) )
               {
                  in = PICO_ERROR_TIMEOUT;
               }
               break;
         }
      }

      if( queue_try_remove( &display_queue, &out ) )
      {
         out = apple2output( out );
         if( out >= 0x20 )
         {
            col++;
         }
         if( out == 0x0d )
         {
            putchar( '\n' );
            col = 0;
         }
         if( col > 40 )
         {
            putchar( '\n' );
            col = 1;
         }

         if( out > 0 )
         {
            while( time_us_64() < next_write )
            {
               tight_loop_contents();
            }
            putchar( out );
            next_write = time_us_64() + write_delay_us;
         }
      }

      tight_loop_contents();
   }
}


static inline void handle_ram()
{
   // assume that data bus direction is already set
   if( state & bus_config.mask_rw )
   {
      // read from memory and write to bus
      gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
   }
   else
   {
      // read from bus and write to memory write
      memory[address] = (gpio_get_all() >> bus_config.shift_data); // truncate is intended
   }
}


static inline void handle_pia()
{
   int c;
   switch( address )
   {
      case 0xD010: // PIA.A: keyboard
         if( state & bus_config.mask_rw )
         {
            // CPU reads register
            if( queue_try_remove( &keyboard_queue, &c ) )
            {
               memory[address] = input2apple( c );
            }
            gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
         }
         else
         {
            // CPU writes register
            memory[address] = (gpio_get_all() >> bus_config.shift_data);
         }
         break;
      case 0xD011: // PIA.A: control register
         if( state & bus_config.mask_rw )
         {
            // CPU reads register
            memory[address] &= 0x7f;
            memory[address] |= queue_is_empty( &keyboard_queue ) ? 0 : 0x80;
            gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
         }
         else
         {
            // CPU writes register
            memory[address] = (gpio_get_all() >> bus_config.shift_data);
         }
         break;
      case 0xD012: // PIA.B: display
         if( state & bus_config.mask_rw )
         {
            // CPU reads register
            memory[address] &= 0x7f;
            memory[address] |= queue_is_empty( &display_queue ) ? 0 : 0x80;
            gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
         }
         else
         {
            // CPU writes register
            memory[address] = (gpio_get_all() >> bus_config.shift_data);
            c = memory[address];
            if( enable_print )
            {
               queue_try_add( &display_queue, &c );
            }
         }
         break;
      case 0xD013: // PIA.B: control register
         if( state & bus_config.mask_rw )
         {
            // CPU reads register
            gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
            memory[address] &= 0x7f;
         }
         else
         {
            // CPU writes register
            memory[address] = (gpio_get_all() >> bus_config.shift_data);
            enable_print = true;
         }
         break;
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
   for(cyc = 0; cyc < 2000000; ++cyc)
#else
   for(;;)
#endif
   {
      if( cycles_left_reset )
      {
         --cycles_left_reset;
         gpio_clr_mask( bus_config.mask_reset );
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
      if( (address & 0xFFF0) == 0xD010 )
      {
         // handle_io
         handle_pia();
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
   printf( "%.08f sec: %.0fHz\n", time_exec, time_mhz );
   printf( "bus has terminated\n" );
#endif
}


int main()
{
   stdio_init_all();
   uart_set_translate_crlf( uart0, true );

   queue_init( &keyboard_queue, sizeof(int), 256 );
   queue_init( &display_queue, sizeof(int), 1 );

   memset( &memory[0x0000], 0x00, sizeof(memory) );
   memcpy( &memory[0x02F0], &printloop[0], sizeof(printloop) );
   memcpy( &memory[0x0280], &cpudetect_0280[0], sizeof(cpudetect_0280) );
   memcpy( &memory[0xE000], &krusader_e000[0], sizeof(krusader_e000) );

   bus_init();

   multicore_launch_core1( run_bus );
   run_console();

   return 0;
}
