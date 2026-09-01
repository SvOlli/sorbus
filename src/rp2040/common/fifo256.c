/**
 * Copyright (c) 2026 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * a very minimalistic fifo implementation
 * - doesn't care about multithreading
 * - 256 bytes fifo fixed
 * - fast
 * - can be inlined
 * - intended to be used for uart communication
 */

#ifdef FIFO256_INLINE
#define STATIC_INLINE static inline
#else
#include "fifo256.h"
#define STATIC_INLINE /*static inline*/
#endif

#include "generic_helper.h"

#include <stdio.h>

#define FIFO256_SIZE (256)


STATIC_INLINE int fifo256_debug( char *b, size_t bsize, fifo256_t *fifo, bool verbose )
{
   int used = 0;
   used += snprintf( b+used, bsize-used,
                     "head=0x%02x tail=0x%02x\n", fifo->head, fifo->tail );
   if( verbose )
   {
      used += snprint_hexdump_buffer( b+used, bsize-used, &(fifo->buffer[0]),
                                      FIFO256_SIZE, 0, false );
   }
   return used;
}


STATIC_INLINE void fifo256_init( fifo256_t *fifo )
{
   fifo->head = 0;
   fifo->tail = 0;
}


STATIC_INLINE uint8_t fifo256_count( const fifo256_t *fifo )
{
   return (uint8_t)( fifo->head - fifo->tail );
}


STATIC_INLINE uint8_t fifo256_free( const fifo256_t *fifo )
{
   return (uint8_t)(FIFO256_SIZE - 1 - fifo256_count( fifo ));
}


STATIC_INLINE bool fifo256_put( fifo256_t *fifo, uint8_t data )
{
   uint8_t next_head = fifo->head;
   if( (++next_head) == fifo->tail )
   {
      return false; /* buffer full */
   }
   fifo->buffer[fifo->head] = data;
   fifo->head = next_head;
   return true;
}


STATIC_INLINE bool fifo256_get( fifo256_t *fifo, uint8_t *data )
{
   if( fifo->head == fifo->tail )
   {
      return false; /* buffer empty */
   }
   *data = fifo->buffer[fifo->tail];
   fifo->buffer[fifo->tail] = 0;
   ++(fifo->tail);
   return true;
}

#ifdef FIFO256_INLINE
#undef STATIC_INLINE
#endif

