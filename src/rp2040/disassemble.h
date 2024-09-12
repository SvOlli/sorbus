
#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H DISASSEMBLE_H

#include <stdint.h>

typedef enum {
   UNKNOWN = 0, CPU6502, CPU65C02, CPU65CE02, CPU65816
} cputype_t;

void disass_cpu( cputype_t cpu );
uint8_t disass_bytes( uint8_t opcode );
char *disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 );

#endif
