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
#include <hardware/clocks.h>

#include "native/common.h"
#include "native/event_queue.h"

#define SWITCH_CORES 0


bi_decl(bi_program_name("Sorbus Computer Native Core"))
bi_decl(bi_program_description("implement an own home computer flavor"))
bi_decl(bi_program_url("https://xayax.net/sorbus/"))

#include "bus.h"

queue_t queue_uart_read;
queue_t queue_uart_write;


int main()
{
   // setup UART
   stdio_init_all();
   console_set_crlf( true );

#if 0
   // give some time to connect to console
   sleep_ms( 2000 );
#endif

   // for toying with overclocking
   set_sys_clock_khz( 133000, false );

   // setup between UART core and bus core
   queue_init( &queue_uart_read,  sizeof(int), 240 );
   queue_init( &queue_uart_write, sizeof(int), 128 );

   // setup mutex for event queue
   queue_event_init();

#if SWITCH_CORES
   // run interactive console in core1
   multicore_launch_core1( console_run );

   // setup the bus and run the bus in core0
   bus_run();
#else
   // setup the bus and run the bus core
   multicore_launch_core1( bus_run );

   // run interactive console -> should never return
   console_run();
#endif

   // keep the compiler happy
   return 0;
}
