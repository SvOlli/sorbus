/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */


#include "da_base.h"

#include <stdio.h>
#include <string.h>


#define DA_PICK_MNEMONIC(o) (((o) >>  0) & 0xFF)
#define DA_PICK_ADDRMODE(o) (((o) >>  8) & 0x3F)
#define DA_PICK_RESERVED(o) (((o) >> 14) & 0x01)
#define DA_PICK_BYTES(o)    (((o) >> 15) & 0x07)
#define DA_PICK_CYCLES(o)   (((o) >> 18) & 0x0F)
#define DA_PICK_EXTRA(o)    (((o) >> 22) & 0x03)
#define DA_PICK_JUMP(o)     (((o) >> 24) & 0x01)
#define DA_PICK_MX(o)       (((o) >> 25) & 0x03)


const uint32_t *da_get_opcodes( cputype_t cpu )
{
   switch( cpu )
   {
      case CPU_6502RA:
      case CPU_6502:
         return &da_opcodes6502[0];
      case CPU_65SC02:
         return &da_opcodes65sc02[0];
      case CPU_65C02:
         return &da_opcodes65c02[0];
      case CPU_65816:
         return &da_opcodes65816[0];
      case CPU_65CE02:
         return &da_opcodes65ce02[0];
      default:
         fprintf( stderr, __FILE__ "(%d): internal error: 0x%02x\n", __LINE__,
                  cpu );
   }
   return 0;
}


da_mnemonic_t da_pick_mnemonic( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_MNEMONIC( opcodes[opcode] );
}


da_addrmode_t da_pick_addrmode( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_ADDRMODE( opcodes[opcode] );
}


bool da_pick_reserved( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_RESERVED( opcodes[opcode] );
}


uint8_t da_pick_bytes( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_BYTES( opcodes[opcode] );
}


uint8_t da_pick_cycles( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_CYCLES( opcodes[opcode] );
}


uint8_t da_pick_extra( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_EXTRA( opcodes[opcode] );
}


bool da_pick_jump( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_JUMP( opcodes[opcode] );
}


uint8_t da_pick_mx( cputype_t cpu, uint8_t opcode )
{
   const uint32_t *opcodes = da_get_opcodes( cpu );
   return DA_PICK_MX( opcodes[opcode] );
}


bool da_is_imm16( const da_fullinfo_t fullinfo )
{
   if( !fullinfo.n816 )
   {
      /* emulation mode */
      return false;
   }
   if( DA_PICK_MX(da_opcodes65816[fullinfo.data]) & 0x2 )
   {
      /* opcode is affected by M flag */
      return fullinfo.m816;
   }
   if( DA_PICK_MX(da_opcodes65816[fullinfo.data]) & 0x1 )
   {
      /* opcode is affected by X flag */
      return fullinfo.x816;
   }
   return false;
}


uint8_t da_pick_bytes816( cputype_t cpu, da_fullinfo_t fullinfo )
{
   return da_pick_bytes( cpu, fullinfo.data ) + da_is_imm16( fullinfo );
}


bool da_is_imm16_mx( cputype_t cpu, uint8_t opcode, bool m, bool x )
{
   if( cpu == CPU_65816 )
   {
      uint8_t mx = da_pick_mx( cpu, opcode );
      if( ((mx == 1) && x) || ((mx == 2) && m) )
      {
         return true;
      }
   }
   return false;
}


const char *da_mnemonic_string( cputype_t cpu, uint8_t p0 )
{
   return da_mnemonics[da_pick_mnemonic( cpu, p0 )];
}


uint8_t da_fullinfo_isequal( cputype_t cpu, da_fullinfo_t fi1, da_fullinfo_t fi2 )
{
   uint8_t bytes = 0;
   /* sample data must match, otherwise further testing makes no sense */
   if( (fi1.raw & 0x3FFFFFFF) != (fi2.raw & 0x3FFFFFFF) )
   {
      return '!';
   }

   bytes = da_pick_bytes( cpu, fi1.data );
   /* adjust number of bytes on 65816 CPU */

   if( da_pick_mx( cpu, fi1.data ) & 0x01 )
   {
      if( fi1.m816 != fi2.m816 )
      {
         return '!';
      }
      if( !fi1.m816 )
      {
         ++bytes;
      }
   }

   if( da_pick_mx( cpu, fi1.data ) & 0x02 )
   {
      if( fi1.x816 != fi2.x816 )
      {
         return '!';
      }
      if( !fi1.x816 )
      {
         ++bytes;
      }
   }

   if( fi1.dataused < (bytes-1) )
   {
      return '?';
   }

   if( fi2.dataused < (bytes-1) )
   {
      return '?';
   }

   if( bytes > 1 )
   {
      if( fi1.data1 != fi2.data1 )
      {
         return '!';
      }
   }
   if( bytes > 2 )
   {
      if( fi1.data2 != fi2.data2 )
      {
         return '!';
      }
   }
   if( bytes > 3 )
   {
      if( fi1.data3 != fi2.data3 )
      {
         return '!';
      }
   }

   return '=';
}


static inline int _da_sn_extra_data( char *b, size_t bsize, uint8_t data, bool valid, bool capital )
{
   if( valid )
   {
      return snprintf( b, bsize, capital ? " %02X" : " %02x", data );
   }
   return snprintf( b, bsize, "   " );
}


static inline int  _da_sn_text( char *b, size_t bsize, uint8_t ch, bool valid )
{
   if( !valid )
   {
      ch = 0x20;
   }
   if( ch < 0x20 )
   {
      return snprintf( b, bsize, "%c[;7m%c%c[;m", 0x1b, ch | 0x40, 0x1b );
   }
   return snprintf( b, bsize, "%c", ch );
}


int da_sn_fullinfo( char *b, size_t bsize, cputype_t cpu,
                    const char f, da_fullinfo_t fullinfo, da_flags_t flags )
{
   size_t used = 0;
   uint8_t showbytes = 0;
   bool capital = false;

   if( bsize )
   {
      /* need to make sure that there's at least on character left */

      if( f <= '@' )
      {
         *b = f;
         ++used;
      }
      else switch( f )
      {
         case '[':
         case ']':
         case '{':
         case '|':
         case '}':
            *b = f;
            ++used;
            break;
         case 'a':
            used += snprintf( b+used, bsize-used, "%04x", fullinfo.address );
            break;
         case 'A':
            used += snprintf( b+used, bsize-used, "%04X", fullinfo.address );
            break;
         case 'c':
            *b = '0' + da_pick_cycles( cpu, fullinfo.data );
            ++used;
            break;
         case 'C':
            /* extra cycles is defined as
             * 0: none
             * 1: on page cross
             * 2: on branch, with extra on page cross */
            *b = " pb"[da_pick_extra( cpu, fullinfo.data )];
            ++used;
            break;
         case 'd':
            used += snprintf( b+used, bsize-used, "%02x", fullinfo.data );
            break;
         case 'D':
            used += snprintf( b+used, bsize-used, "%02X", fullinfo.data );
            break;
         case 'e':
            *b = '0'+fullinfo.eval;
            ++used;
            break;
         case 'I':
            *b = fullinfo.irq ? ' ' : 'I';
            ++used;
            break;
         case 'N':
            *b = fullinfo.nmi ? ' ' : 'N';
            ++used;
            break;
         case 'n':
            used += snprintf( b+used, bsize-used, "%c%c%c",
                              fullinfo.n816 ? 'n' : ' ',
                              fullinfo.m816 ? 'm' : ' ',
                              fullinfo.x816 ? 'x' : ' ' );
            break;
         case 'o':
            showbytes = min( fullinfo.dataused+1, da_pick_bytes( cpu, fullinfo.data ) );
            capital   = false;
            break;
         case 'O':
            showbytes = min( fullinfo.dataused+1, da_pick_bytes( cpu, fullinfo.data ) );
            capital   = true;
            break;
         case 'R': // reset
            *b = fullinfo.reset ? ' ' : 'R';
            ++used;
            break;
         case 't': // byte as text
            used += _da_sn_text( b+used, bsize-used, fullinfo.data, true );
            break;
         case 'T': // additions bytes as text
            showbytes = 0x10 | min( fullinfo.dataused+1, da_pick_bytes( cpu, fullinfo.data ) );
            break;
         case 'S': // stop = !RDY
            *b = fullinfo.rdy ? ' ' : 'S';
            ++used;
            break;
         case 'w': // read/write
            *b = fullinfo.rw ? 'r' : 'w';
            ++used;
            break;
         case 'x':
            showbytes = fullinfo.dataused+1;
            capital   = false;
            break;
         case 'X':
            showbytes = fullinfo.dataused+1;
            capital   = true;
            break;
         case 'Y': // disassemble always
            // evil hack: set confidence to high value
            fullinfo.eval = 6;
            /* slip through */
         case 'y': // disassemble when confident
            // TODO: assert bsize >= 14
            if( fullinfo.eval >= 3 )
            {
               used += da_sn_text( b+used, bsize-used, cpu, fullinfo, flags );
            }
            if( flags & DA_FLAG_PAD )
            {
               /* padding with spaces, assumption: nothing is longer than
                * LDA  $123456,X
                * 12345678901234 */
               while( used < 14 )
               {
                  if( used >= bsize )
                  {
                     break;
                  }
                  b[used++] = ' ';
               }
            }
            break;
         case 'Z': // debug
            // TODO: assert bsize >= 16
            used += snprintf( b+used, bsize-used, "%016lx", fullinfo.raw );
            break;
         default: // sufficiant error reporting
            fprintf( stderr, __FILE__
               "(%d): internal error: 0x%02X ('%c')\n",
               __LINE__, f, f );
            break;
      }
   }

   if( showbytes > 0 )
   {
      if( (cpu == CPU_65816) && da_is_imm16( fullinfo ) )
      {
         ++showbytes;
      }

      if( showbytes >= 0x10 )
      {
         showbytes -= 0x11;
         used += _da_sn_text( b+used, bsize-used, fullinfo.data1, showbytes >= 1 );
         used += _da_sn_text( b+used, bsize-used, fullinfo.data2, showbytes >= 2 );
         used += _da_sn_text( b+used, bsize-used, fullinfo.data3, showbytes >= 3 );
      }
      else
      {
         --showbytes; // first byte displayed with 'd' or 'o'
         // optional TODO: add assert for 9 bytes available
         used += _da_sn_extra_data( b+used, bsize-used, fullinfo.data1, showbytes >= 1, capital );
         used += _da_sn_extra_data( b+used, bsize-used, fullinfo.data2, showbytes >= 2, capital );
         used += _da_sn_extra_data( b+used, bsize-used, fullinfo.data3, showbytes >= 3, capital );
      }
   }

   b[used] = '\0';
   return used;
}


int da_snf_fullinfo( char *b, size_t bsize, cputype_t cpu,
                     const char *format, da_fullinfo_t fullinfo, da_flags_t flags )
{
   const char *f;
   size_t used = 0;

   for( f = format; *f; ++f )
   {
      if( *f == 'E' )
      {
         if( fullinfo.eval > DA_EVAL_MIN )
         {
            continue;
         }
         else
         {
            break;
         }
      }
      if( used >= bsize )
      {
         break;
      }
      used += da_sn_fullinfo( b+used, bsize-used, cpu, *f, fullinfo,
                              *(f+1) ? flags | DA_FLAG_PAD : flags );
   }
   return used;
}


int da_sn_text( char *b, size_t bsize, cputype_t cpu,
                da_fullinfo_t fullinfo, da_flags_t flags )
{
   int      used = 0;
   uint16_t addr = fullinfo.address;
   uint8_t  d0   = fullinfo.data;
   uint8_t  d1   = fullinfo.data1;
   uint8_t  d2   = fullinfo.data2;
   uint8_t  d3   = fullinfo.data3;
   da_addrmode_t addrmode = da_pick_addrmode( cpu, d0 );

   if( (addrmode == ZPN) || (addrmode == ZPNR) )
   {
      used += snprintf( b+used, bsize-used, "%s%d ",
                        da_mnemonic_string( cpu, d0 ), (d0 >> 4) & 7 );
   }
   else
   {
      bool reserved = da_pick_reserved( cpu, d0 );
      if( (cpu == CPU_6502RA) &&
          (da_pick_mnemonic( cpu, d0 ) == ROR ) )
      {
         // we are running a Rev.A, so ROR is an undefined opcode
         reserved = true;
      }
      used += snprintf( b+used, bsize-used, "%s%c ",
                        da_mnemonic_string( cpu, d0 ), reserved ? '.' : ' ' );
   }

   if( (flags & DA_FLAG_BRK_OVERRIDE) && (d0 == 0x00) )
   {
      addrmode = IMP;
   }
   switch( addrmode )
   {
      case ABS:   // OPC $1234
         used += snprintf( b+used, bsize-used, "$%02X%02X",       d2, d1 );
         break;
      case ABSIL: // OPC [$1234]
         used += snprintf( b+used, bsize-used, "[$%02X%02X]",     d2, d1 );
         break;
      case ABSL:  // OPC $123456
         used += snprintf( b+used, bsize-used, "$%02X%02X%02X",   d3, d2, d1 );
         break;
      case ABSLX: // OPC $123456,X
         used += snprintf( b+used, bsize-used, "$%02X%02X%02X,X", d3, d2, d1 );
         break;
      case ABSLY: // OPC $123456,Y
         used += snprintf( b+used, bsize-used, "$%02X%02X%02X,Y", d3, d2, d1 );
         break;
      case ABSX:  // OPC $1234,X
         used += snprintf( b+used, bsize-used, "$%02X%02X,X",     d2, d1 );
         break;
      case ABSY:  // OPC $1234,Y
         used += snprintf( b+used, bsize-used, "$%02X%02X,Y",     d2, d1 );
         break;
      case ABSZ:  // OPC $1234,Z
         used += snprintf( b+used, bsize-used, "$%02X%02X,Z",     d2, d1 );
         break;
      case AI:    // OPC ($1234)
         used += snprintf( b+used, bsize-used, "($%02X%02X)",     d2, d1 );
         break;
      case AIX:   // OPC ($1234,X)
         used += snprintf( b+used, bsize-used, "($%02X%02X,X)",   d2, d1 );
         break;
      case IMP:   // OPC
         // strip off trainling spaces here
         used -= (b[used-2] == ' ') ? 2 : 1;
         break;
      case IMM:   // OPC #$01
         if( da_is_imm16( fullinfo ) )
         {
            used += snprintf( b+used, bsize-used, "#$%02X%02X", d2, d1 );
         }
         else
         {
            used += snprintf( b+used, bsize-used, "#$%02X",     d1 );
         }
         break;
      case IMM2:  // OPC #$01,#$02
         used += snprintf( b+used, bsize-used, "#$%02X,#$%02X", d1, d2 );
         break;
      case IMML:  // OPC #$1234
         used += snprintf( b+used, bsize-used, "#$%02X%02X",    d2, d1 );
         break;
      case REL:   // OPC LABEL
         used += snprintf( b+used, bsize-used, "$%04X",         (((addr+2) + (int8_t)d1)) & 0xFFFF );
         break;
      case RELL:  // OPC LABEL
         used += snprintf( b+used, bsize-used, "$%04X",         (((addr+3) + (int16_t)(d1 | (d2 << 8)))) & 0xFFFF );
         break;
      case RELSY:   // OPC #$01
         used += snprintf( b+used, bsize-used, "(#$%02X,S),Y",  d1 );
         break;
      case ZP:    // OPC $12
      case ZPN:   // OPC# $12
         used += snprintf( b+used, bsize-used, "$%02X",         d1 );
         break;
      case ZPI:   // OPC ($12)
         used += snprintf( b+used, bsize-used, "($%02X)",       d1 );
         break;
      case ZPIX:  // OPC ($12,X)
         used += snprintf( b+used, bsize-used, "($%02X,X)",     d1 );
         break;
      case ZPIY:  // OPC ($12),Y
         used += snprintf( b+used, bsize-used, "($%02X),Y",     d1 );
         break;
      case ZPIZ:  // OPC ($12),Z
         used += snprintf( b+used, bsize-used, "($%02X),Z",     d1 );
         break;
      case ZPIL:  // OPC [$12]
         used += snprintf( b+used, bsize-used, "[$%02X]",       d1 );
         break;
      case ZPILY: // OPC [$12],Y
         used += snprintf( b+used, bsize-used, "[$%02X],Y",     d1 );
         break;
      case ZPISY: // OPC ($12,S),Y
         used += snprintf( b+used, bsize-used, "($%02X,S),Y",   d1 );
         break;
      case ZPNR:  // OPC# $12,LABEL
         used += snprintf( b+used, bsize-used, "$%02X,$%04X",   d1, ((addr+3) + (int8_t)d2) & 0xFFFF );
         break;
      case ZPS:   // OPC $12,S
         used += snprintf( b+used, bsize-used, "$%02X,S",       d1 );
         break;
      case ZPX:   // OPC $12,X
         used += snprintf( b+used, bsize-used, "$%02X,X",       d1 );
         break;
      case ZPY:   // OPC $12,Y
         used += snprintf( b+used, bsize-used, "$%02X,Y",       d1 );
         break;
      default:
         strncpy( b-5, "internal error", bsize+5 ); // overwrite opcode
         break;
   }

   return used;
}


const char *da_cputype_name( cputype_t cpu )
{
   /* this needs to be aligned with the enum in base_type.h */
   const char *cpuname[] = {
      "UNKNOWN",
      "NMOS 6502",
      "65C02",
      "65C816",
      "65CE02",
      "6502 Rev.A",
      "65SC02",
      "SWEET16"
   };

   if( cpu >= CPU_UNDEF )
   {
      cpu = CPU_ERROR;
   }
   return cpuname[cpu];
}


int da_sn_cpuname( char *b, size_t bsize, cputype_t cpu )
{
   return snprintf( b, bsize, "%s", da_cputype_name( cpu ) );
}
