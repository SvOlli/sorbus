/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a Native custom platform
 * for the Sorbus Computer
 */

#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../rp2040_purple.h"

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/rand.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include <hardware/clocks.h>

#include "common.h"

#include "../bus.h"
#if 0
#include "gpio_oc.h"
#endif

#include "event_queue.h"
#include "dhara_flash.h"
#include "../dhara/error.h"


// this is where the write protected area starts
#define ROM_START (0xE000)
// this is where the kernel is in raw flash
#define FLASH_KERNEL_START (FLASH_KERNEL_START_TXT)

// setup data for binary_info
#define FLASH_DRIVE_INFO  "fs image at " __XSTRING(FLASH_DRIVE_START_TXT)
bi_decl(bi_program_feature(FLASH_DRIVE_INFO))
#define FLASH_KERNEL_INFO "kernel   at " __XSTRING(FLASH_KERNEL_START_TXT)
bi_decl(bi_program_feature(FLASH_KERNEL_INFO))

uint8_t ram[0x10000];   // 64k of RAM and I/O
uint8_t rom[FLASH_DRIVE_START_TXT-FLASH_KERNEL_START_TXT]; // buffer for roms
const uint8_t *romvec;  // pointer into current ROM/RAM bank at $E000
uint32_t state;
uint32_t address;

// measure time used for processing one million clock cycles (mcc)
uint64_t time_per_mcc          = 1; // in case a division is done

// no need to volatile these, as they are only used within bus_run loop
uint32_t timer_irq_total       = 0;
uint32_t timer_nmi_total       = 0;

bool nmi_timer_triggered       = false;
bool irq_timer_triggered       = false;
bool cpu_running               = false;
bool cpu_running_request       = true;

uint32_t buslog_states[256]    = { 0 };
uint32_t watchdog_cycles_total = 0;

uint16_t dhara_flash_size = 0;
#define DHARA_SYNC_DELAY (20000)

// magic addresses that cannot be addressed otherwise
#define MEM_ADDR_UART_CONTROL (0xDF0B)
#define MEM_ADDR_ID_LBA       (0xDF70)
#define MEM_ADDR_ID_MEM       (0xDF72)

/******************************************************************************
 * internal functions
 ******************************************************************************/


static inline void bus_data_write( uint8_t data )
{
#if 0
   gpio_oc_set_by_mask( bus_config.mask_data, ((uint32_t)data) << bus_config.shift_data );
#else
   gpio_put_masked( bus_config.mask_data, ((uint32_t)data) << bus_config.shift_data );
#endif
}


static inline uint8_t bus_data_read()
{
   return (gpio_get_all() >> bus_config.shift_data);
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
   queue_try_add( &queue_uart_write, &type );
}


static inline void handle_ramrom()
{
   if( state & bus_config.mask_rw )
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
   else
   {
      // always write to RAM
      ram[address] = bus_data_read();
   }
}


static inline void event_estimate_cpufreq( void *data )
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


static inline void event_watchdog( void *data )
{
   // event handler for watchdog timer
   // this event is max 1 time in queue

   system_trap( SYSTEM_WATCHDOG );
}


static inline void watchdog_setup( uint8_t value, uint8_t config )
{
   config &= 0x03;
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


static inline void event_timer_nmi( void *data )
{
   // event handler for timer nmi
   // this event is max 1 time in queue

   // trigger NMI
   gpio_clr_mask( bus_config.mask_nmi );

   // set state for reading
   nmi_timer_triggered = true;

   // restart time if requested
   if( timer_nmi_total )
   {
      queue_event_add( timer_nmi_total, event_timer_nmi, 0 );
   }
}


static inline void event_timer_irq( void *data )
{
   // event handler for timer irq
   // this event is max 1 time in queue

   // trigger IRQ
   gpio_clr_mask( bus_config.mask_irq );

   // set state for reading
   irq_timer_triggered = true;

   // restart time if requested
   if( timer_irq_total )
   {
      queue_event_add( timer_irq_total, event_timer_irq, 0 );
   }
}


static inline void timer_setup( uint8_t value, uint8_t config )
{
   config &= 0x07;
   // config bits (decoded from address)
   //           0        1
   // bit 0: lobyte   hibyte
   // bit 1: repeat   oneshot
   // bit 2: irq      nmi

   // check timer to use
   if( config & (1<<2) )
   {
      // nmi
      if( config & (1<<0) )
      {
         // highbyte: store and start timer
         timer_nmi_total |= value << 8;
         queue_event_add( timer_nmi_total, event_timer_nmi, 0 );
         if( config & (1<<1) )
         {
            // oneshot
            timer_nmi_total = 0;
         }
      }
      else
      {
         // lowbyte: store and stop timer
         queue_event_cancel( event_timer_nmi );
         timer_nmi_total = value;
      }
   }
   else
   {
      // irq
      if( config & (1<<0) )
      {
         // highbyte: store and start timer
         timer_irq_total |= value << 8;
         queue_event_add( timer_irq_total, event_timer_irq, 0 );
         if( config & (1<<1) )
         {
            // oneshot
            timer_irq_total = 0;
         }
      }
      else
      {
         // lowbyte: store and stop timer
         queue_event_cancel( event_timer_irq );
         timer_irq_total = value;
      }
   }
}


static inline void event_clear_reset( void *data )
{
   gpio_set_mask( bus_config.mask_reset );
}


static inline void system_reset()
{
   int dummy;

   if( queue_event_contains( event_clear_reset ) )
   {
      // reset sequence is already initiated
      return;
   }

   // clear some internal states
   timer_nmi_total       = 0;
   nmi_timer_triggered   = false;
   timer_irq_total       = 0;
   irq_timer_triggered   = false;
   watchdog_cycles_total = 0;

   // clear event queue and setup end of reset event
   queue_event_reset();
   queue_event_add( 8, event_clear_reset, 0 );
   // start measurement of cpu speed once every million clocks
   queue_event_add( 9, event_estimate_cpufreq, 0 );

   // setup bank
   set_bank( 1 );

   while( queue_try_remove( &queue_uart_read, &dummy ) )
   {
      // just loop until queue is empty
   }
   while( queue_try_remove( &queue_uart_write, &dummy ) )
   {
      // just loop until queue is empty
   }

   // setup serial console
   console_set_crlf( true );
   // this needs to be set, as core0 cannot access RAM
   ram[MEM_ADDR_UART_CONTROL] = 0x01;
}


static inline void event_flash_sync( void *data )
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

   ram[address] = 0x00;
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
}


void debug_dump_state( int lineno, uint32_t _state )
{
      printf( "%3d:%04x %c %02x %c%c%c%c\n",
         lineno,
         (_state & bus_config.mask_address) >> (bus_config.shift_address),
         (_state & bus_config.mask_rw) ? 'r' : 'w',
         (_state & bus_config.mask_data) >> (bus_config.shift_data),
         (_state & bus_config.mask_reset) ? ' ' : 'R',
         (_state & bus_config.mask_nmi) ? ' ' : 'N',
         (_state & bus_config.mask_irq) ? ' ' : 'I',
         (_state & bus_config.mask_rdy) ? ' ' : 'S' );
}


void debug_backtrace()
{
   printf( "system trap triggered, last cpu actions:\n" );
   for( int i = 0; i < count_of(buslog_states); ++i )
   {
      uint32_t _state = buslog_states[(i + _queue_cycle_counter) & 0xFF];
      debug_dump_state( count_of(buslog_states)-i, _state );
   }
   debug_dump_state( 0, gpio_get_all() );
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
   if( h == event_timer_irq )
   {
      return "event_timer_irq";
   }
   if( h == event_timer_nmi )
   {
      return "event_timer_nmi";
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
   printf( "done.\n" );
}

void debug_clocks()
{
   uint f_pll_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
   uint f_pll_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
   uint f_rosc     = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
   uint f_clk_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
   uint f_clk_adc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
   uint f_clk_rtc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
   uint time_hz = (double)1000000.0 / ((double)(time_per_mcc) / CLOCKS_PER_SEC / 10000);

   printf("\n");
   printf("PLL_SYS:             %3d.%03dMHz\n", f_pll_sys / 1000, f_pll_sys % 1000 );
   printf("PLL_USB:             %3d.%03dMHz\n", f_pll_usb / 1000, f_pll_usb % 1000 );
   printf("ROSC:                %3d.%03dMHz\n", f_rosc    / 1000, f_rosc    % 1000 );
   printf("CLK_SYS:             %3d.%03dMHz\n", f_clk_sys / 1000, f_clk_sys % 1000 );
   printf("CLK_PERI:            %3d.%03dMHz\n", f_clk_peri / 1000, f_clk_peri % 1000 );
   printf("CLK_USB:             %3d.%03dMHz\n", f_clk_usb / 1000, f_clk_usb % 1000 );
   printf("CLK_ADC:             %3d.%03dMHz\n", f_clk_adc / 1000, f_clk_adc % 1000 );
   printf("CLK_RTC:             %3d.%03dMHz\n", f_clk_rtc / 1000, f_clk_rtc % 1000 );
   printf("65C02 CLK:           %3d.%06dMHz\n", time_hz / 1000000, time_hz % 1000000 );
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


void debug_hexdump( uint8_t *memory, uint32_t size, uint16_t address )
{
   for( uint32_t i = 0; i < size; i += 0x10 )
   {
      printf( "%04x:", address + i );

      for( uint8_t j = 0; j < 0x10; ++j )
      {
         uint16_t a = address + i + j;
         if( (i + j) > size )
         {
            while( j < 0x10 )
            {
               printf( "   " );
               ++j;
            }
            break;
         }
         printf( " %02x", memory[a] );
      }
      printf( "  " );
      for( uint8_t j = 0; j < 0x10; ++j )
      {
         uint16_t a = address + i + j;
         if( (i + j) > size )
         {
            break;
         }
         uint8_t v = memory[a];
         if( (v >= 32) && (v <= 127) )
         {
            printf( "%c", v );
         }
         else
         {
            printf( "." );
         }
      }

      printf( "\n" );
   }
}


void debug_internal_drive()
{
   dhara_flash_info_t dhara_info;
   uint8_t dhara_buffer[SECTOR_SIZE];
   uint16_t dhara_sector = 0;

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
   printf("dhara read sector $%04x:\n", dhara_sector );

   debug_hexdump( dhara_buffer, SECTOR_SIZE, 0 );
}


static inline void handle_io()
{
   uint8_t data = state >> bus_config.shift_data;
   bool success;

   if( state & bus_config.mask_rw )
   {
      // I/O read
      switch( address & 0xFF )
      {
         case 0x02: // random
            bus_data_write( rand() & 0xFF );
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
         case 0x10: // is timer for IRQ running
         case 0x11:
         case 0x12:
         case 0x13: // fall throughs are intended
            bus_data_write( irq_timer_triggered ? 0x80 : 0x00 );
            irq_timer_triggered = false;
            gpio_set_mask( bus_config.mask_irq );
            break;
         case 0x14: // is timer for NMI running
         case 0x15:
         case 0x16:
         case 0x17: // fall throughs are intended
            bus_data_write( nmi_timer_triggered ? 0x80 : 0x00 );
            nmi_timer_triggered = false;
            gpio_set_mask( bus_config.mask_nmi );
            break;
         case 0x20: // is timer for watchdog running
         case 0x21:
         case 0x22:
         case 0x23: // fall throughs are intended
            bus_data_write( watchdog_cycles_total ? 0x80 : 0x00 );
            break;
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
         case 0x0B: // UART read: enable crlf conversion
            console_set_crlf( data & 1 );
            handle_ramrom(); // make sure register is mirrored to RAM for read
            break;
         case 0x0E: // console UART write
            queue_try_add( &queue_uart_write, &data );
            break;
         case 0x10: // setup timer for IRQ or NMI
         case 0x11:
         case 0x12:
         case 0x13:
         case 0x14:
         case 0x15:
         case 0x16:
         case 0x17: // fall throughs are intended
            timer_setup( data, address & 0x07 );
            break;
         case 0x20: // setup timer for watchdog
         case 0x21:
         case 0x22:
         case 0x23: // fall throughs are intended
            watchdog_setup( data, address & 0x03 );
            break;
         case 0x74: // dma read from flash disk
         case 0x75: // dma write to flash disk
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
void bus_run()
{
   bus_init();
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
      if( (address & 0xFF00) == 0xDF00 )
      {
         // internal I/O
         handle_io();
      }
      else if( (address <= 0x0003) || ((address & 0xF000) == 0xD000) )
      {
         // external i/o: keep hands off the bus
         gpio_set_dir_in_masked( bus_config.mask_data );
      }
      else
      {
         handle_ramrom();
      }

      // done: set clock to low
      gpio_clr_mask( bus_config.mask_clock );

      // log last states
      if( state & bus_config.mask_rdy )
      {
         buslog_states[(_queue_cycle_counter) & 0xff] = gpio_get_all();
      }
   }
}


void system_init()
{
   memset( &ram[0x0000], 0x00, sizeof(ram) );
   memcpy( &rom[0x0000], (const void*)FLASH_KERNEL_START, sizeof(rom) );
   srand( get_rand_32() );
   dhara_flash_size = dhara_flash_init();
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
