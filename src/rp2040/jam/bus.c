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

// this is where the write protected area starts
#define ROM_START (0xE000)
// this is where the kernel is in raw flash
#define FLASH_KERNEL_START (FLASH_KERNEL_START_TXT)

// setup data for binary_info
#define FLASH_DRIVE_INFO  "fs image at " __XSTRING(FLASH_DRIVE_START_TXT)
bi_decl(bi_program_feature(FLASH_DRIVE_INFO))
#define FLASH_KERNEL_INFO "kernel   at " __XSTRING(FLASH_KERNEL_START_TXT)
bi_decl(bi_program_feature(FLASH_KERNEL_INFO))

uint8_t ram[0x10000] = { 0 }; // 64k of RAM and I/O
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
#define BUSLOG_SIZE (512)
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


void set_bank( uint8_t bank )
{
   // rom banks are counted starting with 1, as 0 is RAM
   if( bank > (sizeof(rom) / 0x2000) )
   {
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
      romvec = &rom[(bank - 1) * 0x2000];
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


static inline void handle_ramrom()
{
   if( state & bus_config.mask_rw )
   {
      // read from memory and write to bus
#if 0
      // this is nicer
      bus_data_write( get_memory( address ) );
#else
      // this is faster
      if( address < ROM_START )
      {
         bus_data_write( ram[address] );
      }
      else
      {
         bus_data_write( romvec[address & 0x1FFF] );
      }
#endif
   }
   else
   {
      // always write to RAM
      ram[address] = bus_data_read();
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


static inline void watchdog_setup( uint8_t value )
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


static inline void timer_cycle_ack()
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


static inline void timer_ms_setup( uint8_t value )
{
   uint8_t config = address & 0x03;

   uint32_t offset = ((config & 2) >> 1);
   struct repeating_timer *t = &timer_ms[offset];
   uint16_t *tv = (uint16_t*)&ram[address & 0xFFFE];

   // using RAM to store config
   handle_ramrom();

   cancel_repeating_timer( t );

   if( *tv )
   {
      add_repeating_timer_us( -100 * (*tv), callback_timer_ms, (void*)offset, t );
   }
}


static inline void timer_ms_ack()
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
   console_set_crlf( true );
   console_set_flowcontrol( false );
   // reflect changes from above in control register
   // this needs to be set here, as console_set_* cannot access RAM
   ram[MEM_ADDR_UART_CONTROL] = 0x01;
   // reset Sorbus ID pointer
   sorbus_id_p = sorbus_id;
}


static inline void handle_scratch_mem( uint8_t flags )
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


static inline void handle_flash_dma()
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
         // unused addresses are treated like RAM
         handle_ramrom();
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


static inline uint32_t getaddr( uint32_t _state )
{
   return (_state & bus_config.mask_address) >> (bus_config.shift_address);
}


static inline uint32_t getdata( uint32_t _state )
{
   return (_state & bus_config.mask_data) >> (bus_config.shift_data);
}


uint32_t debug_getbus( int i )
{
   uint32_t state = buslog_states[(i + buslog_index) & (BUSLOG_SIZE-1)];
   uint32_t prev2 = buslog_states[(i + buslog_index - 2) & (BUSLOG_SIZE-1)];
   uint32_t prev  = buslog_states[(i + buslog_index - 1) & (BUSLOG_SIZE-1)];
   uint32_t next  = buslog_states[(i + buslog_index + 1) & (BUSLOG_SIZE-1)];

   bool has_follow = ((getaddr( state ) + 1) == getaddr( next ));
   bool is_read = (state & bus_config.mask_rw) && (next & bus_config.mask_rw);
   bool is_follow = ((getaddr( prev ) + 1) == getaddr( state ));
   bool is_rts_stack = (getdata( prev2 ) == 0x60 );
   if( is_read && has_follow && !is_follow && !is_rts_stack )
   {
      state |= 0x80000000;
   }
   return state;
}



void debug_raw_backtrace()
{
   printf( "TRACE_START %s\n", cputype_name( cputype ) );
   for( int i = buslog_index; i < (buslog_index + BUSLOG_SIZE); ++i )
   {
      printf( "%08x\n", buslog_states[i & (BUSLOG_SIZE-1)] );
   }
   printf( "TRACE_END\n" );
}


void debug_backtrace()
{
   check_cpu_is_halted();

   printf( "historian:\n" );

   disass_show( DISASS_SHOW_NOTHING );
   disass_historian_t d = disass_historian_init( cputype ? cputype : CPU_6502,
                                                 &buslog_states[0],
                                                 BUSLOG_SIZE,
                                                 buslog_index & (BUSLOG_SIZE)-1 );
   for( int i = 0; i < BUSLOG_SIZE; ++i )
   {
      int index = (i+buslog_index) & (BUSLOG_SIZE-1);
      if( !disass_historian_entry( d, index ) )
      {
         break;
      }
      printf( "%3d:%s:%s\n", BUSLOG_SIZE-i,
              decode_trace( buslog_states[index], false, 0 ),
              disass_historian_entry( d, index ) );
   }
   disass_historian_done( d );
}

void debug_memorydump()
{
   const uint16_t show_size = 0x100;
   static uint16_t lastaddr = 0x0400;

   check_cpu_is_halted();

   for(;;)
   {
      printf( "%s mem ($%04X): ", cputype_name( cputype ), lastaddr );
      int32_t addr = get_16bit_address( lastaddr );
      printf( "\n" );
      if( addr < 0 )
      {
         lastaddr -= show_size;
         return;
      }
      // TODO: switch this somehow to get_memory()
      hexdump( get_memory, addr, show_size );
      lastaddr = addr + show_size;
   }
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


const char* debug_handler_name( queue_event_handler_t h )
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
}


void debug_queue_event( const char *text )
{
   queue_event_t *event;
   int i = 0;
   printf( "%s: %016llx:%016llx\n", text, _queue_cycle_counter, _queue_next_timestamp );
   for( event = _queue_next_event; event; event = event->next )
   {
      printf( "%02d:%016llx:%p:%-24s:%08x\n",
              i++,
              event->timestamp,
              event->handler,
              debug_handler_name( event->handler ),
              event->data );
   }
   for( i = 0; i < 2; ++i )
   {
      printf( "timer%d (%s): id=%d us=%lld d=%u\n", i, i ? "NMI" : "IRQ",
              timer_ms[i].alarm_id, timer_ms[i].delay_us,
              (uint32_t)timer_ms[i].user_data );
   }
}

void debug_clocks()
{
   char cpu_clk[] = "                     ";
   uint f_pll_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
   uint f_pll_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
   uint f_rosc     = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
   uint f_clk_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
   uint f_clk_adc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
   uint f_clk_rtc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
   uint time_hz = (double)1000000.0 / ((double)(time_per_mcc) / CLOCKS_PER_SEC / 10000);

   check_cpu_is_halted();

   cpu_clk[snprintf( &cpu_clk[0], sizeof(cpu_clk)-1, "%s CLK:", cputype_name( cputype ) )] = ' ';
   printf("\n");
   printf("PLL_SYS:             %3d.%03dMHz\n", f_pll_sys / 1000, f_pll_sys % 1000 );
   printf("PLL_USB:             %3d.%03dMHz\n", f_pll_usb / 1000, f_pll_usb % 1000 );
   printf("ROSC:                %3d.%03dMHz\n", f_rosc    / 1000, f_rosc    % 1000 );
   printf("CLK_SYS:             %3d.%03dMHz\n", f_clk_sys / 1000, f_clk_sys % 1000 );
   printf("CLK_PERI:            %3d.%03dMHz\n", f_clk_peri / 1000, f_clk_peri % 1000 );
   printf("CLK_USB:             %3d.%03dMHz\n", f_clk_usb / 1000, f_clk_usb % 1000 );
   printf("CLK_ADC:             %3d.%03dMHz\n", f_clk_adc / 1000, f_clk_adc % 1000 );
   printf("CLK_RTC:             %3d.%03dMHz\n", f_clk_rtc / 1000, f_clk_rtc % 1000 );
   printf("%s%3d.%06dMHz\n", cpu_clk, time_hz / 1000000, time_hz % 1000000 );
}


void debug_heap()
{
   extern char __StackLimit, __bss_end__;
   struct mallinfo m = mallinfo();

   uint32_t total_heap = &__StackLimit  - &__bss_end__;
   uint32_t free_heap = total_heap - m.uordblks;

   printf("\n");
   printf( "total heap: %08x (%d)\n", total_heap, total_heap );
   printf( "free  heap: %08x (%d)\n", free_heap,  free_heap );
}


void debug_internal_drive()
{
   dhara_flash_info_t dhara_info;
   uint8_t dhara_buffer[SECTOR_SIZE];
   uint16_t dhara_sector = 0;

   check_cpu_is_halted();

   dhara_flash_info( dhara_sector, &dhara_buffer[0], &dhara_info );
   uint64_t hw_size  = dhara_info.erase_cells * dhara_info.erase_size;
   uint64_t lba_size = dhara_info.sectors * dhara_info.sector_size;
   printf("\n");
   printf("dhara hw sector size:  %08x (%d)\n", dhara_info.erase_size, dhara_info.erase_size );
   printf("dhara hw num sectors:  %08x (%d)\n", dhara_info.erase_cells, dhara_info.erase_cells );
   printf("dhara hw size:         %.2fMB\n", (float)hw_size / (1024*1024) );
   printf("dhara page size:       %08x (%d)\n", dhara_info.page_size, dhara_info.page_size );
   printf("dhara pages:           %08x (%d)\n", dhara_info.pages, dhara_info.pages );
   printf("dhara lba sector size: %08x (%d)\n", dhara_info.sector_size, dhara_info.sector_size );
   printf("dhara lba num sectors: %08x (%d)\n", dhara_info.sectors, dhara_info.sectors );
   printf("dhara lba size:        %.2fMB\n", (float)lba_size / (1024*1024) );
   printf("dhara gc ratio         %08x (%d)\n", dhara_info.gc_ratio, dhara_info.gc_ratio );
   printf("dhara read status:     %d\n", dhara_info.read_status );
   printf("dhara read error:      %s\n", dhara_strerror( dhara_info.read_errcode ) );
#if 0
   printf("dhara read sector $%04x:\n", dhara_sector );

   hexdump_buffer( dhara_buffer, SECTOR_SIZE );
#else
   uint16_t *lba = (uint16_t*)&ram[MEM_ADDR_ID_LBA];
   uint16_t *mem = (uint16_t*)&ram[MEM_ADDR_ID_MEM];
   printf("dhara last used sector: $%04x\n", *lba );
   hexdump( get_memory, (*mem)-0x80, SECTOR_SIZE );
#endif
}


#if 0
void debug_sysconfig()
{
   printf( "UVBRK:%04X  ", *((uint16_t*)&ram[0xDF78]) );
   printf( "UVNMI:%04X  ", *((uint16_t*)&ram[0xDF7A]) );
   printf( "UVNBK:%04X  ", *((uint16_t*)&ram[0xDF7C]) );
   printf( "UVIRQ:%04X\n", *((uint16_t*)&ram[0xDF7E]) );
}
#endif


static inline void handle_io()
{
   static union {
      uint32_t value;
      uint8_t  reg[4];
   } shadow_cycle_count;

   uint8_t data = gpio_get_all() >> bus_config.shift_data;
   bool success;

   if( state & bus_config.mask_rw )
   {
      // I/O read
      switch( address & 0xFF )
      {
         case 0x01: // Sorbus ID
            bus_data_write( *sorbus_id_p );
            if( *sorbus_id_p )
            {
               ++sorbus_id_p;
            }
            else
            {
               sorbus_id_p = sorbus_id;
            }
            break;
         case 0x02: // random
            bus_data_write( rand() & 0xFF );
            break;
         case 0x04: // CPU features
            bus_data_write( cpufeatures[cputype] );
            break;
         case 0x0C: // console UART read
            success = queue_try_remove( &queue_uart_read, &data );
            bus_data_write( success ? data : 0x00 );
            break;
         case 0x0D: // console UART read queue
            bus_data_write( queue_get_level( &queue_uart_read )  );
            break;
         case 0x0F: // console UART write queue
            bus_data_write( queue_get_level( &queue_uart_write )  );
            break;
         case 0x10: // cycle timer for IRQ running
         case 0x11:
         case 0x12:
         case 0x13:
         case 0x14: // cycle timer for NMI running
         case 0x15:
         case 0x16:
         case 0x17: // fall throughs are intended
            timer_cycle_ack();
            break;
         case 0x18: // ms timer for IRQ running
         case 0x19:
         case 0x1A: // ms timer for NMI running
         case 0x1B: // fall throughs are intended
            timer_ms_ack();
            break;
         case 0x20: // is timer for watchdog running
         case 0x21:
         case 0x22:
         case 0x23: // fall throughs are intended
            bus_data_write( watchdog_cycles_total ? 0x80 : 0x00 );
            break;
         case 0x24:
            shadow_cycle_count.value = (uint32_t)_queue_cycle_counter;
            // fall through
         case 0x25:
         case 0x26:
         case 0x27:
            bus_data_write( shadow_cycle_count.reg[address & 3] );
            break;
         /* 0x2C-0x2F used as RAM for BRK routine */
         /* 0x30-0x37 used as RAM for System Monitor */
         /* 0x78-0x7F used as RAM for system vectors */
         /* 0x76 used to store Z register with int macro on 65CE02 */
         default:
            // everything else is handled like RAM by design
            handle_ramrom();
            break;
      }
   }
   else
   {
      // I/O write
      data = bus_data_read();
      switch( address & 0xFF )
      {
         case 0x00: // set bankswitch register for $E000-$FFFF
            set_bank( bus_data_read() );
            handle_ramrom(); // make sure register is mirrored to RAM for read
            break;
         case 0x01: // DEBUG only!
            system_trap( SYSTEM_TRAP );
            break;
         case 0x03: // scratch-1k
            handle_scratch_mem( data );
            break;
         case 0x0B: // UART read: enable crlf conversion
            console_set_crlf( data & 1 );
            console_set_flowcontrol( data & 2 );
            handle_ramrom(); // make sure register is mirrored to RAM for read
            break;
         case 0x0E: // console UART write
            queue_try_add( &queue_uart_write, &data );
            break;
         case 0x10: // setup cycle timer for IRQ
         case 0x11:
         case 0x12:
         case 0x13:
         case 0x14: // setup cycle timer for NMI
         case 0x15:
         case 0x16:
         case 0x17: // fall throughs are intended
            timer_ms_setup( data );
            break;
         case 0x18: // setup ms timer for IRQ
         case 0x19:
         case 0x1A: // setup ms timer for NMI
         case 0x1B: // fall throughs are intended
            timer_ms_setup( data );
            break;
         case 0x20: // setup timer for watchdog
         case 0x21:
         case 0x22:
         case 0x23: // fall throughs are intended
            watchdog_setup( data );
            break;
         /* 0x2C-0x2F used as RAM for BRK routine */
         /* 0x30-0x37 used as RAM for System Monitor */
         /* 0x78-0x7F used as RAM for system vectors */
         case 0x74: // dma read from flash disk
         case 0x75: // dma write to flash disk
         /* 0x76 used to store Z register with int macro on 65CE02 */
         case 0x77: // flash disk trim, no dma address used
            // access is strobe: written data does not matter
            handle_flash_dma();
            break;
         default:
            // everything else is handled like RAM by design
            handle_ramrom();
            break;
      }
   }
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


void bus_run()
{
   bus_init();
   system_cpu_detect();
   system_init();
   system_reboot();

   // when not running as speed test run main loop forever
   for(;;)
   {
      // check if internal events need processing
      queue_event_process();

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
         // external i/o then to set the GPIOs to write for a brief time
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
         handle_ramrom();
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
