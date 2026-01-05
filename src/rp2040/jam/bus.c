/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rp2040_purple.h"

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/rand.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include <hardware/clocks.h>

#include "jam.h"

#include "bus.h"

#include "event_queue.h"
#include "dhara_flash.h"
#include "3rdparty/dhara/error.h"

#include "cpu_detect.h"
#include "disassemble.h"

#define MALLOC_RAM (0)
// this is where the write protected area starts
#define ROM_START (0xE000)
// this is where the kernel is in raw flash
#define FLASH_KERNEL_START (FLASH_KERNEL_START_TXT)

// setup data for binary_info
#define FLASH_DRIVE_INFO  "fs image at " __XSTRING(FLASH_DRIVE_START_TXT)
bi_decl(bi_program_feature(FLASH_DRIVE_INFO))
#define FLASH_KERNEL_INFO "kernel   at " __XSTRING(FLASH_KERNEL_START_TXT)
bi_decl(bi_program_feature(FLASH_KERNEL_INFO))

typedef void (*io_read_handler_t)();
typedef void (*io_write_handler_t)( uint8_t data );

#if MALLOC_RAM
uint8_t *ram = 0;
#else
uint8_t ram[0x10000] = { 0 }; // 64k of RAM and I/O
#endif
uint8_t rom[FLASH_DRIVE_START_TXT-FLASH_KERNEL_START_TXT]; // buffer for roms
const uint8_t *romvec;        // pointer into current ROM/RAM bank at $E000
uint32_t state;
uint32_t address;

// measure time used for processing one million clock cycles (mcc)
uint64_t time_per_mcc          = 1; // in case a division is done

struct repeating_timer timer_ms[2];
uint32_t timer_cycle_total[2]  = { 0, 0 };
bool timer_cycle_triggered[2]  = { false, false };
bool timer_ms_triggered[2]     = { false, false };
const uint32_t timer_mask[2]   = { BUS_CONFIG_mask_irq, BUS_CONFIG_mask_nmi };

bool cpu_running               = false;
bool cpu_running_request       = true;

// number of states should be power of 2
#define BUSLOG_SIZE (1024)
#if (BUSLOG_SIZE & (BUSLOG_SIZE-1))
#error BUSLOG_SIZE is not a power of 2
#endif
uint32_t buslog_states[BUSLOG_SIZE] = { 0 };
uint     buslog_index               = 0;
uint32_t watchdog_cycles_total      = 0;

uint16_t dhara_flash_size = 0;
#define DHARA_SYNC_DELAY (100000)

// identification
const uint8_t sorbus_id[] = { 'S', 'B', 'C', '2', '3', 1, 2, 0 };
const uint8_t *sorbus_id_p;
cputype_t cputype = CPU_UNDEF;
const static uint8_t cpufeatures[] =
{
   0x00, // CPU_ERROR
   0x01, // CPU_6502:   NMOS
   0x06, // CPU_65C02:  CMOS | bit set/reset/branch
   0x12, // CPU_65816:  CMOS | 16 bit
   0x0e, // CPU_65CE02: CMOS | bit set/reset/branch | z-register
   0x21, // CPU_6502RA: NMOS without ROR
   0x02, // CPU_65SC02: CMOS
   0x00  // CPU_UNDEF
};

// magic addresses that cannot be addressed otherwise
#define MEM_ADDR_UART_CONTROL (0xDF0B)
#define MEM_ADDR_ID_LBA       (0xDF70)
#define MEM_ADDR_ID_MEM       (0xDF72)

/******************************************************************************
 * internal functions
 ******************************************************************************/


static inline void bus_data_write( uint8_t data )
{
   gpio_put_masked( bus_config.mask_data, ((uint32_t)data) << bus_config.shift_data );
}


static inline uint8_t bus_data_read()
{
   return (gpio_get_all() >> bus_config.shift_data);
}


void check_cpu_is_halted()
{
   while(gpio_get_all() & bus_config.mask_rdy)
   {
      printf( "\rinternal error: cpu should be stopped." );
   }
}


void system_trap( int type )
{
   // when stopped don't create more traps
   if( !(state & bus_config.mask_rdy) )
   {
      return;
   }

   // notify core 0 to take over
   if( queue_try_add( &queue_uart_write, &type ) )
   {
      // stop the CPU as soon as possible
      gpio_clr_mask( bus_config.mask_rdy );
   }
}


static inline uint8_t get_memory( uint16_t addr )
{
   if( addr < ROM_START )
   {
      return ram[addr];
   }
   else
   {
      return romvec[addr & 0x1FFF];
   }
}


void handle_read_ramrom()
{
   // read from memory and write to bus
   if( address < ROM_START )
   {
      bus_data_write( ram[address] );
   }
   else
   {
      bus_data_write( romvec[address & 0x1FFF] );
   }
}


void handle_write_ram( uint8_t data )
{
   ram[address] = data;
}


void set_bank( uint8_t bank )
{
   // cache bank for later reading
   ram[address] = bank;
   // rom banks are counted starting with 1, as 0 is RAM
   if( bank > (sizeof(rom) / 0x2000) )
   {
      // fallback bank is always kernel
      bank = 1;
   }
   if( bank == 0 )
   {
      romvec = &ram[0xE000];
   }
   else
   {
      // if there are too many banks, the data could be copied from flash
      // to a single 8K RAM buffer during each bankswitch
      // right now with only 3 banks, all banks are copied from flash at startup
      romvec = &rom[(bank - 1) * 0x2000];
   }
}


static inline void event_estimate_cpufreq( __unused void *data )
{
   // event handler for estimating the 65C02 CPU speed
   // this event is exactly 1 time in queue

   static uint64_t time_last = 0;
   uint64_t time_now = time_us_64();

   if( state & bus_config.mask_rdy )
   {
      // only count time when CPU is not halted
      time_per_mcc = time_now - time_last;
   }
   time_last = time_now;

   queue_event_add( 1000000, event_estimate_cpufreq, 0 );
}


static inline void event_watchdog( __unused void *data )
{
   // event handler for watchdog timer
   // this event is max 1 time in queue

   system_trap( SYSTEM_WATCHDOG );
}


void watchdog_setup( uint8_t value )
{
   uint8_t config = address & 0x03;
   if( config )
   {
      if( queue_event_contains( event_watchdog ) )
      {
         // timer is running: restart
         queue_event_cancel( event_watchdog );
         queue_event_add( watchdog_cycles_total, event_watchdog, 0 );
      }
      else
      {
         // not running, set 24 bit value
         watchdog_cycles_total &= ~(0xFF << ((config-1) * 8));
         watchdog_cycles_total |= (value << ((config-1) * 8));
         if( config == 3 )
         {
            // start watchdog
            queue_event_add( watchdog_cycles_total, event_watchdog, 0 );
         }
      }
   }
   else
   {
      // stop watchdog
      queue_event_cancel( event_watchdog );
      watchdog_cycles_total = 0;
   }
}


static inline void event_timer_cycle( void *data )
{
   // data is 32 bit value of 0=IRQ or 1=NMI
   int offset = (int)data;

   // trigger interrupt
   gpio_clr_mask( timer_mask[offset] );

   // set state for reading
   timer_cycle_triggered[offset] = true;

   // restart if requested
   if( timer_cycle_total[offset] )
   {
      queue_event_add( timer_cycle_total[offset], event_timer_cycle, (void*)offset );
   }
}


static inline void timer_cycle_setup( uint8_t value )
{
   uint8_t config = address & 0x07;
   // config bits (decoded from address)
   //           0        1
   // bit 0: lobyte   hibyte
   // bit 1: repeat   oneshot
   // bit 2: irq      nmi
   uint32_t offset = (config & 4) >> 2;

   if( config & (1<<0) )
   {
      // highbyte: store and start timer
      timer_cycle_total[offset] |= value << 8;
      queue_event_add( timer_cycle_total[offset], event_timer_cycle, (void*)offset );
      if( config & (1<<1) )
      {
         // oneshot
         timer_cycle_total[offset] = 0;
      }
   }
   else
   {
      // lowbyte: store and stop timer
      queue_event_cancel_data( event_timer_cycle, (void*)offset );
      timer_cycle_total[offset] = value;
   }
}


void timer_cycle_ack()
{
   // identift interrupt to work with
   uint32_t offset = (address & 4) >> 2;
   // set bit 7 if interrupt was triggered
   bus_data_write( timer_cycle_triggered[offset] ? 0x80 : 0x00 );

   if( timer_cycle_triggered[offset] )
   {
      timer_cycle_triggered[offset] = false;
      // check if all other interrupt source prevents release
      if( !timer_ms_triggered[offset] )
      {
         gpio_set_mask( bus_config.mask_nmi );
      }
   }
}


static bool callback_timer_ms( struct repeating_timer *t )
{
   uint32_t offset = (uint32_t)t->user_data;

   timer_ms_triggered[offset] = true;
   gpio_clr_mask( timer_mask[offset] );

   return true;
}


void timer_ms_setup( uint8_t value )
{
   uint8_t config = address & 0x03;

   uint32_t offset = ((config & 2) >> 1);
   struct repeating_timer *t = &timer_ms[offset];
   uint16_t *tv = (uint16_t*)&ram[address & 0xFFFE];

   // using RAM to store config
   handle_write_ram( value );

   cancel_repeating_timer( t );

   if( *tv )
   {
      add_repeating_timer_us( -100 * (*tv), callback_timer_ms, (void*)offset, t );
   }
}


void timer_ms_ack()
{
   uint8_t config = address & 0x03;
   // identift interrupt to work with
   uint32_t offset = ((config & 2) >> 1);
   // set bit 7 if interrupt was triggered
   bus_data_write( timer_ms_triggered[offset] ? 0x80 : 0x00 );

   if( timer_ms_triggered[offset] )
   {
      timer_ms_triggered[offset] = false;
      // check if all other interrupt source prevents release
      if( !timer_cycle_triggered[offset] )
      {
         gpio_set_mask( timer_mask[offset] );
      }
   }
}


static inline void event_clear_reset( __unused void *data )
{
   gpio_set_mask( bus_config.mask_reset );
}


static inline void system_reset()
{
   int i;

   if( queue_event_contains( event_clear_reset ) )
   {
      // reset sequence is already initiated
      return;
   }

   // clear some internal states
   for( i = 0; i < 2; ++i )
   {
      timer_cycle_total[i]     = 0;
      timer_cycle_triggered[i] = false;
      timer_ms_triggered[i]    = false;
      // stop ms timers
      (void)cancel_repeating_timer( &timer_ms[i] );
      // clear out ms timer registers
      ram[0xDF18+(i>>1)]       = 0;
      ram[0xDF19+(i>>1)]       = 0;
   }
   watchdog_cycles_total = 0;

   // clear event queue and setup end of reset event
   queue_event_reset();
   queue_event_add( 8, event_clear_reset, 0 );
   // start measurement of cpu speed once every million clocks
   queue_event_add( 9, event_estimate_cpufreq, 0 );

   // setup bank
   set_bank( 1 );

   while( queue_try_remove( &queue_uart_read, &i ) )
   {
      // just loop until queue is empty
   }
   while( queue_try_remove( &queue_uart_write, &i ) )
   {
      // just loop until queue is empty
   }

   // setup serial console
   console_set_uart( 1 );
   // reflect changes from above in control register
   // this needs to be set here, as console_set_* cannot access RAM
   ram[MEM_ADDR_UART_CONTROL] = 0x01;
   // reset Sorbus ID pointer
   sorbus_id_p = sorbus_id;
}


void handle_scratch_mem( uint8_t flags )
{
   // not accessable RAM is used as scratch memory:
   // $D000-$D3FF 1k of scratch RAM, can be copied from/to $0000-$03FF
   uint8_t tmpbuf[0x100];
   int i;
   for( i = 0; i < 4; ++i )
   {
      if( flags & (1 << i) )
      {
         switch( flags & 0xC0 )
         {
            case 0x40:
               memcpy( &ram[0xD000+i*0x100], &ram[i*0x100], 0x100 );
               break;
            case 0x80:
               memcpy( &ram[i*0x100], &ram[0xD000+i*0x100], 0x100 );
               break;
            case 0xC0:
               memcpy( &tmpbuf[0], &ram[i*0x100], 0x100 );
               memcpy( &ram[i*0x100], &ram[0xD000+i*0x100], 0x100 );
               memcpy( &ram[0xD000+i*0x100], &tmpbuf[0], 0x100 );
               break;
         }
      }
   }
}


static inline void event_flash_sync( __unused void *data )
{
   // event handler for flushing dhara journal
   // this event is max 1 time in queue

   dhara_flash_sync();
}


static void handle_flash_dma( uint8_t value )
{
   uint16_t *lba = (uint16_t*)&ram[MEM_ADDR_ID_LBA];
   uint16_t *mem = (uint16_t*)&ram[MEM_ADDR_ID_MEM];
   int retval = 0;

   // signalize that work was started
   ram[address] = 0x00;
   // sanity checks
   if( !dhara_flash_size )
   {
      // size = 0: no dhara image found
      ram[address] |= 0xF0;
   }
   // filter out bad ranges, removing second line would write protect ROM
   if( (*mem < 0x0004) ||                                    // zeropage I/O
       ((*mem > (0xD000-SECTOR_SIZE)) && (*mem < 0xDF80)) || // $Dxxx I/O
       (*mem > (0x10000-SECTOR_SIZE)) )                      // would go out of bounds
   {
      // DMA would run into I/O which is not possible, only RAM works
      ram[address] |= 0xF1;
   }
   if( *lba >= 0x8000 ) // could also be if( *lba >= dhara_flash_size )
   {
      // only 32768 sectors are available
      ram[address] |= 0xF2;
   }
   if( ram[address] )
   {
      // error, do not continue
      return;
   }

   switch( address & 3 )
   {
      case 0: // read
         retval = dhara_flash_read( *lba, &ram[*mem] );
         if( queue_event_contains( event_flash_sync ) )
         {
            // delay any waiting sync events
            queue_event_cancel( event_flash_sync );
            queue_event_add( DHARA_SYNC_DELAY, event_flash_sync, 0 );
         }
         break;
      case 1: // write
         retval = dhara_flash_write( *lba, &ram[*mem] );
         // drop any waiting sync events, since a new one is created
         queue_event_cancel( event_flash_sync );
         // after ~.02 seconds without additions writes run sync
         queue_event_add( DHARA_SYNC_DELAY, event_flash_sync, 0 );
         break;
      case 3: // trim
         retval = dhara_flash_trim( *lba );
         // drop any waiting sync events, since a new one is created
         queue_event_cancel( event_flash_sync );
         // after ~.02 seconds without additions writes run sync
         queue_event_add( DHARA_SYNC_DELAY, event_flash_sync, 0 );
         break;
      default:
         // unused addresses are treated like RAM, should not be required
         handle_write_ram( value );
         break;
   }

   if( retval )
   {
      // report error, do not continue
      ram[address] = 0xF4;
      return;
   }

   // increment pointers
   (*lba)++;
   (*mem) += SECTOR_SIZE;
   // signalize that operation was completed successfully
   ram[address] = 0x80;
}


void handle_read_sorbus_id()
{
   bus_data_write( *sorbus_id_p );
   if( *sorbus_id_p )
   {
      ++sorbus_id_p;
   }
   else
   {
      sorbus_id_p = sorbus_id;
   }
}


void handle_read_random()
{
   bus_data_write( rand() & 0xFF );
}


void handle_read_cpufeatures()
{
   bus_data_write( cpufeatures[cputype] );
}


void handle_read_uart()
{
   uint8_t data;
   bool success = queue_try_remove( &queue_uart_read, &data );
   bus_data_write( success ? data : 0x00 );
}


void handle_read_uart_rq()
{
   bus_data_write( queue_get_level( &queue_uart_read )  );
}


void handle_read_uart_wq()
{
   bus_data_write( queue_get_level( &queue_uart_write )  );
}


void handle_read_watchdog()
{
   bus_data_write( watchdog_cycles_total ? 0x80 : 0x00 );
}


void handle_read_cyclecounter()
{
   static union {
      uint32_t value;
      uint8_t  reg[4];
   } shadow_cycle_count;

   if( (address & 0xFF) == 0x24 )
   {
      shadow_cycle_count.value = (uint32_t)_queue_cycle_counter;
   }
   bus_data_write( shadow_cycle_count.reg[address & 3] );
}


void handle_write_trap( __unused uint8_t value )
{
   system_trap( SYSTEM_TRAP );
}


void handle_write_uart_cfg( uint8_t data )
{
   // make sure register is mirrored to RAM for read
   ram[address] = data;
   console_set_uart( data );
}


void handle_write_uart( uint8_t data )
{
   queue_try_add( &queue_uart_write, &data );
}


const static io_read_handler_t io_read_handlers[0x100] =
{
   /* read $DF00 */ handle_read_ramrom,
   /* read $DF01 */ handle_read_sorbus_id,
   /* read $DF02 */ handle_read_random,
   /* read $DF03 */ handle_read_cpufeatures,
   /* read $DF04 */ handle_read_ramrom,
   /* read $DF05 */ handle_read_ramrom,
   /* read $DF06 */ handle_read_ramrom,
   /* read $DF07 */ handle_read_ramrom,
   /* read $DF08 */ handle_read_ramrom,
   /* read $DF09 */ handle_read_ramrom,
   /* read $DF0A */ handle_read_ramrom,
   /* read $DF0B */ handle_read_ramrom,
   /* read $DF0C */ handle_read_uart,
   /* read $DF0D */ handle_read_uart_rq,
   /* read $DF0E */ handle_read_ramrom,
   /* read $DF0F */ handle_read_uart_wq,
   /* read $DF10 */ timer_cycle_ack,
   /* read $DF11 */ timer_cycle_ack,
   /* read $DF12 */ timer_cycle_ack,
   /* read $DF13 */ timer_cycle_ack,
   /* read $DF14 */ timer_cycle_ack,
   /* read $DF15 */ timer_cycle_ack,
   /* read $DF16 */ timer_cycle_ack,
   /* read $DF17 */ timer_cycle_ack,
   /* read $DF18 */ timer_ms_ack,
   /* read $DF19 */ timer_ms_ack,
   /* read $DF1A */ timer_ms_ack,
   /* read $DF1B */ timer_ms_ack,
   /* read $DF1C */ handle_read_ramrom,
   /* read $DF1D */ handle_read_ramrom,
   /* read $DF1E */ handle_read_ramrom,
   /* read $DF1F */ handle_read_ramrom,
   /* read $DF20 */ handle_read_watchdog,
   /* read $DF21 */ handle_read_watchdog,
   /* read $DF22 */ handle_read_watchdog,
   /* read $DF23 */ handle_read_watchdog,
   /* read $DF24 */ handle_read_cyclecounter,
   /* read $DF25 */ handle_read_cyclecounter,
   /* read $DF26 */ handle_read_cyclecounter,
   /* read $DF27 */ handle_read_cyclecounter,
   /* read $DF28 */ handle_read_ramrom,
   /* read $DF29 */ handle_read_ramrom,
   /* read $DF2A */ handle_read_ramrom,
   /* read $DF2B */ handle_read_ramrom,
   /* read $DF2C */ handle_read_ramrom, // used as RAM for BRK routine
   /* read $DF2D */ handle_read_ramrom, // used as RAM for BRK routine
   /* read $DF2E */ handle_read_ramrom, // used as RAM for BRK routine
   /* read $DF2F */ handle_read_ramrom, // used as RAM for BRK routine
   /* read $DF30 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF31 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF32 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF33 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF34 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF35 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF36 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF37 */ handle_read_ramrom, // used as RAM for System Monitor
   /* read $DF38 */ handle_read_ramrom,
   /* read $DF39 */ handle_read_ramrom,
   /* read $DF3A */ handle_read_ramrom,
   /* read $DF3B */ handle_read_ramrom,
   /* read $DF3C */ handle_read_ramrom,
   /* read $DF3D */ handle_read_ramrom,
   /* read $DF3E */ handle_read_ramrom,
   /* read $DF3F */ handle_read_ramrom,
   /* read $DF40 */ handle_read_ramrom,
   /* read $DF41 */ handle_read_ramrom,
   /* read $DF42 */ handle_read_ramrom,
   /* read $DF43 */ handle_read_ramrom,
   /* read $DF44 */ handle_read_ramrom,
   /* read $DF45 */ handle_read_ramrom,
   /* read $DF46 */ handle_read_ramrom,
   /* read $DF47 */ handle_read_ramrom,
   /* read $DF48 */ handle_read_ramrom,
   /* read $DF49 */ handle_read_ramrom,
   /* read $DF4A */ handle_read_ramrom,
   /* read $DF4B */ handle_read_ramrom,
   /* read $DF4C */ handle_read_ramrom,
   /* read $DF4D */ handle_read_ramrom,
   /* read $DF4E */ handle_read_ramrom,
   /* read $DF4F */ handle_read_ramrom,
   /* read $DF50 */ handle_read_ramrom,
   /* read $DF51 */ handle_read_ramrom,
   /* read $DF52 */ handle_read_ramrom,
   /* read $DF53 */ handle_read_ramrom,
   /* read $DF54 */ handle_read_ramrom,
   /* read $DF55 */ handle_read_ramrom,
   /* read $DF56 */ handle_read_ramrom,
   /* read $DF57 */ handle_read_ramrom,
   /* read $DF58 */ handle_read_ramrom,
   /* read $DF59 */ handle_read_ramrom,
   /* read $DF5A */ handle_read_ramrom,
   /* read $DF5B */ handle_read_ramrom,
   /* read $DF5C */ handle_read_ramrom,
   /* read $DF5D */ handle_read_ramrom,
   /* read $DF5E */ handle_read_ramrom,
   /* read $DF5F */ handle_read_ramrom,
   /* read $DF60 */ handle_read_ramrom,
   /* read $DF61 */ handle_read_ramrom,
   /* read $DF62 */ handle_read_ramrom,
   /* read $DF63 */ handle_read_ramrom,
   /* read $DF64 */ handle_read_ramrom,
   /* read $DF65 */ handle_read_ramrom,
   /* read $DF66 */ handle_read_ramrom,
   /* read $DF67 */ handle_read_ramrom,
   /* read $DF68 */ handle_read_ramrom,
   /* read $DF69 */ handle_read_ramrom,
   /* read $DF6A */ handle_read_ramrom,
   /* read $DF6B */ handle_read_ramrom,
   /* read $DF6C */ handle_read_ramrom,
   /* read $DF6D */ handle_read_ramrom,
   /* read $DF6E */ handle_read_ramrom,
   /* read $DF6F */ handle_read_ramrom,
   /* read $DF70 */ handle_read_ramrom,
   /* read $DF71 */ handle_read_ramrom,
   /* read $DF72 */ handle_read_ramrom,
   /* read $DF73 */ handle_read_ramrom,
   /* read $DF74 */ handle_read_ramrom,
   /* read $DF75 */ handle_read_ramrom,
   /* read $DF76 */ handle_read_ramrom, // used to store Z register with int macro on 65CE02
   /* read $DF77 */ handle_read_ramrom,
   /* read $DF78 */ handle_read_ramrom, // used as RAM for system vector user BRK
   /* read $DF79 */ handle_read_ramrom, // used as RAM for system vector user BRK
   /* read $DF7A */ handle_read_ramrom, // used as RAM for system vector NMI ($FFFA)
   /* read $DF7B */ handle_read_ramrom, // used as RAM for system vector NMI ($FFFB)
   /* read $DF7C */ handle_read_ramrom, // used as RAM for system vector user IRQ
   /* read $DF7D */ handle_read_ramrom, // used as RAM for system vector user IRQ
   /* read $DF7E */ handle_read_ramrom, // used as RAM for system vector IRQ ($FFFE)
   /* read $DF7F */ handle_read_ramrom, // used as RAM for system vector IRQ ($FFFF)
   
   /* starting with $DF80 everthing must be reading ram */
   /* read $DF80 */ handle_read_ramrom,
   /* read $DF81 */ handle_read_ramrom,
   /* read $DF82 */ handle_read_ramrom,
   /* read $DF83 */ handle_read_ramrom,
   /* read $DF84 */ handle_read_ramrom,
   /* read $DF85 */ handle_read_ramrom,
   /* read $DF86 */ handle_read_ramrom,
   /* read $DF87 */ handle_read_ramrom,
   /* read $DF88 */ handle_read_ramrom,
   /* read $DF89 */ handle_read_ramrom,
   /* read $DF8A */ handle_read_ramrom,
   /* read $DF8B */ handle_read_ramrom,
   /* read $DF8C */ handle_read_ramrom,
   /* read $DF8D */ handle_read_ramrom,
   /* read $DF8E */ handle_read_ramrom,
   /* read $DF8F */ handle_read_ramrom,
   /* read $DF90 */ handle_read_ramrom,
   /* read $DF91 */ handle_read_ramrom,
   /* read $DF92 */ handle_read_ramrom,
   /* read $DF93 */ handle_read_ramrom,
   /* read $DF94 */ handle_read_ramrom,
   /* read $DF95 */ handle_read_ramrom,
   /* read $DF96 */ handle_read_ramrom,
   /* read $DF97 */ handle_read_ramrom,
   /* read $DF98 */ handle_read_ramrom,
   /* read $DF99 */ handle_read_ramrom,
   /* read $DF9A */ handle_read_ramrom,
   /* read $DF9B */ handle_read_ramrom,
   /* read $DF9C */ handle_read_ramrom,
   /* read $DF9D */ handle_read_ramrom,
   /* read $DF9E */ handle_read_ramrom,
   /* read $DF9F */ handle_read_ramrom,
   /* read $DFA0 */ handle_read_ramrom,
   /* read $DFA1 */ handle_read_ramrom,
   /* read $DFA2 */ handle_read_ramrom,
   /* read $DFA3 */ handle_read_ramrom,
   /* read $DFA4 */ handle_read_ramrom,
   /* read $DFA5 */ handle_read_ramrom,
   /* read $DFA6 */ handle_read_ramrom,
   /* read $DFA7 */ handle_read_ramrom,
   /* read $DFA8 */ handle_read_ramrom,
   /* read $DFA9 */ handle_read_ramrom,
   /* read $DFAA */ handle_read_ramrom,
   /* read $DFAB */ handle_read_ramrom,
   /* read $DFAC */ handle_read_ramrom,
   /* read $DFAD */ handle_read_ramrom,
   /* read $DFAE */ handle_read_ramrom,
   /* read $DFAF */ handle_read_ramrom,
   /* read $DFB0 */ handle_read_ramrom,
   /* read $DFB1 */ handle_read_ramrom,
   /* read $DFB2 */ handle_read_ramrom,
   /* read $DFB3 */ handle_read_ramrom,
   /* read $DFB4 */ handle_read_ramrom,
   /* read $DFB5 */ handle_read_ramrom,
   /* read $DFB6 */ handle_read_ramrom,
   /* read $DFB7 */ handle_read_ramrom,
   /* read $DFB8 */ handle_read_ramrom,
   /* read $DFB9 */ handle_read_ramrom,
   /* read $DFBA */ handle_read_ramrom,
   /* read $DFBB */ handle_read_ramrom,
   /* read $DFBC */ handle_read_ramrom,
   /* read $DFBD */ handle_read_ramrom,
   /* read $DFBE */ handle_read_ramrom,
   /* read $DFBF */ handle_read_ramrom,
   /* read $DFC0 */ handle_read_ramrom,
   /* read $DFC1 */ handle_read_ramrom,
   /* read $DFC2 */ handle_read_ramrom,
   /* read $DFC3 */ handle_read_ramrom,
   /* read $DFC4 */ handle_read_ramrom,
   /* read $DFC5 */ handle_read_ramrom,
   /* read $DFC6 */ handle_read_ramrom,
   /* read $DFC7 */ handle_read_ramrom,
   /* read $DFC8 */ handle_read_ramrom,
   /* read $DFC9 */ handle_read_ramrom,
   /* read $DFCA */ handle_read_ramrom,
   /* read $DFCB */ handle_read_ramrom,
   /* read $DFCC */ handle_read_ramrom,
   /* read $DFCD */ handle_read_ramrom,
   /* read $DFCE */ handle_read_ramrom,
   /* read $DFCF */ handle_read_ramrom,
   /* read $DFD0 */ handle_read_ramrom,
   /* read $DFD1 */ handle_read_ramrom,
   /* read $DFD2 */ handle_read_ramrom,
   /* read $DFD3 */ handle_read_ramrom,
   /* read $DFD4 */ handle_read_ramrom,
   /* read $DFD5 */ handle_read_ramrom,
   /* read $DFD6 */ handle_read_ramrom,
   /* read $DFD7 */ handle_read_ramrom,
   /* read $DFD8 */ handle_read_ramrom,
   /* read $DFD9 */ handle_read_ramrom,
   /* read $DFDA */ handle_read_ramrom,
   /* read $DFDB */ handle_read_ramrom,
   /* read $DFDC */ handle_read_ramrom,
   /* read $DFDD */ handle_read_ramrom,
   /* read $DFDE */ handle_read_ramrom,
   /* read $DFDF */ handle_read_ramrom,
   /* read $DFE0 */ handle_read_ramrom,
   /* read $DFE1 */ handle_read_ramrom,
   /* read $DFE2 */ handle_read_ramrom,
   /* read $DFE3 */ handle_read_ramrom,
   /* read $DFE4 */ handle_read_ramrom,
   /* read $DFE5 */ handle_read_ramrom,
   /* read $DFE6 */ handle_read_ramrom,
   /* read $DFE7 */ handle_read_ramrom,
   /* read $DFE8 */ handle_read_ramrom,
   /* read $DFE9 */ handle_read_ramrom,
   /* read $DFEA */ handle_read_ramrom,
   /* read $DFEB */ handle_read_ramrom,
   /* read $DFEC */ handle_read_ramrom,
   /* read $DFED */ handle_read_ramrom,
   /* read $DFEE */ handle_read_ramrom,
   /* read $DFEF */ handle_read_ramrom,
   /* read $DFF0 */ handle_read_ramrom,
   /* read $DFF1 */ handle_read_ramrom,
   /* read $DFF2 */ handle_read_ramrom,
   /* read $DFF3 */ handle_read_ramrom,
   /* read $DFF4 */ handle_read_ramrom,
   /* read $DFF5 */ handle_read_ramrom,
   /* read $DFF6 */ handle_read_ramrom,
   /* read $DFF7 */ handle_read_ramrom,
   /* read $DFF8 */ handle_read_ramrom,
   /* read $DFF9 */ handle_read_ramrom,
   /* read $DFFA */ handle_read_ramrom,
   /* read $DFFB */ handle_read_ramrom,
   /* read $DFFC */ handle_read_ramrom,
   /* read $DFFD */ handle_read_ramrom,
   /* read $DFFE */ handle_read_ramrom,
   /* read $DFFF */ handle_read_ramrom
};


const static io_write_handler_t io_write_handlers[0x100] =
{
   /* write $DF00 */ set_bank,
   /* write $DF01 */ handle_write_trap,
   /* write $DF02 */ handle_write_ram,
   /* write $DF03 */ handle_scratch_mem,
   /* write $DF04 */ handle_write_ram,
   /* write $DF05 */ handle_write_ram,
   /* write $DF06 */ handle_write_ram,
   /* write $DF07 */ handle_write_ram,
   /* write $DF08 */ handle_write_ram,
   /* write $DF09 */ handle_write_ram,
   /* write $DF0A */ handle_write_ram,
   /* write $DF0B */ handle_write_uart_cfg,
   /* write $DF0C */ handle_write_ram,
   /* write $DF0D */ handle_write_ram,
   /* write $DF0E */ handle_write_uart,
   /* write $DF0F */ handle_write_ram,
   /* write $DF10 */ timer_cycle_setup,
   /* write $DF11 */ timer_cycle_setup,
   /* write $DF12 */ timer_cycle_setup,
   /* write $DF13 */ timer_cycle_setup,
   /* write $DF14 */ timer_cycle_setup,
   /* write $DF15 */ timer_cycle_setup,
   /* write $DF16 */ timer_cycle_setup,
   /* write $DF17 */ timer_cycle_setup,
   /* write $DF18 */ timer_ms_setup,
   /* write $DF19 */ timer_ms_setup,
   /* write $DF1A */ timer_ms_setup,
   /* write $DF1B */ timer_ms_setup,
   /* write $DF1C */ handle_write_ram,
   /* write $DF1D */ handle_write_ram,
   /* write $DF1E */ handle_write_ram,
   /* write $DF1F */ handle_write_ram,
   /* write $DF20 */ watchdog_setup,
   /* write $DF21 */ watchdog_setup,
   /* write $DF22 */ watchdog_setup,
   /* write $DF23 */ watchdog_setup,
   /* write $DF24 */ handle_write_ram,
   /* write $DF25 */ handle_write_ram,
   /* write $DF26 */ handle_write_ram,
   /* write $DF27 */ handle_write_ram,
   /* write $DF28 */ handle_write_ram,
   /* write $DF29 */ handle_write_ram,
   /* write $DF2A */ handle_write_ram,
   /* write $DF2B */ handle_write_ram,
   /* write $DF2C */ handle_write_ram, // used as RAM for BRK routine
   /* write $DF2D */ handle_write_ram, // used as RAM for BRK routine
   /* write $DF2E */ handle_write_ram, // used as RAM for BRK routine
   /* write $DF2F */ handle_write_ram, // used as RAM for BRK routine
   /* write $DF30 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF31 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF32 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF33 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF34 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF35 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF36 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF37 */ handle_write_ram, // used as RAM for System Monitor
   /* write $DF38 */ handle_write_ram,
   /* write $DF39 */ handle_write_ram,
   /* write $DF3A */ handle_write_ram,
   /* write $DF3B */ handle_write_ram,
   /* write $DF3C */ handle_write_ram,
   /* write $DF3D */ handle_write_ram,
   /* write $DF3E */ handle_write_ram,
   /* write $DF3F */ handle_write_ram,
   /* write $DF40 */ handle_write_ram,
   /* write $DF41 */ handle_write_ram,
   /* write $DF42 */ handle_write_ram,
   /* write $DF43 */ handle_write_ram,
   /* write $DF44 */ handle_write_ram,
   /* write $DF45 */ handle_write_ram,
   /* write $DF46 */ handle_write_ram,
   /* write $DF47 */ handle_write_ram,
   /* write $DF48 */ handle_write_ram,
   /* write $DF49 */ handle_write_ram,
   /* write $DF4A */ handle_write_ram,
   /* write $DF4B */ handle_write_ram,
   /* write $DF4C */ handle_write_ram,
   /* write $DF4D */ handle_write_ram,
   /* write $DF4E */ handle_write_ram,
   /* write $DF4F */ handle_write_ram,
   /* write $DF50 */ handle_write_ram,
   /* write $DF51 */ handle_write_ram,
   /* write $DF52 */ handle_write_ram,
   /* write $DF53 */ handle_write_ram,
   /* write $DF54 */ handle_write_ram,
   /* write $DF55 */ handle_write_ram,
   /* write $DF56 */ handle_write_ram,
   /* write $DF57 */ handle_write_ram,
   /* write $DF58 */ handle_write_ram,
   /* write $DF59 */ handle_write_ram,
   /* write $DF5A */ handle_write_ram,
   /* write $DF5B */ handle_write_ram,
   /* write $DF5C */ handle_write_ram,
   /* write $DF5D */ handle_write_ram,
   /* write $DF5E */ handle_write_ram,
   /* write $DF5F */ handle_write_ram,
   /* write $DF60 */ handle_write_ram,
   /* write $DF61 */ handle_write_ram,
   /* write $DF62 */ handle_write_ram,
   /* write $DF63 */ handle_write_ram,
   /* write $DF64 */ handle_write_ram,
   /* write $DF65 */ handle_write_ram,
   /* write $DF66 */ handle_write_ram,
   /* write $DF67 */ handle_write_ram,
   /* write $DF68 */ handle_write_ram,
   /* write $DF69 */ handle_write_ram,
   /* write $DF6A */ handle_write_ram,
   /* write $DF6B */ handle_write_ram,
   /* write $DF6C */ handle_write_ram,
   /* write $DF6D */ handle_write_ram,
   /* write $DF6E */ handle_write_ram,
   /* write $DF6F */ handle_write_ram,
   /* write $DF70 */ handle_write_ram,
   /* write $DF71 */ handle_write_ram,
   /* write $DF72 */ handle_write_ram,
   /* write $DF73 */ handle_write_ram,
   /* write $DF74 */ handle_flash_dma,
   /* write $DF75 */ handle_flash_dma,
   /* write $DF76 */ handle_write_ram, // used to store Z register with int macro on 65CE02
   /* write $DF77 */ handle_flash_dma,
   /* write $DF78 */ handle_write_ram, // used as RAM for system vector user BRK
   /* write $DF79 */ handle_write_ram, // used as RAM for system vector user BRK
   /* write $DF7A */ handle_write_ram, // used as RAM for system vector NMI ($FFFA)
   /* write $DF7B */ handle_write_ram, // used as RAM for system vector NMI ($FFFB)
   /* write $DF7C */ handle_write_ram, // used as RAM for system vector user IRQ
   /* write $DF7D */ handle_write_ram, // used as RAM for system vector user IRQ
   /* write $DF7E */ handle_write_ram, // used as RAM for system vector IRQ ($FFFE)
   /* write $DF7F */ handle_write_ram, // used as RAM for system vector IRQ ($FFFF)

   /* starting with $DF80 everthing must be writing ram */
   /* write $DF80 */ handle_write_ram,
   /* write $DF81 */ handle_write_ram,
   /* write $DF82 */ handle_write_ram,
   /* write $DF83 */ handle_write_ram,
   /* write $DF84 */ handle_write_ram,
   /* write $DF85 */ handle_write_ram,
   /* write $DF86 */ handle_write_ram,
   /* write $DF87 */ handle_write_ram,
   /* write $DF88 */ handle_write_ram,
   /* write $DF89 */ handle_write_ram,
   /* write $DF8A */ handle_write_ram,
   /* write $DF8B */ handle_write_ram,
   /* write $DF8C */ handle_write_ram,
   /* write $DF8D */ handle_write_ram,
   /* write $DF8E */ handle_write_ram,
   /* write $DF8F */ handle_write_ram,
   /* write $DF90 */ handle_write_ram,
   /* write $DF91 */ handle_write_ram,
   /* write $DF92 */ handle_write_ram,
   /* write $DF93 */ handle_write_ram,
   /* write $DF94 */ handle_write_ram,
   /* write $DF95 */ handle_write_ram,
   /* write $DF96 */ handle_write_ram,
   /* write $DF97 */ handle_write_ram,
   /* write $DF98 */ handle_write_ram,
   /* write $DF99 */ handle_write_ram,
   /* write $DF9A */ handle_write_ram,
   /* write $DF9B */ handle_write_ram,
   /* write $DF9C */ handle_write_ram,
   /* write $DF9D */ handle_write_ram,
   /* write $DF9E */ handle_write_ram,
   /* write $DF9F */ handle_write_ram,
   /* write $DFA0 */ handle_write_ram,
   /* write $DFA1 */ handle_write_ram,
   /* write $DFA2 */ handle_write_ram,
   /* write $DFA3 */ handle_write_ram,
   /* write $DFA4 */ handle_write_ram,
   /* write $DFA5 */ handle_write_ram,
   /* write $DFA6 */ handle_write_ram,
   /* write $DFA7 */ handle_write_ram,
   /* write $DFA8 */ handle_write_ram,
   /* write $DFA9 */ handle_write_ram,
   /* write $DFAA */ handle_write_ram,
   /* write $DFAB */ handle_write_ram,
   /* write $DFAC */ handle_write_ram,
   /* write $DFAD */ handle_write_ram,
   /* write $DFAE */ handle_write_ram,
   /* write $DFAF */ handle_write_ram,
   /* write $DFB0 */ handle_write_ram,
   /* write $DFB1 */ handle_write_ram,
   /* write $DFB2 */ handle_write_ram,
   /* write $DFB3 */ handle_write_ram,
   /* write $DFB4 */ handle_write_ram,
   /* write $DFB5 */ handle_write_ram,
   /* write $DFB6 */ handle_write_ram,
   /* write $DFB7 */ handle_write_ram,
   /* write $DFB8 */ handle_write_ram,
   /* write $DFB9 */ handle_write_ram,
   /* write $DFBA */ handle_write_ram,
   /* write $DFBB */ handle_write_ram,
   /* write $DFBC */ handle_write_ram,
   /* write $DFBD */ handle_write_ram,
   /* write $DFBE */ handle_write_ram,
   /* write $DFBF */ handle_write_ram,
   /* write $DFC0 */ handle_write_ram,
   /* write $DFC1 */ handle_write_ram,
   /* write $DFC2 */ handle_write_ram,
   /* write $DFC3 */ handle_write_ram,
   /* write $DFC4 */ handle_write_ram,
   /* write $DFC5 */ handle_write_ram,
   /* write $DFC6 */ handle_write_ram,
   /* write $DFC7 */ handle_write_ram,
   /* write $DFC8 */ handle_write_ram,
   /* write $DFC9 */ handle_write_ram,
   /* write $DFCA */ handle_write_ram,
   /* write $DFCB */ handle_write_ram,
   /* write $DFCC */ handle_write_ram,
   /* write $DFCD */ handle_write_ram,
   /* write $DFCE */ handle_write_ram,
   /* write $DFCF */ handle_write_ram,
   /* write $DFD0 */ handle_write_ram,
   /* write $DFD1 */ handle_write_ram,
   /* write $DFD2 */ handle_write_ram,
   /* write $DFD3 */ handle_write_ram,
   /* write $DFD4 */ handle_write_ram,
   /* write $DFD5 */ handle_write_ram,
   /* write $DFD6 */ handle_write_ram,
   /* write $DFD7 */ handle_write_ram,
   /* write $DFD8 */ handle_write_ram,
   /* write $DFD9 */ handle_write_ram,
   /* write $DFDA */ handle_write_ram,
   /* write $DFDB */ handle_write_ram,
   /* write $DFDC */ handle_write_ram,
   /* write $DFDD */ handle_write_ram,
   /* write $DFDE */ handle_write_ram,
   /* write $DFDF */ handle_write_ram,
   /* write $DFE0 */ handle_write_ram,
   /* write $DFE1 */ handle_write_ram,
   /* write $DFE2 */ handle_write_ram,
   /* write $DFE3 */ handle_write_ram,
   /* write $DFE4 */ handle_write_ram,
   /* write $DFE5 */ handle_write_ram,
   /* write $DFE6 */ handle_write_ram,
   /* write $DFE7 */ handle_write_ram,
   /* write $DFE8 */ handle_write_ram,
   /* write $DFE9 */ handle_write_ram,
   /* write $DFEA */ handle_write_ram,
   /* write $DFEB */ handle_write_ram,
   /* write $DFEC */ handle_write_ram,
   /* write $DFED */ handle_write_ram,
   /* write $DFEE */ handle_write_ram,
   /* write $DFEF */ handle_write_ram,
   /* write $DFF0 */ handle_write_ram,
   /* write $DFF1 */ handle_write_ram,
   /* write $DFF2 */ handle_write_ram,
   /* write $DFF3 */ handle_write_ram,
   /* write $DFF4 */ handle_write_ram,
   /* write $DFF5 */ handle_write_ram,
   /* write $DFF6 */ handle_write_ram,
   /* write $DFF7 */ handle_write_ram,
   /* write $DFF8 */ handle_write_ram,
   /* write $DFF9 */ handle_write_ram,
   /* write $DFFA */ handle_write_ram,
   /* write $DFFB */ handle_write_ram,
   /* write $DFFC */ handle_write_ram,
   /* write $DFFD */ handle_write_ram,
   /* write $DFFE */ handle_write_ram,
   /* write $DFFF */ handle_write_ram
};

static inline void handle_io()
{
   if( state & bus_config.mask_rw )
   {
      // I/O read
      io_read_handlers[address & 0xFF]();
   }
   else
   {
      // I/O write
      io_write_handlers[address & 0xFF]( gpio_get_all() >> bus_config.shift_data );
   }
}


/******************************************************************************
 * debug functions
 ******************************************************************************/
static const char* debug_handler_name( queue_event_handler_t h )
{
   if( h == event_clear_reset )
   {
      return "event_clear_reset";
   }
   if( h == event_estimate_cpufreq )
   {
      return "event_estimate_cpufreq";
   }
   if( h == event_flash_sync )
   {
      return "event_flash_sync";
   }
   if( h == event_timer_cycle )
   {
      return "event_timer_cycle";
   }
   if( h == event_watchdog )
   {
      return "event_watchdog";
   }
   return "(unknown?)";
}


static void debug_info_eventqueue( char *buffer, size_t size )
{
   int printed;
   queue_event_t *event;
   int i = 0;
   const char *timer_names[2] = { "NMI", "IRQ" };

   for( i = 0; i < count_of(timer_names); ++i )
   {
      printed = snprintf( buffer, size,
                          "timer%d (%s):    id=%ld us=%lld d=%lu\n"
                          , i, timer_names[i]
                          , timer_ms[i].alarm_id, timer_ms[i].delay_us
                          , (uint32_t)timer_ms[i].user_data
                        );
      buffer += printed;
      size   -= printed;
      if( size <= 0 )
      {
         /* buffer is full */
         return;
      }
   }

   printed = snprintf( buffer, size,
                       "cycle counter:   %016llx\n"
                       "next timerstamp: %016llx\n\n"
                       "id|       timestamp|handler_function        |param\n"
                       , _queue_cycle_counter
                       , _queue_next_timestamp
                     );
   buffer += printed;
   size   -= printed;

   for( i = 0, event = _queue_next_event; event; event = event->next )
   {
      printed = snprintf( buffer, size,
                          "%02x|%016llx|%-24s|%8p\n"
                          , i++
                          , event->timestamp
                          , debug_handler_name( event->handler )
                          , event->data
                        );
      buffer += printed;
      size   -= printed;
      if( size <= 0 )
      {
         /* buffer is full */
         return;
      }
   }
}


static void debug_info_clocks( char *buffer, size_t size )
{
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
   uint time_hz = (double)1000000.0 / ((double)(time_per_mcc) / CLOCKS_PER_SEC / 10000);

   check_cpu_is_halted();

   snprintf( buffer, size,
             "  CLK_SYS: %3u.%03d   MHz\n"
             " CLK_PERI: %3d.%03d   MHz\n"
             "%9s: %3ld.%06ldMHz"
             , f_clk_sys / 1000, (f_clk_sys % 1000)
             , f_clk_peri / 1000, (f_clk_peri % 1000)
             , cputype_name( cputype ), time_hz / 1000000, time_hz % 1000000 );
}


static void debug_info_heap( char *buffer, size_t size )
{
   extern char __StackLimit, __bss_end__;
   struct mallinfo m = mallinfo();
   uint32_t total_heap = &__StackLimit  - &__bss_end__;
   uint32_t free_heap = total_heap - m.uordblks;

   snprintf( buffer, size,
             "heap total: %6lu\n"
             "      free: %6lu\n"
             "   minimum: %6lu"
             , total_heap, free_heap, mf_checkheap() );
}


static void debug_info_sysvectors( char *buffer, size_t size )
{
   snprintf( buffer, size,
             "UVBRK:%04X UVNBI:%04X\n"
             "UVNMI:%04X UVIRQ:%04X\n"
             "B0NMI:%04X B0IRQ:%04X"
             , *((uint16_t*)&ram[0xDF78])
             , *((uint16_t*)&ram[0xDF7C])
             , *((uint16_t*)&ram[0xDF7A])
             , *((uint16_t*)&ram[0xDF7E])
             , *((uint16_t*)&ram[0xFFFA])
             , *((uint16_t*)&ram[0xFFFE]) );
}


static void debug_info_internaldrive( char *buffer, size_t size )
{
   dhara_flash_info_t dhara_info;
   uint16_t *lba = (uint16_t*)&ram[MEM_ADDR_ID_LBA];
   uint16_t *mem = (uint16_t*)&ram[MEM_ADDR_ID_MEM];

   check_cpu_is_halted();

   dhara_flash_info( &dhara_info );
   uint64_t hw_size  = dhara_info.erase_cells * dhara_info.erase_size;
   uint64_t lba_size = dhara_info.sectors * dhara_info.sector_size;

   snprintf( buffer, size,
      /*01*/ "hw sector size:  %08lx (%lu)\n"
      /*02*/ "hw num sectors:  %08lx (%lu)\n"
      /*03*/ "hw size:         %6.2fMB\n"
      /*04*/ "page size:       %08lx (%lu)\n"
      /*05*/ "pages:           %08lx (%lu)\n"
      /*06*/ "lba sector size: %08lx (%lu)\n"
      /*07*/ "lba num sectors: %08lx (%lu)\n"
      /*08*/ "lba size:        %6.2fMB\n"
      /*09*/ "gc ratio         %08lx (%lu)\n"
      /*10*/ "read status:     %lu\n"
      /*11*/ "read error:      %s\n"
      /*12*/ "ID_LBA ($DF70):  %04x (sector used for next transfer)\n"
      /*13*/ "ID_MEM ($DF72):  %04x (memory used for next transfer)\n"
      /*01*/ , dhara_info.erase_size, dhara_info.erase_size
      /*02*/ , dhara_info.erase_cells, dhara_info.erase_cells
      /*03*/ , (float)hw_size / (0x100000)   // convert to megabytes
      /*04*/ , dhara_info.page_size, dhara_info.page_size
      /*05*/ , dhara_info.pages, dhara_info.pages
      /*06*/ , dhara_info.sector_size, dhara_info.sector_size
      /*07*/ , dhara_info.sectors, dhara_info.sectors
      /*08*/ , (float)lba_size / (0x100000)  // convert to megabytes
      /*09*/ , dhara_info.gc_ratio, dhara_info.gc_ratio
      /*10*/ , dhara_info.read_status
      /*11*/ , dhara_strerror( dhara_info.read_errcode )
      /*12*/ , *lba
      /*13*/ , *mem
   );
}


/******************************************************************************
 * public functions
 ******************************************************************************/
void system_cpu_detect()
{
   cputype = cpu_detect( false );
retry:
   if( cputype == CPU_ERROR )
   {
      bool success;
      uint8_t data;
      printf( "  cpu could not be detected, retrying (SPACE for debug)\r" );
      success = queue_try_remove( &queue_uart_read, &data );
      if( success && (data == ' ') )
      {
         cputype = cpu_detect( true );
         printf( "power jumper set?\n" );
      }

      goto retry;
   }
}


//void __no_inline_not_in_flash_func(bus_run)()
void bus_run()
{
#if MALLOC_RAM
   if( !ram )
   {
      ram = (uint8_t*)calloc( 0x10000, sizeof(uint8_t) );
   }
#endif
   bus_init();
   system_cpu_detect();
   system_init();
   system_reboot();

   // when not running as speed test run main loop forever
   for(;;)
   {
      // check if internal events need processing
      if( state & bus_config.mask_rdy )
      {
         queue_event_process();
      }

      // LOW ACTIVE
      if( !(state & bus_config.mask_reset) )
      {
         system_reset();
      }

      // done: set clock to high
      gpio_set_mask( bus_config.mask_clock );

      // bus should be still valid from clock low
      state = gpio_get_all();

      // setup bus direction so I/O can settle
      if( state & bus_config.mask_rw )
      {
         // read from memory and write to bus
         gpio_set_dir_out_masked( bus_config.mask_data );
         // note to future self: check if there's a better way for
         // external i/o than to set the GPIOs to write for a brief time
      }
      else
      {
         // read from bus and write to memory write
         gpio_set_dir_in_masked( bus_config.mask_data );
      }

      address = ((state & bus_config.mask_address) >> bus_config.shift_address);

      // setup data
      //if( (address & 0xFF00) == 0xDF00 )
      if( (address >> 8) == 0xDF ) // this is faster
      {
         // internal I/O
         handle_io();
      }
      //else if( (address <= 0x0003) || ((address & 0xF000) == 0xD000) )
      else if( (address <= 0x0003) || ((address >> 12) == 0xD) )
      {
         // external i/o: keep hands off the bus
         gpio_set_dir_in_masked( bus_config.mask_data );
      }
      else
      {
         if( state & bus_config.mask_rw )
         {
            handle_read_ramrom();
         }
         else
         {
            handle_write_ram( gpio_get_all() >> bus_config.shift_data );
         }
      }

      // log last states
      if( state & bus_config.mask_rdy )
      {
         // state is not valid on the data when RP2040 is writing
         buslog_states[buslog_index++ & (BUSLOG_SIZE-1)] = gpio_get_all();
      }

      // done: set clock to low
      gpio_clr_mask( bus_config.mask_clock );
   }
}


void system_init()
{
   memset( &ram[0x0000], 0x00, sizeof(ram) );
   memcpy( &rom[0x0000], (const void*)FLASH_KERNEL_START, sizeof(rom) );
   srand( get_rand_32() );
   dhara_flash_size = dhara_flash_init();
   disass_set_cpu( cputype );
}


void system_reboot()
{
   // clear out cpu state log
   memset( &buslog_states[0], 0, sizeof(buslog_states) );

   // make sure that RDY line is high
   gpio_set_mask( bus_config.mask_rdy | bus_config.mask_irq | bus_config.mask_nmi );

   // pull reset line to system_reset gets executed
   gpio_clr_mask( bus_config.mask_reset );
}


uint8_t debug_banks()
{
   return sizeof(rom) >> 13; // ROMSIZE / 0x2000;
}


uint8_t debug_peek( uint8_t bank, uint16_t addr )
{
   if( addr < 0xE000 )
   {
      return ram[addr];
   }
   if( (bank > 0) && (bank <= debug_banks()) )
   {
      /* assembling address in ROM array is a bit more complicated */
      return rom[((bank-1) << 13) | (addr & 0x1FFF)];
   }
   else
   {
      /* bank0: RAM under ROM */
      return ram[addr];
   }
}


void debug_poke( uint8_t bank, uint16_t addr, uint8_t value )
{
   if( (addr < 0xE000) || !bank )
   {
      ram[addr] = value;
   }
}


const char *debug_get_info( debug_info_t page )
{
   static char buffer[1024] = { 0 };

   buffer[0] = '\0';
   switch( page )
   {
      case DEBUG_INFO_HEAP:
         debug_info_heap( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_CLOCKS:
         debug_info_clocks( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_SYSVECTORS:
         debug_info_sysvectors( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_INTERNALDRIVE:
         debug_info_internaldrive( &buffer[0], sizeof(buffer)-1 );
         break;
      case DEBUG_INFO_EVENTQUEUE:
         debug_info_eventqueue( &buffer[0], sizeof(buffer)-1 );
         break;
      default:
         break;
   }
   return &buffer[0];
}


void debug_disassembler()
{
   static uint16_t lastaddr = 0x0400;
   uint16_t addr = 0;
   int count = 0;

   check_cpu_is_halted();

   for(;;)
   {
      printf( "%s disass ($%04X): ", cputype_name( cputype ), lastaddr );
      int32_t getaddr = get_16bit_address( lastaddr );
      printf( "\n" );
      if( getaddr < 0 )
      {
         return;
      }

      addr = (uint16_t)(getaddr & 0xFFFF);
      for( count = 0; count < 16; ++count )
      {
         disass_show( DISASS_SHOW_ADDRESS | DISASS_SHOW_HEXDUMP );
         puts( disass( addr, get_memory(addr), get_memory(addr+1), get_memory(addr+2), get_memory(addr+3) ) );
         //printf( "%s ; %d\n",
         //   disass( addr, get_memory(addr), get_memory(addr+1), get_memory(addr+2), get_memory(addr+3) ),
         //   disass_bytes( get_memory(addr) );
         addr += disass_bytes( get_memory(addr) );
      }
      lastaddr = addr;
   }
}


void debug_raw_backtrace()
{
   printf( "\nTRACE_START %s\n", cputype_name( cputype ) );
   for( int i = buslog_index; i < (buslog_index + BUSLOG_SIZE); ++i )
   {
      printf( "%08lx\n", buslog_states[i & (BUSLOG_SIZE-1)] );
   }
   printf( "TRACE_END\n" );
}


void debug_get_backtrace( cputype_t *cpu, uint32_t **trace, uint32_t *entries, uint32_t *start )
{
   *cpu     = cputype ? cputype : CPU_65SC02;
   *trace   = &buslog_states[0];
   *entries = BUSLOG_SIZE;
   *start   = buslog_index & (BUSLOG_SIZE)-1;
}


cputype_t debug_get_cpu()
{
   return cputype;
}
