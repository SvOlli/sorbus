/**
 * Copyright (c) 2025 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <hardware/clocks.h>

#include <sound_bus.h>


void read_handler( uint8_t addr )
{
   multicore_fifo_push_blocking( 0xc0000000 | (addr << 8) );
}


void write_handler( uint8_t addr, uint8_t data )
{
   multicore_fifo_push_blocking( 0x80000000 | (addr << 8) | data );
}


void logger()
{
   uint32_t bus;
   for(;;)
   {
      bus     = multicore_fifo_pop_blocking();
#if 0
      switch( bus >> 30 )
      {
         case 0x2:
            printf( "write: %08x\n", bus );
            break;
         case 0x3:
            printf( "read:  %08x\n", bus );
            break;
         default:
            printf( "fail:  %08x\n", bus );
            break;
      }
#endif
   }
}


void waitkey()
{
   while( getchar_timeout_us(333333) == PICO_ERROR_TIMEOUT )
   {
      printf( "\rpress any key " );
   }
   putchar( 10 );
}


int main()
{
   int i;

   // setup UART
   stdio_init_all();

   // for toying with overclocking
#if 0
   set_sys_clock_khz( 240000, false );
#else
   set_sys_clock_khz( 133000, false );
#endif

   sound_bus_init();
   sound_bus_read_handler_set( read_handler );
   sound_bus_write_handler_set( write_handler );
   // setup return values
   for( i = 0; i < 0x100; ++i )
   {
      sound_bus_read_values[i] = i ^ 0xff;
   }

   multicore_launch_core1( logger );

   waitkey();

   sound_bus_loop();

   // keep the compiler happy
   return 0;
}
