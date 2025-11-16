
#ifndef _NATIVE_COMMON_H_
#define _NATIVE_COMMON_H_ _NATIVE_COMMON_H_

#include <stdint.h>
#include "common/generic_helper.h"

typedef enum console_type_e {
   CONSOLE_TYPE_65C02,
   CONSOLE_TYPE_RP2040
} console_type_t;

#define SYSTEM_TRAP     (0x1000)
#define SYSTEM_WATCHDOG (0x1001)

// communication between Core0 and Core1 via terminal
extern queue_t queue_uart_read;
extern queue_t queue_uart_write;

// Core0: handle (user) I/O
void console_type_set( console_type_t type );
void console_run();
void console_set_crlf( bool enable );
void console_set_flowcontrol( bool enable );
void cpu_halt( bool stop );

// Core1: handle 65c02 bus
void bus_run();
void system_init();
void system_reboot();

// Core1: debug output routines called from Core0 when CPU is stopped
typedef enum {
   DEBUG_INFO_ERR = 0,
   DEBUG_INFO_HEAP,
   DEBUG_INFO_CLOCKS,
   DEBUG_INFO_SYSVECTORS,
   DEBUG_INFO_INTERNALDRIVE,
   DEBUG_INFO_EVENTQUEUE
} debug_info_t;
const char *debug_get_info( debug_info_t page );
void debug_get_backtrace( cputype_t *cpu, uint32_t **trace, uint32_t *entries, uint32_t *start );
void debug_raw_backtrace();
void debug_disassembler();
uint8_t debug_banks();
uint8_t debug_peek( uint8_t bank, uint16_t addr );
void debug_poke( uint8_t bank, uint16_t addr, uint8_t value );

#endif
