
#include "disassemble.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


bool bc_atend( disass_fulltrace_t d, uint32_t pos )
{
   fullinfo_t fullinfo = d->fullinfo[pos];
   /* an entry all zero cannot be taked from bus
    * but buffer is initialized with 0, so this is unused buffer
    * however, an eval value could already be set */
   fullinfo.eval = 0;
   return (fullinfo.raw == 0);
}


bool bc_pagecross( uint16_t addr0, uint16_t addr1 )
{
   return (addr0 >> 8) != (addr1 >> 8);
}


/*
 * typically, this is done by just sampling pin 1 (DIP)
 * however, this pin is not sampled, so this needs to be done in software
 */
uint8_t bc_vectorpull( disass_fulltrace_t d, uint32_t pos, uint16_t addr, bool rw )
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
uint8_t bc_interrupt( disass_fulltrace_t d, uint32_t pos )
{
   //fullinfo_t *fullinfo = d->fullinfo;
   //uint16_t address0 = fullinfo[pos+0].address;
   //uint16_t address1 = fullinfo[pos+1].address;

   uint8_t cycles = 0;

   /* for simplicity moved to own function */
   if( d->cpu == CPU_65816 )
   {
      return bc_interrupt_65816( d, pos );
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


uint8_t bc_jump_rel8( disass_fulltrace_t d, uint32_t pos )
{
   fullinfo_t *fullinfo = d->fullinfo;

   /* sanity checks */
   uint16_t dest = fullinfo[pos].address + 2; // origin of relative branch

   /* first check if access pattern matches */
   if( ((fullinfo[pos+0].address + 1) != fullinfo[pos+1].address) ||
       ((fullinfo[pos+1].address + 1) != fullinfo[pos+2].address) )
   {
      fprintf( stderr, "internal error @ %s(%d): $%02X\n",
               __FILE__, __LINE__, fullinfo[pos].data );
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
       bc_pagecross( fullinfo[pos].address+2, dest ) )
   {
      return 4;      /* branch taken with page crossing */
   }
   return 2;         /* no branch */
}


uint8_t bc_jump( disass_fulltrace_t d, uint32_t pos )
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
         break;
      default:
         break;
   }
   return pick_cycles( d, pos );
}


#if 0
/*
 * 16 bit in 65816 is hard to detect from trace
 * in emulation mode SEP/REP have no effect on M/X flags
 * best chance: determine 16bit mode from access pattern
 * and save it for next time
 */

static uint8_t find_16bit_rmw_end( disass_fulltrace_t d, uint32_t pos )
{
/*
   0:040a r ee    :INC  $0380
   1:040b r 80    :
   2:040c r 03    :
   3:0380 r 01    :
   4:0381 r 00    :
   5:0381 r 00    :
   6:0381 w 00    :
   7:0380 w 02    :

   0:040d r 0e    :ASL  $0382
   1:040e r 81    :
   2:040f r 03    :
   3:0382 r 00    :
   4:0383 r 00    :
   5:0383 r 00    :
   6:0383 w 00    :
   7:0382 w 00    :
*/
   fullinfo_t *fullinfo = d->fullinfo;
   uint8_t i;

   for( i = 2; i < 5; ++i )
   {
      uint16_t base_addr = fullinfo[pos+i].address;
      if( (fullinfo[pos+i+1].address == base_addr+1) &&
          (fullinfo[pos+i+2].address == base_addr+1) &&
          (fullinfo[pos+i+3].address == base_addr+1) &&
          (fullinfo[pos+i+4].address == base_addr+0) &&
          (fullinfo[pos+i+0].rw) &&
          (fullinfo[pos+i+1].rw) &&
          (fullinfo[pos+i+2].rw) &&
          (!fullinfo[pos+i+3].rw) &&
          (!fullinfo[pos+i+4].rw) )
      {
          /* access pattern found: return next expected opcode offset */
          return i+5;
      }
   }
   return 0;
}
#endif


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


uint8_t disassemble_cycles( disass_fulltrace_t d, uint32_t pos )
{
   uint8_t is_interrupt = 0;   /* and also reset */

   if( bc_atend( d, pos ) )
   {
      return 0x80;
   }

   if( d->cpu == CPU_65816 )
   {
      is_interrupt = bc_interrupt_65816( d, pos );
   }
   else
   {
      is_interrupt = bc_interrupt( d, pos );
   }
   if( is_interrupt > 0 )
   {
      /* returns cycles used by irq or'ed with marker */
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
         fprintf( stderr, "internal error @ %s(%d): 0x%02x\n",
                  __FILE__, __LINE__, d->cpu );
         return 0x81;
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
