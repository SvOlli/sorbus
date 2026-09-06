/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"
#include "bus.h"
#include "cpu_detect.h"
#include "da_base.h"
#include "event_queue.h"

#include <pico/multicore.h>
//#include <hardware/clocks.h>

// number of states should be power of 2
#define BUSLOG_SIZE (1024)
#if (BUSLOG_SIZE & (BUSLOG_SIZE-1))
#error BUSLOG_SIZE is not a power of 2
#endif
uint32_t buslog_states[BUSLOG_SIZE] = { 0 };
uint     buslog_index               = 0;

uint8_t  ram[0x10000] = { 0 };   // 64k of RAM and I/O
uint8_t  rom[FLASH_DRIVE_START_TXT-FLASH_KERNEL_START_TXT]; // buffer for roms
const uint8_t *romvec;           // pointer into current ROM/RAM bank at $E000
uint16_t address = 0;
uint32_t bus_stop_cause = 0;
uint32_t state;


void debug_raw_backtrace()
{
   printf( "\nTRACE_START %s\n", da_cputype_name( cputype ) );
   for( int i = buslog_index; i < (buslog_index + BUSLOG_SIZE); ++i )
   {
      printf( "%08lx\n", buslog_states[i & (BUSLOG_SIZE-1)] );
   }
   printf( "TRACE_END\n" );
}


void debug_get_backtrace( uint32_t **trace, uint32_t *entries, uint32_t *start )
{
   *trace   = &buslog_states[0];
   *entries = BUSLOG_SIZE;
   *start   = buslog_index & (BUSLOG_SIZE - 1);
}


static inline void bus_data_write( uint8_t data )
{
   gpio_put_masked( BUS_CONFIG_mask_data, ((uint32_t)data) << BUS_CONFIG_shift_data );
}


static inline uint8_t bus_data_read()
{
   return (gpio_get_all() >> BUS_CONFIG_shift_data);
}


static inline void handle_ramrom()
{
   // address is set as global variable
   if( gpio_get_all() & BUS_CONFIG_mask_rw )
   {
      //handle_read_ramrom();
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
      // fetch again, as write data is available later on the bus than address
      // data. also, gpio_get_all() is faster than accessing a variable
      ram[address] = gpio_get_all() >> BUS_CONFIG_shift_data;
   }
}

void bus_run()
{
   puts( "bus_run()" );
   for(;;)
   {
      // check if internal events need processing
      if( state & BUS_CONFIG_mask_rdy )
      {
#if QUEUE_EVENT_INLINE
         if( _queue_next_timestamp == ++_queue_cycle_counter )
         {
            uint32_t              full_id;
            queue_event_t         *current;

            current               = _queue_next_event;
            _queue_next_event     = _queue_next_event->next;
            _queue_next_timestamp = _queue_next_event ? _queue_next_event->timestamp : 0;

            full_id = current->full_id;

            queue_event_drop( current );
            // full_id: 0x0evvvvvv: 0e: event number, vvvvvv: 24-bit data
            multicore_fifo_push_blocking( full_id );
         }
#else
         queue_event_process();
#endif

         // LOW ACTIVE
         if( !(state & BUS_CONFIG_mask_reset) )
         {
            // reset is a bit of a tricky beast
            // it will be triggered by setting BUS_CONFIG_mask_reset to 0
            // this is just the handler for it
            if( !queue_event_contains( BUSMSG_RESET_CLEAR ) )
            {
               // two events are started by design triggered during reset
               // clearing of the reset line (in case it was done by the Sorbus)
               queue_event_add( 8, BUSMSG_RESET_CLEAR );
               // before that, run the reset rountines of the Sorbus
               queue_event_add( 1, BUSMSG_RESET_START );
            }
         }
      }

      // done: set clock to high
      gpio_set_mask( BUS_CONFIG_mask_clock );

      // bus should be still valid from clock low
      state = gpio_get_all();
//printf( "%08x\n", state);

      // setup bus direction so I/O can settle
      if( state & BUS_CONFIG_mask_rw )
      {
         // read from memory and write to bus
         gpio_set_dir_out_masked( BUS_CONFIG_mask_data );
         // note to future self: check if there's a better way for
         // external i/o than to set the GPIOs to write for a brief time
      }
      else
      {
         // read from bus and write to memory write
         gpio_set_dir_in_masked( BUS_CONFIG_mask_data );
      }

      address = ((state & BUS_CONFIG_mask_address) >> BUS_CONFIG_shift_address);

      /* memory map as handled here
       * x = handled here
       * $0000-$0003   external I/O
       * $0004-$CFFF x RAM
       * $D000-$DDFF   external I/O
       * $DE00-$DEFF   reserved (now: RAM)
       * $DF00-$DF7F x internal I/O (or just RAM)
       * $DF80-$DFFF x RAM
       * $E000-$FFFF x ROM/RAM
       */
      // setup data
      //if( (address <= 0x0003) || ((address & 0xF800) == 0xD000) )
      if( (address < 0x0004) || ((address >> 11) == 0x1A) )
      {
         // external i/o: keep hands off the bus
         gpio_set_dir_in_masked( BUS_CONFIG_mask_data );
      }
      else
      {
         handle_ramrom();
      }

      // log last states
      if( state & BUS_CONFIG_mask_rdy )
      {
         // state is not valid on the data when RP2040 is writing
         buslog_states[buslog_index++ & (BUSLOG_SIZE-1)] = gpio_get_all();

         //if( (address & 0xFE00) == 0xDE00 )
         //if( ((address >> 8) & 0xFE) == 0xDE )
         if( (address >> 9) == 0x6F ) // this is faster
         {
            uint32_t full_id = BUSMSG_TYPE_IO | (gpio_get_all() & BUS_CONFIG_mask_input);
//printf( __FILE__ "(%d): push %08x\n", __LINE__, full_id );
            multicore_fifo_push_blocking( full_id );
         }
      }

      // done: set clock to low
      gpio_clr_mask( BUS_CONFIG_mask_clock );
   }
}
