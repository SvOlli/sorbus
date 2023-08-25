/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a Native custom platform
 * for the Sorbus Computer
 */

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "../rp2040_purple.h"
#include <pico/rand.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>

#include <hardware/clocks.h>

#include "common.h"
#include "native_rom.h"

#include "../bus.h"
#if 0
#include "gpio_oc.h"
#endif

#include "event_queue.h"


// set this to 5000000 to run for 5 million cycles while keeping time
#define SPEED_TEST 5000000
// this is where the write protected area starts
#define ROM_START (0xE000)
// the number of clock cycles a timer interrupt is triggered
#define INTERRUPT_LENGTH (8)

uint8_t memory[0x10000]; // 64k of RAM/ROM and I/O
uint32_t state;
uint32_t address;

// no need to volatile these, as they are only used within bus_run loop
uint32_t timer_irq_total       = 0;
uint32_t timer_nmi_total       = 0;

bool nmi_timer_triggered       = false;
bool irq_timer_triggered       = false;

uint32_t watchdog_states[128]  = { 0 };
uint32_t watchdog_cycles_total = 0;


/******************************************************************************
 * internal functions
 ******************************************************************************/
static void handle_rdy( void *data )
{
   bool stop = (bool)data;

   if( stop )
   {
      gpio_clr_mask( bus_config.mask_rdy );
   }
   else
   {
      gpio_set_mask( bus_config.mask_rdy );
   }
}




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
   // as of now only one bank exists
   memcpy( &memory[ROM_START], &native_rom[0], sizeof(native_rom) );
}


static inline void handle_ram()
{
   // assume that data bus direction is already set
   if( state & bus_config.mask_rw )
   {
      // read from memory and write to bus
      bus_data_write( memory[address] );
   }
   else
   {
      // read from bus and write to memory write
      // WozMon (and Krusader) is in ROM, so only accept writes below romstart
      if( address < ROM_START )
      {
         memory[address] = bus_data_read();
      }
   }
}


static inline void trigger_watchdog()
{
   // first step: halt CPU
   gpio_clr_mask( bus_config.mask_rdy );
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
      if( watchdog_cycles_total )
      {
         // timer is running: restart
         queue_event_cancel( watchdog_trigger );
         queue_event_add( watchdog_cycles_total, watchdog_trigger, 0 );
      }
      else
      {
         // set register
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
   set_bank( 0 );
   queue_event_init();
   queue_event_add( 16, timer_nmi_triggered, 0 );
   while( queue_try_remove( &queue_uart_read, &dummy ) )
   {
      // just loop until queue is empty
   }
   while( queue_try_remove( &queue_uart_write, &dummy ) )
   {
      // just loop until queue is empty
   }
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
            bus_data_write( get_rand_32() & 0xFF );
            break;
         case 0xFF: /* read bank number */
            bus_data_write( 0 ); // no bank switching implemented, yet
            break;
         default:
            /* everything else is handled like RAM by design */
            handle_ram();
            break;
      }
   }
   else
   {
      /* I/O write */
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
         case 0xFC: /* console UART write */
            queue_try_add( &queue_uart_write, &data );
            break;
         case 0xFF: /* set bankswitch register for $F000-$FFFF */
            set_bank( bus_data_read() );
            break;
         default:
            /* everything else is handled like RAM by design */
            handle_ram();
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

   uint f_pll_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
   uint f_pll_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
   uint f_rosc     = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
   uint f_clk_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
   uint f_clk_adc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
   uint f_clk_rtc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

   time_start = time_us_64();
   for(cyc = 0; cyc < SPEED_TEST; ++cyc)
#else
   // when not running as speed test run main loop forever
   for(;;)
#endif
   {
      // LOW ACTIVE
      if( !(state & bus_config.mask_reset) )
      {
         system_reset();
      }

      // check if internal events need processing
#if 1
      queue_event_process();
#else
   ++_queue_cycle_counter;
   // while instead of if, because more than one entry may have same timestamp
   while( _queue_cycle_counter == _queue_next_timestamp )
   {
      queue_event_t *current = _queue_next_event;
      _queue_next_event      = _queue_next_event->next;
      _queue_next_timestamp  = _queue_next_event ? _queue_next_event->timestamp : 0;

      current->handler( current->data );
      queue_event_drop( current );
   }
#endif

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
         handle_ram();
      }

      // done: set clock to low
      gpio_clr_mask( bus_config.mask_clock );

      // log last states
      watchdog_states[(_queue_cycle_counter) & 0x7f] = gpio_get_all();
   }

#if SPEED_TEST
   time_end = time_us_64();
   time_exec = (double)(time_end - time_start) / CLOCKS_PER_SEC / 10000;
   time_hz = (double)cyc / time_exec;

   for( int i = 0; i < 0x80; ++i )
   {
      uint32_t _state = watchdog_states[i];
      printf( "%3d:%04x %c %02x %c%c%c%c\n",
         i,
         (_state & bus_config.mask_address) >> (bus_config.shift_address),
         (_state & bus_config.mask_rw) ? 'r' : 'w',
         (_state & bus_config.mask_data) >> (bus_config.shift_data),
         (_state & bus_config.mask_reset) ? ' ' : 'R',
         (_state & bus_config.mask_nmi) ? ' ' : 'N',
         (_state & bus_config.mask_irq) ? ' ' : 'I',
         (_state & bus_config.mask_rdy) ? ' ' : 'S' );
   }
   printf( "E000:" );
   for( int i = 0xE000; i < 0xE010; ++i )
   {
      printf( " %02x", memory[i] );
   }
   printf("\n[...]\nFFF0:");
   for( int i = 0xFFF0; i < 0x10000; ++i )
   {
      printf( " %02x", memory[i] );
   }
   printf("\n");
   printf("PLL_SYS:             %3d.%03dMhz\n", f_pll_sys / 1000, f_pll_sys % 1000 );
   printf("PLL_USB:             %3d.%03dMhz\n", f_pll_usb / 1000, f_pll_usb % 1000 );
   printf("ROSC:                %3d.%03dMhz\n", f_rosc    / 1000, f_rosc % 1000 );
   printf("CLK_SYS:             %3d.%03dMhz\n", f_clk_sys / 1000, f_clk_sys % 1000 );
   printf("CLK_PERI:            %3d.%03dMhz\n", f_clk_peri / 1000, f_clk_peri % 1000 );
   printf("CLK_USB:             %3d.%03dMhz\n", f_clk_usb / 1000, f_clk_usb % 1000 );
   printf("CLK_ADC:             %3d.%03dMhz\n", f_clk_adc / 1000, f_clk_adc % 1000 );
   printf("CLK_RTC:             %3d.%03dMhz\n", f_clk_rtc / 1000, f_clk_rtc % 1000 );
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
#if 0
   queue_event_add( 1, handle_rdy, (void*)stop );
#else
   if( stop )
   {
      gpio_clr_mask( bus_config.mask_rdy );
   }
   else
   {
      gpio_set_mask( bus_config.mask_rdy );
   }
#endif
}


// intended to be replaced by rom_load and ram_load in the future
bool system_memory_load( uint16_t address, uint16_t size, uint8_t *data )
{
   if( (address + size) > 0x10000 )
   {
      return false;
   }
   memcpy( &memory[address], data, size );
   return true;
}


void system_init()
{
   memset( &memory[0x0000], 0x00, sizeof(memory) );
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
