
#ifndef _NATIVE_COMMON_H_
#define _NATIVE_COMMON_H_ _NATIVE_COMMON_H_

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
void debug_backtrace();
void debug_clocks();
void debug_disassembler();
void debug_heap();
void debug_internal_drive();
void debug_memorydump();
void debug_queue_event( const char *text );
void debug_raw_backtrace();

#endif
