
#ifndef __SORBUS_RTE_H__
#define __SORBUS_RTE_H__ __SORBUS_RTE_H__

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

uint8_t debug_banks();

void debug_poke( uint8_t bank, uint16_t addr, uint8_t value );

uint8_t debug_peek( uint8_t bank, uint16_t addr );

bool debug_loadfile( uint16_t addr, const char *filename );

uint8_t *loadfile( const char *filename, ssize_t *filesize );

#endif
