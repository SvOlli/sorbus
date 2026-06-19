
#ifndef TOOLS_H
#define TOOLS_H TOOLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>


/* for cputype_t, peek_t, poke_t and count_of */
#include "../disassemble/base_types.h"


/*
 * hexdump some data
 */
void print_hexdump_buffer( uint8_t bank, const uint8_t *memory, uint32_t size,
                           bool showbank );
void print_hexdump( peek_t peek, uint8_t bank, uint16_t address, uint32_t size,
                    bool showbank );

/* Read a 4-digt address from console and check if it is below "lastaddr" */
int32_t get_16bit_address( uint16_t lastaddr );


/* wrappers that use heaptrack */
void *ht_malloc( size_t size );
void ht_free( void *ptr );
void *ht_calloc( size_t nmemb, size_t size );
void *ht_realloc( void *ptr, size_t size );
uint32_t ht_freemem();
uint32_t ht_freemin();


/* convert a character from 8bit to 16bit UTF representation
 * according to charset */
uint16_t cs_to_cs16( uint8_t ch, uint8_t cs );

/*
 * like putchar, print a character
 * unlike putchar, it can print an UTF-8 sequence of a predefined charset
 * defined charsets:
 * 0: pass through; native UTF-8
 * 1: Sorbus handpicked configuration
 */
int cs_put_utf8( uint8_t ch, uint8_t cs );

/*
 * same as cs_put_utf8, but returns instead of printing returns
 * string that represents a single UTF-8 character
 */
const char *cs_to_utf8( uint8_t ch, uint8_t cs );

/*
 * same as above, but with an snprintf-like interface
 */
int cs_sn_utf8( char *b, size_t size, uint8_t ch, uint8_t cs );

/*
 * same as above, but with debug extensions:
 * 0x7f is displayed as 0xFFFD ("error questionmark" symbol)
 * 0x00-0x1f will be displayed as inverted 0x40-0x5f
 * NOTE: an inverted character will need 10(!) bytes in string
 */
int cs_sn_utf8_debug( char *b, size_t bsize, uint8_t ch, uint8_t cs );

/*
 * counts text to be written, answering
 * "if you cursor is a position 0, on what position will it be after
 *  printing this string?"
 * - UTF-8 sequences will be counted as "1"
 * - ANSI/VT100 sequences will be counted as "0"
 */
int cs_textlen( const char *string );

/*
 * library function to upload data to RAM via xmodem
 */
int xmodem_receive( poke_t poke, uint16_t addr );

#endif
