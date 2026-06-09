
#ifndef __SORBUS_RTE_H__
#define __SORBUS_RTE_H__ __SORBUS_RTE_H__

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "da_base.h"


/* generic stuff */

/* get enum from e.g. "65C02" */
cputype_t text2cputype( const char *text );
/* load a file in memory, will always append a null byte at end */
uint8_t *loadfile( const char *filename, ssize_t *filesize );


/* fake memory for simulation */

/* number of banks */
uint8_t memsim_banks();
/* poke to memory */
void memsim_poke( uint8_t bank, uint16_t addr, uint8_t value );
/* peek into memory */
uint8_t memsim_peek( uint8_t bank, uint16_t addr );
/* load a file into memory */
bool memsim_loadfile( uint16_t addr, const char *filename );


/* reading an handling traces (load needs to be done by loadfile with addnull=true) */

/* will replace end marker with null byte to terminate text */
const char *trace_get_start( char *start, cputype_t *cputype );
/* from data prepared by trace_get_start(), create a fulltrace */
da_fullinfo_t *trace_get_fulltrace( const char *start, uint32_t *size );
/* from data prepared by trace_get_start(), create a trace */
uint32_t *trace_get_trace( const char *start, uint32_t *size );
/* convert fulltrace (64 bit with additional info) back to a simple (30 bit) trace */
uint32_t *fulltrace2trace( da_fullinfo_t *refbuffer, uint32_t size );

#endif
