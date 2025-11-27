/*
 * Copyright 2006 Arastra, Inc.
 * Copyright 2001, 2002 Georges Menie (www.menie.org)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 * ---------------------------------------------------------------------------
 * This code was modified to fit better the RP2040 code of the Sorbus Computer
 */
#include "generic_helper.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>


#define SOH          0x01
#define STX          0x02
#define EOT          0x04
#define ACK          0x06
#define NAK          0x15
#define CAN          0x18
#define CTRLZ        0x1A
#define DLY_1S       1000
#define DLY_1_5S     1500
#define DLY_2S       2000
#define MAXRETRANS   25


static int _inbyte( int msec )
{
   int c = getchar_timeout_us( msec*1000 );
   if( c == PICO_ERROR_TIMEOUT )
   {
      return -1;
   }
   return c;
}


static unsigned short crc16_ccitt( const uint8_t *buf, int sz )
{
   unsigned short crc = 0;
   while (--sz >= 0)
   {
      int i;
      crc ^= (unsigned short) *buf++ << 8;
      for( i = 0; i < 8; i++ )
      {
         if( crc & 0x8000 )
         {
            crc = (crc << 1) ^ 0x1021;
         }
         else
         {
            crc <<= 1;
         }
      }
   }
   return crc;
}


static bool check( int crc, const uint8_t *buf, int sz )
{
   if( crc )
   {
      unsigned short crc = crc16_ccitt(buf, sz);
      unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
      if( crc == tcrc )
      {
         return true;
      }
   }
   else
   {
      int i;
      uint8_t cks = 0;
      for( i = 0; i < sz; ++i )
      {
         cks += buf[i];
      }
      if( cks == buf[sz] )
      {
         return true;
      }
   }
   return false;
}


static void flushinput(void)
{
   /* wait 1.5s for a byte */
   while( _inbyte(DLY_1_5S) >= 0 )
      ;
}


int xmodem_receive( poke_t poke, uint16_t addr )
{
   bool valid = true;
   uint8_t xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
   uint8_t *p;
   int bufsz, crc = 0;
   uint8_t trychar = 'C';
   uint8_t packetno = 1;
   int i, c, len = 0;
   int retry, retrans = MAXRETRANS;
   for(;;)
   {
      for( retry = 0; retry < 16; ++retry)
      {
         if (trychar)
         {
            putchar( trychar );
         }
         if( (c = _inbyte((DLY_1S)<<1)) >= 0 )
         {
            switch (c)
            {
               case SOH:
                  bufsz = 128;
                  goto start_recv;
               case STX:
                  bufsz = 1024;
                  goto start_recv;
               case EOT:
                  flushinput();
                  putchar(ACK);
                  return len; /* normal end */
               case CAN:
                  if ((c = _inbyte(DLY_1S)) == CAN)
                  {
                     flushinput();
                     putchar(ACK);
                     return -1; /* canceled by remote */
                  }
                  break;
               default:
                  break;
            }
         }
      }
      if( trychar == 'C' )
      {
         trychar = NAK;
         continue;
      }
      flushinput();
      putchar(CAN);
      putchar(CAN);
      putchar(CAN);
      return -2; /* sync error */

start_recv:
      if( trychar == 'C' )
      {
         crc = 1;
      }
      trychar = 0;
      p = xbuff;
      *p++ = (uint8_t)c;
      for (i = 0;  i < (bufsz+(crc?1:0)+3); ++i)
      {
         if ((c = _inbyte(DLY_1S)) < 0)
         {
            goto reject;
         }
         *p++ = (uint8_t)c;
      }
      if( (xbuff[1] == (uint8_t)(~xbuff[2])) &&
          ((xbuff[1] == packetno) || (xbuff[1] == (uint8_t)packetno-1)) &&
          check(crc, &xbuff[3], bufsz) )
      {
         if( xbuff[1] == packetno )
         {
            int count = 0x10000 - addr - len;
            int i;
            if( count > bufsz )
            {
               count = bufsz;
            }
            for( i = 0; i < count; ++i )
            {
               if( valid )
               {
                  poke( 0, addr++, xbuff[3+i] );
                  if( addr == 0 )
                  {
                     /* wrapped, no more writes */
                     valid = false;
                  }
                  ++len;
               }
            }
            ++packetno;
            retrans = MAXRETRANS+1;
         }
         if (--retrans <= 0)
         {
            flushinput();
            putchar(CAN);
            putchar(CAN);
            putchar(CAN);
            return -3; /* too many retry error */
         }
         putchar(ACK);
         continue;
      }

reject:
      flushinput();
      putchar(NAK);
   }
}
