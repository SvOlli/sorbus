/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"

#include <hardware/uart.h>
#include <pico/util/queue.h>

//#define FIFO256_INLINE FIFO256_INLINE
// include code as inline
//#include "fifo256.c"

//
// internal functions
//


fifo256_t uart_in_queue, uart_out_queue;

void console_set_uart( uint8_t data )
{
   console_crlf_enabled = data & 1;
   console_flowcontrol_enabled = data & 2;
   console_charset = (data >> 2) & 0x03;
   uart_set_translate_crlf( uart0, console_crlf_enabled );
   // TODO: move
   //set_sys_clock_khz( data & 0x80 ? 200000 : 133000, false );
}


static inline void uart_input_was_read( uint16_t address )
{
   uint8_t data;

   if( fifo256_get( &uart_in_queue, &data ) )
   {
      // there was something in the queue, let's use it
      ram[address+0] = data;
      ram[address+1] = fifo256_count( &uart_in_queue ) + 1;
      return;
   }

   // nothing in queue: clear everything
   ram[address+0] = 0;
   ram[address+1] = 0;
}


static inline void uart_output_was_written( uint16_t address )
{
   if( fifo256_put( &uart_out_queue, ram[address+0] ) )
   {
      ram[address+1] = fifo256_count( &uart_out_queue );
      return;
   }

   // buffer overflow... really?
   ram[MEM_ADDR_UART_CONTROL] |= 0x40;
}


//
// API functions
//


void io_post_uart( bool rw, uint8_t data, uint16_t address )
{
   switch( address & 0x7 )
   {
      case 3: // config control
         // clear error bits
         ram[address] &= 0x3F;
         if( !rw )
         {
            // write on control register
            console_set_uart( data );
         }
         break;
      case 4: // uart in queue read
         uart_input_was_read( address );
         break;
      case 6: // uart out queue write
         uart_output_was_written( address );
         break;
      default: // queue sizes will be updated with en-/dequeueing
         break;
   }
}


void uart_reset()
{
   fifo256_init( &uart_in_queue );
   ram[MEM_ADDR_UART_CONTROL]    = 0x05;
   console_set_uart( ram[MEM_ADDR_UART_CONTROL] );
   ram[MEM_ADDR_UART_READ]       = 0x00;
   ram[MEM_ADDR_UART_READ_SIZE]  = 0x00;
   ram[MEM_ADDR_UART_WRITE]      = 0x00;
   ram[MEM_ADDR_UART_WRITE_SIZE] = 0x00;
}
