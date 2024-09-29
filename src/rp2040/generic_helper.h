
#ifndef TOOLS_H
#define TOOLS_H TOOLS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * all detectable CPU instruction sets
 */
typedef enum {
   CPU_ERROR, CPU_6502, CPU_65C02, CPU_65816, CPU_65CE02, CPU_65SC02, CPU_UNDEF
} cputype_t;

/*
 * convert enum to a string for display purposes
 */
const char *cputype_name( cputype_t cputype );


/*
 * hexdump some data
 */
void hexdump( uint8_t *memory, uint16_t address, uint32_t size );

/*
 * TODO!
 */
const char* decode_trace( uint32_t state, bool bank_enabled, uint8_t bank );

#endif
