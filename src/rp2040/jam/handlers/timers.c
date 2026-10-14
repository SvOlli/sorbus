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

#include <string.h>

/*
 * this is almost a rewrite of the timer code
 *
 * system is now like this: the system has 8 timers reflecting every
 * combination of:
 * - repeat/oneshot
 * - irq/nmi
 * - cycle/10th of ms based
 */

// bit 0: lobyte   hibyte
// bit 1: repeat   oneshot
// bit 2: irq      nmi
// bit 3: cycle    ms/10

const uint32_t timer_mask[2]     = { BUS_CONFIG_mask_irq, BUS_CONFIG_mask_nmi };
const uint16_t timer_triggered_value[2] = { 0x0000, 0x8080 };
const uint8_t  timer_triggered_mask[2]  = { 0x05, 0x0A };

uint8_t timer_triggered          = 0x00;

repeating_timer_t timer_ms[4];
int64_t timer_ms_saved[4];
void *timer_userdata_saved[4];
// need to cache values written to RAM as they are overwritten by status
uint16_t timer_cache[4]          = { 0x0000, 0x0000, 0x0000, 0x0000 };
int64_t timer_remaining_us[4]    = { 0, 0, 0, 0 };


//
// internal functions
//


static void set_state( uint8_t id, bool value )
{
   // mask out relevant bits
   id &= 0x0003;
   // for indexed access
   bool nmi = id & 1;

   // set state
   if( value )
   {
      // trigger interrupt
      gpio_clr_mask( timer_mask[nmi] );
      // remember triggered id in bitfield
      timer_triggered |= (1 << id);
   }
   else
   {
      // clear remembered id in bitfield
      timer_triggered &= ~(1 << id);
      // any other IRQs or NMIs left?
      if( !(timer_triggered & timer_triggered_mask[nmi]) )
      {
         // no -> clear interrupt
         gpio_set_mask( timer_mask[nmi] );
      }
   }
   // set both bytes of timer registers (lo/hi)
   *(uint16_t*)&ram[MEM_ADDR_TIMERS | (id >> 1)] =
      // with a value of $80 for each byte at once
      timer_triggered_value[value];
}


static bool callback_timer_ms( repeating_timer_t *t )
{
   // data is not a pointer, but an uint32_t
   uint32_t id = (uint32_t)(t->user_data-1);
   // bit 0 contains single shot=1 instead of repeat=0
   bool repeat = !(id & 2);

   set_state( id, true );
   if( repeat )
   {
      // make sure that the timer has the correct value (again)
      t->delay_us = (int64_t)-100 * timer_cache[id];
   }
   else
   {
      // after single shot clear out timer
      timer_cache[id] = 0;
      t->user_data = 0;
   }
   return repeat;
}


//
// API functions
//


void timers_freeze()
{
   for( int id = 0; id < 4; ++id )
   {
      repeating_timer_t *t = &timer_ms[id];
      timer_remaining_us[id] = remaining_alarm_time_us( t->alarm_id );
      timer_userdata_saved[id] = t->user_data;
      cancel_repeating_timer( t );
   }
}


void timers_unfreeze()
{
   for( int id = 0; id < 4; ++id )
   {
      repeating_timer_t *t = &timer_ms[id];
      if( timer_userdata_saved[id] )
      {
         add_repeating_timer_us( timer_remaining_us[id], callback_timer_ms, timer_userdata_saved[id], t );
      }
   }
}


void timers_reset()
{
   timers_freeze();
   timer_triggered = 0x00;
   memset( &timer_cache[0],        0, sizeof( timer_cache ) );
   memset( &timer_ms[0],           0, sizeof( timer_ms ) );
   memset( &timer_remaining_us[0], 0, sizeof( timer_remaining_us ) );
}


void io_post_timer( bool rw, uint8_t data, uint16_t address )
{
   // config bits (decoded from address)
   //           0        1
   // bit 0: lobyte   hibyte
   // bit 1: irq      nmi
   // bit 2: repeat   oneshot
   uint8_t config = address & 0x07;
   bool highbyte  = address & 0x01;
   uint32_t id    = config >> 1;
   repeating_timer_t *t = &timer_ms[id];
   uint8_t value  = ram[address];

   // it make sense to clear out memory here:
   // read: after an acknoledge it should be $00
   // write: since it's write only, the state must be cleared out preparing
   //        for next read (will be set to $80 by the event handler)
   if( rw )
   {
      // after reading interrupt clear is all we need
      set_state( id, false );
      return;
   }

   // the following code is only for handling writes
   // since it's a write only, also clear out state preparing for next read
   // will be set to $80 by the event handler
   if( highbyte )
   {
      timer_cache[id] = (timer_cache[id] & 0x00FF) | (value << 8);
   }
   else
   {
      timer_cache[id] = (timer_cache[id] & 0xFF00) | value;
   }

   // make sure timer is stopped before (re)starting
   cancel_repeating_timer( t );
   t->user_data = 0; // indicate disabled timer

   if( timer_cache[id] && highbyte )
   {
      add_repeating_timer_us( -100 * timer_cache[id], callback_timer_ms,
                              (void*)(id+1), t );
   }

   // clear flag for later read
   set_state( address, false );
}


int info_timers( char *b, size_t bsize )
{
   int   used = 0;
   int   i;

   used += snprintf( b+used, bsize-used, "ID | config     |  value | remain\n" );
   for( i = 0; i < 4; ++i )
   {
      if( (used+1) >= bsize )
      {
         break;
      }
      used += snprintf( b+used, bsize-used,
                        "%2d | %s %s | %4d.%01d | %4d.%01d \n",
                        i, i&1 ? "NMI":"IRQ", i&2 ? "single":"repeat",
                        timer_cache[i]/10, timer_cache[i]%10,
                        timer_remaining_us[i]/1000, (timer_remaining_us[i]%1000)/100 );
   }
   return used;
}
