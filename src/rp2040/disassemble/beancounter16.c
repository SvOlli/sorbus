
#include "disassemble.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


uint8_t disassemble_beancounter_single_65816( disass_fulltrace_t d, uint32_t pos )
{
   uint8_t     i           = 0;
   uint8_t     cycles      = pick_cycles( d, pos );
   uint8_t     bytes       = pick_bytes( d, pos );
   uint16_t    address     = 0;
   fullinfo_t  *fullinfo   = d->fullinfo;

   /* first check if interrupt by pattern */
   /*TODO*/

   /* decide which address should pop up */
   if( pick_jump( d, pos ) )
   {
      /* TODO */
      switch( pick_addrmode( d, pos ) )
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
            /* relative branch should not differ from 65C02 */
            return bc_jump_rel8( d, pos );
         default:
            fprintf( stderr, "internal error @ %s(%d): 0x%02x\n", 
                     __FILE__, __LINE__, pick_addrmode( d, pos ) );
            return 0x80;
      }
   }
   else
   {
      address = fullinfo[pos].address + bytes;
      /* check if target address is found */
      for( i = 2; i < BOUNDSBUFFER; ++i )
      {
         if( address == fullinfo[pos+i].address )
         {
            return i;
         }
         if( fullinfo[pos+i].raw == 0x0 )
         {
            return i;
         }
      }

      fprintf( stderr, "internal error @ %s(%d): 0x%08lx\n",
               __FILE__, __LINE__, fullinfo[pos+i-1].raw );
      return 0x80;
   }
}


uint8_t bc_vectorpull_65816( disass_fulltrace_t d, uint32_t pos, uint16_t addr )
{
   fullinfo_t *fullinfo = d->fullinfo;
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
   return 8;
}


/* returns 0, if no interrupt/reset, else clockcycles required */
uint8_t bc_interrupt_65816( disass_fulltrace_t d, uint32_t pos )
{
   uint8_t cycles = 0;

   /* check for interrupts in emulated mode */

   if( !cycles )
   {
      /* check COP */
      cycles = bc_vectorpull( d, pos, 0xFFF4, false );
   }

   if( !cycles )
   {
      /* check ABORT */
      cycles = bc_vectorpull( d, pos, 0xFFF8, false );
   }

   if( !cycles )
   {
      /* check NMI */
      cycles = bc_vectorpull( d, pos, 0xFFFA, false );
   }

   if( !cycles )
   {
      /* check RESET */
      cycles = bc_vectorpull( d, pos, 0xFFFC, false );
   }

   if( !cycles )
   {
      /* check IRQ */
      cycles = bc_vectorpull( d, pos, 0xFFFE, false );
   }

   /* check for interrupts in native mode */

   if( !cycles )
   {
      /* check COP */
      cycles = bc_vectorpull_65816( d, pos, 0xFFE4 );
   }

   if( !cycles )
   {
      /* check BRK */
      cycles = bc_vectorpull_65816( d, pos, 0xFFE6 );
   }

   if( !cycles )
   {
      /* check ABORT */
      cycles = bc_vectorpull_65816( d, pos, 0xFFE8 );
   }

   if( !cycles )
   {
      /* check NMI */
      cycles = bc_vectorpull_65816( d, pos, 0xFFEA );
   }

   if( !cycles )
   {
      /* check IRQ */
      cycles = bc_vectorpull_65816( d, pos, 0xFFEE );
   }

   return cycles;
}


uint8_t bc_65816( disass_fulltrace_t d, uint32_t pos )
{
   uint8_t     i           = 0;
   uint8_t     cycles      = pick_cycles( d, pos );
   uint8_t     bytes       = pick_bytes( d, pos );
   uint16_t    address     = 0;
   fullinfo_t  *fullinfo   = d->fullinfo;

//printf("%s:%d: %04x\n", __FILE__, __LINE__, d->fullinfo[pos].address);
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

   if( pick_jump( d, pos ) )
   {
      /* any kind of jump should have a defined number of clock cycles */
      switch( pick_addrmode( d, pos ) )
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
               fullinfo_t *fullinfo = d->fullinfo;

               /* sanity checks */
               uint16_t dest = fullinfo[pos].address + 2; // origin of relative branch

               /* first check if access pattern matches */
               if( ((fullinfo[pos+0].address + 1) != fullinfo[pos+1].address) ||
                   ((fullinfo[pos+1].address + 1) != fullinfo[pos+2].address) )
               {
//printf("%s:%d: %04x\n", __FILE__, __LINE__, d->fullinfo[pos].address);
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
         case IMM:
            /* used by BRK and COP (65816) */
            return pick_cycles( d, pos ) + (fullinfo[pos].n816 ? 1 : 0);
         case IMP:
            /* RTS, RTL or RTI */
            switch( pick_mnemonic( d, pos ) )
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
//printf("%s:%d: %04x %d\n", __FILE__, __LINE__, d->fullinfo[pos].address, pick_cycles( d, pos ) );
                  return pick_cycles( d, pos ) + (fullinfo[pos].n816 ? 1 : 0);
               case RTS:
               case RTL:
                  return pick_cycles( d, pos );
               default:
                  fprintf( stderr, "internal error @ %s(%d): 0x%02x $%02X\n",
                           __FILE__, __LINE__, pick_mnemonic( d, pos ),
                           fullinfo[pos].data );
                  return 0x80;
            }
         default:
            fprintf( stderr, "internal error @ %s(%d): 0x%02x $%02X\n",
                     __FILE__, __LINE__, pick_addrmode( d, pos ),
                     fullinfo[pos].data );
            return 0x80;
      }
   }
   else
   {
      /* try to keep track of CPU mode */
      switch( pick_mnemonic( d, pos ) )
      {
         case CLC:
            d->flag_c = FLAG_UNSET;
            break;
         case SEC:
            d->flag_c = FLAG_SET;
            break;
         case XCE:
            {
               cpu_flag_t tmp = d->flag_c;
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


      if( is_imm16( d->fullinfo[pos] ) )
      {
         ++bytes;
      }

      /* decide which address should pop up */
      address = fullinfo[pos].address + bytes;

      /* check if target address is found */
      for( i = is_imm16( d->fullinfo[pos] ) ? 3 : 2; i < BOUNDSBUFFER; ++i )
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
//printf("%s:%d: %04x %d %016lx\n", __FILE__, __LINE__, d->fullinfo[pos].address, i, fullinfo[pos+i].raw);
         if( bc_atend( d, pos+i ) )
         {
            /* end of buffer? */
            return i;
         }
      }
   }
//printf("%s:%d: %04x %016lx\n", __FILE__, __LINE__, d->fullinfo[pos].address, d->fullinfo[pos+i].raw);
   return 0x80;
}

