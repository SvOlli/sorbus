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

#include "../bus.h"

#include "event_queue.h"
extern void system_trap();

console_type_t console_type;
bool console_crlf_enabled;

bool console_wants_stop = false;
bool bus_wants_stop     = false;
int  invoke_type = 0;


void console_reset()
{
   gpio_clr_mask( bus_config.mask_reset);
}


void console_cpu_pause( bool stop )
{
   if( stop )
   {
      gpio_clr_mask( bus_config.mask_rdy );
   }
   else
   {
      gpio_set_mask( bus_config.mask_rdy );
      console_wants_stop = false;
      bus_wants_stop     = false;
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
   int in;
   bool leave = false;

   const char *invoke = "magic key combo";

   console_cpu_pause( true );

   switch( invoke_type )
   {
      case SYSTEM_TRAP:
         invoke = "trap";
         break;
      case SYSTEM_WATCHDOG:
         invoke = "watchdog";
         break;
      default:
         break;
   }
   printf( "\n%s triggered! CPU stopped using RDY\n"
           , invoke );

   while( !leave )
   {
      printf( "\nB)acktrace, E)vent queue, H)eap, I)nternal drive, S)peeds"
              "\nC)ontinue, R)eboot ? " );
      in = toupper( getchar() );
      switch( in )
      {
         case 'B':
            debug_backtrace();
            break;
         case 'E':
            debug_queue_event( "Event queue" );
            break;
         case 'H':
            debug_heap();
            break;
         case 'I':
            debug_internal_drive();
            break;
         case 'S':
            debug_clocks();
            break;
         case 'C':
            leave = true;
            break;
         case 'R':
            leave = true;
            system_reboot();
            break;
      }
   }

   console_cpu_pause( false );

   console_type = CONSOLE_TYPE_65C02;

   // restore CRLF setting for 65C02
   uart_set_translate_crlf( uart0, console_crlf_enabled );
}


void console_65c02()
{
   int in = PICO_ERROR_TIMEOUT, out;

   if( in == PICO_ERROR_TIMEOUT )
   {
      in = getchar_timeout_us(10);
      if( (in == 0x1d) && (console_crlf_enabled) ) /* 0x1d = CTRL+] */
      {
         invoke_type = in;
         console_wants_stop = true;
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
      if( (out == SYSTEM_TRAP) || (out == SYSTEM_WATCHDOG) )
      {
         invoke_type = out;
         bus_wants_stop = true;
         console_type = CONSOLE_TYPE_RP2040;
      }
      else if( out == 0x7f )
      {
         const char bs_seq[] = { 8, ' ', 8, 0 };
         printf( bs_seq );
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
