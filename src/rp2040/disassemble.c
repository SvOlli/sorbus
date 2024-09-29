
#include "disassemble.h"

#include <stdio.h>
#include <string.h>

static uint8_t mx_flag_816 = 0;
static disass_show_t show_flags = DISASS_SHOW_NOTHING;
void disass_mx816( bool m, bool x )
{
   mx_flag_816 = (m ? 1 : 0) | (x ? 2 : 0);
}

void disass_show( disass_show_t show )
{
   show_flags = show;
}

typedef struct {
   const char *name;
   uint32_t variant;
} opcode_t;

typedef enum {
   UNDEF = 0,
   ABS,   // OPC $1234
   ABSIL, // OPC [$1234]
   ABSL,  // OPC $123456
   ABSLX, // OPC $123456,X
   ABSLY, // OPC $123456,Y
   ABSX,  // OPC $1234,X
   ABSY,  // OPC $1234,Y
   ABSZ,  // OPC $1234,Z
   AI,    // OPC ($1234)
   AIL,   // OPC ($123456)
   AIX,   // OPC ($1234,X)
   IMP,   // OPC
   IMM,   // OPC #$01
   IMM2,  // OPC #$01,#$02
   IMML,  // OPC #$1234
   REL,   // OPC LABEL
   RELL,  // OPC LABEL
   RELSY, // OPC (LABEL,S),Y
   ZP,    // OPC $12
   ZPI,   // OPC ($12)
   ZPIL,  // OPC [$12]
   ZPILY, // OPC [$12],Y
   ZPISY, // OPC ($12,S),Y
   ZPN,   // OPC# $12
   ZPNR,  // OPC# $12,LABEL
   ZPS,   // OPC $12,S
   ZPX,   // OPC $12,X
   ZPY,   // OPC $12,Y
   ZPIX,  // OPC ($12,X)
   ZPIY,  // OPC ($12),Y
   ZPIZ,  // OPC ($12),Z
   ADDREND
} addrmode_t;

/* at bit:
 *  |  bits used:                             expected values
 *  0: 6: addressmode (enum addrmode)         0-(ADDREND-1)
 *  6: 1: reserved / undefined                0-1
 *  7: 3: bytes used                          0-7 (redundant, should be included in addressmode)
 * 10: 4: clock cycles used                   0-15
 * 14: 2: extra clock cycle                   0-2 (1=pagecross 2=branchtaken+pagecross)
 * 16: 2: extra byte used when 16 bit A,X,Y   0-2 (65816 only, 1=A 2=X,Y)
 * 18: 2: jump                                0-2 (for clairvoyant, 1=jump, 2=conditional)
 * 20:    unused
 */

/*
 * jump types
 *  0: none
 *  1: branch
 *  2: long branch
 *  3: absolute
 *  4: long absolute
 *  5: indirect
 *  6: indirect,x
 *  7: stack (rts,rti)
 */

#define OPCODE(name, am, reserved, bytes, cycles, extra, mx, jump) \
   { name, (uint32_t)am | reserved << 6 | bytes << 7 | cycles << 10 | extra << 14 | mx << 16 | jump << 18 }
#define PICK_OPCODE(o)   ( o->variant        & 0x3F)
#define PICK_RESERVED(o) ((o->variant >> 6)  & 0x01)
#define PICK_BYTES(o)    ((o->variant >> 7)  & 0x07)
#define PICK_CYCLES(o)   ((o->variant >> 10) & 0x0F)
#define PICK_EXTRA(o)    ((o->variant >> 14) & 0x03)
#define PICK_MX(o)       ((o->variant >> 16) & 0x03)
#define PICK_JUMP(o)     ((o->variant >> 18) & 0x03)

opcode_t opcodes6502[] = {
#include "opcodes6502.tab"
};

opcode_t opcodes65c02[] = {
#include "opcodes65c02.tab"
};

opcode_t opcodes65816[] = {
#include "opcodes65816.tab"
};

opcode_t opcodes65ce02[] = {
#include "opcodes65ce02.tab"
};

opcode_t opcodes65sc02[] = {
#include "opcodes65sc02.tab"
};

static opcode_t *disass_opcodes = 0;

int disass_debug_info( uint32_t id )
{
   switch(id)
   {
      case 0:
         return (int)ADDREND;
      default:
         return 0;
   }
}

uint8_t disass_jsr_offset( uint8_t opcode )
{
   opcode_t *o = &disass_opcodes[opcode];
   switch( opcode )
   {
      case 0x20:
         return ((o->variant >> 10) & 0xF) -1;
      default:
         return 0;
   }
}

void disass_cpu( cputype_t cpu )
{
   switch( cpu )
   {
      case CPU_6502:
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

uint8_t disass_bytes( uint8_t p0 )
{
   uint8_t retval = 0;
   opcode_t *o = 0;

   if( !disass_opcodes )
   {
      return 0;
   }
   
   o = disass_opcodes + p0;
   
   switch( PICK_OPCODE(o) )
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

const char *disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 )
{
   static char buffer[64] = { 0 };
   char *b = buffer;
   size_t bsize = sizeof(buffer)-1;
   opcode_t *o = 0;

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
   
   o = disass_opcodes + p0;
   
   switch( PICK_OPCODE(o) )
   {
      case ABS:   // OPC $1234
         snprintf( b, bsize, "%s  $%04X",         o->name, p1 | (p2 << 8) );
         break;
      case ABSIL: // OPC [$1234]
         snprintf( b, bsize, "%s  [$%04X]",       o->name, p1 | (p2 << 8) );
         break;
      case ABSL:  // OPC $123456
         snprintf( b, bsize, "%s  $%06X",         o->name, p1 | (p2 << 8) | (p3 << 16) );
         break;
      case ABSLX: // OPC $123456,X
         snprintf( b, bsize, "%s  $%06X,X",       o->name, p1 | (p2 << 8) | (p3 << 16) );
         break;
      case ABSLY: // OPC $123456,Y
         snprintf( b, bsize, "%s  $%06X,Y",       o->name, p1 | (p2 << 8) | (p3 << 16) );
         break;
      case ABSX:  // OPC $1234,X
         snprintf( b, bsize, "%s  $%04X,X",       o->name, p1 | (p2 << 8) );
         break;
      case ABSY:  // OPC $1234,Y
         snprintf( b, bsize, "%s  $%04X,Y",       o->name, p1 | (p2 << 8) );
         break;
      case ABSZ:  // OPC $1234,Z
         snprintf( b, bsize, "%s  $%04X,Z",       o->name, p1 | (p2 << 8) );
         break;
      case AI:    // OPC ($1234)
         snprintf( b, bsize, "%s  ($%04X)",       o->name, p1 | (p2 << 8) );
         break;
      case AIL:   // OPC ($123456)
         snprintf( b, bsize, "%s  ($%06X)",       o->name, p1 | (p2 << 8) | (p3 << 16) );
         break;
      case AIX:   // OPC ($1234,X)
         snprintf( b, bsize, "%s  ($%04X,X)",     o->name, p1 | (p2 << 8) );
         break;
      case IMP:   // OPC
         snprintf( b, bsize, "%s",                o->name );
         break;
      case IMM:   // OPC #$01
         snprintf( b, bsize, "%s  #$%02X",        o->name, p1 );
         break;
      case IMM2:  // OPC #$01,#$02
         snprintf( b, bsize, "%s  #$%02X,#$%02X", o->name, p1, p2 );
         break;
      case IMML:  // OPC #$1234
         snprintf( b, bsize, "%s  #$%04X",        o->name, p1 | (p2 << 8) );
         break;
      case REL:   // OPC LABEL
         snprintf( b, bsize, "%s  $%04X",         o->name, ((addr+2) + (int8_t)p1) & 0xFFFF );
         break;
      case RELL:  // OPC LABEL
         snprintf( b, bsize, "%s  $%04X",         o->name, ((addr+3) + (int16_t)(p1 | (p2 << 8))) & 0xFFFF );
         break;
      case RELSY:   // OPC #$01
         snprintf( b, bsize, "%s  (#$%02X,S),Y",  o->name, p1 );
         break;
      case ZP:    // OPC $12
         snprintf( b, bsize, "%s  $%02X",         o->name, p1 );
         break;
      case ZPN:   // OPC# $12
         snprintf( b, bsize, "%s%d $%02X",        o->name, (p0 >> 4) & 7, p1 );
         break;
      case ZPI:   // OPC ($12)
         snprintf( b, bsize, "%s  ($%02X)",       o->name, p1 );
         break;
      case ZPIX:  // OPC ($12,X)
         snprintf( b, bsize, "%s  ($%02X,X)",     o->name, p1 );
         break;
      case ZPIY:  // OPC ($12),Y
         snprintf( b, bsize, "%s  ($%02X),Y",     o->name, p1 );
         break;
      case ZPIZ:  // OPC ($12),Z
         snprintf( b, bsize, "%s  ($%02X),Z",     o->name, p1 );
         break;
      case ZPIL:  // OPC [$12]
         snprintf( b, bsize, "%s  [$%02X]",       o->name, p1 );
         break;
      case ZPILY: // OPC [$12],Y
         snprintf( b, bsize, "%s  [$%02X],Y",     o->name, p1 );
         break;
      case ZPISY: // OPC ($12,S),Y
         snprintf( b, bsize, "%s  ($%02X,S),Y",   o->name, p1 );
         break;
      case ZPNR:  // OPC# $12,LABEL
         snprintf( b, bsize, "%s%d $%02X,$%04X",  o->name, (p0 >> 4) & 7, p1, (addr+3) + (int8_t)p2 );
         break;
      case ZPS:   // OPC $12,S
         snprintf( b, bsize, "%s  $%02X,S",       o->name, p1 );
         break;
      case ZPX:   // OPC $12,X
         snprintf( b, bsize, "%s  $%02X,X",       o->name, p1 );
         break;
      case ZPY:   // OPC $12,Y
         snprintf( b, bsize, "%s  $%02X,Y",       o->name, p1 );
         break;
      default:
         strncpy( b, "internal error", bsize );
         break;
   }

   return buffer;
}


#if 0

typedef enum predict_variant{
   PREDICT_NONE         = 0,
   PREDICT_ADDRESS      = 1 << 0,
   PREDICT_CYCLES       = 1 << 1,
   PREDICT_CORRECTION   = 1 << 2
} predict_variant_t;

typedef struct {
   predict_variant_t variant;
   uint16_t          address;
   uint8_t           cycles;
   uint8_t           extra_cycles;
} predict_t;

static struct {
   uint16_t address;
   uint8_t  data;
} clairvoyant_history[0x10] = { 0 }; // size needs to be power of 2;
static int clairvoyant_index = 0;


void clairvoyant_history_add( uint16_t address, uint8_t data )
{
   clairvoyant_history[clairvoyant_index].address = address;
   clairvoyant_history[clairvoyant_index].data    = data;

   ++clairvoyant_index;
   clairvoyant_index &= count_of(clairvoyant_history) - 1;
}


bool clairvoyant( uint32_t address, uint32_t flags, disass_peek_t peek )
{
   bool retval = false;
   static predict_t correction = { 0 };
   static predict_t prediction = { 0 };

   /*
    * 0: next address without branch
    * 1: branch target
    * 2: interrupts
    */
   static uint32_t next      = 0xffffffff;
   static uint32_t jump      = 0xffffffff;
   static uint32_t interrupt = 0xffffffff;

   if( next == 0xffffffff )
   {
      /* first call: initialize */
      next = address )
   }

   if( interrupt == address )
   {
      /* branch was taken, correct output */
      next = address;
      interrupt = 0xffffffff;
   }
   else if( branch == address )
   {
      /* interrupt, reset, etc */
      next = address;
      interrupt = 0xffffffff;
   }

   if( next == address )
   {
      uint8_t opcode = peek(address+0);
      uint8_t byte1  = peek(address+1);
      uint8_t byte2  = peek(address+2);
      uint8_t byte3  = peek(address+3);

      disass( address, opcode, byte1, byte2, byte3 );
      switch( PICK_JUMP(opcode) )
      {
         case  1: // branch
            next = address + 2;
            jump = address + 2 + (int8_t)byte1;
            break;
         case  2: // long branch
            next = address + 3;
            jump = address + 3 + (int16_t)(byte1 | (byte2 << 8));
            break;
         case  3: // absolute
            next = (uint32_t)(byte1 | (byte2 << 8));
            jump = next;
            break;
         case  4: // long absolute
            next = (uint32_t)(byte1 | (byte2 << 8) | (byte3 << 16) );
            jump = next;
            break;
         case  5: // indirect
            next = (uint32_t)(byte1 | (byte2 << 8));
            jump = (uint32_t)(peek(next+0) | (peek(next+1) << 8));
            next = jump;
            break;
         case  6: // indirect,x
            break;
         case  7: // stack (rts,rti)
            break;
         default:
            next = address + PICK_BYTES(opcode);
            break;
      }
   }
}

#endif
