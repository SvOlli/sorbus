
#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H DISASSEMBLE_H

#include <stdbool.h>
#include <stdint.h>

#include "generic_helper.h" // for cputype_t

#define DISASS_MAX_STR_LENGTH    (40)

/*  */
typedef enum {
   DISASS_SHOW_NOTHING = 0,
   DISASS_SHOW_ADDRESS = 1 << 0,
   DISASS_SHOW_HEXDUMP = 1 << 1
} disass_show_t;

/*  */
typedef struct disass_historian_s *disass_historian_t;

/* set cpu instruction set to disassemble */
void disass_cpu( cputype_t cpu );

/* for 65816, set if accumulator or index register is running in 16 bit mode */
void disass_mx816( bool m, bool x );

/* get the offset of the final byte */
/* 65CE02 jsr: 5, all other 6 cycles */
uint8_t disass_jsr_offset( uint8_t opcode );

/* should disassembler show current address */
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
disass_historian_t disass_historian_init( uint32_t *trace, uint32_t entries );

/*  */
void disass_historian_done( disass_historian_t d );

/*  */
const char *disass_historian_entry( disass_historian_t d, uint32_t entry );


#if 0
/* basic idea is to have two different algorithmic disassemblers
 * historian looks at a decides on a tracelog what was executed
 * clairvoyant look at memory and guesses what will be executed and what
 * the next instruction will be */

/* tries to guess if a new instruction is executed */
/* needs to be fed every cycle of bus activity */
/* on 65816, disass_mx816() needs to be run beforehand (not implemented) */
bool clairvoyant( const uint8_t *memory, uint16_t address, uint8_t data );
#endif

#endif
