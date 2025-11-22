
#ifndef TOOLS_H
#define TOOLS_H TOOLS_H

#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>


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
 * callback function for hexdump to get/set memory data
 * bank is either bank of 65816 or custom banking
 * address is typically uint16_t
 * note: functions MUST NOT trigger any events
 */
typedef uint8_t (*peek_t)(uint8_t,uint16_t);
typedef void (*poke_t)(uint8_t,uint16_t,uint8_t);

/*
 * convert enum to a string for display purposes
 */
const char *cputype_name( cputype_t cputype );


/*
 * hexdump some data
 */
void print_hexdump_buffer( uint8_t bank, uint8_t *memory, uint32_t size, bool showbank );
void print_hexdump( peek_t peek, uint8_t bank, uint16_t address, uint32_t size, bool showbank );

/*
 * print trace data in format: aaaa r dd
 */
const char* decode_trace( uint32_t state, bool bank_enabled, uint8_t bank );

/* Read a 4-digt address from console and check if it is below "lastaddr" */
int32_t get_16bit_address( uint16_t lastaddr );

/* on RP2040 does two things:
 * - check if current free memory is below recoded minimum
 * - return recorded minimum of free memory
 * on host should return 0 */
uint32_t mf_checkheap();


/*
 * like putchar, print a character
 * unlike putchar, it can print an UTF-8 sequence of a predefined charset
 * defined charsets:
 * 0: pass through; native UTF-8
 * 1: Sorbus handpicked configuration
 */
int putcharset( uint8_t ch, uint8_t cs );

#endif
