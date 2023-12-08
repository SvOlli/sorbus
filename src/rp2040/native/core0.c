/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a Native custom platform
 * for the Sorbus Computer
 */

#include <stdio.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include "common.h"
#include "menu.h"


static console_type_t console_type;


void console_type_set( console_type_t type )
{
   console_type = type;
}


void console_rp2040()
{
   print_welcome();
   menu_run();
   // system_reboot(); should not be called from Core 0 
   console_type_set(CONSOLE_TYPE_65C02);
}


void console_65c02()
{
   int in = PICO_ERROR_TIMEOUT, out;

   if( in == PICO_ERROR_TIMEOUT )
   {
      in = getchar_timeout_us(10);
      if( in == '~' )
      {
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
      //printf("%02x ",out );
      putchar( out );
   }
}


void console_run()
{
   multicore_lockout_victim_init();

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
