/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a Native custom platform
 * for the Sorbus Computer
 */

#include <stdbool.h>
#include <stdio.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include "common.h"
#include "menu.h"

#include "../bus.h"

#include "event_queue.h"
extern void system_trap();

console_type_t console_type;
bool console_crlf_enabled;

bool console_wants_stop = false;
bool bus_wants_stop     = false;


void console_reset()
{
   gpio_clr_mask( bus_config.mask_reset);
}


void console_cpu_pause( bool stop )
{
printf( "console_cpu_pause:%d\n", stop ? 1 : 0 );
   if( stop )
   {
      gpio_clr_mask( bus_config.mask_rdy );
   }
   else
   {
      gpio_set_mask( bus_config.mask_rdy );
   }
}


void console_set_crlf( bool enable )
{
   uart_set_translate_crlf( uart0, enable );
   console_crlf_enabled = enable;
}


void console_type_set( console_type_t type )
{
   console_type = type;
}


void console_rp2040()
{
   printf( "\nwatchdog triggered! CPU stopped using RDY\n" );

   printf( "\nrebooting system!\n" );
   menu_run();
   console_cpu_pause( false );
   system_reboot();
   console_type = CONSOLE_TYPE_65C02;
}


void console_65c02()
{
   int in = PICO_ERROR_TIMEOUT, out;

   if( in == PICO_ERROR_TIMEOUT )
   {
      in = getchar_timeout_us(10);
      if( in == 0x1d ) /* 0x1d = CTRL+] */
      {
         console_cpu_pause( true );
         console_type = CONSOLE_TYPE_RP2040;
         in = PICO_ERROR_TIMEOUT;
      }
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
      if( out == SYSTEM_TRAP )
      {
         console_cpu_pause( true );
         console_type = CONSOLE_TYPE_RP2040;
      }
      else
      {
         //printf("%02x ",out );
         putchar( out );
      }
   }
}


void console_run()
{
   multicore_lockout_victim_init();
   console_type_set( CONSOLE_TYPE_65C02 );

   for(;;)
   {
      switch( console_type )
      {
         case CONSOLE_TYPE_65C02:
            console_65c02();
            break;
         case CONSOLE_TYPE_RP2040:
            console_rp2040();
            break;
      }

      tight_loop_contents();
   }
}
