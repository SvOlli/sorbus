
#include "disassemble.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

typedef enum {
   CPU65816_M16_X16 = 0x00,
   CPU65816_M16_X8  = 0x10,
   CPU65816_M8_X16  = 0x20,
   CPU65816_M8_X8   = 0x30,
} cpu65816_mx_t;

/* variables shared between all functions */
#define SAMPLESIZE (10)
uint32_t sample[SAMPLESIZE];
cpu65816_mx_t cpu65816_mx;

/*
 * 16 bit in 65816 is hard to detect from trace
 * in emulation mode SEP/REP have no effect on M/X flags
 * best chance: determine 16bit mode from access pattern
 * and save it for next time
 */


static bool bc_pagecross( uint16_t p0, uint16_t p1 )
{
   return (p0 >> 8) != (p1 >> 8);
}


static uint8_t bc_jump_rel8( disass_fulltrace_t d, uint32_t pos )
{
   fullinfo_t *fullinfo = d->fullinfo;

   /* sanity checks */
   uint16_t dest = fullinfo[pos].address + 2; // origin of relative branch

   /* first check if access pattern matches */
   if( ((fullinfo[pos+0].address + 1) != fullinfo[pos+1].address) ||
       ((fullinfo[pos+1].address + 1) != fullinfo[pos+2].address) )
   {
      return 0x80;   /* error */
   }

   dest += (int8_t)fullinfo[pos+1].data;
   //dest += (int8_t)fullinfo[pos].data1;
   if( fullinfo[pos+3].address == dest )
   {
      return 3;      /* branch taken */
   }
   if( (fullinfo[pos+4].address == dest) &&
       bc_pagecross( fullinfo[pos].address+2, dest ) )
   {
      return 4;      /* branch taken with page crossing */
   }
   return 2;         /* no branch */
}


/* returns 0, if no interrupt/reset, else clockcycles required */
static uint8_t bc_interrupt_65816( disass_fulltrace_t d, uint32_t pos )
{
   return 0; /* not implemented for now */
}


static uint8_t bc_vectorpull( disass_fulltrace_t d, uint32_t pos, uint16_t addr, bool rw )
{
   fullinfo_t *fullinfo = d->fullinfo;
   uint16_t vector = 0;

/* typical IRQ (65C02):
   0:0445 r 4c    :JMP  $0445 ; gets interrupted
   1:0445 r 4c    :
   2:01eb w 04    :
   3:01ea w 45    :
   4:01e9 w 21    :
   5:fffe r 5d    :
   6:ffff r 04    :
   7:045d r 48    :PHA
*/
   /* check 1: stack access (TODO: check stack wrapping) */
   if( ((fullinfo[pos+2].address) != (fullinfo[pos+3].address+1)) ||
       ((fullinfo[pos+3].address) != (fullinfo[pos+4].address+1)) ||
       (fullinfo[pos+2].rw != rw) ||
       (fullinfo[pos+3].rw != rw) ||
       (fullinfo[pos+4].rw != rw) )
   {
      return 0;
   }
   /* check 2: vector access */
   if( (fullinfo[pos+5].address != (addr+0)) ||
       (fullinfo[pos+6].address != (addr+1)) )
   {
      return 0;
   }
   vector = fullinfo[pos+5].data | (fullinfo[pos+6].data << 8);
   /* check 3: vector taken */
   if( vector != fullinfo[pos+7].address )
   {
      return 0;
   }
   return 7;
}


/* returns 0, if no interrupt/reset, else clockcycles required */
static uint8_t bc_interrupt( disass_fulltrace_t d, uint32_t pos )
{
   //fullinfo_t *fullinfo = d->fullinfo;
   //uint16_t address0 = fullinfo[pos+0].address;
   //uint16_t address1 = fullinfo[pos+1].address;

   uint8_t cycles = 0;

   /* for simplicity moved to own function */
   if( d->cpu == CPU_65816 )
   {
      cycles = bc_interrupt_65816( d, pos );
   }

   if( !cycles )
   {
      /* check NMI */
      cycles = bc_vectorpull( d, pos, 0xFFFA, false );
   }

   if( !cycles )
   {
      /* check IRQ */
      cycles = bc_vectorpull( d, pos, 0xFFFE, false );
   }

   if( !cycles )
   {
      /* check reset */
      /* reset reads from stack instead of write, except for 65SC02 */
      cycles = bc_vectorpull( d, pos, 0xFFFC, d->cpu != CPU_65SC02 );
   }

   return cycles;
}


static uint8_t bc_jump( disass_fulltrace_t d, uint32_t pos )
{
   fullinfo_t  *fullinfo      = d->fullinfo;

   switch( pick_mnemonic( d, pos ) )
   {
      case BCC:
      case BCS:
      case BEQ:
      case BMI:
      case BNE:
      case BPL:
      case BRA:   /* not on NMOS 6502 */
      case BVC:
      case BVS:
         return bc_jump_rel8( d, pos );
      case JMP:
         /* bugfix within 65(S)C02
          * 0:0400 r 6c    :JMP  ($04FF)
          * 1:0401 r ff    :
          * 2:0402 r 04    :
          * 3:0402 r 04    : !! extra cycle for adjusting pagecross !!
          * 4:04ff r 03    :
          * 5:0500 r 04    :
          */
         if( fullinfo[pos+2].address == fullinfo[pos+3].address )
         {
            return pick_cycles( d, pos )+1;
         }
      default:
         break;
   }
   return pick_cycles( d, pos );
}


static uint8_t bc_6502( disass_fulltrace_t d, uint32_t pos )
{
   fullinfo_t  *fullinfo      = d->fullinfo;
   //uint16_t    address        = fullinfo[pos].address;
   //uint16_t    next_address   = pick_bytes( d, pos ) + address;
   uint8_t     cycles         = pick_cycles( d, pos );

   if( pick_extra( d, pos ) )
   {
      if( pick_jump( d, pos ) )
      {
         return bc_jump( d, pos );
      }
      else
      {
         if( pick_addrmode( d, pos ) == ZPIY )
         {
            if( (fullinfo[pos+4].address+0x100) == (fullinfo[pos+5].address) )
            {
               /* on 6502 page crossing reads first without highbyte carried over */
               ++cycles;
            }
         }
         else
         {
            if( (fullinfo[pos+3].address+0x100) == (fullinfo[pos+4].address) )
            {
               /* on 6502 page crossing reads first without highbyte carried over */
               ++cycles;
            }
         }
      }
   }
   return cycles;
}


static uint8_t bc_65c02( disass_fulltrace_t d, uint32_t pos )
{
   fullinfo_t  *fullinfo      = d->fullinfo;
   //uint16_t    address        = fullinfo[pos].address;
   //uint16_t    next_address   = pick_bytes( d, pos ) + address;
   uint8_t     cycles         = pick_cycles( d, pos );

   if( pick_jump( d, pos ) )
   {
      return bc_jump( d, pos );
   }

   if( pick_extra( d, pos ) )
   {
      switch( pick_addrmode( d, pos ) )
      {
         case ABSX:
         case ABSY:
            /* check for extra cycle during because of page crossing
             * 0:0402 r b9    :LDA  $0480,Y
             * 1:0403 r 80    :
             * 2:0404 r 04    :
             * 3:0404 r 04    : !! pagecross extra cycle !!
             * 4:057f r 00    :
             */
            if( fullinfo[pos+2].address == fullinfo[pos+3].address )
            {
               ++cycles;
            }
            /* check for something not found in documentation:
             * 0:0402 r fe    :INC  $0500,X
             * 1:0403 r 00    :
             * 2:0404 r 05    :
             * 3:0500 r 02    :
             * 4:0500 r 02    :
             * 5:0500 r 02    : !! a third access !! why ??
             * 6:0500 w 03    :
             */
            else if( (fullinfo[pos+3].address == fullinfo[pos+4].address) &&
                     (fullinfo[pos+3].address == fullinfo[pos+5].address) &&
                     (fullinfo[pos+3].address == fullinfo[pos+6].address) )
            {
               ++cycles;
            }
            break;
         case ZPIY:
            /* check for extra cycle during because of page crossing
             * 0:0402 r b1    :LDA  ($10),Y
             * 1:0403 r 10    :
             * 2:0010 r 80    :
             * 3:0011 r 04    :
             * 4:0011 r 04    : !! pagecross extra cycle !!
             * 5:057f r 00    :
             */
            if( fullinfo[pos+3].address == fullinfo[pos+4].address )
            {
               ++cycles;
            }
            break;
         default:
            /* no command with page crossing */
            break;
      }
   }

   switch( pick_mnemonic( d, pos ) )
   {
      case ADC:
      case SBC:
         /* BCD takes an extra cycle here
          *  :0400 r f8    :SED
          *  :0401 r e9    :
          * 0:0401 r e9    :SBC  #$20
          * 1:0402 r 20    :
          * 2:0403 r d8    : !! extra cycle for adjusting flags according to BCD !!
          * 3:0403 r d8    :CLD
          */
         if( fullinfo[pos+2].address == fullinfo[pos+3].address )
         {
            ++cycles;
         }
         break;
      default:
         break;
   }

   return cycles;
}


static uint8_t bc_65ce02( disass_fulltrace_t d, uint32_t pos )
{
   /*
    * From page 7 of http://archive.6502.org/datasheets/mos_65ce02_mpu.pdf
    * "Note that the number of machine cycles for every instruction remains
    *  fixed regardless of decimal mode and page boundries"
    */
   return pick_cycles( d, pos );
}


static uint8_t bc_65816( disass_fulltrace_t d, uint32_t pos )
{
   /* a lot of to do here, but this is better than nothing */
   return pick_cycles( d, pos );
}


#if 0
uint32_t disassemble_beancounter_findreset( cputype_t cputype, 
   uint32_t *trace, uint32_t entries, uint32_t current )
{

/* typical reset (6502, 65C02)
   0:8aff r 00    :
   1:0089 r d0    :
   2:d0ff r 00    :
   3:d0ff r 00    :
   4:0100 r 18    :
   5:01ff r 00    :
   6:01fe r a2    :
   7:fffc r 1e    :
   8:fffd r 00    :
   9:001e r a2    :LDX  #$00
*/

/* reset 65SC02
  0:0001 r 5c    :
  1:0001 r 5c    :
  2:0001 r 5c    :
  3:0100 w 00    :
  4:01ff w 01    :
  5:01fe w 62    :
  6:fffc r 1e    :
  7:fffd r 00    :
  8:001e r a2    :LDX  #$00
*/

/* reset 65CE02
  TODO: check with 16 bit SP and SP != $01xx
  0:01f8 r 86    :
  1:01f8 r 86    :
  2:01f8 r 86    :
  3:01f7 r e8    :
  4:01f6 r e8    :
  5:fffc r 1e    :
  6:fffd r 00    :
  7:001e r a2    :LDX  #$00
*/

/* reset 65816: same as 65SC02, but reads instead of writes
  0:001b r 4c    :
  1:001b r 4c    :
  2:001b r 4c    :
  3:01ee r 90    :
  4:01ed r 05    :
  5:01ec r b0    :
  6:fffc r 1e    :
  7:fffd r 00    :
  8:001e r a2    :LDX  #$00
*/
   uint32_t i;
   bool found = false;

   for( i = 0; i < entries; ++i )
   {
      uint32_t addr5 = trace_address( trace[re(current-5)] );
      uint32_t addr4 = trace_address( trace[re(current-4)] );
      uint32_t addr3 = trace_address( trace[re(current-3)] );
      uint32_t addr2 = trace_address( trace[re(current-2)] );
      uint32_t addr1 = trace_address( trace[re(current-1)] );
      uint16_t addr0 = trace_address( trace[re(current-0)] );
      uint16_t data_addr = trace_address( trace[re(data-2)] ) | (trace_address( trace[re(data-1)] << 8) );
      
      if( (addr1 == 0xFFFD) && (addr2 == 0xFFFC) &&
          (addr0 == data_addr) )
      {
      }
   }

   return 0xFFFFFFFF;
}
#endif


uint8_t disassemble_cycles( disass_fulltrace_t d, uint32_t pos )
{
   uint8_t is_interrupt;   /* and also reset */

   is_interrupt = bc_interrupt( d, pos );
   if( is_interrupt > 0 )
   {
      return is_interrupt | 0xF0;
   }

   switch( d->cpu )
   {
      case CPU_6502:
      case CPU_6502RA:
         return bc_6502( d, pos );
      case CPU_65SC02:
      case CPU_65C02:
         return bc_65c02( d, pos );
      case CPU_65CE02:
         return bc_65ce02( d, pos ); // 100% done
      case CPU_65816:
         return bc_65816( d, pos );
      default:
         return 0x80;
   }
}


uint8_t disassemble_beancounter_single( disass_fulltrace_t d, uint32_t pos )
{
   fullinfo_t  *fullinfo      = d->fullinfo;
   uint8_t     cycles = 0;
   uint32_t    i;

   cycles = disassemble_cycles( d, pos );
   if( cycles >= 0xF0 )
   {
      /* interrupt */
      cycles &= 0x0F;
      for( i = 0; i < cycles; ++i )
      {
         fullinfo[pos+i].eval = EVAL_MIN;
      }
   }
   else if( cycles >= 0x80 )
   {
      /* internal error */
      for( i = pos; i < d->entries; ++i )
      {
         fullinfo[i].eval = EVAL_MIN;
      }
      return cycles;
   }
   else
   {
      /* normal instruction, tag accordingly */
      fullinfo[pos].eval = EVAL_MAX;
      for( i = 1; i < cycles; ++i )
      {
         fullinfo[pos+i].eval = EVAL_MIN;
      }
   }
   pos += cycles;
   fullinfo[pos].eval = EVAL_MAX;

   return cycles;
}


void disassemble_beancounter( disass_fulltrace_t d, uint32_t start )
{
   fullinfo_t  *fullinfo      = d->fullinfo;
   uint8_t cycles = 0;
   uint32_t pos = start;
   uint32_t i;
   for(;;)
   {
      cycles = disassemble_cycles( d, pos );
      if( cycles >= 0xF0 )
      {
         /* interrupt */
         cycles &= 0x0F;
         for( i = 0; i < cycles; ++i )
         {
            fullinfo[pos+i].eval = EVAL_MIN;
         }
      }
      else if( cycles >= 0x80 )
      {
         /* internal error */
         for( i = pos; i < d->entries; ++i )
         {
            fullinfo[i].eval = EVAL_MIN;
         }
         return;
      }
      else
      {
         /* normal instruction, tag accordingly */
         fullinfo[pos].eval = EVAL_MAX;
         for( i = 1; i < cycles; ++i )
         {
            fullinfo[pos+i].eval = EVAL_MIN;
         }
      }
      pos += cycles;
      if( pos > d->entries )
      {
         return;
      }
   }
}
