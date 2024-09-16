
#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H DISASSEMBLE_H

#include <stdbool.h>
#include <stdint.h>

#include "cpu_detect.h" // for cputype_t

void disass_cpu( cputype_t cpu );
void disass_mx816( bool m, bool x );
void disass_show_address( bool enable );
int disass_debug_info( uint32_t id );
const char *disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 );
uint8_t disass_bytes( uint8_t opcode );

#endif
