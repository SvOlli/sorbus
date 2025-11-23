
#include "disassemble.h"

#include <stdio.h>
#include <string.h>

#include "bus.h"

static disass_show_t show_flags  = DISASS_SHOW_NOTHING;
static cputype_t cputype         = CPU_ERROR;
static uint8_t mx_flag_816       = 0;

void disass_mx816( bool m, bool x )
{
   mx_flag_816 = (m ? 1 : 0) | (x ? 2 : 0);
}

void disass_show( disass_show_t show )
{
   show_flags = show;
}

/* at bit:
 *  |  bits used:                             expected values
 *  0: 6: addressmode (enum addrmode)         0-(ADDREND-1)
 *  6: 1: reserved / undefined                0-1
 *  7: 3: bytes used                          0-7 (redundant, should be included in addressmode)
 * 10: 4: clock cycles used                   0-15
 * 14: 2: extra clock cycle                   0-2 (1=pagecross 2=branchtaken+pagecross)
 * 16: 1: jump                                0-1 (1=jump or conditional)
 * 17: 3: extra byte used when 16 bit A,X,Y   0,1,2,4 (65816 only, 1=A 2=X,Y 4=bank on stack)
 * 20:    unused
 */

/* needs to be aligned with mnemonic_t */
const char *mnemonics[] = {
   "???",
   "ADC",
   "AHX",
   "ALR",
   "ANC",
   "AND",
   "ARR",
   "ASL",
   "ASR",
   "ASW",
   "AUG",
   "AXS",
   "BBR",
   "BBS",
   "BCC",
   "BCS",
   "BEQ",
   "BIT",
   "BMI",
   "BNE",
   "BPL",
   "BRA",
   "BRK",
   "BRL",
   "BSR",
   "BVC",
   "BVS",
   "CLC",
   "CLD",
   "CLE",
   "CLI",
   "CLV",
   "CMP",
   "COP",
   "CPX",
   "CPY",
   "CPZ",
   "DCP",
   "DEC",
   "DEW",
   "DEX",
   "DEY",
   "DEZ",
   "EOR",
   "INC",
   "INW",
   "INX",
   "INY",
   "INZ",
   "ISC",
   "JML",
   "JMP",
   "JSL",
   "JSR",
   "KIL",
   "LAS",
   "LAX",
   "LDA",
   "LDX",
   "LDY",
   "LDZ",
   "LSR",
   "LXA",
   "MVN",
   "MVP",
   "NEG",
   "NOP",
   "ORA",
   "PEA",
   "PEI",
   "PER",
   "PHA",
   "PHB",
   "PHD",
   "PHK",
   "PHP",
   "PHW",
   "PHX",
   "PHY",
   "PHZ",
   "PLA",
   "PLB",
   "PLD",
   "PLP",
   "PLX",
   "PLY",
   "PLZ",
   "RAA",
   "REP",
   "RLA",
   "RMB",
   "ROL",
   "ROR",
   "ROW",
   "RRA",
   "RTI",
   "RTL",
   "RTN",
   "RTS",
   "SAX",
   "SBC",
   "SEC",
   "SED",
   "SEE",
   "SEI",
   "SEP",
   "SHX",
   "SHY",
   "SLO",
   "SMB",
   "SRE",
   "STA",
   "STP",
   "STX",
   "STY",
   "STZ",
   "TAB",
   "TAS",
   "TAX",
   "TAY",
   "TAZ",
   "TBA",
   "TCD",
   "TCS",
   "TDC",
   "TRB",
   "TSB",
   "TSC",
   "TSX",
   "TSY",
   "TXA",
   "TXS",
   "TXY",
   "TYA",
   "TYS",
   "TYX",
   "TZA",
   "WAI",
   "WDM",
   "XAA",
   "XBA",
   "XCE",
   0
};

#define OPCODE(mn, am, reserved, bytes, cycles, extra, jump, mxe) \
   (uint32_t)mn | (uint16_t)am << 8 | reserved << 14 | bytes << 15 | cycles << 18 | extra << 22 | jump << 24 | mxe << 25

uint32_t opcodes6502[0x100] = {
#include "opcodes6502.tab"
};

uint32_t opcodes65c02[0x100] = {
#include "opcodes65c02.tab"
};

uint32_t opcodes65816[0x100] = {
#include "opcodes65816.tab"
};

uint32_t opcodes65ce02[0x100] = {
#include "opcodes65ce02.tab"
};

uint32_t opcodes65sc02[0x100] = {
#include "opcodes65sc02.tab"
};


static uint32_t *disass_opcodes = 0;


bool trace_is_write( uint32_t trace )
{
   return (trace & bus_config.mask_rw) == 0;
}


uint16_t trace_address( uint32_t trace )
{
   return (trace >> bus_config.shift_address);
}


uint8_t trace_data( uint32_t trace )
{
   return (trace >> bus_config.shift_data);
}


void disass_set_cpu( cputype_t cpu )
{
   cputype = cpu;
   switch( cpu )
   {
      case CPU_6502:
      case CPU_6502RA:
         disass_opcodes = &opcodes6502[0];
         break;
      case CPU_65C02:
         disass_opcodes = &opcodes65c02[0];
         break;
      case CPU_65816:
         disass_opcodes = &opcodes65816[0];
         break;
      case CPU_65CE02:
         disass_opcodes = &opcodes65ce02[0];
         break;
      case CPU_65SC02:
         disass_opcodes = &opcodes65sc02[0];
         break;
      default:
         disass_opcodes = 0;
   }
}


cputype_t disass_get_cpu()
{
   return cputype;
}


#if 0
uint8_t disass_bytes_by_addrmode( uint8_t p0 )
{
   uint8_t retval = 0;

   if( !disass_opcodes )
   {
      return 0;
   }

   switch( PICK_ADDRMODE(disass_opcodes[p0]) )
   {
      case IMP:   // OPC
         retval = 1;
         break;
      case IMM:   // OPC #$01
      case REL:   // OPC LABEL
      case RELSY: // OPC LABEL
      case ZP:    // OPC $12
      case ZPN:   // OPC# $12
      case ZPI:   // OPC ($12)
      case ZPIX:  // OPC ($12,X)
      case ZPIY:  // OPC ($12),Y
      case ZPIZ:  // OPC ($12),Z
      case ZPIL:  // OPC [$12]
      case ZPILY: // OPC [$12],Y
      case ZPISY: // OPC ($12,S),Y
      case ZPS:   // OPC $12,S
      case ZPX:   // OPC $12,X
      case ZPY:   // OPC $12,Y
         retval = 2;
         break;
      case ABS:   // OPC $1234
      case ABSIL: // OPC [$1234]
      case ABSLY: // OPC $123456,Y
      case ABSX:  // OPC $1234,X
      case ABSY:  // OPC $1234,Y
      case ABSZ:  // OPC $1234,Z
      case AI:    // OPC ($1234)
      case AIX:   // OPC ($1234,X)
      case IMM2:  // OPC #$01,#$02
      case IMML:  // OPC #$1234
      case RELL:  // OPC LABEL
      case ZPNR:  // OPC# $12,LABEL
         retval = 3;
         break;
      case ABSL:  // OPC $123456
      case ABSLX: // OPC $123456,X
      case AIL:   // OPC ($123456)
         retval = 4;
         break;
      default:
         break;
   }
   return retval;
}
#endif


uint32_t disass_opcode( uint8_t oc )
{
   return disass_opcodes[oc];
}


uint8_t disass_bytes( uint8_t p0 )
{
   if( !disass_opcodes )
   {
      return 0;
   }

   if( cputype == CPU_65816 )
   {
      if( PICK_MXE(disass_opcodes[p0]) & mx_flag_816 )
      {
         return PICK_BYTES(disass_opcodes[p0]) + 1;
      }
   }
   return PICK_BYTES(disass_opcodes[p0]);
}


uint8_t disass_basecycles( uint8_t p0 )
{
   if( !disass_opcodes )
   {
      return 0;
   }

   if( cputype == CPU_65816 )
   {
      if( PICK_MXE(disass_opcodes[p0]) & mx_flag_816 )
      {
         return PICK_CYCLES(disass_opcodes[p0]) + 1;
      }
   }
   return PICK_CYCLES(disass_opcodes[p0]);
}


addrmode_t disass_addrmode( uint8_t p0 )
{
   if( !disass_opcodes )
   {
      return 0;
   }

   return PICK_ADDRMODE(disass_opcodes[p0]);
}


mnemonic_t disass_mnemonic( uint8_t p0 )
{
   if( !disass_opcodes )
   {
      return 0;
   }

   return PICK_MNEMONIC(disass_opcodes[p0]);
}


const char *disass_mnemonic_string( uint8_t p0 )
{
   return mnemonics[disass_mnemonic( p0 )];
}


bool disass_is_jump( uint8_t p0 )
{
   if( !disass_opcodes )
   {
      return 0;
   }

   return PICK_JUMP(disass_opcodes[p0]);
}


uint8_t disass_extracycles( uint8_t p0 )
{
   if( !disass_opcodes )
   {
      return 0;
   }

   return PICK_EXTRA(disass_opcodes[p0]);
}


bool disass_bcdextracycles( uint8_t p0 )
{
   if( (cputype == CPU_65C02) ||
       (cputype == CPU_65SC02) )
   {
      mnemonic_t mnemonic = PICK_MNEMONIC(disass_opcodes[p0]);

      if( (mnemonic == ADC) || (mnemonic == SBC) )
      {
         return true;
      }
   }
   return false;
}

static int snprimm( char *b, size_t bsize, uint8_t p0, uint8_t p1, uint8_t p2 )
{
   uint8_t mxe = PICK_MXE(disass_opcodes[p0]);
   if( cputype == CPU_65816 )
   {
      if( mxe & mx_flag_816 )
      {
         return snprintf( b, bsize, "#$%04X",  p1 | (p2 << 8) );
      }
   }
   /* default: */
   return snprintf( b, bsize, "#$%02X",        p1 );
}

static const char *_disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, bool brkoverride )
{
   static char buffer[64] = { 0 };
   char *b = buffer;
   size_t bsize = sizeof(buffer)-1;

   if( !disass_opcodes )
   {
      buffer[0] = 0;
      strncpy( b, "CPU not set", bsize );
      return b;
   }

   if( show_flags & DISASS_SHOW_ADDRESS )
   {
      int p = snprintf( b, bsize, "$%04X: ", addr );
      b += p;
      bsize -= p;
   }
   if( show_flags & DISASS_SHOW_HEXDUMP )
   {
      int i, s = disass_bytes( p0 );
      uint8_t bytes[] = { p0, p1, p2, p3 };
      if( brkoverride && (p0 == 0x00) )
      {
         s = 1;
      }
      for( i = 0; i < (sizeof(bytes)/sizeof(bytes[0])); ++i )
      {
         if( i < s )
         {
            snprintf( b, bsize, "%02X ", bytes[i] );
         }
         else
         {
            snprintf( b, bsize, "   " );
         }
         b += 3;
         bsize -= 3;
      }
   }

   addrmode_t addrmode = PICK_ADDRMODE(disass_opcodes[p0]);

   if( (addrmode == ZPN) || (addrmode == ZPNR) )
   {
      snprintf( b, bsize, "%s%d ", disass_mnemonic_string(p0), (p0 >> 4) & 7 );
   }
   else
   {
      bool reserved = PICK_RESERVED(disass_opcodes[p0]);
      if( (cputype == CPU_6502RA) &&
          (PICK_MNEMONIC(disass_opcodes[p0]) == ROR ) )
      {
         // we are running a Rev.A, so ROR is an undefined opcode
         reserved = true;
      }
      snprintf( b, bsize, "%s%c ", disass_mnemonic_string(p0), reserved ? '.' : ' ' );
   }
   b += 5;
   bsize -= 5;

   if( brkoverride && (p0 == 0x00) )
   {
      addrmode = IMP;
   }
   switch( addrmode )
   {
      case ABS:   // OPC $1234
         snprintf( b, bsize, "$%04X",         p1 | (p2 << 8) );
         break;
      case ABSIL: // OPC [$1234]
         snprintf( b, bsize, "[$%04X]",       p1 | (p2 << 8) );
         break;
      case ABSL:  // OPC $123456
         snprintf( b, bsize, "$%06X",         p1 | (p2 << 8) | (p3 << 16) );
         break;
      case ABSLX: // OPC $123456,X
         snprintf( b, bsize, "$%06X,X",       p1 | (p2 << 8) | (p3 << 16) );
         break;
      case ABSLY: // OPC $123456,Y
         snprintf( b, bsize, "$%06X,Y",       p1 | (p2 << 8) | (p3 << 16) );
         break;
      case ABSX:  // OPC $1234,X
         snprintf( b, bsize, "$%04X,X",       p1 | (p2 << 8) );
         break;
      case ABSY:  // OPC $1234,Y
         snprintf( b, bsize, "$%04X,Y",       p1 | (p2 << 8) );
         break;
      case ABSZ:  // OPC $1234,Z
         snprintf( b, bsize, "$%04X,Z",       p1 | (p2 << 8) );
         break;
      case AI:    // OPC ($1234)
         snprintf( b, bsize, "($%04X)",       p1 | (p2 << 8) );
         break;
      case AIL:   // OPC ($123456)
         snprintf( b, bsize, "($%06X)",       p1 | (p2 << 8) | (p3 << 16) );
         break;
      case AIX:   // OPC ($1234,X)
         snprintf( b, bsize, "($%04X,X)",     p1 | (p2 << 8) );
         break;
      case IMP:   // OPC
         // strip off trainling spaces here
         *(b-1) = '\0';
         if( *(b-2) == ' ' )
         {
            *(b-2) = '\0';
         }
         break;
      case IMM:   // OPC #$01
         //snprintf( b, bsize, "#$%02X",        p1 );
         snprimm( b, bsize,               p0, p1, p2 );
         break;
      case IMM2:  // OPC #$01,#$02
         snprintf( b, bsize, "#$%02X,#$%02X", p1, p2 );
         break;
      case IMML:  // OPC #$1234
         snprintf( b, bsize, "#$%04X",        p1 | (p2 << 8) );
         break;
      case REL:   // OPC LABEL
         snprintf( b, bsize, "$%04X",         ((addr+2) + (int8_t)p1) & 0xFFFF );
         break;
      case RELL:  // OPC LABEL
         snprintf( b, bsize, "$%04X",         ((addr+3) + (int16_t)(p1 | (p2 << 8))) & 0xFFFF );
         break;
      case RELSY:   // OPC #$01
         snprintf( b, bsize, "(#$%02X,S),Y",  p1 );
         break;
      case ZP:    // OPC $12
      case ZPN:   // OPC# $12
         snprintf( b, bsize, "$%02X",         p1 );
         break;
      case ZPI:   // OPC ($12)
         snprintf( b, bsize, "($%02X)",       p1 );
         break;
      case ZPIX:  // OPC ($12,X)
         snprintf( b, bsize, "($%02X,X)",     p1 );
         break;
      case ZPIY:  // OPC ($12),Y
         snprintf( b, bsize, "($%02X),Y",     p1 );
         break;
      case ZPIZ:  // OPC ($12),Z
         snprintf( b, bsize, "($%02X),Z",     p1 );
         break;
      case ZPIL:  // OPC [$12]
         snprintf( b, bsize, "[$%02X]",       p1 );
         break;
      case ZPILY: // OPC [$12],Y
         snprintf( b, bsize, "[$%02X],Y",     p1 );
         break;
      case ZPISY: // OPC ($12,S),Y
         snprintf( b, bsize, "($%02X,S),Y",   p1 );
         break;
      case ZPNR:  // OPC# $12,LABEL
         snprintf( b, bsize, "$%02X,$%04X",   p1, (addr+3) + (int8_t)p2 );
         break;
      case ZPS:   // OPC $12,S
         snprintf( b, bsize, "$%02X,S",       p1 );
         break;
      case ZPX:   // OPC $12,X
         snprintf( b, bsize, "$%02X,X",       p1 );
         break;
      case ZPY:   // OPC $12,Y
         snprintf( b, bsize, "$%02X,Y",       p1 );
         break;
      default:
         strncpy( b-5, "internal error", bsize+5 ); // overwrite opcode
         break;
   }

   return buffer;
}


const char *disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 )
{
   return _disass( addr, p0, p1, p2, p3, false );
}


const char *disass_brk1( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 )
{
   return _disass( addr, p0, p1, p2, p3, true );
}


const char *disass_trace( fullinfo_t fullinfo )
{
   return disass( fullinfo.address, fullinfo.data, fullinfo.data1, fullinfo.data2, fullinfo.data3 );
}
