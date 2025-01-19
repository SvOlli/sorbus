/**
 * Copyright (c) 2025 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a 32x32 pixel framebuffer
 * for the Sorbus Computer
 */

#ifndef FB32X32_H
#define FB32X32_H FB32X32_H

#include <stdint.h>

/* data shared for speed */

/* memory as sniffed from the bus */
/* bus writes, control reads */
/* size used: 0x10000, rest is to prevent out-of-bounds access */
extern uint8_t mem_cache[0x12000];

/* framebuffer to be displayed */
/* control writes hardware reads */
/* size used: 0x400, rest is to prevent out-of-bounds access */
extern uint8_t framebuffer[0x800];


/* functions */

/*  */
void bus_init();

/*  */
void control_init();

/* initialize PIO machine, must be run after bus_init() */
void hardware_init();

/* sniffs the bus and feeds control data to control_loop() */
void bus_loop();

/* processes control data providid by bus_loop() */
void control_loop();

/* define entry in color table */
/* rgb values are (lower) 4 bit only */
void hardware_setcolor( uint8_t index, uint8_t r, uint8_t g, uint8_t b );

/* write framebuffer to LEDs */
void hardware_flush();

#endif
