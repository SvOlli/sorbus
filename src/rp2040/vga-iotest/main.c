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

#include "vga_bus.pio.h"


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

   waitkey();

   // keep the compiler happy
   return 0;
}
