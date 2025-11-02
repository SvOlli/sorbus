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
#include "xmodem.h"
#include "generic_helper.h"

#include "event_queue.h"
extern void system_trap();

console_type_t console_type;
bool console_crlf_enabled;
bool console_flowcontrol_enabled;

bool console_wants_stop = false;
bool bus_wants_stop     = false;
int  invoke_type = 0;

/* Xmodem can run two chunk sizes, either 1024 or 128 */
#define XMODEM_BUFFER_SIZE 1024
#define XMODEM_CHUNK_SIZE 128

uint32_t mem_upload_p=0;
uint32_t mem_start_p=0;

uint8_t local_buf[XMODEM_BUFFER_SIZE];
extern uint8_t ram[0x10000];


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


void console_set_flowcontrol( bool enable )
{
   console_flowcontrol_enabled = enable;
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



/* For xmodem receive, we need to provide the HW specific functions*/
int _inbyte(int msec)
{
   int c = getchar_timeout_us( msec*1000 );
   if( c == PICO_ERROR_TIMEOUT )
   {
      return -1;
   }
   return c;
}

void _outbyte(unsigned char c)
{
    putchar(c);
}


int received_chunk(unsigned char * buf, int size)
{
   /* handles the download of a chunk to memory */
   uint16_t offset = 0;

   /* Copy it to ram */
   if (mem_upload_p == 0 )
   {
      /* fetch loadadress */
      mem_upload_p = buf[0]+(0x100*buf[1]);
      offset = 2;
      size -= 2;
      mem_start_p = mem_upload_p;
   }
   if ((mem_upload_p+size) < 0x10000)
   {
      memcpy( &ram[mem_upload_p], &buf[offset], size );
      mem_upload_p += size;
   }
   else
   {
      //TODO:  Overflow detected , check how we can stop upload
   }

}


int32_t print_upload_menu()
{
   int32_t in_addr;

   printf ("\n\nUploading file to address ? \n0: use load adress \n");
   printf ("ctrl-c to cancel\n");
   in_addr= get_16bit_address(0xe000);
   if (in_addr<0){
     printf("\nTransfer canceled\n");
   }
   return in_addr;
}


void run_upload(uint32_t upload_address)
{
   /* Xmodem-upload */
   int32_t ret = -1;
   int in_char;

   mem_upload_p = upload_address;
   mem_start_p  = mem_upload_p;
   printf( "\nUploading to adress 0x%04X\n", mem_upload_p );
   printf( "Start XModem Transfer now:\n" );
   ret = xmodemReceive();
   printf( "\n\n\npress a key \n\n" );
   in_char = getchar_timeout_us(10000000);

   if (ret<0)
   {
      printf( "\n\n\nFailure in reception %d\n\n", ret );
   }
   else
   {
      printf( "\n\n\nReception successful. 0x%04X - 0x%04X \n\n", mem_start_p, mem_start_p + ret );
   }

   return;
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
   {          /* 12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
      printf( "\nB)acktrace, D)isassemble, E)vent queue, H)eap, I)nternal drive, M)emory dump,"
              "\nS)peeds, U)pload"
              "\nC)ontinue, R)eboot ? " );
      in = toupper( getchar() );
      switch( in )
      {
         case '!':
            printf( "%c\n", in );
            debug_raw_backtrace();
            break;
         case 'B':
            printf( "%c\n", in );
            debug_backtrace();
            break;
         case 'D':
            printf( "%c\n", in );
            debug_disassembler();
            break;
         case 'E':
            printf( "%c\n", in );
            debug_queue_event( "Event queue" );
            break;
         case 'H':
            printf( "%c\n", in );
            debug_heap();
            break;
         case 'I':
            printf( "%c\n", in );
            debug_internal_drive();
            break;
         case 'M':
            printf( "%c\n", in );
            debug_memorydump();
            break;
         case 'S':
            printf( "%c\n", in );
            debug_clocks();
            break;
         case 'U':
            printf( "%c\n", in );
            leave = true;
            {
               int32_t upload_addr = print_upload_menu();
               if (upload_addr>=0)
               {
                  run_upload(upload_addr);
               }
            }
            break;
         case 'C':
            printf( "%c\n", in );
            leave = true;
            break;
         case 'R':
            printf( "%c\n", in );
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
      if( ((in == 0x1d)|| (in == 0x5e)) && (console_crlf_enabled) ) /* 0x1d = CTRL+] */
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
