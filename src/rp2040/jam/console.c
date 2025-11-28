/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include "jam.h"

#include "bus.h"
#include "generic_helper.h"

#include "event_queue.h"
#include "mcurses.h"

extern void system_trap();

console_type_t console_type;
uint8_t console_charset = 0;
bool console_crlf_enabled;
bool console_flowcontrol_enabled;

bool console_wants_stop = false;
bool bus_wants_stop     = false;
int  invoke_type = 0;


static uint8_t hexedit_bank();

static mc_hexedit_t he_config = {
   hexedit_bank,
   debug_peek,
   debug_poke,
   1,
   0x00,
   0x0400,
   0x0400
};

static mc_disass_t da_config = {
   debug_banks,
   debug_peek,
   1,
   CPU_65SC02,
   0x00,
   0x0400,
   false,
   false
};


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


void console_set_uart( uint8_t data )
{
   console_crlf_enabled = data & 1;
   console_flowcontrol_enabled = data & 2;
   console_charset = (data >> 2) & 0x03;
   uart_set_translate_crlf( uart0, console_crlf_enabled );
}


void console_type_set( console_type_t type )
{
   console_type = type;
}


uint8_t hexedit_bank()
{
   if( ++(he_config.bank) > debug_banks() )
   {
      he_config.bank = 0;
   }
   return he_config.bank;
}


void console_rp2040()
{
   int in;
   bool leave = false;
   uint16_t lines, cols;

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

   screen_get_size( &lines, &cols );
   screen_save();
   initscr();
   while( (lines < 20) || (cols < 77) )
   {
      char buffer[256] = { 0 };
      uint8_t ch;
      move( 0, 0 );
      addstr( "Meta-Mode requires at least 77x20." );
      move( 1, 0 );
      snprintf( &buffer[0], sizeof(buffer)-1,
                "Yours is reporting as %dx%d."
                , cols
                , lines
                );
      addstr( &buffer[0] );
      addstr( "Press 'Q' to quit or other key to retry." );
      ch = getch();
      if( (ch | 0x20) == 'q' ) // make lowercase
      {
         goto done;
      }
      screen_get_size( &lines, &cols );
   }

   while( !leave )
   {
      clear();
      mcurses_border( true, 0, 0, lines-1, cols-1 );

      mcurses_textbox( false, lines-6, 1, debug_get_info( DEBUG_INFO_CLOCKS ) );
      mcurses_textbox( false, lines-6, ((screen_get_columns()+1)>>1) - 9, debug_get_info( DEBUG_INFO_SYSVECTORS ) );
      mcurses_textbox( false, lines-6, cols-21, debug_get_info( DEBUG_INFO_HEAP ) );

      mcurses_sorbus_logo( 1, 2 );
      move( 5, 5 );
      addstr( "https://sorbus.xayax.net/");
      mcurses_line_horizontal( true, 6, 0, cols-1 );
      move( 7, 2 );
      addstr( "Meta Menu invoked via " );
      addstr( invoke );
      addstr( ", CPU stopped on RDY" );

      move( 9, 2 );
      addstr( "B)acktrace, M)emory, D)isassemble, U)pload, E)vent queue, I)nternal drive" );
      move( 10, 2 );
      addstr( "C)ontinue, R)eboot ? " );

      in = toupper( getchar() );
      switch( in )
      {
         case '!':
            mcurses_titlebox( true, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Backtrace Dumper",
                              "Turn on logging now\n"
                              "Press any key\n"
                              "Save capture for later use\n"
                              "Turn off logging\n"
                              "Press any key to return to menu"
                              );
            move( lines - 1, 0 );
            if( getch() != 0x03 )
            {
               debug_raw_backtrace();
               getch();
            }
            break;
         case 'B':
            {
               cputype_t cpu;
               uint32_t *trace, entries, start;
               debug_get_backtrace( &cpu, &trace, &entries, &start );
               mcurses_historian( cpu, trace, entries, start );
            }
            break;
         case 'D':
            {
               da_config.cpu = debug_get_cpu();
               mcurses_disassemble( &da_config );
            }
            break;
         case 'E':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Event Queue", debug_get_info( DEBUG_INFO_EVENTQUEUE ) );
            getch();
            break;
         case 'I':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Internal Drive", debug_get_info( DEBUG_INFO_INTERNALDRIVE ) );
            getch();
            break;
         case 'M':
            hexedit( &he_config );
            break;
         case 'U':
            endwin();
            leave = mc_xmodem_upload( debug_poke );
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

done:
   console_cpu_pause( false );

   console_type = CONSOLE_TYPE_65C02;

   // restore CRLF setting for 65C02
   uart_set_translate_crlf( uart0, console_crlf_enabled );

   screen_restore();
}


void console_65c02()
{
   int in = PICO_ERROR_TIMEOUT, out;

   if( in == PICO_ERROR_TIMEOUT )
   {
      in = getchar_timeout_us(10);
      if( ((in == 0x1d) || (in == '^')) && (console_crlf_enabled) ) /* 0x1d = CTRL+] */
      {
         invoke_type = in;
         console_wants_stop = true;
         console_type = CONSOLE_TYPE_RP2040;
         in = PICO_ERROR_TIMEOUT;
      }
   }
   if( in != PICO_ERROR_TIMEOUT )
   {
      if( console_flowcontrol_enabled )
      {
         // wait until everything queue is empty
         while( queue_get_level( &queue_uart_read ) )
         {
         }
         queue_add_blocking( &queue_uart_read, &in );
      }
      else
      {
         // no flow control: hope for the best
         (void)queue_try_add( &queue_uart_read, &in );
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
         /* full delete sequence: backspace, space, backspace */
         putchar( 8 );
         putchar( ' ' );
         putchar( 8 );
      }
      else
      {
         putcharset( out, console_charset );
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
