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


#include <time.h>
#include <hardware/clocks.h>


uint32_t watchdog_cycles_total      = 0;
uint64_t time_per_mcc = 1; // in case a division is done


void event_cpufreq( uint32_t data )
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

   queue_event_add( 1000000, data );
}


//
// API functions
//


void clocks_reset()
{
   watchdog_cycles_total = 0;
   queue_event_cancel( BUSMSG_META_WATCHDOG );
   if( !queue_event_contains( BUSMSG_EVENT_CPUFREQ ) )
   {
      queue_event_add( 1000000, BUSMSG_EVENT_CPUFREQ );
   }
}


int info_clocks( char *buffer, size_t size )
{
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
   uint time_hz = (double)1000000.0 / ((double)(time_per_mcc) / CLOCKS_PER_SEC / 10000);

   return snprintf( buffer, size,
         "  CLK_SYS: %3u.%03d   MHz\n"
         " CLK_PERI: %3d.%03d   MHz\n"
         "%9s: %3ld.%06ldMHz"
         , f_clk_sys / 1000, (f_clk_sys % 1000)
         , f_clk_peri / 1000, (f_clk_peri % 1000)
         , da_cputype_name( cputype ), time_hz / 1000000, time_hz % 1000000 );
}


void event_watchdog( uint32_t data )
{
   // stop system when watchdog timer expires

   bus_stop_cause = data;
}


void io_post_watchdog( bool rw, uint8_t data, uint16_t address )
{
   if( address & 0x04 )
   {
      // cyclecount shares the same 8-byte address space with watchdog
      // so it needs to be worked on within the same handler
      // since its implementation is simple, that's not a problem
      if( !rw )
      {
         // write: copy lower 32 bits of cycle counter to RAM
         *(uint32_t*)(&ram[MEM_ADDR_CYCLECOUNT]) = (uint32_t)_queue_cycle_counter;
      }

      // early return, since we're done with cyclecount
      return;
   }

   // continue with watchdog
   if( rw )
   {
      // reading from non-stop value retriggers the watchdog
      if( queue_event_contains( BUSMSG_META_WATCHDOG ) )
      {
         queue_event_cancel( BUSMSG_META_WATCHDOG );
         queue_event_add( watchdog_cycles_total, BUSMSG_META_WATCHDOG );
      }
   }
   else
   {
      switch( address & 0x03 )
      {
         case 0: // stop
            queue_event_cancel( BUSMSG_META_WATCHDOG );
            watchdog_cycles_total = 0;
            ram[MEM_ADDR_WATCHDOG] = 0x00;
            break;
         case 3: // start
            watchdog_cycles_total = (*(uint32_t*)&ram[MEM_ADDR_WATCHDOG] >> 8);
            queue_event_cancel( BUSMSG_META_WATCHDOG );
            queue_event_add( watchdog_cycles_total, BUSMSG_META_WATCHDOG );
            ram[MEM_ADDR_WATCHDOG] = 0x80;
            break;
         default: // nothing
            break;
      }
   }
}

