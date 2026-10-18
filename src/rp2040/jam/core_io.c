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
#include "generic_helper.h"
#include "mcurses.h"

#include "da_memory.h"

#include <pico/stdio.h>
#include <pico/multicore.h>
//#include <pico/stdlib.h>
//#include <pico/util/queue.h>
//#include <hardware/clocks.h>

#ifdef __OPTIMIZE__
#ifdef __OPTIMIZE_SIZE__
#define GCC_OPTIMIZED "Optimized for size"
#else
#define GCC_OPTIMIZED "Optimized for speed"
#endif
#else
#define GCC_OPTIMIZED "Not optimized"
#endif


typedef void (*io_post_func_t)( bool rw, uint8_t data, uint16_t address );


/* will be used in event functions array */
void handle_io( uint32_t in );

uint8_t console_charset = 0;
bool console_crlf_enabled;
bool console_flowcontrol_enabled;


// callbacks for 0x00xxxxxx & 0x01xxxxxx
// sliced up to handle areas of 8 bytes
const io_post_func_t io_handlers[0x10] = {
   io_irq_misc,         // $df00-$df07: misc like bank, trap, rnd, etc.
   io_irq_uart,         // $df08-$df0f: UART
   0,                   // $df10-$df17: (was timer cycle)
   io_irq_timer,        // $df18-$df1f: timer ms/10
   io_irq_watchdog,     // $df20-$df23: watchdog
   //                      $df24-$df27: cyclecount
   0,                   // $df28-$df2f: unused RAM / kernel variables
   0,                   // $df30-$df37: system monitor variables
   0,                   // $df38-$df3f: unused RAM
   0,                   // $df40-$df47: unused RAM
   0,                   // $df48-$df4f: unused RAM
   0,                   // $df50-$df57: unused RAM
   0,                   // $df58-$df5f: unused RAM
   0,                   // $df60-$df67: unused RAM
   0,                   // $df68-$df6f: unused RAM
   io_irq_intdrive,     // $df70-$df77: internal drive data/strobe registers
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


void info_system( char *b, int bsize )
{
   int used = 0;

   used += snprintf( b+used, bsize-used,
                     "Sorbus JAM Version " SORBUS_JAM_VERSION
                     "\nCompiled with: gcc " __VERSION__
                     "\n" GCC_OPTIMIZED
                     "\nUsing PICO SDK: " PICO_SDK_VERSION_STRING
                     "\n\nMulticover FIFOs overflows: %u"
                     "\nLast reported error:\n"
                     , multicore_fifo_overflow_counter );
   used += internal_error_info( b+used, bsize-used );
   used += snprintf( b+used, bsize-used,
                     "\nPress 'C' to clear last error\n" );
}


const char *debug_get_info( debug_info_t page )
{
   static char buffer[1280] = { 0 };
   int used = 0;

   buffer[0] = '\0';
   switch( page )
   {
      case DEBUG_INGO_SYSINFO:
         info_system( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_HEAP:
         ht_info( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_CLOCKS:
         info_clocks( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_CYCLECOUNT:
         info_cyclecount( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_SYSVECTORS:
         info_sysvectors( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_INTERNALDRIVE:
         info_internaldrive( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_TIMERS:
         used += info_timers( &buffer[0], sizeof(buffer)-1 );
         used += snprintf( &buffer[used], sizeof(buffer)-1-used, "Watchdog: " );
         used += info_watchdog( &buffer[used], sizeof(buffer)-1-used );
         break;
      case DEBUG_INFO_BUS_VARS:
         info_bus_vars( &buffer[0], sizeof(buffer)-1 );
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


void fifo_irq_handler()
{
   while( multicore_fifo_rvalid() )
   {
      uint32_t in = multicore_fifo_pop_blocking();

      //if( (msg.address & 0xFF80) == 0xDF00 )
      //0xFF80 >> 7 = 0x01FF, 0xDF00 >> 7 = 0x01BE
      if( ((in >> 7) & 0x01FF) == 0x01BE )
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

   multicore_fifo_clear_irq();
}


static inline void console_65c02()
{
   int in = PICO_ERROR_TIMEOUT;
   uint8_t out;

   bus_stop_cause = 0;
   while( !bus_stop_cause )
   {
      // handle usb uart input
      in = getchar_timeout_us(0);
      if( in != PICO_ERROR_TIMEOUT )
      {
         if( console_crlf_enabled )
         {
            if( in == '\\' )
            {
               int in2 = getchar_timeout_us( 150000 );
               if( in2 == PICO_ERROR_TIMEOUT )
               {
                  uart_input_add( in );
               }
               else if( in2 == '\\' )
               {
                  // only trigger on double keypress
                  bus_stop_cause = META_MAGICKEY;
               }
               else
               {
                  uart_input_add( in );
                  uart_input_add( in2 );
               }
            }
            else if( in == 0x1d ) // 0x1d = CTRL+]
            {
               // TODO: EVENT_QUEUE
               bus_stop_cause = META_MAGICKEY;
            }
            else
            {
               uart_input_add( in );
            }
         }
         else
         {
            // flow control is not possible with a simple fifo,
            // especially when removing take place in the same thread
            // as adding
            uart_input_add( in );
            // need event for keypress
            ram[MEM_ADDR_UART_READ_SIZE] = (uart_in_queue.tail) - (uart_in_queue.head);
         }
      }

      if( fifo256_get( &uart_out_queue, &out ) )
      {
         ram[MEM_ADDR_UART_WRITE_SIZE] = fifo256_count( &uart_out_queue );
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


   watchdog_freeze();
   timers_freeze();

   switch( bus_stop_cause )
   {
      case META_MAGICKEY:
         invoke = "magic key combo";
         break;
      case META_TRAP:
         invoke = "trap";
         break;
      case META_WATCHDOG:
         invoke = "watchdog";
         break;
      case META_INTERROR:
         invoke = "internal error";
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
      addstr( "CPU stopped on RDY, Meta Menu invoked via " );
      addstr( invoke );

      move(  9, 2 );
      addstr( "B)acktrace, M)emory, D)isassemble" );
      move( 10, 2 );
      addstr( "I)nternal Drive, T)imers, Input F)ifo, O)utput Fifo" );
      move( 11, 2 );
      addstr( "!)Dump Backtrace, @)CPUdetect Backtrace, X-Modem U)pload" );
      move( 12, 2 );
      addstr( "?)System Info, Cy)clecount, #)Banks Debug" );
      move( 14, 2 );
      addstr( "C)ontinue, R)eset ? " );

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
         case '#':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Bank Pointers",
                              debug_get_info( DEBUG_INFO_BUS_VARS ) );
            getch();
            break;
         case '?':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "System Information",
                              debug_get_info( DEBUG_INGO_SYSINFO ) );
            if( toupper( getch() ) == 'C' )
            {
               internal_error_clear();
            }
            break;
         case '@':
            mcurses_trace( cputype, cpu_detect_trace(), 0, 0 );
            break;
         case 'B':
            {
               uint32_t *trace, entries, start;
               debug_get_backtrace( &trace, &entries, &start );
               mcurses_trace( cputype, trace, entries, start );
            }
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
         case 'F':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Fifo Input",
                              debug_get_info( DEBUG_INFO_FIFO_INPUT ) );
            getch();
            break;
         case 'I':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Internal Drive",
                              debug_get_info( DEBUG_INFO_INTERNALDRIVE ) );
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
         case 'O':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Fifo Output",
                              debug_get_info( DEBUG_INFO_FIFO_OUTPUT ) );
            getch();
            break;
         case 'P':
            /* TODO: papertape upload */
            break;
         case 'T':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Timers (decimal values in ms)",
                              debug_get_info( DEBUG_INFO_TIMERS ) );
            getch();
            break;
         case 'U':
            leave = mc_xmodem_upload( debug_poke );
            break;
         case 'Y':
            mcurses_titlebox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                              "Cyclecount",
                              debug_get_info( DEBUG_INFO_CYCLECOUNT ) );
            getch();
            break;

         case 'R':
            //gpio_clr_mask( bus_config.mask_reset );
            system_reset( false );
            /* slip through, as reset also leaves */
         case 'C':
            leave = true;
            break;
      }
   }

cancel:
   // restore CRLF setting for 65C02
   uart_set_translate_crlf( uart0, console_crlf_enabled );

   screen_restore();

   timers_unfreeze();
   watchdog_unfreeze();
}


void io_run()
{
   multicore_fifo_drain();

#if SWITCH_CORES
   irq_set_exclusive_handler( SIO_FIFO_IRQ_NUM(1), fifo_irq_handler );
   irq_set_enabled( SIO_FIFO_IRQ_NUM(1), true );
#else
   irq_set_exclusive_handler( SIO_FIFO_IRQ_NUM(0), fifo_irq_handler );
   irq_set_enabled( SIO_FIFO_IRQ_NUM(0), true );
#endif

   system_reset( false );
   for(;;)
   {
      /* switching between modes will be handled by leaving */
      gpio_set_mask( bus_config.mask_rdy );
      console_65c02();
      gpio_clr_mask( bus_config.mask_rdy );
      console_rp2040();
   }
}
