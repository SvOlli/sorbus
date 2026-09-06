/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements JAM (Just Another Machine), a custom Sorbus core
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
#include <hardware/clocks.h>

#include "jam.h"
#include "event_queue.h"
#include "cpu_detect.h"

#define SWITCH_CORES 0

#ifndef SORBUS_VERSION
#define SORBUS_VERSION "0.8"
#endif


#ifdef __OPTIMIZE__
#ifdef __OPTIMIZE_SIZE__
#define SORBUS_OPTIMIZED " optimized for size"
#else
#define SORBUS_OPTIMIZED " optimized for speed"
#endif
#else
#define SORBUS_OPTIMIZED " not optimized"
#endif

#define SORBUS_LONG_VERSION \
"JAM Version " SORBUS_VERSION ", compiled with: gcc " __VERSION__ SORBUS_OPTIMIZED

const char *sorbus_version = SORBUS_LONG_VERSION;

bi_decl(bi_program_name("Sorbus Computer Native Core"))
bi_decl(bi_program_description(SORBUS_LONG_VERSION))
bi_decl(bi_program_url("https://sorbus.xayax.net/"))

#include "bus.h"


void system_cpu_detect()
{
   cpu_detect( false );
   while( cputype == CPU_ERROR )
   {
      int in = PICO_ERROR_TIMEOUT;
      printf( "  cpu could not be detected, retrying (SPACE for debug)\r" );
      in = getchar_timeout_us( 1000 );
      if( in == ' ' )
      {
         cpu_detect( true );
         printf( "power jumper set?\n" );
      }
   }
}


int main()
{
   // setup UART
   stdio_init_all();

#if 1
   // give some time to connect to console
   sleep_ms( 2000 );
   puts( sorbus_version );
#endif

   // for toying with overclocking
   set_sys_clock_khz( 133000, false );

   bus_init();
   system_cpu_detect();
   system_init();

#if 0
   // setup cores
   setup_bus();
   setup_io();
#endif

#if SWITCH_CORES
   // run console and handlers in core1
   multicore_launch_core1( io_run );

   // run the bus in core0
   bus_run();
#else
   // run the bus in core1
   multicore_launch_core1( bus_run );

   // run console and handlers in core0
   io_run();
#endif

   // keep the compiler happy, since we should never get here
   return 0;
}
