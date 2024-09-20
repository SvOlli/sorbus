
#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H DISASSEMBLE_H

#include <stdbool.h>
#include <stdint.h>

#include "cpu_detect.h" // for cputype_t

typedef enum {
   DISASS_SHOW_NOTHING = 0,
   DISASS_SHOW_ADDRESS = 1 << 0,
   DISASS_SHOW_HEXDUMP = 1 << 1
} disass_show_t;

/* set cpu instruction set to disassemble */
void disass_cpu( cputype_t cpu );

/* for 65816, set if accumulator or index register is running in 16 bit mode */
void disass_mx816( bool m, bool x );

/* get the offset of the final byte */
/* 65CE02 jsr: 5, all other 6 cycles */
uint8_t disass_jsr_offset( uint8_t opcode );

/* should disassembler show current address */
void disass_show( disass_show_t show );

const char *disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 );

/* for debugging purposes only */
uint8_t disass_bytes( uint8_t opcode );
int disass_debug_info( uint32_t id );

/* return expected next address to be disassembled */
/* 24 bit value + upper 8 bits as maximum clock cycles */
uint32_t disass_expected();

#endif
