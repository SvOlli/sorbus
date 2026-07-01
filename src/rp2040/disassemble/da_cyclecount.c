/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */


#include "da_trace.h"
#include "da_platform.h"

#include <stdio.h>


bool da_cc_atend( da_trace_t d, uint32_t pos )
{
   da_fullinfo_t fullinfo = d->fullinfo[pos];
   /* an entry all zero cannot be taked from bus
    * but buffer is initialized with 0, so this is unused buffer
    * however, an eval value could already be set */
   fullinfo.eval = 0;
   return (fullinfo.raw == 0);
}


bool da_cc_pagecross( uint16_t addr0, uint16_t addr1 )
{
   return (addr0 >> 8) != (addr1 >> 8);
}


uint8_t da_cc_jump_rel8( da_trace_t d, uint32_t pos )
{
   da_fullinfo_t *fullinfo = d->fullinfo;

   /* sanity checks */
   uint16_t dest = fullinfo[pos].address + 2; // origin of relative branch

   /* first check if access pattern matches */
   if( ((fullinfo[pos+0].address + 1) != fullinfo[pos+1].address) ||
       ((fullinfo[pos+1].address + 1) != fullinfo[pos+2].address) )
   {
      fprintf( stderr, __FILE__
               "(%d): internal error: $%02X\n",
               __LINE__, fullinfo[pos].data );
      return 0x80;   /* error */
   }

   dest += (int8_t)fullinfo[pos+1].data;
   //dest += (int8_t)fullinfo[pos].data1;
   if( fullinfo[pos+3].address == dest )
   {
      /* either no page cross or 65816 native */
      return 3;      /* branch taken, no extra cycle */
   }
   if( (fullinfo[pos+4].address == dest) &&
       da_cc_pagecross( fullinfo[pos].address+2, dest ) )
   {
      return 4;      /* branch taken with page crossing */
   }
   return 2;         /* no branch */
}


uint8_t da_cc_jump( da_trace_t d, uint32_t pos )
{
   da_fullinfo_t  *fullinfo      = d->fullinfo;

   switch( da_pick_mnemonic( d->cpu, fullinfo[pos].data ) )
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
         return da_cc_jump_rel8( d, pos );
      default:
         break;
   }
   return da_pick_cycles( d->cpu, fullinfo[pos].data );
}


static uint8_t da_cc_6502( da_trace_t d, uint32_t pos )
{
   da_fullinfo_t  *fullinfo   = d->fullinfo;
   uint8_t        cycles      = da_pick_cycles( d->cpu, fullinfo[pos].data );

   if( da_pick_extra( d->cpu, fullinfo[pos].data ) )
   {
      if( da_pick_jump( d->cpu, fullinfo[pos].data ) )
      {
         return da_cc_jump( d, pos );
      }
      else
      {
         if( da_pick_addrmode( d->cpu, fullinfo[pos].data ) == ZPIY )
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


static uint8_t da_cc_65c02( da_trace_t d, uint32_t pos )
{
   da_fullinfo_t  *fullinfo   = d->fullinfo;
   uint8_t        cycles      = da_pick_cycles( d->cpu, fullinfo[pos].data );

   if( da_pick_jump( d->cpu, fullinfo[pos].data ) )
   {
      return da_cc_jump( d, pos );
   }

   if( da_pick_extra( d->cpu, fullinfo[pos].data ) )
   {
      switch( da_pick_addrmode( d->cpu, fullinfo[pos].data ) )
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

   switch( da_pick_mnemonic( d->cpu, fullinfo[pos].data ) )
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


static uint8_t da_cc_65ce02( da_trace_t d, uint32_t pos )
{
   /*
    * From page 7 of http://archive.6502.org/datasheets/mos_65ce02_mpu.pdf
    * "Note that the number of machine cycles for every instruction remains
    *  fixed regardless of decimal mode and page boundries"
    * --> 100% done ;-)
    */
   return da_pick_cycles( d->cpu, d->fullinfo[pos].data );
}


static uint8_t da_cc_65816( da_trace_t d, uint32_t pos )
{
   da_fullinfo_t  *fullinfo   = d->fullinfo;
   uint8_t     i           = 0;
   uint8_t     opcode      = fullinfo[pos].data;
   uint8_t     cycles      = da_pick_cycles( d->cpu, opcode );
   uint8_t     bytes       = da_pick_bytes816( d->cpu, fullinfo[pos] );
   uint16_t    address     = 0;

   /* mark in the trace, if there is 16 bit access enabled
      note: fullinfo flags are high active, CPU is low active */
   if( d->flag_e == FLAG_UNSET )
   {
      /* native mode */
      fullinfo[pos].n816 = true;
      fullinfo[pos].m816 = d->flag_m == FLAG_UNSET;
      fullinfo[pos].x816 = d->flag_x == FLAG_UNSET;
   }
   else
   {
      /* emulation mode: always 8 bit */
      fullinfo[pos].n816 = false;
      fullinfo[pos].m816 = false;
      fullinfo[pos].x816 = false;
   }

   if( da_pick_jump( d->cpu, opcode ) )
   {
      /* any kind of jump should have a defined number of clock cycles */
      switch( da_pick_addrmode( d->cpu, opcode ) )
      {
         case ABS:   // OPC $1234
         case ABSL:  // OPC $123456
         case AI:    // OPC ($1234)
            /* in this case timing is constant */
            /* TODO: double check indirect */
            return cycles;
         case AIX:   // OPC ($1234,X)
            /* TODO: double check indirect */
            return cycles;
         case REL:   // OPC LABEL
            {
               da_fullinfo_t *fullinfo = d->fullinfo;

               /* sanity checks */
               uint16_t dest = fullinfo[pos].address + 2; // origin of relative branch

               /* first check if access pattern matches */
               if( ((fullinfo[pos+0].address + 1) != fullinfo[pos+1].address) ||
                   ((fullinfo[pos+1].address + 1) != fullinfo[pos+2].address) )
               {
                  fprintf( stderr, __FILE__
                           "(%d): internal error: 0x%02x $%02X\n",
                           __LINE__, da_pick_addrmode( d->cpu, opcode ),
                           fullinfo[pos].data );
                  return 0x80;   /* error */
               }

               dest += (int8_t)fullinfo[pos+1].data;
               //dest += (int8_t)fullinfo[pos].data1;
               if( fullinfo[pos+3].address == dest )
               {
                  return 3;      /* branch taken */
               }
               if( (fullinfo[pos+4].address == dest) &&
                   da_cc_pagecross( fullinfo[pos].address+2, dest ) )
               {
                  return 4;      /* branch taken with page crossing */
               }
               return 2;         /* no branch */
            }
         case IMM:
            /* used by BRK and COP (65816) */
            return cycles + (fullinfo[pos].n816 ? 1 : 0);
         case IMP:
            /* RTS, RTL or RTI */
            switch( da_pick_mnemonic( d->cpu, opcode ) )
            {
               case RTI:
                  if( d->flag_e == FLAG_UNSET )
                  {
                     /* native mode */
                     uint8_t p = fullinfo[pos+3].data;
                     d->flag_m = p & 0x20 ? FLAG_SET : FLAG_UNSET;
                     d->flag_x = p & 0x10 ? FLAG_SET : FLAG_UNSET;
                     d->flag_c = p & 0x01 ? FLAG_SET : FLAG_UNSET;
                  }
                  else
                  {
                     /* emulation mode */
                     uint8_t p = fullinfo[pos+3].data;
                     d->flag_c = p & 0x01 ? FLAG_SET : FLAG_UNSET;
                  }
                  return cycles + (fullinfo[pos].n816 ? 1 : 0);
               case RTS:
               case RTL:
                  return cycles;
               default:
                  fprintf( stderr, __FILE__
                           "(%d): internal error: 0x%02x $%02X\n",
                           __LINE__, da_pick_mnemonic( d->cpu, opcode ),
                           fullinfo[pos].data );
                  return 0x80;
            }
         default:
            fprintf( stderr, __FILE__
                     "(%d): internal error: 0x%02x $%02X\n",
                     __LINE__, da_pick_addrmode( d->cpu, opcode ),
                     fullinfo[pos].data );
            return 0x80;
      }
   }
   else
   {
      /* try to keep track of CPU mode */
      switch( da_pick_mnemonic( d->cpu, opcode ) )
      {
         case CLC:
            d->flag_c = FLAG_UNSET;
            break;
         case SEC:
            d->flag_c = FLAG_SET;
            break;
         case XCE:
            {
               da_cpu_flag_t tmp = d->flag_c;
               d->flag_c = d->flag_e;
               d->flag_e = tmp;
            }
            break;
         case REP:
            if( d->flag_e == FLAG_UNSET )
            {
               if( fullinfo[pos+1].data & 0x20 )
               {
                  d->flag_m = FLAG_UNSET;
               }
               if( fullinfo[pos+1].data & 0x10 )
               {
                  d->flag_x = FLAG_UNSET;
               }
            }
            return 3;
         case SEP:
            if( d->flag_e == FLAG_UNSET )
            {
               if( fullinfo[pos+1].data & 0x20 )
               {
                  d->flag_m = FLAG_SET;
               }
               if( fullinfo[pos+1].data & 0x10 )
               {
                  d->flag_x = FLAG_SET;
               }
            }
            return 3;
         case ADC:
         case ASL:
         case CMP:
         case CPX:
         case CPY:
         case LSR:
         case ROL:
         case ROR:
         case SBC:
            d->flag_c = FLAG_UNKNOWN;
            break;
         case PHP: // could be read from trace
            d->flag_c = fullinfo[pos+2].data & 0x01 ? FLAG_SET : FLAG_UNSET;
            return 3;
         case PLP: // could be read from trace
            d->flag_c = fullinfo[pos+3].data & 0x01 ? FLAG_SET : FLAG_UNSET;
            return 4;
         case RTI: // could be read from trace
            d->flag_c = fullinfo[pos+3].data & 0x01 ? FLAG_SET : FLAG_UNSET;
            return fullinfo[pos].n816 ? 7 : 6;
         default:
            /* most of the time, there is nothing to do */
            break;
      }

      if( da_is_imm16( d->fullinfo[pos] ) )
      {
         ++bytes;
      }

      /* decide which address should pop up */
      address = fullinfo[pos].address + bytes;

      /* check if target address is found */
      for( i = da_is_imm16( d->fullinfo[pos] ) ? 3 : 2; i < DA_CPU_MAXCYCLES; ++i )
      {
         if( address == fullinfo[pos+i].address )
         {
            if( address == fullinfo[pos+i+1].address )
            {
               /* some opcode use an extra cycle sometimes */
               return i+1;
            }
            return i;
         }
         if( da_cc_atend( d, pos+i ) )
         {
            /* end of buffer? */
            return i;
         }
      }
   }
   return 0x80;
}


/*
 * typically, this is done by just sampling pin 1 (DIP)
 * however, this pin is not sampled, so this needs to be done in software
 */
uint8_t da_cc_vectorpull( da_trace_t d, uint32_t pos, uint16_t addr, bool sc02 )
{
   da_fullinfo_t *fullinfo = d->fullinfo;
   uint16_t vector = 0;
   int i;
   uint8_t stack_lo[3];

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
   /* simplify checking low bytes of stack access for check 1 */
   for( i = 0; i < 3; ++i )
   {
      stack_lo[i] = (fullinfo[pos+2+i].address + i) & 0xFF;
   }
   /* check 1: stack access addresses (only low bytes) */
   if( (stack_lo[0] != stack_lo[1]) || (stack_lo[1] != stack_lo[2]) )
   {
      return 0;
   }
   /* check 2: stack access addresses (high bytes) */
   if( ((fullinfo[pos+2].address & 0xFF00) != 0x0100) ||
       ((fullinfo[pos+3].address & 0xFF00) != 0x0100) ||
       ((fullinfo[pos+4].address & 0xFF00) != 0x0100) )
   {
      /* stack is not fixed at $01xx, let's assume stack is 16 bit */
      if( ((fullinfo[pos+2].address) != (fullinfo[pos+3].address+1)) ||
          ((fullinfo[pos+3].address) != (fullinfo[pos+4].address+1)) )
      {
         return 0;
      }
   }
   /* check 3: stack access mode */
   if( (fullinfo[pos+2].rw != !sc02) ||
       (fullinfo[pos+3].rw != !sc02) ||
       (fullinfo[pos+4].rw != !sc02) )
   {
      return 0;
   }
   /* check 4: vector access */
   if( (fullinfo[pos+5].address != (addr+0)) ||
       (fullinfo[pos+6].address != (addr+1)) )
   {
      return 0;
   }
   vector = fullinfo[pos+5].data | (fullinfo[pos+6].data << 8);
   /* check 5: vector taken */
   if( vector != fullinfo[pos+7].address )
   {
      return 0;
   }
   /* mark everything in between as non-opcode */
   for( i = 0; i < 7; ++i )
   {
      fullinfo[pos+i].eval = DA_EVAL_MIN;
   }
   /* additionally, we now know that we're in emulation mode */
   d->flag_e = FLAG_SET;

   return 7;
}


uint8_t da_cc_vectorpull_65816( da_trace_t d, uint32_t pos, uint16_t addr )
{
   da_fullinfo_t *fullinfo = d->fullinfo;
   uint16_t vector = 0;

/* typical IRQ (native mode, GUESSED NEEDS TRACE UPDATE):
   0:0445 r 4c    :JMP  $0445 ; gets interrupted
   1:0445 r 4c    :
   2:01eb w 00    :           ; push PBR
   3:01ea w 04    :           ; push PCH
   4:01e9 w 45    :           ; push PCL
   5:01e8 w 21    :           ; push P
   6:ffee r 5d    :
   7:ffef r 04    :
   8:045d r 48    :PHA
*/

   /* check 1: stack access: 4 writes to consecutive addresses ) */
   if( ((fullinfo[pos+2].address) != (fullinfo[pos+3].address+1)) ||
       ((fullinfo[pos+3].address) != (fullinfo[pos+4].address+1)) ||
       ((fullinfo[pos+4].address) != (fullinfo[pos+5].address+1)) ||
       (!fullinfo[pos+2].rw) ||
       (!fullinfo[pos+3].rw) ||
       (!fullinfo[pos+4].rw) ||
       (!fullinfo[pos+5].rw) )
   {
      return 0;
   }
   /* check 2: vector access */
   if( (fullinfo[pos+6].address != (addr+0)) ||
       (fullinfo[pos+7].address != (addr+1)) )
   {
      return 0;
   }
   vector = fullinfo[pos+5].data | (fullinfo[pos+6].data << 8);
   /* check 3: vector taken */
   if( vector != fullinfo[pos+7].address )
   {
      return 0;
   }
   /* additionally, we now know that we're in native mode */
   d->flag_e = FLAG_UNSET;

   return 8;
}


/* returns 0, if no interrupt/reset, else clockcycles required */
uint8_t da_cc_interrupt_65816( da_trace_t d, uint32_t pos )
{
   uint8_t cycles = 0;

   /* check for interrupts in emulated mode */

   if( !cycles )
   {
      /* check COP */
      cycles = da_cc_vectorpull( d, pos, 0xFFF4, false );
   }

   if( !cycles )
   {
      /* check ABORT */
      cycles = da_cc_vectorpull( d, pos, 0xFFF8, false );
   }

   if( !cycles )
   {
      /* check NMI */
      cycles = da_cc_vectorpull( d, pos, 0xFFFA, false );
   }

   if( !cycles )
   {
      /* check RESET */
      cycles = da_cc_vectorpull( d, pos, 0xFFFC, false );
      if( cycles )
      {
         printf( "reset detected at: %lu\n", pos );
      }
   }

   if( !cycles )
   {
      /* check IRQ */
      cycles = da_cc_vectorpull( d, pos, 0xFFFE, false );
   }

   /* check for interrupts in native mode */

   if( !cycles )
   {
      /* check COP */
      cycles = da_cc_vectorpull_65816( d, pos, 0xFFE4 );
   }

   if( !cycles )
   {
      /* check BRK */
      cycles = da_cc_vectorpull_65816( d, pos, 0xFFE6 );
   }

   if( !cycles )
   {
      /* check ABORT */
      cycles = da_cc_vectorpull_65816( d, pos, 0xFFE8 );
   }

   if( !cycles )
   {
      /* check NMI */
      cycles = da_cc_vectorpull_65816( d, pos, 0xFFEA );
   }

   if( !cycles )
   {
      /* check IRQ */
      cycles = da_cc_vectorpull_65816( d, pos, 0xFFEE );
   }

   return cycles;
}


/* returns 0, if no interrupt/reset, else clockcycles required */
uint8_t da_cc_interrupt( da_trace_t d, uint32_t pos )
{
   //fullinfo_t *fullinfo = d->fullinfo;
   //uint16_t address0 = fullinfo[pos+0].address;
   //uint16_t address1 = fullinfo[pos+1].address;

   uint8_t cycles = 0;

   /* for simplicity moved to own function */
   if( d->cpu == CPU_65816 )
   {
      return da_cc_interrupt_65816( d, pos );
   }

   if( !cycles )
   {
      /* check NMI */
      cycles = da_cc_vectorpull( d, pos, 0xFFFA, false );
   }

   if( !cycles )
   {
      /* check IRQ */
      cycles = da_cc_vectorpull( d, pos, 0xFFFE, false );
   }

   if( !cycles )
   {
      /* check reset */
      /* reset reads from stack instead of write, except for 65SC02 */
      cycles = da_cc_vectorpull( d, pos, 0xFFFC, d->cpu != CPU_65SC02 );
   }

   return cycles;
}


uint8_t da_cc_cycles( da_trace_t d, uint32_t pos )
{
   uint8_t is_interrupt = 0;   /* and also reset */

   if( da_cc_atend( d, pos ) )
   {
      return 0x80;
   }

   if( d->cpu == CPU_65816 )
   {
      is_interrupt = da_cc_interrupt_65816( d, pos );
   }
   else
   {
      is_interrupt = da_cc_interrupt( d, pos );
   }
   if( is_interrupt > 0 )
   {
      /* returns cycles used by irq or'ed with marker */
      return is_interrupt | 0xF0;
   }

   switch( d->cpu )
   {
      case CPU_6502RA:
      case CPU_6502:
         return da_cc_6502( d, pos );
      case CPU_65SC02:
      case CPU_65C02:
         return da_cc_65c02( d, pos );
      case CPU_65CE02:
         return da_cc_65ce02( d, pos ); // 100% done
      case CPU_65816:
         return da_cc_65816( d, pos );
      default:
         fprintf( stderr, __FILE__
                  "(%d): internal error: 0x%02x\n",
                  __LINE__, d->cpu );
         return 0x81;
   }
}


bool da_cc_start( da_trace_t d, uint32_t start )
{
   da_fullinfo_t  *fullinfo   = d->fullinfo;
   uint8_t     cycles      = 0;
   uint32_t    pos         = start;
   uint32_t    i;
   
   forever()
   {
      if( (pos > d->entries) || da_cc_atend( d, pos ) )
      {
         return true;
      }

      if( !fullinfo[pos].reset )
      {
         /* in reset state, the cpu is not doing anything */
         fullinfo[pos++].eval = DA_EVAL_MIN;
         if( fullinfo[pos].reset )
         {
            /* next cycle is not a reset anymore so there's got to be
             * a vectorpull */
            for( i = 0; !da_cc_atend( d, pos+i ) && (i < 9); ++i )
            {
               fullinfo[pos+i].eval = DA_EVAL_MIN;
               uint8_t v = da_cc_vectorpull( d, pos+i, 0xFFFC, d->cpu == CPU_65SC02 );
               if( v )
               {
                  pos += i + v;
                  break;
               }
            }
         }
         continue;
      }

      cycles = da_cc_cycles( d, pos );

      if( cycles >= 0xF0 )
      {
         /* interrupt */
         cycles &= 0x0F;
         for( i = 0; i < cycles; ++i )
         {
            fullinfo[pos+i].eval = DA_EVAL_MIN;
         }
      }
      else if( cycles >= 0x80 )
      {
         /* internal error */
         for( i = pos; i < d->entries; ++i )
         {
            fullinfo[i].eval = DA_EVAL_MIN;
         }
         return false;
      }
      else
      {
         /* normal instruction, tag accordingly */
         fullinfo[pos].eval = DA_EVAL_MAX;
         for( i = 1; i < cycles; ++i )
         {
            fullinfo[pos+i].eval = DA_EVAL_MIN;
         }
      }

      pos += cycles;
   }
}
