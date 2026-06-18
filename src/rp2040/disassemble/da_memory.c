/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */


/*
 * expected lines of output:

$E032: 0D 4E 4D    ORA  $4D4E                                                   
$E035: 4F 53 20 36 EOR  $362053                                                 
$E039: 35 30       AND  $30,X                                                   

 * or:

$E050: 38 ('8')    SEC
$E051: 60 ('`')    RTS
$E052: A5 ('<UTF8: Yen>')    LDA  $2A
$E053: 2A ('*')

*/

#include "da_memory.h"

#include <stdio.h>
#include <string.h>

#include "../common/generic_helper.h"


da_memory_t da_memory_init()
{
   /* make sure that da_trace_t is zeroed out on creation */
   da_memory_t d = (da_memory_t)ht_calloc( 1, sizeof(*d) );

   return d;
}


void da_memory_done( da_memory_t d )
{
   if( d->linecache )
   {
      memset( d->linecache, 0, d->lines * sizeof(uint16_t) );
      ht_free( d->linecache );
   }
   memset( d, 0, sizeof(*d) );
   ht_free( d );
}


void da_memory_linecache( da_memory_t d, uint16_t lines )
{
   d->lines       = lines;
   d->linecache   = (uint16_t*)ht_realloc( d->linecache, lines * sizeof(uint16_t) );
   memset( d->linecache, 0, lines * sizeof(uint16_t) );
}


uint16_t da_memory_findprev( da_memory_t d, uint16_t address )
{
   uint16_t a = 0, preva = 0;
   int i;

   for( i = 7; i >= 0; --i )
   {
      a = (address-0x20) + i;
      while( a < address )
      {
         preva = a;
         a += da_pick_bytes( d->cpu, d->peek( d->bank, a ) ) +
              da_is_imm16_mx( d->cpu, d->peek( d->bank, d->address ),
                              d->m816, d->x816 );

         if( a == address )
         {
            return preva;
         }
      }
   }
   return address - (d->cpu == CPU_65816 ? 4 : 3);
}


void da_memory_next( da_memory_t d, uint16_t steps )
{
   uint16_t i;
   for( i = 0; i < steps; ++i )
   {
      d->address += da_pick_bytes( d->cpu, d->peek( d->bank, d->address ) ) +
                    da_is_imm16_mx( d->cpu, d->peek( d->bank, d->address ),
                                    d->m816, d->x816 );
   }
}


void da_memory_prev( da_memory_t d, uint16_t steps )
{
   uint16_t i;
   for( i = 0; i < steps; ++i )
   {
      d->address = da_memory_findprev( d, d->address );
   }
}


da_fullinfo_t da_memory_fullinfo( da_memory_t d, int16_t offset )
{
   da_fullinfo_t fullinfo = { 0 };
   uint16_t i, a;

   if( !offset )
   {
      a = d->address;
      /* first line calculate the address for all lines */
      for( i = 0; i < d->lines; ++i )
      {
         d->linecache[i] = a;
         a += da_pick_bytes( d->cpu, d->peek( d->bank, a ) ) +
              da_is_imm16_mx( d->cpu, d->peek( d->bank, a ), d->m816, d->x816 );
      }
   }

   if( (offset >= 0) && (offset < d->lines) )
   {
      a = d->linecache[offset];
      fullinfo.raw = 0x3F000000; /* sane defaults: all flags high */

      fullinfo.address  = a;
      fullinfo.data     = d->peek( d->bank, a   );
      fullinfo.data1    = d->peek( d->bank, a+1 );
      fullinfo.data2    = d->peek( d->bank, a+2 );
      fullinfo.data3    = d->peek( d->bank, a+3 );
      fullinfo.dataused = 3;
      fullinfo.eval     = DA_EVAL_MAX;

      if( d->cpu == CPU_65816 )
      {
         fullinfo.n816  = true;
         fullinfo.m816  = d->m816;
         fullinfo.x816  = d->x816;
      }
   }

   return fullinfo;
}


da_fullinfo_t da_memory_single( cputype_t cpu, peek_t peek,
                     uint8_t bank, uint16_t address, bool m, bool x )
{
   da_fullinfo_t fullinfo;
   fullinfo.raw = 0x3F000000; /* sane defaults: all flags high */

   fullinfo.address  = address;
   fullinfo.data     = peek( bank, address   );
   fullinfo.data1    = peek( bank, address+1 );
   fullinfo.data2    = peek( bank, address+2 );
   fullinfo.data3    = peek( bank, address+3 );
   fullinfo.dataused = 3;
   fullinfo.eval     = DA_EVAL_MAX;

   if( cpu == CPU_65816 )
   {
      fullinfo.n816  = true;
      fullinfo.m816  = m;
      fullinfo.x816  = x;
   }
   return fullinfo;
}
