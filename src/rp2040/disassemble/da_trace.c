/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */


/*
 * expected lines of output:

 1014:e28b r b0    :7:BCS  $E288                                               
 1015:e28c r fb    :0:
 1016:e28d r 20    :0:
 1017:e288 r 20    :7:JSR  $FF00                                               
 1018:e289 r 00    :0:
 1019:e28a r ff    :0:                                                         
 1020:e28a r ff    :0:
 1021:01fd w e2    :0:                                                         
 1022:01fc w 8a    :0:                                                         

 */

#include "da_trace.h"
#include "da_platform.h"
#include "base_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_OUTPUT (0)
#define BOUNDSBUFFER (16)
#if (BOUNDSBUFFER < DA_CPU_MAXCYCLES)
#error BOUNDSBUFFER must be at least the size of max cpu cycles
#endif


/* fill fullinfo from ringbuffer */
static inline void _da_trace_fill( da_trace_t d,
                                   const uint32_t *trace, uint32_t start )
{
   /* d->fullinfo should be zeroed out */
   da_fullinfo_t  *fullinfo   = d->fullinfo;
   const uint32_t entries     = d->entries;
   int            offset;
   int            i;
   int            n;
   uint16_t       expectedaddress;

   /* let's do this in a 2 pass fashion, a bit like an assembler */

   /* pass 1: just copy the raw data in correct order */
   for( i = 0, n = start; i < entries; ++i, ++n )
   {
      /* wrap ringbuffer at end */
      if( n >= entries )
      {
         n = 0;
      }

      /* set up bits 31-0: flags, data, address */
      fullinfo[i].raw   = trace[n];
   }

   /* pass 2: now, let's fill in additional data */
   for( i = 0; i < entries; ++i )
   {
      /* find arguments */
      expectedaddress   = fullinfo[i].address+1;
      offset = 0;
      for( n = 1; n < DA_CPU_MAXCYCLES; ++n )
      {
         if( fullinfo[i+n].address == expectedaddress )
         {
            switch( ++offset )
            {
               case 1:
                  fullinfo[i].data1 = fullinfo[i+n].data;
                  break;
               case 2:
                  fullinfo[i].data2 = fullinfo[i+n].data;
                  break;
               case 3:
                  fullinfo[i].data3 = fullinfo[i+n].data;
                  /* found all parameters, exit from for loop */
                  n = DA_CPU_MAXCYCLES;
                  break;
               default:
                  break;
            }
            ++expectedaddress;
         }
      }
      fullinfo[i].dataused = min( offset, 3 );
   }
}


da_trace_t da_trace_init( cputype_t cpu,
                          uint32_t *ringbuffer,
                          uint32_t entries,
                          uint32_t start )
{
   da_fullinfo_t *fullinfo =
      (da_fullinfo_t*)ht_calloc( entries + 2 * BOUNDSBUFFER,
                                 sizeof(da_fullinfo_t) );
   /* make sure that da_trace_t is zeroed out on creation */
   da_trace_t d = (da_trace_t)ht_calloc( 1, sizeof(*d) );
   d->cpu       = cpu;
   d->entries   = entries;
   d->fullinfo  = fullinfo + BOUNDSBUFFER;
   _da_trace_fill( d, ringbuffer, start );
   return d;
}


void da_trace_done( da_trace_t d )
{
   da_fullinfo_t *fullinfo = d->fullinfo - BOUNDSBUFFER;
   memset( fullinfo, 0, (d->entries + 2 * BOUNDSBUFFER) * sizeof(da_fullinfo_t) );
   ht_free( fullinfo );
   memset( d, 0, sizeof(*d) );
   ht_free( d );
}

