
#ifndef _NATIVE_COMMON_H_
#define _NATIVE_COMMON_H_ _NATIVE_COMMON_H_

typedef enum console_type_e {
   CONSOLE_TYPE_65C02,
   CONSOLE_TYPE_RP2040
} console_type_t;

// communication between Core0 and Core1 via terminal
extern queue_t queue_uart_read;
extern queue_t queue_uart_write;

// Core0: handle (user) I/O
void console_type_set( console_type_t type );
void console_run();

// Core1: handle 65c02 bus
void bus_run();
void system_init();
void system_reboot();
void cpu_halt( bool stop );
void core0_init(void);

#endif

