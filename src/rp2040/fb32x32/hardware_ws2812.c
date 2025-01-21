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

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "fb32x32.h"
#include "ws2812.pio.h"

#define WS2812_PIN 29

bi_decl(bi_program_name("Sorbus Computer 32x32 WS2812"))
bi_decl(bi_program_description("Driver for a 6502asm style LED matrix"))

bi_decl(bi_1pin_with_name(WS2812_PIN, "WS2812_DATA"));

extern const unsigned int translation_matrix[0x400];

uint32_t colortab[0x100];

static uint8_t brightness = 0;


void hardware_setcolor( uint8_t index, uint8_t r, uint8_t g, uint8_t b )
{
   brightness &= 0x03;
   r &= 0x0F;
   g &= 0x0F;
   b &= 0x0F;

   colortab[index] = ((r << 16) | (g << 24) | (b << 8)) << brightness;
}


void hardware_init()
{
   PIO pio = pio0;
   int sm = 0;
   uint offset = pio_add_program(pio, &ws2812_program);

   // slightly "overclocking" WS2812 from .8MHz to 1MHz, works for me
   ws2812_program_init(pio, sm, offset, WS2812_PIN, 1000000, false);
}


void hardware_flush()
{
   int i;
   for( i = 0; i < 0x400; ++i )
   {
      pio_sm_put_blocking( pio0, 0, colortab[framebuffer[translation_matrix[i]]] );
   }
}
