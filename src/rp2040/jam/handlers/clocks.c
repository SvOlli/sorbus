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
static alarm_id_t watchdog_alarm = 0;
static int64_t watchdog_remain = 0;
uint32_t cycles_list[16] = { 0 };
uint8_t cycles_index = 0;


#define busstate gpio_get_all()


static int64_t watchdog_action( __unused alarm_id_t id,
                                __unused void *user_data )
{
   watchdog_alarm = 0;
   bus_stop_cause = META_WATCHDOG;

   return 0;
}


static bool callback_cpufreq( repeating_timer_t *t )
{
   // event handler for estimating the 65C02 CPU speed
   // this event is exactly 1 time in queue

   static uint32_t cycles_last = 0;

   if( busstate & bus_config.mask_rdy )
   {
      // only count time when CPU is not halted
      cycles_per_second = cycle_counter - cycles_last;
      cycles_list[cycles_index++] = cycles_per_second;
      cycles_index &= 0x0f;
   }
   cycles_last = cycle_counter;

   return true;
}


//
// API functions
//


void info_cyclecount( char *buffer, size_t size )
{
   int i;
   int used = 0;

   for( i = cycles_index+1; i != cycles_index; i = (i+1) & 0x0F )
   {
      used += snprintf( buffer+used, size-used-1, "%8d\n", cycles_list[i] );
   }
}


void watchdog_freeze()
{
   if( watchdog_alarm > 0 )
   {
      watchdog_remain = remaining_alarm_time_us( watchdog_alarm );
      if( watchdog_remain >= 0 )
      {
         cancel_alarm( watchdog_alarm );
         watchdog_alarm = 0;
      }
      else
      {
         watchdog_remain = 0;
      }
   }
}


void watchdog_unfreeze()
{
   if( !watchdog_alarm && watchdog_remain )
   {
      watchdog_alarm = add_alarm_in_us( (uint64_t)watchdog_remain,
         watchdog_action, NULL, true );
      watchdog_remain = 0;
   }
}


void clocks_reset()
{
   static repeating_timer_t *t = 0;
   // make sure that it's only called once
   // timer runs always
   if( !t )
   {
      t = ht_calloc( 1, sizeof(repeating_timer_t) );
      // once every second
      add_repeating_timer_ms( -1000, callback_cpufreq, 0, t );
   }
   if( watchdog_alarm )
   {
      cancel_alarm( watchdog_alarm );
   }
   watchdog_alarm = 0;
   watchdog_remain = 0;
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


int info_watchdog( char *buffer, size_t size )
{
   uint32_t watchdog_timeout = (*(uint32_t*)&ram[MEM_ADDR_WATCHDOG] >> 8);
   return snprintf( buffer, size,
                    "%sabled\ntotal:  %8u\nremain: %8lld\n",
                    watchdog_remain ? "en":"dis",
                    watchdog_timeout,
                    watchdog_remain / 1000 );
}


void io_irq_watchdog_w( uint32_t in )
{
   if( in & 0x04 )
   {
      // write: copy lower 32 bits of cycle counter to RAM
      *(uint32_t*)(&ram[MEM_ADDR_CYCLECOUNT]) = cycle_counter;
   }
   else
   {
      switch( in & 0x03 )
      {
         case 0: // stop
            if( watchdog_alarm )
            {
               cancel_alarm( watchdog_alarm );
               watchdog_alarm = 0;
            }
            ram[MEM_ADDR_WATCHDOG] = 0x00;
            break;
         case 3: // start
            if( watchdog_alarm )
            {
               cancel_alarm( watchdog_alarm );
            }
            {
               uint32_t watchdog_timeout = (*(uint32_t*)&ram[MEM_ADDR_WATCHDOG] >> 8);
               watchdog_alarm = add_alarm_in_ms( watchdog_timeout, watchdog_action,
                                                 NULL, true );
            }
            ram[MEM_ADDR_WATCHDOG] = 0x80;
            break;
         default: // nothing
            break;
      }
   }
}


void io_irq_watchdog_r( uint32_t in )
{
   if( !(in & 0x04) )
   {
      if( watchdog_alarm )
      {
         uint32_t watchdog_timeout = (*(uint32_t*)&ram[MEM_ADDR_WATCHDOG] >> 8);
         cancel_alarm( watchdog_alarm );
         watchdog_alarm = add_alarm_in_ms( watchdog_timeout, watchdog_action,
                                           NULL, true );
      }
   }
}
