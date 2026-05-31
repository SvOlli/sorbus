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

#include "../common/generic_helper.h"


da_memory_t da_memory_init()
{
   /* make sure that da_trace_t is zeroed out on creation */
   da_memory_t d = (da_memory_t)ht_calloc( 1, sizeof(*d) );
   
   return d;
}


void da_memory_done( da_memory_t d )
{
   ht_free( d );
}


void da_memory_set_cpu( da_memory_t d, cputype_t cpu )
{
   d->cpu = cpu;
   switch( cpu )
   {
      case CPU_6502RA:
      case CPU_6502:
         d->opcodes = &da_opcodes6502[0];
         break;
      case CPU_65SC02:
         d->opcodes = &da_opcodes65sc02[0];
         break;
      case CPU_65C02:
         d->opcodes = &da_opcodes65c02[0];
         break;
      case CPU_65816:
         d->opcodes = &da_opcodes65816[0];
         break;
      case CPU_65CE02:
         d->opcodes = &da_opcodes65ce02[0];
         break;
      default:
         fprintf( stderr, __FILE__
                  "(%d): internal error: 0x%02x\n",
                  __LINE__, cpu );
         d->opcodes = 0;
   }
}


void da_memory_set_mx816( da_memory_t d, bool m, bool x )
{
   d->m = m;
   d->x = x;
}


void da_memory_set_address( da_memory_t d, uint8_t bank, uint16_t address )
{
   d->bank    = bank;
   d->address = address;
}


void da_memory_set_datatype( da_memory_t d, uint16_t addr, da_data_type_t t )
{
   d->type[addr] = t;
}


uint16_t da_memory_next( da_memory_t d )
{
   if( d->bytemode )
   {
      return d->address + 1;
   }
   return d->address + da_pick_bytes( d->cpu, d->peek( d->bank, d->address ) );
}


uint16_t da_memory_prev( da_memory_t d )
{
   if( d-> bytemode )
   {
      return d->address - 1;
   }
   return d->address - 1;
}


da_fullinfo_t da_memory_fullinfo( da_memory_t d, uint16_t address )
{
   da_fullinfo_t fullinfo;
   fullinfo.raw = 0x3F000000; /* sane defaults: all flags high */

   fullinfo.address  = d->address;
   fullinfo.data     = d->peek( d->bank, d->address   );
   fullinfo.data1    = d->peek( d->bank, d->address+1 );
   fullinfo.data2    = d->peek( d->bank, d->address+2 );
   fullinfo.data3    = d->peek( d->bank, d->address+3 );
   fullinfo.dataused = 3;
   fullinfo.eval     = 7;

   if( d->cpu == CPU_65816 )
   {
      fullinfo.n816  = true;
      fullinfo.m816  = d->m;
      fullinfo.x816  = d->x;
   }

   switch( d->type[address] )
   {
      case TYPE_UNCHECKED:
         break;
      case TYPE_GUESSED_DATA:
      case TYPE_MANUAL_DATA:
         fullinfo.eval     = DA_EVAL_MIN;
         break;
      case TYPE_GUESSED_CODE:
      case TYPE_MANUAL_CODE:
         fullinfo.eval     = DA_EVAL_MAX;
         break;
      default:
         fprintf( stderr, __FILE__
                  "(%d): internal error: $%04x 0x%02x\n",
                  __LINE__, address, d->type[address] );
   }
   return fullinfo;
}
