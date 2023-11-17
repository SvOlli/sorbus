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

#include "native/common.h"
#include "bus.h"

bi_decl(bi_program_name("Sorbus Computer Native Core"))
bi_decl(bi_program_description("implement an own home computer flavor"))
bi_decl(bi_program_url("https://xayax.net/sorbus/"))


#include "cpm_rom.h"
#include "bdos_rom.h"
#define CPM_START 0x0801
#define BDOS_START 0x0f00
extern uint8_t memory[0x10000];
queue_t queue_uart_read;
queue_t queue_uart_write;


int main()
{
   // setup UART
   stdio_init_all();
   uart_set_translate_crlf( uart0, true );

   // give some time to connect to console
   sleep_ms(2000);

   // for toying with overclocking
   set_sys_clock_khz ( 133000, false );

   // setup between UART core and bus core
   queue_init( &queue_uart_read,  sizeof(int), 128 );
   queue_init( &queue_uart_write, sizeof(int), 128 );

   // setup the bus and run the bus core
   bus_init();
   system_init();
   // copy cpm-startup
   memcpy( &memory[CPM_START], &cpm_rom[2], sizeof(cpm_rom)-2 );  // has startaddress in front
   memcpy( &memory[BDOS_START], &bdos_rom[0], sizeof(bdos_rom) );

   system_reboot();
   multicore_launch_core1( bus_run );

   // run interactive console -> should never return
   console_run();

   return 0;
}
