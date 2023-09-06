/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a Native custom platform
 * for the Sorbus Computer
 */

#include <ctype.h>
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


// set this to 5000000 to run for 5 million cycles while keeping time
//#define SPEED_TEST 5000000
// the number of clock cycles a timer interrupt is triggered
#define INTERRUPT_LENGTH (8)
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

// no need to volatile these, as they are only used within bus_run loop
uint32_t timer_irq_total       = 0;
uint32_t timer_nmi_total       = 0;

bool nmi_timer_triggered       = false;
bool irq_timer_triggered       = false;

uint32_t watchdog_states[256]  = { 0 };
uint32_t watchdog_cycles_total = 0;

uint16_t dhara_flash_size = 0;

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
      romvec = &rom[(bank - 1) * 0x2000];
   }
   ram[0xDFFF] = bank;
//   printf( "set_bank(%02x) -> %p\n", bank, romvec );
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


static inline void watchdog_trigger( void *data )
{
   uint32_t mode = (uint32_t)data; // 0 = watchdog, 1 = key
   //watchdog_triggered = true;
}


static inline void watchdog_setup( uint8_t value, uint8_t config )
{
   config &= 0x03;
   if( config )
   {
      if( queue_event_contains( watchdog_trigger ) )
      {
         // timer is running: restart
         queue_event_cancel( watchdog_trigger );
         queue_event_add( watchdog_cycles_total, watchdog_trigger, 0 );
      }
      else
      {
         // not running, set 24 bit value
         watchdog_cycles_total &= ~(0xFF << ((config-1) * 8));
         watchdog_cycles_total |= (value << ((config-1) * 8));
         if( config == 3 )
         {
            // start watchdog
            queue_event_add( watchdog_cycles_total, watchdog_trigger, 0 );
         }
      }
   }
   else
   {
      // stop watchdog
      queue_event_cancel( watchdog_trigger );
      watchdog_cycles_total = 0;
   }
}


static inline void timer_nmi_clear( void *data )
{
   gpio_set_mask( bus_config.mask_nmi );
}


static inline void timer_nmi_triggered( void *data )
{
   // trigger NMI
   gpio_clr_mask( bus_config.mask_nmi );

   // clear NMI signal soon
   queue_event_add( INTERRUPT_LENGTH, timer_nmi_clear, 0 );

   // restart time if requested
   if( timer_nmi_total )
   {
      queue_event_add( timer_nmi_total, timer_nmi_triggered, 0 );
   }
}


static inline void timer_irq_clear( void *data )
{
   gpio_set_mask( bus_config.mask_irq );
}


static inline void timer_irq_triggered( void *data )
{
   // trigger IRQ
   gpio_clr_mask( bus_config.mask_irq );

   // clear IRQ signal soon
   queue_event_add( INTERRUPT_LENGTH, timer_irq_clear, 0 );

   // restart time if requested
   if( timer_irq_total )
   {
      queue_event_add( timer_irq_total, timer_irq_triggered, 0 );
   }
}


static inline void timer_setup( uint8_t value, uint8_t config )
{
   config &= 0x07;
   // config bits (decoded from address)
   //           0        1
   // bit 0: lowbyte  highbyte
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
         queue_event_add( timer_nmi_total, timer_nmi_triggered, 0 );
         if( config & (1<<1) )
         {
            // oneshot
            timer_nmi_total = 0;
         }
      }
      else
      {
         // lowbyte: store and stop timer
         queue_event_cancel( timer_nmi_triggered );
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
         queue_event_add( timer_irq_total, timer_irq_triggered, 0 );
         if( config & (1<<1) )
         {
            // oneshot
            timer_irq_total = 0;
         }
      }
      else
      {
         // lowbyte: store and stop timer
         queue_event_cancel( timer_irq_triggered );
         timer_irq_total = value;
      }
   }
}


static inline void reset_clear( void *data )
{
   gpio_set_mask( bus_config.mask_reset );
}


static inline void system_reset()
{
   int dummy;
   static int cycles_since_start = 0;
   
   // if we're the one causing the reset, make sure it's not for ever
   if( ++cycles_since_start > 8 )
   {
      gpio_set_mask( bus_config.mask_reset );
      cycles_since_start = 0;
   }

   timer_nmi_total       = 0;
   nmi_timer_triggered   = false;
   timer_irq_total       = 0;
   irq_timer_triggered   = false;
   watchdog_cycles_total = 0;
   set_bank( 1 );
   queue_event_reset();
   // just a test, remove
   //queue_event_add( 16, timer_nmi_triggered, 0 );
   while( queue_try_remove( &queue_uart_read, &dummy ) )
   {
      // just loop until queue is empty
   }
   while( queue_try_remove( &queue_uart_write, &dummy ) )
   {
      // just loop until queue is empty
   }
}


static inline void handle_flash_sync( void *data )
{
   dhara_flash_sync();
}


static inline void handle_flash_dma()
{
   uint16_t *lba = (uint16_t*)&ram[0xDF70];
   uint16_t *mem = (uint16_t*)&ram[0xDF72];
   int retval = 0;

   ram[address] = 0x00;
   if( !dhara_flash_size )
   {
      // size = 0: no dhara image found
      ram[address] |= 0xF0;
   }
   // filter out bad ranges, removing second line would write protect ROM
   if( (*mem < 0x0004) ||                                       // zeropage I/O
       ((*mem > (0xD000-SECTOR_SIZE)) && (*mem < ROM_START)) || // $Dxxx I/O
       (*mem > (0x10000-SECTOR_SIZE)) )                         // would go out of bounds
   {
      // DMA would run into I/O which is not possible, only RAM works
      ram[address] |= 0xF1;
   }
   if( *lba >= 0x8000 )
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
         if( queue_event_contains( handle_flash_sync ) )
         {
            // delay any waiting sync events
            queue_event_cancel( handle_flash_sync );
            queue_event_add( 20000, handle_flash_sync, 0 );
         }
         break;
      case 1: // write
         retval = dhara_flash_write( *lba, &ram[*mem] );
         // drop any waiting sync events, since a new one is created
         queue_event_cancel( handle_flash_sync );
         // after ~.02 seconds without additions writes run sync
         queue_event_add( 20000, handle_flash_sync, 0 );
         break;
      case 3: // trim
         retval = dhara_flash_trim( *lba );
         // drop any waiting sync events, since a new one is created
         queue_event_cancel( handle_flash_sync );
         // after ~.02 seconds without additions writes run sync
         queue_event_add( 20000, handle_flash_sync, 0 );
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
   for( int i = 0; i < count_of(watchdog_states); ++i )
   {
      uint32_t _state = watchdog_states[(i + _queue_cycle_counter) & 0xFF];
      debug_dump_state( count_of(watchdog_states)-i, _state );
   }
   debug_dump_state( 0, gpio_get_all() );
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
   
   printf("\n");
   printf("PLL_SYS:             %3d.%03dMhz\n", f_pll_sys / 1000, f_pll_sys % 1000 );
   printf("PLL_USB:             %3d.%03dMhz\n", f_pll_usb / 1000, f_pll_usb % 1000 );
   printf("ROSC:                %3d.%03dMhz\n", f_rosc    / 1000, f_rosc % 1000 );
   printf("CLK_SYS:             %3d.%03dMhz\n", f_clk_sys / 1000, f_clk_sys % 1000 );
   printf("CLK_PERI:            %3d.%03dMhz\n", f_clk_peri / 1000, f_clk_peri % 1000 );
   printf("CLK_USB:             %3d.%03dMhz\n", f_clk_usb / 1000, f_clk_usb % 1000 );
   printf("CLK_ADC:             %3d.%03dMhz\n", f_clk_adc / 1000, f_clk_adc % 1000 );
   printf("CLK_RTC:             %3d.%03dMhz\n", f_clk_rtc / 1000, f_clk_rtc % 1000 );
}

void debug_internal_read_sector(uint16_t dhara_sector){

   uint8_t dhara_buffer[SECTOR_SIZE];

   dhara_flash_read(dhara_sector,dhara_buffer);
  
   printf("dhara read sector $%04x:\n", dhara_sector );
   for( int i = 0; i < SECTOR_SIZE; ++i )
   {
      printf( " %02x", dhara_buffer[i] );
      if( (i & 0xF) == 0xF )
      {
         printf( "\n" );
      }
   }

 }  

void debug_internal_drive()
{
   dhara_flash_info_t dhara_info;
   
   uint16_t dhara_sector = 0;
   uint8_t dhara_buffer[SECTOR_SIZE];

   dhara_flash_info( dhara_sector, &dhara_buffer[0], &dhara_info );
   uint64_t hw_size  = dhara_info.erase_cells * dhara_info.erase_size;
   uint64_t lba_size = dhara_info.sectors * dhara_info.sector_size;
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
   debug_internal_read_sector(dhara_sector);

}


void system_trap()
{
   debug_backtrace();
}


static inline void handle_io()
{
   // TODO: split this up in reads and writes
   uint8_t data = state >> bus_config.shift_data;
   bool success;

   if( state & bus_config.mask_rw )
   {
      /* I/O read */
      switch( address & 0xFF )
      {
         case 0x00:
         case 0x01:
         case 0x02:
         case 0x03:
            bus_data_write( irq_timer_triggered ? 0x80 : 0x00 );
            irq_timer_triggered = false;
            break;
         case 0x04:
         case 0x05:
         case 0x06:
         case 0x07:
            bus_data_write( nmi_timer_triggered ? 0x80 : 0x00 );
            nmi_timer_triggered = false;
            break;
         case 0x08:
         case 0x09:
         case 0x0A:
         case 0x0B:
            bus_data_write( watchdog_cycles_total ? 0x80 : 0x00 );
            break;
         case 0xFA: /* console UART read */
            success = queue_try_remove( &queue_uart_read, &data );
            bus_data_write( success ? data : 0x00 );
            break;
         case 0xFB: /* console UART read queue */
            bus_data_write( queue_get_level( &queue_uart_read )  );
            break;
         case 0xFD: /* console UART write queue */
            bus_data_write( queue_get_level( &queue_uart_write )  );
            break;
         case 0xFE: /* random */
            bus_data_write( rand() & 0xFF );
            break;
         case 0xFF: /* read bank number */
            /* just slip through to ram shadow */
         default:
            /* everything else is handled like RAM by design */
            handle_ramrom();
            break;
      }
   }
   else
   {
      /* I/O write */
      data = bus_data_read();
      switch( address & 0xFF )
      {
         case 0x00:
         case 0x01:
         case 0x02:
         case 0x03:
         case 0x04:
         case 0x05:
         case 0x06:
         case 0x07:
            timer_setup( data, address & 0x07 );
            break;
         case 0x08:
         case 0x09:
         case 0x0A:
         case 0x0B:
            watchdog_setup( data, address & 0x03 );
            break;
         case 0x74: /* dma read from flash disk */
         case 0x75: /* dma write to flash disk */
         case 0x77: /* flash disk trim, no dma address used */
            /* access is strobe: written data does not matter */
            handle_flash_dma();
            break;
         case 0xFA: /* UART read: enable crlf conversion */
            uart_set_translate_crlf( uart0, data & 1 );
            break;
         case 0xFC: /* console UART write */
            queue_try_add( &queue_uart_write, &data );
            break;
         case 0xFE: /* DEBUG only! */
            system_trap();
            system_reboot();
            break;
         case 0xFF: /* set bankswitch register for $E000-$FFFF */
            /* when changing from 0xFF make sure to adjust in set_bank() */
            set_bank( bus_data_read() );
            break;
         default:
            /* everything else is handled like RAM by design */
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
#if SPEED_TEST
   uint64_t time_start, time_end;
   double time_exec;
   uint32_t time_hz;
   uint32_t cyc;

   bus_init();
   system_init();
   system_reboot();

   time_start = time_us_64();
   for(cyc = 0; cyc < SPEED_TEST; ++cyc)
#else
   bus_init();
   system_init();
   system_reboot();

   // when not running as speed test run main loop forever
   for(;;)
#endif
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
         /* internal i/o */
         handle_io();
      }
      else if( (address <= 0x0003) || ((address & 0xF000) == 0xD000) )
      {
         /* external i/o: keep hands off the bus */
         gpio_set_dir_in_masked( bus_config.mask_data );
      }
      else
      {
         handle_ramrom();
      }

      // done: set clock to low
      gpio_clr_mask( bus_config.mask_clock );

      // log last states
      watchdog_states[(_queue_cycle_counter) & 0xff] = gpio_get_all();
   }

#if SPEED_TEST
   time_end = time_us_64();
   time_exec = (double)(time_end - time_start) / CLOCKS_PER_SEC / 10000;
   time_hz = (double)cyc / time_exec;

   debug_backtrace();
   debug_clocks();
   debug_internal_drive();

   printf( "\nROM:" );
   for( int i = ROM_START; i < ROM_START+0x10; ++i )
   {
      printf( " %02x", ram[i] );
   }
   printf("\nFFF0:");
   for( int i = 0xFFF0; i < 0x10000; ++i )
   {
      printf( " %02x", ram[i] );
   }
   printf("\n[...]\n0200:");
   for( int i = 0x0200; i < 0x0210; ++i )
   {
      printf( " %02x", ram[i] );
   }

   for(;;)
   {
      printf( "\rbus has terminated after %d cycles in %.06f seconds: %d.%03dMHz ",
              cyc, time_exec, time_hz / 1000000, (time_hz % 1000000) / 1000 );
      sleep_ms(2000);
   }
#endif
}


void cpu_halt( bool stop )
{
   if( stop )
   {
      gpio_clr_mask( bus_config.mask_rdy );
   }
   else
   {
      gpio_set_mask( bus_config.mask_rdy );
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
   // make sure that RDY line is high
   gpio_set_mask( bus_config.mask_rdy | bus_config.mask_irq | bus_config.mask_nmi );

   // pull reset line for a couple of cycles
   gpio_clr_mask( bus_config.mask_reset );
   queue_event_add( INTERRUPT_LENGTH, reset_clear, 0 );
   cpu_halt( false );
}


