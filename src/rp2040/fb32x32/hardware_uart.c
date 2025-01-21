/**
 * Copyright (c) 2025 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a 32x32 pixel framebuffer
 * for the Sorbus Computer
 */

/*
 * Protocol:
 * $00: reset (1 byte)
 * $01: data (1024 bytes)
 * - 1024 x 8-bit color index
 * $02: color (4 bytes)
 * - 8-bit index
 * - 8-bit r (only lower 4 bit used)
 * - 8-bit g b (upper 4 bit g, lower b)
 */

/*
 * untested prototype
 * TODO:
 * - set baudrate(?)
 * - write client
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "fb32x32.h"

bi_decl(bi_program_name("Sorbus Computer 32x32 USB UART"))
bi_decl(bi_program_description("Driver for a 6502asm style USB UART simulator"))

uint32_t colortab[0x100];

#define RESET  (0x00)
#define PIXELS (0x01)
#define COLOR  (0x02)


void hardware_setcolor( uint8_t index, uint8_t r, uint8_t g, uint8_t b )
{
   r &= 0x0F;
   g &= 0x0F;
   b &= 0x0F;

   putchar( COLOR );
   putchar( index );
   putchar( r );
   putchar( (g << 4) | b );
}


void hardware_init()
{
   // just make sure we aren't in data transmission
   int i;
   for( i = 0; i < 0x400; ++i )
   {
      putchar( RESET );
   }
}


void hardware_flush()
{
   int i;
   putchar( PIXELS );
   for( i = 0; i < 0x400; ++i )
   {
      putchar( framebuffer[i] );
   }
}
