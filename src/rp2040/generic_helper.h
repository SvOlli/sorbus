
#ifndef TOOLS_H
#define TOOLS_H TOOLS_H

#include <stdbool.h>
#include <stdint.h>


#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

/*
 * all detectable CPU instruction sets
 */
typedef enum {
   CPU_ERROR=0, CPU_6502, CPU_65C02, CPU_65816, CPU_65CE02, CPU_6502RA, CPU_65SC02, CPU_UNDEF
} cputype_t;

/*
 * functions to define interface for reading from and writing to memory
 */
typedef uint8_t (*peek_t)(uint16_t);
typedef void (*poke_t)(uint16_t,uint8_t);

/*
 * convert enum to a string for display purposes
 */
const char *cputype_name( cputype_t cputype );


/*
 * hexdump some data
 */
void hexdump_buffer( const uint8_t *memory, uint32_t size );
void hexdump( peek_t peek, uint16_t address, uint32_t size );

/*
 * print trace data in format: aaaa r dd
 */
const char* decode_trace( uint32_t state, bool bank_enabled, uint8_t bank );

#endif
