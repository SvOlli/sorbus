
#ifndef BASE_TYPES_H
#define BASE_TYPES_H BASE_TYPES_H

#include <stdint.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

#ifndef forever
#define forever() for(;;)
#endif

/*
 * all detectable CPU instruction sets
 * Changes here need to be adjusted in cputype_name as well
 * Needs to be aligned with return values of cpu_detect 6502 code
 */
typedef enum {
   CPU_ERROR=0,
   CPU_6502,
   CPU_65C02,
   CPU_65816,
   CPU_65CE02,
   CPU_6502RA,
   CPU_65SC02,
   CPU_UNDEF
} cputype_t;

/*
 * callback function for hexdump to get/set memory data
 * also used by memory disassembler
 *
 * bank    is either a16-a23 of 65816 or custom banking
 * address is a0-a15
 * data    is d0-d7
 * note: functions MUST NOT trigger any events
 */
/* in: bank, address, out: data */
typedef uint8_t (*peek_t)(uint8_t,uint16_t);
/* in: bank, address, data */
typedef void (*poke_t)(uint8_t,uint16_t,uint8_t);

#endif

