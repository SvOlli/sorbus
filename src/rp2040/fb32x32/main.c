/**
 * Copyright (c) 2025 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a 32x32 pixel framebuffer
 * for the Sorbus Computer
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <hardware/clocks.h>

#include "fb32x32.h"


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
   // setup UART
   stdio_init_all();

   // for toying with overclocking
#if 0
   set_sys_clock_khz( 240000, false );
#else
   set_sys_clock_khz( 133000, false );
#endif

   //waitkey();

   // order is important
   bus_init();
   hardware_init();
   control_init();

   // core1 handles control registers and updates the LEDs
   multicore_launch_core1( control_loop );

   // core0 waits for data, should never return
   bus_loop();

   // keep the compiler happy
   return 0;
}
