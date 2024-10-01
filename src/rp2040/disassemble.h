
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

/* return expected next address to be disassembled */
/* 24 bit value + upper 8 bits as maximum clock cycles */
uint32_t disass_expected();


/*  */
disass_historian_t disass_historian_init( uint32_t *trace, uint32_t entries, uint32_t start );

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
