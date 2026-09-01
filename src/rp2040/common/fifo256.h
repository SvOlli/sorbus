/**
 * Copyright (c) 2026 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _FIFO256_H_
#define _FIFO256_H_ _FIFO256_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
   uint8_t buffer[0x100];
   volatile uint8_t head; // set by producer
   volatile uint8_t tail; // set by consumer
} fifo256_t;

/* just reset head and tail */
void fifo256_init( fifo256_t *fifo );
/* Returns the number of bytes currently stored (0..255) */
uint8_t fifo256_count( const fifo256_t *fifo );
/* Returns available space (0..255) */
uint8_t fifo256_free( const fifo256_t *fifo );
/* Push a single byte into the fifo */
bool fifo256_put( fifo256_t *fifo, uint8_t data );
/* Pop a single byte from the fifo */
bool fifo256_get( fifo256_t *fifo, uint8_t *data );
/* Print out the current state of the fifo for debugging */
int fifo256_debug( char *b, size_t bsize, fifo256_t *fifo, bool verbose );

#endif

