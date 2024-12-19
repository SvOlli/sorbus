/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"


#define WS2812_PIN 29

uint8_t led_framebuffer[0x400];
uint32_t colortab[0x100];
extern unsigned int translation_matrix[0x400];

uint32_t andvalues[4] = {
   0x7f7f7f7f, 0x3f3f3f3f, 0x1f1f1f1f, 0x0f0f0f0f
};


uint32_t pal_pepto_pal[16] = {
   0x000000, 0xffffff, 0x68372b, 0x70a4b2, 0x6f3d86, 0x588d43, 0x352879, 0xb8c76f,
   0x6f4f25, 0x433900, 0x9a6759, 0x444444, 0x6c6c6c, 0x9ad284, 0x6c5eb5, 0x959595
};

uint32_t pal_pepto_ntsc[16] = {
   0x000000, 0xffffff, 0x67372b, 0x70a3b1, 0x6f3d86, 0x588c42, 0x342879, 0xb7c66e,
   0x6f4e25, 0x423800, 0x996659, 0x434343, 0x6b6b6b, 0x9ad183, 0x6b5eb5, 0x959595
};

uint32_t pal_vice[16] = {
   0x000000, 0xfdfefc, 0xbe1a24, 0x30e6c6, 0xb41ae2, 0x1fd21e, 0x211bae, 0xdff60a,
   0xb84104, 0x6a3304, 0xfe4a57, 0x424540, 0x70746f, 0x59fe59, 0x5f53fe, 0xa4a7a2
};

uint32_t pal_svolli[16] = {
   0x000000, 0xffffff, 0xbe1a24, 0x30e6c6, 0xb41ae2, 0x1fd21e, 0x211bae, 0xdff60a,
   0xb84104, 0x6a3304, 0xfe4a57, 0x202020, 0x404040, 0x59fe59, 0x5f53fe, 0x808080
};


void led_init()
{
   PIO pio = pio0;
   int sm = 0;
   uint offset = pio_add_program(pio, &ws2812_program);

   ws2812_program_init(pio, sm, offset, WS2812_PIN, 1000000, false);
}


static inline void mkc64pal( uint32_t *pal )
{
   int c;
   for( c = 0; c < 0x100; ++c )
   {
      /* target format 0xGGRRBB00 */
      colortab[c] =
         ((pal[c & 0x0f] & 0xff0000)      ) |
         ((pal[c & 0x0f] & 0x00ff00) << 16) |
         ((pal[c & 0x0f] & 0x0000ff) <<  8);
   }
}


void led_setcolors( uint8_t tab, uint8_t bright )
{
   tab &= 0x0F;
   bright &= 0x03;

   uint32_t c, r, g, b, i;

   switch( tab )
   {
      case 0:
         for( c = 0; c < 0x100; ++c )
         {
            // convert 0bRRGGBBII to 0bGGII0000RRII0000BBII0000

            r = (c & 0xc0) >> 6;
            g = (c & 0x30) >> 4;
            b = (c & 0x0c) >> 2;
            i = (c & 0x03) >> 0;

            colortab[c] =
               (r << 22) |
               (g << 30) |
               (b << 14) |
               (i << 28) | (i << 20) | (i << 12);
         }
         break;
      case 6:
         mkc64pal( &pal_svolli[0] );
         break;
      case 7:
         mkc64pal( &pal_vice[0] );
         break;
      case 8:
         mkc64pal( &pal_pepto_pal[0] );
         break;
      case 9:
         mkc64pal( &pal_pepto_ntsc[0] );
         break;
      default:
         printf( "fail: unknown colortab\n" );
         break;
   }

   for( c = 0; c < 0x100; ++c )
   {
      colortab[c] >>= (1+bright);
      colortab[c] &= andvalues[bright];
   }
}


void led_flush()
{
   int i;
   for( i = 0; i < 0x400; ++i )
   {
      pio_sm_put_blocking(pio0, 0, colortab[led_framebuffer[translation_matrix[i]]]);
   }
}
