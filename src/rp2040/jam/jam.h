
#ifndef _JAM_H_
#define _JAM_H_ _JAM_H_

#include "fifo256.h"
#include "common/generic_helper.h"
#include "flash_config.h"

#include <stdbool.h>
#include <stdint.h>


// ids for the event queue
#define BUSMSG_EVENT_RESET       (0x01000000)
#define BUSMSG_RESET_START       (BUSMSG_EVENT_RESET |   0x0001)
#define BUSMSG_RESET_CLEAR       (BUSMSG_EVENT_RESET |   0x0002)

#define BUSMSG_TYPE_IO           (0x02000000) // write, read +0x01000000
#define BUSMSG_IO_WRITE          (BUSMSG_TYPE_IO   | 0x00000000)
#define BUSMSG_IO_READ           (BUSMSG_TYPE_IO   | 0x01000000)

#define BUSMSG_EVENT_META        (0x04000000)
#define BUSMSG_META_MAGICKEY     (BUSMSG_EVENT_META  |   0x0001)
#define BUSMSG_META_TRAP         (BUSMSG_EVENT_META  |   0x0002)
#define BUSMSG_META_WATCHDOG     (BUSMSG_EVENT_META  |   0x0003)

#define BUSMSG_EVENT_CPUFREQ     (0x05000000)
#define BUSMSG_EVENT_TIMER       (0x06000000)
#define BUSMSG_EVENT_FLASH_SYNC  (0x07000000)


#define MEM_ADDR_BANK            (0xDF00)
#define MEM_ADDR_SBCID           (0xDF01)
#define MEM_ADDR_TRAP            (0xDF01)
#define MEM_ADDR_RANDOM          (0xDF02)
#define MEM_ADDR_XRAMSW          (0xDF03)
#define MEM_ADDR_CPUID           (0xDF04)

#define MEM_ADDR_UART_CONTROL    (0xDF0B)

#define MEM_ADDR_TIMERS          (0xDF10)

#define MEM_ADDR_WATCHDOG        (0xDF20)
#define MEM_ADDR_CYCLECOUNT      (0xDF24)

#define MEM_ADDR_INT_DRIVE       (0xDF70)
#define MEM_ADDR_ID_LBA          (MEM_ADDR_INT_DRIVE + 0)
#define MEM_ADDR_ID_MEM          (MEM_ADDR_INT_DRIVE + 2)


// "mirrors" of FFFA and FFFE executed by jmp ($)
#define MEM_ADDR_UVNMI           (0xDF7A)
#define MEM_ADDR_UVIRQ           (0xDF7E)

// IRQ gets checked by default code into BRK no NBI (non-BRK-interrupt)
#define MEM_ADDR_UVBRK           (0xDF78)
#define MEM_ADDR_UVNBI           (0xDF7C)

// this is where the write protected area starts
#define ROM_START                (0xE000)
// this is where the kernel is in raw flash
#define FLASH_KERNEL_START       (FLASH_KERNEL_START_TXT)


#define XRAMSW_STATUS_BIT        (0x80)
#define NUMBER_OF_BANKS          (sizeof(rom) >> 13) // ROMSIZE / 0x2000


// main.c
// version string for meta mode
extern const char *sorbus_version;

// core_bus.c
// 64k of RAM and I/O cache
extern uint8_t ram[0x10000];
// buffer for roms
extern uint8_t rom[FLASH_DRIVE_START_TXT-FLASH_KERNEL_START_TXT];
// pointer into current ROM/RAM bank at $E000
extern const uint8_t *romvec;
// state of bus
extern uint32_t state;
// hardware/clocks.c
extern uint64_t time_per_mcc;

// setup everything required for the bus core to run
void setup_bus();
// main loop for the bus core
void main_bus();

// core_io.c
// exit 65c02 iohandler/console loop
extern uint32_t bus_stop_cause;

// enter meta mode by leaving 65c02 iohandler/console loop
void event_meta( uint32_t value );

// setup everything required for the i/o core to run
void setup_io();
// main loop for the i/o core
void main_io();

typedef union
{
   uint32_t       raw      :32;
   struct {
      /* similar to trace data */
      uint16_t    address  :16;
      uint8_t     data     : 8; /* only valid during write */
      uint8_t     type     : 8; /* r/!w is encoded as lowest bit of type */
   };
} busmsg_t;

typedef enum console_type_e {
   CONSOLE_TYPE_65C02,
   CONSOLE_TYPE_RP2040
} console_type_t;

// Core0: handle (user) I/O
void console_type_set( console_type_t type );
void console_run();
void console_set_uart( uint8_t charset );
void cpu_halt( bool stop );

// Core0: handle internal I/O
void io_run();
void io_post_misc( bool rw, uint8_t data, uint16_t address );
void io_post_uart( bool rw, uint8_t data, uint16_t address );
void io_post_timer( bool rw, uint8_t data, uint16_t address );
void io_post_watchdog( bool rw, uint8_t data, uint16_t address );
void io_post_drive( bool rw, uint8_t data, uint16_t address );

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
   DEBUG_INFO_TIMERS,
   DEBUG_INFO_EVENTQUEUE
} debug_info_t;
const char *debug_get_info( debug_info_t page );

void debug_get_backtrace( uint32_t **trace, uint32_t *entries, uint32_t *start );
void debug_raw_backtrace();
uint8_t debug_peek( uint8_t bank, uint16_t addr );
void debug_poke( uint8_t bank, uint16_t addr, uint8_t value );

// clocks.c
void event_cpufreq( uint32_t data );
void clocks_reset();
int info_clocks( char *buffer, size_t size );
void io_post_watchdog( bool rw, uint8_t data, uint16_t address );

// drive.c
void event_flash_sync( __unused uint32_t value );
void io_post_intdrive( bool rw, uint8_t data, uint16_t address );
int info_internaldrive( char *buffer, size_t size );

// misc.c
void misc_reset();
void io_post_misc( bool rw, uint8_t data, uint16_t address );
int info_sysvectors( char *buffer, size_t size );

// reset.c
void event_reset( uint32_t value );
void system_init();

// timers.c
void timers_reset();
void event_timer_cycle( uint32_t value );
void io_post_timer( bool rw, uint8_t data, uint16_t address );
int info_timers( char *b, size_t bsize );

// uart.c
extern bool console_crlf_enabled;
extern bool console_flowcontrol_enabled;
extern uint8_t console_charset;
extern fifo256_t uart_in_queue, uart_out_queue;
void io_post_uart( bool rw, uint8_t data, uint16_t address );
void event_watchdog( uint32_t data );
void uart_reset();

static inline bool uart_input_add( uint8_t data )
{
   if( !fifo256_put( &uart_in_queue, data ) )
   {
      // buffer full: flag an error
      ram[MEM_ADDR_UART_CONTROL] |= 0x80;
      return false;
   }
   return true;
}

#if 0
static inline bool uart_output_fetch( uint8_t *data )
{
   return fifo256_get( &uart_out_queue, data );
}
#else
// to make sure, we're as fast as possible
#define uart_output_fetch( data ) fifo256_get( &uart_out_queue, data )
#endif

#endif
