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


static inline void uart_input_was_read()
{
   uint8_t data;

   // first of all drop first element of queue
   if( fifo256_drop( &uart_in_queue ) )
   {
      // now let's check if there still is data left in queue
      if( fifo256_peek( &uart_in_queue, &data ) )
      {
         // there was something in the queue, let's use it
         ram[MEM_ADDR_UART_READ]      = data;
         ram[MEM_ADDR_UART_READ_SIZE] = fifo256_count( &uart_in_queue );
         return;
      }
   }

   // nothing in queue: clear everything
   ram[MEM_ADDR_UART_READ]      = 0;
   ram[MEM_ADDR_UART_READ_SIZE] = 0;
}


//
// API functions
//


void io_irq_uart_w( uint32_t in )
{
   uint8_t data;
   switch( in & 0x07 )
   {
      case 3: // config control
         console_set_uart( ram[MEM_ADDR_UART_CONTROL] );
         break;
      case 4: // uart in queue read
         // first of all drop first element of queue
         if( fifo256_drop( &uart_in_queue ) )
         {
            // now let's check if there still is data left in queue
            if( fifo256_peek( &uart_in_queue, &data ) )
            {
               // there was something in the queue, let's use it
               ram[MEM_ADDR_UART_READ]      = data;
               ram[MEM_ADDR_UART_READ_SIZE] = fifo256_count( &uart_in_queue );
               return;
            }
         }
         // nothing in queue: clear everything
         ram[MEM_ADDR_UART_READ]      = 0;
         ram[MEM_ADDR_UART_READ_SIZE] = 0;
         break;
      case 6: // uart out queue write
         if( fifo256_put( &uart_out_queue, ram[MEM_ADDR_UART_WRITE] ) )
         {
            ram[MEM_ADDR_UART_WRITE_SIZE] = fifo256_count( &uart_out_queue );
            return;
         }
         // buffer overflow... really?
         ram[MEM_ADDR_UART_CONTROL] |= 0x40;
         break;
      default:
         break;
   }
}


void io_irq_uart_r( uint32_t in )
{
   uint8_t data;

   switch( in & 0x07 )
   {
      case 3: // config control
         // clear error bits
         ram[MEM_ADDR_UART_CONTROL] &= 0x3F;
         break;
      case 4: // uart in queue read
         // first of all drop first element of queue
         if( fifo256_drop( &uart_in_queue ) )
         {
            // now let's check if there still is data left in queue
            if( fifo256_peek( &uart_in_queue, &data ) )
            {
               // there was something in the queue, let's use it
               ram[MEM_ADDR_UART_READ]      = data;
               ram[MEM_ADDR_UART_READ_SIZE] = fifo256_count( &uart_in_queue );
               return;
            }
         }
         // nothing in queue: clear everything
         ram[MEM_ADDR_UART_READ]      = 0;
         ram[MEM_ADDR_UART_READ_SIZE] = 0;
         break;
      default:
         break;
   }
}


void uart_reset()
{
   fifo256_init( &uart_in_queue );
   fifo256_init( &uart_out_queue );
   ram[MEM_ADDR_UART_CONTROL]    = 0x05;
   console_set_uart( ram[MEM_ADDR_UART_CONTROL] );
}
