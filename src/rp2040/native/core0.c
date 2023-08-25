/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a Native custom platform
 * for the Sorbus Computer
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include "common.h"


static console_type_t console_type;


void console_type_set( console_type_t type )
{
   console_type = type;
}


void console_rp2040()
{
   printf( "watchdog triggered! CPU stopped using RDY\n" );

   printf( "\nrebooting system!\n" );
   system_reboot();
}


void console_65c02()
{
   int in = PICO_ERROR_TIMEOUT, out;

   if( in == PICO_ERROR_TIMEOUT )
   {
      in = getchar_timeout_us(10);
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
