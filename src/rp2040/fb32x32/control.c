/**
 * Copyright (c) 2025 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a 32x32 pixel framebuffer
 * for the Sorbus Computer
 */

/*
 * TODO:
 * - double buffer colormap
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pico/multicore.h>
#include <pico/platform.h>

#include "common/bus.h"
#include "fb32x32.h"


uint8_t framebuffer[0x800];

uint16_t pal_custom[3][256] = { 0 };

const uint16_t pal_c64[16] = {
   0x000, 0xfff, 0xb12, 0x3ec, 0xb1e, 0x1d1, 0x21a, 0xdf0,
   0xb40, 0x630, 0xf45, 0x222, 0x444, 0x5f5, 0x55f, 0x888
};

const uint16_t pal_c16[128] = {
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

const uint16_t pal_a800[256] = {
   0x000, 0x111, 0x222, 0x333, 0x444, 0x555, 0x666, 0x777,
   0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xeee, 0xfff,
   0x100, 0x210, 0x320, 0x430, 0x540, 0x650, 0x860, 0x970,
   0xa91, 0xba2, 0xcb3, 0xdc4, 0xed5, 0xfe7, 0xff8, 0xff9,
   0x300, 0x400, 0x510, 0x620, 0x730, 0x840, 0x950, 0xa61,
   0xb72, 0xc83, 0xda4, 0xeb6, 0xfc7, 0xfd8, 0xfe9, 0xffb,
   0x400, 0x500, 0x600, 0x710, 0x820, 0x931, 0xa43, 0xb54,
   0xc65, 0xd86, 0xe97, 0xfa8, 0xfb9, 0xfca, 0xfdc, 0xfed,
   0x400, 0x501, 0x601, 0x712, 0x823, 0x934, 0xa45, 0xb57,
   0xc68, 0xd79, 0xe8a, 0xf9b, 0xfac, 0xfbd, 0xfcf, 0xfdf,
   0x303, 0x404, 0x504, 0x605, 0x716, 0x827, 0x949, 0xa5a,
   0xb6b, 0xc7c, 0xd8d, 0xe9e, 0xfae, 0xfbe, 0xfce, 0xfdf,
   0x105, 0x206, 0x407, 0x518, 0x629, 0x73a, 0x84b, 0x95c,
   0xa6d, 0xb7e, 0xc8e, 0xd9e, 0xeae, 0xfbe, 0xfcf, 0xfef,
   0x007, 0x107, 0x208, 0x319, 0x42a, 0x53c, 0x65d, 0x76e,
   0x87f, 0x98f, 0xa9f, 0xbaf, 0xcbf, 0xecf, 0xfdf, 0xfef,
   0x006, 0x007, 0x019, 0x12a, 0x23b, 0x34c, 0x46d, 0x57e,
   0x68f, 0x89f, 0x9af, 0xabf, 0xbcf, 0xcdf, 0xdef, 0xeff,
   0x004, 0x016, 0x027, 0x039, 0x14a, 0x26b, 0x37c, 0x48d,
   0x59e, 0x6af, 0x7bf, 0x8cf, 0x9df, 0xaef, 0xbff, 0xcff,
   0x012, 0x023, 0x035, 0x046, 0x057, 0x178, 0x289, 0x39b,
   0x4ac, 0x5bd, 0x6ce, 0x7df, 0x8ef, 0xaff, 0xbff, 0xcff,
   0x020, 0x031, 0x042, 0x053, 0x064, 0x175, 0x287, 0x398,
   0x4a9, 0x5ba, 0x6cb, 0x7ec, 0x8fd, 0x9fe, 0xbff, 0xcff,
   0x020, 0x031, 0x041, 0x051, 0x161, 0x272, 0x383, 0x494,
   0x5b6, 0x6c7, 0x7d8, 0x8e9, 0x9fa, 0xafb, 0xbfc, 0xcfc,
   0x020, 0x031, 0x041, 0x151, 0x261, 0x371, 0x481, 0x592,
   0x6a3, 0x7b4, 0x8c5, 0x9d6, 0xaf7, 0xcf8, 0xdf9, 0xefa,
   0x010, 0x020, 0x230, 0x340, 0x450, 0x560, 0x670, 0x780,
   0x8a1, 0x9b3, 0xac4, 0xbd5, 0xce6, 0xdf7, 0xef8, 0xff8,
   0x100, 0x210, 0x320, 0x430, 0x640, 0x750, 0x870, 0x980,
   0xa91, 0xba2, 0xcb3, 0xdc4, 0xed6, 0xfe7, 0xff8, 0xff9
};

static inline void dmacopy( uint8_t *dest, uint8_t *src,
                            uint8_t width, uint8_t height,
                            uint8_t step )
{
   // adjust api parameters
   // range:   $00-$1f
   // meaning: $01-$20
   ++width;
   ++height;
   // range:   $00-$ff
   // meaning: $01-$100
   ++step;

   uint8_t x, y;
   uint8_t *d = dest;
   uint8_t *s = src;
   uint8_t dextra = 32   - width;
   int16_t sextra = step - width;

   for( y = 0; y < height; ++y )
   {
      for( x = 0; x < width; ++x )
      {
         (*d++) = (*s++);
      }
      s += sextra;
      d += dextra;
   }
}


static inline void dmacopytrans( uint8_t *dest, uint8_t *src,
                                 uint8_t width, uint8_t height,
                                 uint8_t step, uint8_t mode, uint8_t tcol )
{
   // adjust api parameters
   // range:   $00-$1f
   // meaning: $01-$20
   ++width;
   ++height;
   // range:   $00-$ff
   // meaning: $01-$100
   ++step;

   uint8_t x, y;
   uint8_t *d = dest;
   uint8_t *s = src;
   uint8_t dextra = 32   - width;
   int16_t sextra = step - width;

   for( y = 0; y < height; ++y )
   {
      for( x = 0; x < width; ++x, ++s, ++d )
      {
         // do not replace tcol on source
         if( (mode & 0x01) && (*s == tcol) ) continue;
         switch( mode & 0x06 )
         {
            case 0x02:
               // do not replace tcol on destination
               if( *d == tcol ) continue;
               break;
            case 0x04:
               // only replace tcol on destination
               if( *d != tcol ) continue;
               break;
            default: // 0x00, 0x06
               // skip, do nothing
               break;
         }
         // no more exceptions, let's copy
         *d = *s;
      }
      s += sextra;
      d += dextra;
   }
}


static inline void setpalette( const uint16_t *palette, uint16_t elements )
{
   uint16_t c;
   uint8_t r, g, b;

   for( c = 0; c < 0x100; ++c )
   {
      r = (palette[c & (elements-1)] & 0xf00) >> 8;
      g = (palette[c & (elements-1)] & 0x0f0) >> 4;
      b = (palette[c & (elements-1)] & 0x00f) >> 0;
      hardware_setcolor( c, r, g, b );
   }
}


static inline void setcolormap( uint8_t data )
{
   uint32_t c, r, g, b, i;
   uint8_t tab = data & 0x3F;

   switch( tab )
   {
      case 0: // default colormap
         for( c = 0; c < 0x100; ++c )
         {
            // convert 0bRRGGBBII to 0b0000RRII0000GGII0000BBII
            r = (c >> 6);
            g = (c >> 4) & 3;
            b = (c >> 2) & 3;
            i = (c     ) & 3;
            hardware_setcolor( c, (r << 2) | i, (g << 2) | i, (b << 2) | i );
         }
         break;
      case 1: // custom colormap 1
      case 2: // custom colormap 2
      case 3: // custom colormap 3
         setpalette( &pal_custom[data-1][0], 0x100 );
         break;
      case 4: // C64 colormap
         setpalette( &pal_c64[0], count_of(pal_c64) );
         break;
      case 5: // C16 colormap
         setpalette( &pal_c16[0], count_of(pal_c16) );
         break;
      case 6: // Atari 800 colormap
         setpalette( &pal_a800[0], count_of(pal_a800) );
         break;
      case 7: // Atari 800 colormap, SvOlli style
         {
            uint16_t c, i;
            uint8_t r, g, b;

            for( c = 0; c < 0x100; ++c )
            {
               if( c & 0x08 )
               {
                  i = (c & 0xf0) | (0x0f - ((c & 0x07) << 1));
               }
               else
               {
                  i = (c & 0xf0) | ((c & 0x07) << 1);
               }
               r = (pal_a800[i] & 0xf00) >> 8;
               g = (pal_a800[i] & 0x0f0) >> 4;
               b = (pal_a800[i] & 0x00f) >> 0;
               hardware_setcolor( c, r, g, b );
            }
         }
         break;
      default:
         break;
   }
}


static inline void setcustomcolor( uint8_t id, uint8_t rgb,
                                   uint8_t pos, uint8_t value )
{
   switch( rgb )
   {
      case 0: // r
         pal_custom[id][pos] = pal_custom[id][pos] & 0x0FF | ((value & 0x0F) << 8);
         break;
      case 1: // g
         pal_custom[id][pos] = pal_custom[id][pos] & 0xF0F | ((value & 0x0F) << 4);
         break;
      case 2: // b
         pal_custom[id][pos] = pal_custom[id][pos] & 0xFF0 |  (value & 0x0F);
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
   uint8_t  customid = 0x01;
   uint16_t colidx[3]= { 0, 0, 0 };
   int8_t   custom_color_palette_step = 1;

   for(;;)
   {
      bus     = multicore_fifo_pop_blocking();
      address = bus >> BUS_CONFIG_shift_address;
      data    = bus >> BUS_CONFIG_shift_data;
      if( bus & BUS_CONFIG_mask_rw )
      {
         continue;
      }

      switch( address & 0x1f )
      {
         case 0x00: // copy data with flush
            if( data )
            {
               dmacopytrans( &framebuffer[destaddr], &mem_cache[srcaddr],
                                width, height, step, data, tcol );
            }
            else
            {
               dmacopy( &framebuffer[destaddr], &mem_cache[srcaddr],
                           width, height, step );
            }
            hardware_flush();
            break;
         case 0x01: // copy data without flush
            if( data )
            {
               dmacopytrans( &framebuffer[destaddr], &mem_cache[srcaddr],
                                width, height, step, data, tcol );
            }
            else
            {
               dmacopy( &framebuffer[destaddr], &mem_cache[srcaddr],
                           width, height, step );
            }
            break;
         case 0x02: // src addr low
            srcaddr  =  (srcaddr & 0xFF00) | data;
            break;
         case 0x03: // src addr high
            srcaddr  =  (srcaddr & 0x00FF) | (data << 8);
            break;
         case 0x04: // dest addr low
            destaddr = (destaddr & 0x0300) | data;
            break;
         case 0x05: // dest addr high
            destaddr = (destaddr & 0x00FF) | ((data & 0x03) << 8);
            break;
         case 0x06: // dest x
            destaddr = (destaddr & 0x03E0) | (data & 0x1F);
            break;
         case 0x07: // dest y
            destaddr = (destaddr & 0x001F) | ((data & 0x1F) << 5);
            break;
         case 0x08: // width
            width    = (data & 0x1F);
            break;
         case 0x09: // height
            height   = (data & 0x1F);
            break;
         case 0x0a: // step
            step     = data;
            break;
         case 0x0b: // transparent color index
            tcol     = data;
            break;
         case 0x0c: // set colormap
            setcolormap( data );
            break;
         case 0x10: // set custom colormap index
            if( ((data & 0x7f) >= 1) && ((data & 0x7f) <= 3) )
            {
               customid  = data & 3;
               if( data & 0x80 )
               {
                  colidx[0] = 0xFF;
                  colidx[1] = 0xFF;
                  colidx[2] = 0xFF;
                  custom_color_palette_step = -1;
               }
               else
               {
                  colidx[0] = 0;
                  colidx[1] = 0;
                  colidx[2] = 0;
                  custom_color_palette_step = 1;
               }
            }
            else
            {
               customid  = 0;
            }
            break;
         case 0x11: // set custom colormap red
         case 0x12: // set custom colormap green
         case 0x13: // set custom colormap blue
            if( customid && (colidx[(address&3)-1] < 0x100) )
            {
               setcustomcolor( customid-1, (address&3)-1,
                               colidx[(address&3)-1], data );
               colidx[(address&3)-1] += custom_color_palette_step;
            }
            break;
         default:
            break;
      }
   }
}
