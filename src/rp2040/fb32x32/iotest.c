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

#include <fb32x32.h>
#include <fb32x32_bus.pio.h>

#define FB32X32_BUS_PIO pio1
#define FB32X32_BUS_SM  0


void bus_init();


void waitkey()
{
   while( getchar_timeout_us(333333) == PICO_ERROR_TIMEOUT )
   {
      printf( "\rpress any key " );
   }
   putchar( 10 );
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


int main()
{
   int i;

   uint32_t bus;
   uint16_t address;

   // setup UART
   stdio_init_all();

   // for toying with overclocking
#if 0
   set_sys_clock_khz( 240000, false );
#else
   set_sys_clock_khz( 133000, false );
#endif

   bus_init();
   // setup return values
#if 0
   for( i = 0; i < 0x100; ++i )
   {
      sound_bus_read_values[i] = i ^ 0xff;
   }
#endif

   multicore_launch_core1( logger );

   waitkey();

   for(;;)
   {
      bus = pio_sm_get_blocking( FB32X32_BUS_PIO, FB32X32_BUS_SM );
      printf( "%07x\n", bus );
   }

   // keep the compiler happy
   return 0;
}
