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

#include <time.h>
#include <hardware/clocks.h>


uint32_t cycles_per_second = 0;


#define busstate gpio_get_all()

static bool callback_cpufreq( struct repeating_timer *t )
{
   // event handler for estimating the 65C02 CPU speed
   // this event is exactly 1 time in queue

   static uint32_t cycles_last = 0;

   if( busstate & bus_config.mask_rdy )
   {
      // only count time when CPU is not halted
      cycles_per_second = cycle_counter - cycles_last;
   }
   cycles_last = cycle_counter;

   return true;
}


//
// API functions
//


void clocks_reset()
{
   static struct repeating_timer *t = 0;
   // make sure that it's only called once
   // timer runs always
   if( !t )
   {
      t = ht_calloc( 1, sizeof(struct repeating_timer) );
      // once every second
      add_repeating_timer_ms( -1000, callback_cpufreq, 0, t );
   }
}


int info_clocks( char *buffer, size_t size )
{
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);

   return snprintf( buffer, size,
         "  CLK_SYS: %3u.%03d   MHz\n"
         " CLK_PERI: %3d.%03d   MHz\n"
         "%9s: %3ld.%06ldMHz"
         , f_clk_sys / 1000, (f_clk_sys % 1000)
         , f_clk_peri / 1000, (f_clk_peri % 1000)
         , da_cputype_name( cputype )
         , cycles_per_second / 1000000, cycles_per_second % 1000000 );
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
         *(uint32_t*)(&ram[MEM_ADDR_CYCLECOUNT]) = cycle_counter;
      }

      // early return, since we're done with cyclecount
      return;
   }

#if 0
   // TODO: adjust for time
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
#endif
}

