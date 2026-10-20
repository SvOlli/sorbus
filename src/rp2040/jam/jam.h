
#ifndef _JAM_H_
#define _JAM_H_ _JAM_H_

#ifndef SORBUS_JAM_VERSION
#define SORBUS_JAM_VERSION "0.8"
#endif

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
#define META_MAGICKEY            (BUSMSG_EVENT_META  |   0x0001)
#define META_TRAP                (BUSMSG_EVENT_META  |   0x0002)
#define META_WATCHDOG            (BUSMSG_EVENT_META  |   0x0003)
#define META_INTERROR            (BUSMSG_EVENT_META  |   0x0004)

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
#define MEM_ADDR_UART_READ       (0xDF0C)
#define MEM_ADDR_UART_READ_SIZE  (0xDF0D)
#define MEM_ADDR_UART_WRITE      (0xDF0E)
#define MEM_ADDR_UART_WRITE_SIZE (0xDF0F)

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


#define XRAMSW_DONE_BIT          (0x80)
#define NUMBER_OF_BANKS          (sizeof(rom) >> 13) // ROMSIZE / 0x2000


// main.c
// version string for meta mode
extern const char *sorbus_version;

// default is core 0: I/O, core 1: bus -> setting to non-zero switches
#define SWITCH_CORES 0

// core_bus.c
void setup_bankptrs();
// measure cycles per second and also use for logindex
extern uint32_t cycle_counter;          // this is also hard bound to buslog_states index


// 64k of RAM and I/O cache
extern uint8_t ram[0x10000];
// buffer for roms
extern uint8_t rom[FLASH_DRIVE_START_TXT-FLASH_KERNEL_START_TXT];
// pointer into current ROM/RAM bank at $E000 (minus $E000 to compensate offset)
extern const uint8_t *romptr;
// list of possible values for romvec
extern uint8_t *bankptrs[0x100];

// core_io.c
// exit 65c02 iohandler/console loop
extern uint32_t bus_stop_cause;

// functions required for meta-menu
uint8_t debug_peek( uint8_t bank, uint16_t addr );
void debug_poke( uint8_t bank, uint16_t addr, uint8_t value );

// Core0: handle (user) I/O
void console_set_uart( uint8_t charset );

// Core0: handle internal I/O
void io_run();


// Core0: debug output routines called from Core1 when CPU is stopped
typedef enum {
   DEBUG_INFO_ERR = 0,
   DEBUG_INGO_SYSINFO,
   DEBUG_INFO_HEAP,
   DEBUG_INFO_CLOCKS,
   DEBUG_INFO_CYCLECOUNT,
   DEBUG_INFO_SYSVECTORS,
   DEBUG_INFO_INTERNALDRIVE,
   DEBUG_INFO_TIMERS,
   DEBUG_INFO_BUS_VARS,
   DEBUG_INFO_FIFO_INPUT,
   DEBUG_INFO_FIFO_OUTPUT
} debug_info_t;
const char *debug_get_info( debug_info_t page );

// core_bus.c
// Core1: handle 65c02 bus
extern uint32_t multicore_fifo_overflow_counter;
void bus_run();

void info_bus_vars( char *b, size_t bsize );
void debug_get_backtrace( uint32_t **trace, uint32_t *entries, uint32_t *start );
void debug_raw_backtrace();

// clocks.c
void clocks_reset();
int info_clocks( char *buffer, size_t size );
int info_watchdog( char *buffer, size_t size );
void info_cyclecount( char *buffer, size_t size );
void watchdog_freeze();
void watchdog_unfreeze();
void io_irq_watchdog_w( uint32_t in );
void io_irq_watchdog_r( uint32_t in );

// drive.c
extern uint32_t internaldrive_job;
void io_irq_intdrive_w( uint32_t in );
int info_internaldrive( char *buffer, size_t size );
void internaldrive_check_job();

// error.c
void internal_error( const char *file, int line,
                     const char *message, uint32_t value );
int internal_error_info( char *b, size_t bsize );
void internal_error_clear();

// misc.c
void misc_reset();
void io_irq_misc_w( uint32_t in );
void io_irq_misc_r( uint32_t in );
int info_sysvectors( char *buffer, size_t size );

// reset.c
void system_reset( bool init );

// timers.c
void timers_reset();
void io_irq_timer_w( uint32_t in );
int info_timers( char *b, size_t bsize );
void timers_freeze();
void timers_unfreeze();

// uart.c
extern bool console_crlf_enabled;
extern bool console_flowcontrol_enabled;
extern uint8_t console_charset;
extern fifo256_t uart_in_queue, uart_out_queue;
void io_irq_uart_w( uint32_t in );
void io_irq_uart_r( uint32_t in );
void uart_reset();

// TODO: move to core_io.c
static inline bool uart_input_add( uint8_t data )
{
   if( !fifo256_count( &uart_in_queue ) )
   {
      // no data, also add to memory
      ram[MEM_ADDR_UART_READ] = data;
   }
   if( !fifo256_put( &uart_in_queue, data ) )
   {
      // buffer full: flag an error
      ram[MEM_ADDR_UART_CONTROL] |= 0x80;
      return false;
   }
   ram[MEM_ADDR_UART_READ_SIZE] = fifo256_count( &uart_in_queue );
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
