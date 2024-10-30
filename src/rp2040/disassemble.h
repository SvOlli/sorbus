
#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H DISASSEMBLE_H

#include <stdbool.h>
#include <stdint.h>

#include "generic_helper.h" // for cputype_t

#define DISASS_MAX_STR_LENGTH    (40)

/* decide on what to display with disassembly */
typedef enum {
   DISASS_SHOW_NOTHING = 0,
   DISASS_SHOW_ADDRESS = 1 << 0,
   DISASS_SHOW_HEXDUMP = 1 << 1
} disass_show_t;

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

typedef enum {
   DISASS_JMP_CUSTOM = 0,
   DISASS_JMP_ABS,
   DISASS_JMP_REL,
   DISASS_JMP_RELL,
} disass_jmp_t;



/* state of historian "class" */
typedef struct disass_historian_s *disass_historian_t;

/* vector pull is handled differently by 65CE02 and 65816 as
   compared to 65(S)(C)02 */
typedef bool (*disass_is_vector_pull_t)( uint32_t addr0,
   uint32_t addr1, uint32_t addr2, uint32_t addr3, uint32_t addr4 );

/* set cpu instruction set to disassemble */
void disass_cpu( cputype_t cpu );

/* for 65816, set if accumulator or index register is running in 16 bit mode */
void disass_mx816( bool m, bool x );

/* get the offset of the final byte */
/* 65CE02 jsr: 5, all other 6 cycles */
uint8_t disass_jsr_offset( uint8_t opcode );

/* what should disassembler show? (see typedef above for details) */
void disass_show( disass_show_t show );

/* run disassembler for bytes of continuous memory */
const char *disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 );

/* for debugging purposes only */
uint8_t disass_bytes( uint8_t opcode );
int disass_debug_info( uint32_t id );

/* get minimum clockcycles used by an opcode */
uint8_t disass_basecycles( uint8_t p0 );

/* get maximum number of extra clockcycles (e.g. for pagecrossing) */
uint8_t disass_extracycles( uint8_t p0 );

/* get address mode */
addrmode_t disass_addrmode( uint8_t p0 );

/* check if opcode is an instruction that might jump */
bool disass_is_jump( uint8_t p0 );

/* check if trace value is writing to bus */
bool trace_is_write( uint32_t trace );

/* get address bus state from trace */
uint16_t trace_address( uint32_t trace );

/* get data bus state from trace */
uint8_t trace_data( uint32_t trace );


/*  */
disass_historian_t disass_historian_init( cputype_t cputype,
   uint32_t *trace, uint32_t entries, uint32_t start );

/*  */
void disass_historian_done( disass_historian_t d );

/*  */
const char *disass_historian_entry( disass_historian_t d, uint32_t entry );


#if 0
/* basic idea is to have two different algorithmic disassemblers
 * historian looks at a decides on a tracelog what was executed
 * clairvoyant look at memory and guesses what will be executed and what
 * the next instruction will be */

/* callback function for clairvoyant to get memory data */
/* address is typically uint16_t, but can be 24 bit when 65816 is used */
/* note: peek function MUST NOT trigger any events */
typedef uint8_t(*disass_peek_t)(uint16_t);

/* tries to guess if a new instruction is executed */
/* needs to be fed every cycle of bus activity */
/* on 65816, disass_mx816() needs to be run beforehand (not implemented) */
bool clairvoyant( uint32_t address, uint32_t flags, disass_peek_t peek );


#endif

#endif
