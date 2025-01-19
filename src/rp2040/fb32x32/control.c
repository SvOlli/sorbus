/**
 * Copyright (c) 2025 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a 32x32 pixel framebuffer
 * for the Sorbus Computer
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pico/multicore.h>

#include "common/bus.h"
#include "fb32x32.h"


uint8_t framebuffer[0x800];

uint16_t pal_c64[16] = {
   0x000, 0xfff, 0xb12, 0x3ec, 0xb1e, 0x1d1, 0x21a, 0xdf0,
   0xb40, 0x630, 0xf45, 0x222, 0x444, 0x5f5, 0x55f, 0x888
};

uint16_t pal_c16[128] = {
   0x000, 0x222, 0x611, 0x044, 0x506, 0x050, 0x229, 0x330,
   0x620, 0x430, 0x240, 0x613, 0x042, 0x038, 0x418, 0x150,
   0x000, 0x333, 0x722, 0x055, 0x617, 0x060, 0x33a, 0x440,
   0x730, 0x540, 0x350, 0x714, 0x053, 0x149, 0x529, 0x160,
   0x000, 0x444, 0x833, 0x166, 0x728, 0x161, 0x33b, 0x550,
   0x731, 0x640, 0x360, 0x825, 0x164, 0x249, 0x62a, 0x260,
   0x000, 0x555, 0x944, 0x277, 0x839, 0x282, 0x45c, 0x660,
   0x842, 0x750, 0x470, 0x936, 0x275, 0x35a, 0x73b, 0x370,
   0x000, 0x888, 0xb66, 0x499, 0xa5b, 0x4a4, 0x77e, 0x881,
   0xb74, 0xa82, 0x791, 0xb68, 0x4a7, 0x58d, 0x96e, 0x6a2,
   0x000, 0xbbb, 0xe99, 0x7cc, 0xd8e, 0x7d7, 0xaaf, 0xbb4,
   0xea7, 0xcb5, 0xac4, 0xe9b, 0x7da, 0x8bf, 0xc9f, 0x9d5,
   0x000, 0xccc, 0xfbb, 0x9ee, 0xfaf, 0x9f9, 0xccf, 0xdd6,
   0xfc9, 0xed7, 0xce6, 0xfad, 0x9ec, 0xadf, 0xebf, 0xbf7,
   0x000, 0xfff, 0xfee, 0xcff, 0xfdf, 0xcfd, 0xfff, 0xff9,
   0xffc, 0xffa, 0xff9, 0xfef, 0xcff, 0xdff, 0xfef, 0xefa
};

static void dmacopy( uint8_t *dest, uint8_t *src,
                     uint8_t width, uint8_t height,
                     uint8_t step )
{
   uint8_t x, y;
   uint8_t *d = dest;
   uint8_t *s = src;

   ++height;
   ++width;
   ++step;

   uint8_t sline = step - width;
   uint8_t dline = 32   - width;

#if 0
   // normalized at set parameter
   width  &= 0x1f;
   height &= 0x1f;
#endif

   for( y = 0; y < height; ++y )
   {
      for( x = 0; x < width; ++x )
      {
         (*d++) = (*s++);
      }
      s += sline;
      d += dline;
   }
}


static void dmacopytrans( uint8_t *dest, uint8_t *src,
                          uint8_t width, uint8_t height,
                          uint8_t step, uint8_t mode, uint8_t tcol )
{
   uint8_t x, y;
   uint8_t *d = dest;
   uint8_t *s = src;

   ++height;
   ++width;
   ++step;

   uint8_t sline = step - width;
   uint8_t dline = 32   - width;

#if 0
   // normalized at set parameter
   width  &= 0x1f;
   height &= 0x1f;
#endif

   for( y = 0; y <= height; ++y )
   {
      for( x = 0; x <= width; ++x )
      {
         if( (*d == tcol) || (*s == tcol) )
         {
            continue;
         }
         (*d++) = (*s++);
      }
      s += sline;
      d += dline;
   }
}


static inline void setcolormap( uint8_t data )
{
   uint32_t c, r, g, b, i;
   uint8_t tab = data & 0x3F;

   switch( tab )
   {
      case 0:
         for( c = 0; c < 0x100; ++c )
         {
            // convert 0bRRGGBBII to 0b0000RRII0000GGII0000BBII

#if 1
            r = (c >> 6);
            g = (c >> 4) & 3;
            b = (c >> 2) & 3;
            i = (c     ) & 3;
#else
            r = (c & 0xc0) >> 6;
            g = (c & 0x30) >> 4;
            b = (c & 0x0c) >> 2;
            i = (c & 0x03) >> 0;
#endif

            hardware_setcolor( c, (r << 2) | i, (g << 2) | i, (b << 2) | i );
         }
         break;
      case 1:
         break;
      case 4:
         for( c = 0; c < 0x100; ++c )
         {
            r = (pal_c64[c & 0x0f] & 0xf00) >> 8;
            g = (pal_c64[c & 0x0f] & 0x0f0) >> 4;
            b = (pal_c64[c & 0x0f] & 0x00f) >> 0;

            hardware_setcolor( c, r, g, b );
         }
         break;
      case 5:
         for( c = 0; c < 0x100; ++c )
         {
            r = (pal_c16[c & 0x7f] & 0xf00) >> 8;
            g = (pal_c16[c & 0x7f] & 0x0f0) >> 4;
            b = (pal_c16[c & 0x7f] & 0x00f) >> 0;

            hardware_setcolor( c, r, g, b );
         }
         break;
      default:
         break;
   }

}


void control_init()
{
   memset( &framebuffer[0], sizeof(framebuffer), 0x00 );
   setcolormap( 0 );
   hardware_flush();
}


void control_loop()
{
   uint32_t bus;
   uint16_t address;
   uint8_t  data;
   uint16_t srcaddr  = 0xCC00;
   uint16_t destaddr = 0x0000;
   uint8_t  width    = 0x1F;    // range 0x00..0x1F only
   uint8_t  height   = 0x1F;    // range 0x00..0x1F only
   uint8_t  step     = 0x1F;
   uint8_t  tcol     = 0x00;

   for(;;)
   {
      bus     = multicore_fifo_pop_blocking();
      address = bus >> BUS_CONFIG_shift_address;
      data    = bus >> BUS_CONFIG_shift_data;

      switch( address & 0x0f )
      {
         case 0x00:
            if( data )
            {
               dmacopytrans( &framebuffer[destaddr], &mem_cache[srcaddr], width, height, step, data, tcol );
            }
            else
            {
               dmacopy( &framebuffer[destaddr], &mem_cache[srcaddr], width, height, step );
            }
            hardware_flush();
            break;
         case 0x01:
            if( data )
            {
               dmacopytrans( &framebuffer[destaddr], &mem_cache[srcaddr], width, height, step, data, tcol );
            }
            else
            {
               dmacopy( &framebuffer[destaddr], &mem_cache[srcaddr], width, height, step );
            }
            break;
         case 0x02:
            srcaddr  =  (srcaddr & 0xFF00) | data;
            break;
         case 0x03:
            srcaddr  =  (srcaddr & 0x00FF) | (data << 8);
            break;
         case 0x04:
            destaddr = (destaddr & 0x0300) | data;
            break;
         case 0x05:
            destaddr = (destaddr & 0x00FF) | ((data & 0x03) << 8);
            break;
         case 0x06:
            destaddr = (destaddr & 0x03E0) | (data & 0x1F);
            break;
         case 0x07:
            destaddr = (destaddr & 0x001F) | ((data & 0x1F) << 5);
            break;
         case 0x08:
            width    = (data & 0x1F);
            break;
         case 0x09:
            height   = (data & 0x1F);
            break;
         case 0x0a:
            step     = data;
            break;
         case 0x0b:
            tcol     = data;
            break;
         case 0x0c:
            setcolormap( data );
            break;
         default:
            break;
      }
   }
}

