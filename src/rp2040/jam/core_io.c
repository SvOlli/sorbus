/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"
#include "bus.h"
#include "cpu_detect.h"
#include "event_queue.h"
#include "generic_helper.h"
#include "mcurses.h"

#include "da_memory.h"

#include <pico/stdio.h>
#include <pico/multicore.h>
//#include <pico/stdlib.h>
//#include <pico/util/queue.h>
//#include <hardware/clocks.h>

#ifndef SORBUS_VERSION
#define SORBUS_VERSION "0.8"
#endif


typedef void (*event_func_t)( uint32_t data );
typedef void (*io_post_func_t)( bool rw, uint8_t data, uint16_t address );

/* will be used in event functions array */
void handle_io( uint32_t in );

uint8_t console_charset = 0;
bool console_crlf_enabled;
bool console_flowcontrol_enabled;


const event_func_t events[0x10] = {
   0,                   // 0x00xxxxxx
   event_reset,         // 0x01xxxxxx
   handle_io,           // 0x02xxxxxx
   handle_io,           // 0x03xxxxxx
   event_meta,          // 0x04xxxxxx
   event_cpufreq,       // 0x05xxxxxx
   event_timer_cycle,   // 0x06xxxxxx
   event_flash_sync,    // 0x07xxxxxx
   0,                   // 0x08xxxxxx
   0,                   // 0x09xxxxxx
   0,                   // 0x0axxxxxx
   0,                   // 0x0bxxxxxx
   0,                   // 0x0cxxxxxx
   0,                   // 0x0dxxxxxx
   0,                   // 0x0exxxxxx
   0                    // 0x0fxxxxxx
};


// callbacks for 0x02xxxxxx & 0x03xxxxxx
// sliced up to handle areas of 8 bytes
const io_post_func_t io_handlers[0x10] = {
   io_post_misc,        // $df00-$df07: misc like bank, trap, rnd, etc.
   io_post_uart,        // $df08-$df0f: UART
   io_post_timer,       // $df10-$df17: timer cycle
   io_post_timer,       // $df18-$df1f: timer ms/10
   io_post_watchdog,    // $df20-$df27: watchdog / cyclecount
   0,                   // $df28-$df2f: unused RAM / kernel variables
   0,                   // $df30-$df37: system monitor variables
   0,                   // $df38-$df3f: unused RAM
   0,                   // $df40-$df47: unused RAM
   0,                   // $df48-$df4f: unused RAM
   0,                   // $df50-$df57: unused RAM
   0,                   // $df58-$df5f: unused RAM
   0,                   // $df60-$df67: unused RAM
   0,                   // $df68-$df6f: unused RAM
   io_post_intdrive,    // $df70-$df77: internal drive data/strobe registers
   0                    // $df78-$df7f: RAM: interrupt vectors
};


uint8_t debug_peek( uint8_t bank, uint16_t addr )
{
   if( addr < 0xE000 )
   {
      return ram[addr];
   }
   if( (bank > 0) && (bank <= NUMBER_OF_BANKS) )
   {
      /* assembling address in ROM array is a bit more complicated */
      return rom[((bank-1) << 13) | (addr & 0x1FFF)];
   }
   else
   {
      /* bank0: RAM under ROM */
      return ram[addr];
   }
}


void debug_poke( uint8_t bank, uint16_t addr, uint8_t value )
{
   if( (addr < 0xE000) || !bank )
   {
      ram[addr] = value;
   }
}


const char *debug_get_info( debug_info_t page )
{
   static char buffer[1280] = { 0 };

   buffer[0] = '\0';
   switch( page )
   {
      case DEBUG_INFO_INTERNALERROR:
         internal_error_info( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_HEAP:
         ht_info( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_CLOCKS:
         info_clocks( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_SYSVECTORS:
         info_sysvectors( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_INTERNALDRIVE:
         info_internaldrive( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_TIMERS:
         info_timers( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_EVENTQUEUE:
         queue_event_info( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_FIFO_INPUT:
         fifo256_debug( &buffer[0], sizeof(buffer)-1, &uart_in_queue, true );
         break;
      case DEBUG_INFO_FIFO_OUTPUT:
         fifo256_debug( &buffer[0], sizeof(buffer)-1, &uart_out_queue, true );
         break;
      default:
         break;
   }
   return &buffer[0];
}


void handle_io( uint32_t in )
{
   //if( (msg.address & 0xFF80) == 0xDF00 )
   //0xFF80 >> 7 = 0x01FF, 0xDF80 >> 7 = 0x01BF
   if( ((in >> 7) & 0x01FF) == 0x01BF )
   {
      /* in area $DF00-$DF7F */
      io_post_func_t handler = io_handlers[(in >> 3) & 0x0F];
      if( handler )
      {
         // handler is available, call it
         handler( (in >> 24) & 1, in >> 16, in );
      }
   }
}


void event_meta( uint32_t value )
{
   bus_stop_cause = value;
}


#if 0
// needs to be rewrite
// new format of event queue should output like "event_name" + 16bit hex value
// timers should also get own info seqment

void info_eventqueue( char *buffer, size_t size )
{
   int printed;
   queue_event_t *event;
   int i = 0;
   const char *timer_names[2] = { "NMI", "IRQ" };

   for( i = 0; i < count_of(timer_names); ++i )
   {
      printed = snprintf( buffer, size,
                          "timer%d (%s):    id=%ld us=%lld d=%lu\n"
                          , i, timer_names[i]
                          , timer_ms[i].alarm_id, timer_ms[i].delay_us
                          , (uint32_t)timer_ms[i].user_data
                        );
      buffer += printed;
      size   -= printed;
      if( size <= 0 )
      {
         /* buffer is full */
         return;
      }
   }

   printed = snprintf( buffer, size,
                       "cycle counter:   %016llx\n"
                       "next timerstamp: %016llx\n\n"
                       "id|       timestamp|handler_function        |param\n"
                       , _queue_cycle_counter
                       , _queue_next_timestamp
                     );
   buffer += printed;
   size   -= printed;

   for( i = 0, event = _queue_next_event; event; event = event->next )
   {
      printed = snprintf( buffer, size,
                          "%02x|%016llx|%-24s|%8p\n"
                          , i++
                          , event->timestamp
                          , debug_handler_name( event->handler )
                          , event->data
                        );
      buffer += printed;
      size   -= printed;
      if( size <= 0 )
      {
         /* buffer is full */
         return;
      }
   }
}


static inline void handle_bus( uint32_t in )
{
   uint8_t index = (in >> 16);
   // I/O service functions will be called
   switch( in >> 24 )
   {
      case 0x01: // call C function
         {
            event_func_t event = events[(in >> 16) & 0x0F];
            if( event )
            {
               event( in );
            }
         }
         break;
#if 0
      case 0x02: // check if it's internal I/O write
      case 0x03: // check if it's internal I/O read
         //if( (msg.address & 0xFF80) == 0xDF00 )
         //0xFF80 >> 7 = 0x01FF, 0xDF80 >> 7 = 0x01BF
         if( ((in >> 7) & 0x01FF) == 0x01BF )
         {
            /* in area $DF00-$DF7F */
            io_post_func_t handler = io_handlers[(in >> 3) & 0x0F];
            if( handler )
            {
               // handler is available, call it
               handler( (in >> 24) & 1, in >> 16, in );
            }
         }
         break;
#else
      case 0x02: // check if it's internal I/O write
         //if( (msg.address & 0xFF80) == 0xDF00 )
         //0xFF80 >> 7 = 0x01FF, 0xDF80 >> 7 = 0x01BF
         if( ((in >> 7) & 0x01FF) == 0x01BF )
         {
            /* in area $DF00-$DF7F */
            io_post_func_t handler = io_handlers[(in >> 3) & 0x0F];
            if( handler )
            {
               // handler is available, call it
               handler( false, in >> 16, in );
            }
         }
         break;
      case 0x03: // check if it's internal I/O read
         //if( (msg.address & 0xFF80) == 0xDF00 )
         //0xFF80 >> 7 = 0x01FF, 0xDF80 >> 7 = 0x01BF
         if( ((in >> 7) & 0x01FF) == 0x01BF )
         {
            /* in area $DF00-$DF7F */
            io_post_func_t handler = io_handlers[(in >> 3) & 0x0F];
            if( handler )
            {
               // handler is available, call it
               handler( true, in >> 16, in );
            }
         }
         break;
#endif
      default:
         break;
   }
}
#endif


static inline void console_65c02()
{
   int in = PICO_ERROR_TIMEOUT;
   uint8_t out;

   bus_stop_cause = 0;
   while( !bus_stop_cause )
   {
      // handle bus and event fifo
      while( multicore_fifo_rvalid() )
      {
         uint32_t fifo = multicore_fifo_pop_blocking();

         if( (fifo >> 24) >= 0x10 )
         {
            internal_error( __FILE__, __LINE__, "fifo element", fifo );
         }

         event_func_t handler = events[(fifo >> 24) & 0x0F];
         if( handler )
         {
            // handler is available, call it
            handler( fifo );
         }
      }

      // handle usb uart input
      in = getchar_timeout_us(0);
      if( in != PICO_ERROR_TIMEOUT )
      {
         if( console_crlf_enabled )
         {
            if( in == '`' )
            {
               in = getchar_timeout_us( 100000 );
               if( in == '`' )
               {
                  queue_event_add( 1, BUSMSG_META_MAGICKEY );
               }
               // make sure that a '`' get sent if timeout of other key occurs
               in = '`';
            }
            else if( in == 0x1d ) // 0x1d = CTRL+]
            {
               queue_event_add( 1, BUSMSG_META_MAGICKEY );
            }
         }
         else
         {
            // flow control is not possible with a simple fifo,
            // especially when removing take place in the same thread
            // as adding
            uart_input_add( in );
         }
      }

      if( uart_output_fetch( &out ) )
      {
         if( out == 0x7f )
         {
            /* full delete sequence: backspace, space, backspace */
            putchar( 8 );
            putchar( ' ' );
            putchar( 8 );
         }
         else
         {
            cs_put_utf8( out, console_charset );
         }
      }
   }
}


static inline void console_rp2040()
{
   int in;
   bool leave = false;
   uint16_t lines, cols;
   static mc_damem_t   *mc_da = 0;
   static mc_hexedit_t *mc_he = 0;

   const char *invoke = "unknown";

   switch( bus_stop_cause )
   {
      case BUSMSG_META_MAGICKEY:
         invoke = "magic key combo";
         break;
      case BUSMSG_META_TRAP:
         invoke = "trap";
         break;
      case BUSMSG_META_WATCHDOG:
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
         goto cancel;
      }
      if( (ch | 0x20) == 'f' ) // force 80x23
      {
         lines = 23;
         cols  = 80;
      }
      else
      {
         screen_get_size( &lines, &cols );
      }
   }

   while( !leave )
   {
      screen_get_size( &lines, &cols );
      clear();
      mcurses_border( true, 0, 0, lines-1, cols-1 );

      mcurses_textbox( false, lines-6, 1, debug_get_info( DEBUG_INFO_CLOCKS ) );
      mcurses_textbox( false, lines-6, ((screen_get_columns()+1)>>1) - 9, debug_get_info( DEBUG_INFO_SYSVECTORS ) );
      mcurses_textbox( false, lines-6, cols-21, debug_get_info( DEBUG_INFO_HEAP ) );

      mcurses_sorbus_logo( 1, 2 );
      move( 5, 5 );
      addstr( "JAM: Just Another Machine | https://sorbus.xayax.net/jam/" );
      mcurses_line_horizontal( true, 6, 0, cols-1 );
      move( 7, 2 );
      addstr( sorbus_version );
      move( 8, 2 );
      addstr( "Meta Menu invoked via " );
      addstr( invoke );
      addstr( ", CPU stopped on RDY" );

      move( 10, 2 );
      addstr( "B)acktrace, M)emory, D)isassemble, U)pload, E)vent queue," );
      move( 11, 2 );
      addstr( "I)nternal drive, T)imers," );
      move( 12, 2 );
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
         case '?':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Internal Error", debug_get_info( DEBUG_INFO_INTERNALERROR ) );
            getch();
            break;
         case 'B':
            {
               uint32_t *trace, entries, start;
               debug_get_backtrace( &trace, &entries, &start );
               mcurses_trace( cputype, trace, entries, start );
            }
            break;
         case '@':
            mcurses_trace( cputype, cpu_detect_trace(), 0, 0 );
            break;
         case 'D':
            {
               if( !mc_da )
               {
                  /* initialize on first run */
                  mc_da = (mc_damem_t*)ht_calloc( 1, sizeof( mc_damem_t ) );
                  mc_da->banks         = NUMBER_OF_BANKS;
                  mc_da->dam           = da_memory_init();
                  mc_da->dam->address  = 0x400;
                  mc_da->dam->bank     = 1;
                  mc_da->dam->cpu      = cputype;
                  mc_da->dam->peek     = debug_peek;
               }
               da_memory_linecache( mc_da->dam, lines );
               mcurses_damem( mc_da );
            }
            break;
         case 'E':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Event Queue", debug_get_info( DEBUG_INFO_EVENTQUEUE ) );
            getch();
            break;
         case 'F':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Fifo Input", debug_get_info( DEBUG_INFO_FIFO_INPUT ) );
            getch();
            break;
         case 'G':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Fifo Output", debug_get_info( DEBUG_INFO_FIFO_OUTPUT ) );
            getch();
            break;
         case 'I':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Internal Drive", debug_get_info( DEBUG_INFO_INTERNALDRIVE ) );
            getch();
            break;
         case 'M':
            if( !mc_he )
            {
               mc_he = (mc_hexedit_t*)ht_calloc( 1, sizeof( mc_hexedit_t ) );
               mc_he->address = 0x0400;
               mc_he->bank    = 1;
               mc_he->banks   = NUMBER_OF_BANKS;
               mc_he->charset = 1;
               mc_he->peek    = debug_peek;
               mc_he->poke    = debug_poke;
               mc_he->topleft = 0x0400;
            }
            hexedit( mc_he );
            break;
         case 'T':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Timers", debug_get_info( DEBUG_INFO_TIMERS ) );
            getch();
            break;
         case 'U':
            leave = mc_xmodem_upload( debug_poke );
            break;
         case 'V':
            mcurses_textbox( false, MC_TEXT_CENTER, MC_TEXT_CENTER, sorbus_version );
            getch();
            break;
         case 'C':
            leave = true;
            break;
         case 'R':
            leave = true;
            gpio_clr_mask( bus_config.mask_reset );
            break;
      }
   }

cancel:
   // restore CRLF setting for 65C02
   uart_set_translate_crlf( uart0, console_crlf_enabled );

   screen_restore();
}


void io_run()
{
   multicore_fifo_drain();
   for(;;)
   {
      /* switching between modes will be handled by leaving */
      gpio_set_mask( bus_config.mask_rdy );
      console_65c02();
      gpio_clr_mask( bus_config.mask_rdy );
      console_rp2040();
   }
}
