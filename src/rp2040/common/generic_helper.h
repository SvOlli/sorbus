
#ifndef TOOLS_H
#define TOOLS_H TOOLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>


#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

/* for cputype_t and debug_trace() */
#include "../disassemble/disassemble.h"

/*
 * callback function for hexdump to get/set memory data
 * bank is either bank of 65816 or custom banking
 * address is typically uint16_t
 * note: functions MUST NOT trigger any events
 */
typedef uint8_t (*peek_t)(uint8_t,uint16_t);
typedef void (*poke_t)(uint8_t,uint16_t,uint8_t);


/*
 * hexdump some data
 */
void print_hexdump_buffer( uint8_t bank, const uint8_t *memory, uint32_t size,
                           bool showbank );
void print_hexdump( peek_t peek, uint8_t bank, uint16_t address, uint32_t size,
                    bool showbank );

/* Read a 4-digt address from console and check if it is below "lastaddr" */
int32_t get_16bit_address( uint16_t lastaddr );

/* on RP2040 does two things:
 * - check if current free memory is below recoded minimum
 * - return recorded minimum of free memory
 * on host should return 0 */
uint32_t mf_checkheap();

/* wrappers that use mf_checkheap */
void *mf_malloc(size_t size);
void mf_free(void *ptr);
void *mf_calloc(size_t nmemb, size_t size);
void *mf_realloc(void *ptr, size_t size);


/* convert a character from 8bit to 16bit UTF representation
 * according to charset */
uint16_t tocs16( uint8_t ch, uint8_t cs );

/*
 * like putchar, print a character
 * unlike putchar, it can print an UTF-8 sequence of a predefined charset
 * defined charsets:
 * 0: pass through; native UTF-8
 * 1: Sorbus handpicked configuration
 */
int putcharset( uint8_t ch, uint8_t cs );

/*
 * same as putcharset, but returns instead of printing returns
 * string that represents a single UTF-8 character
 */
const char *tocharset( uint8_t ch, uint8_t cs );

/*
 * library function to upload data to RAM via xmodem
 */
int xmodem_receive( poke_t poke, uint16_t addr );

#endif
