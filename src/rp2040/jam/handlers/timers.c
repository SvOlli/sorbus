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
#include "event_queue.h"

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
const uint8_t  timer_triggered_mask[2]  = { 0x33, 0xCC };

uint8_t timer_triggered          = 0x00;

struct repeating_timer timer_ms[4];
int64_t timer_ms_saved[4];
// need to cache values written to RAM as they are overwritten by status
uint16_t timer_cache[8]          = { 0x0000, 0x0000, 0x0000, 0x0000,
                                     0x0000, 0x0000, 0x0000, 0x0000 };
// status values for not triggered and triggered

//
// internal functions
//


static void set_state( uint8_t id, bool value )
{
   // mask out relevant bits
   id &= 0x0007;
   // for indexed access
   bool nmi = id & 2;

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
      if( !timer_triggered_mask[nmi] )
      {
         // no -> clear interrupt
         gpio_set_mask( timer_mask[nmi] );
      }
   }
   // set all 4 bytes of timer registers (lo/hi, single/repeat)
   *(uint16_t*)&ram[MEM_ADDR_TIMERS | (id >> 1)] =
      // with a value of $80 for each byte at once
      timer_triggered_value[value];
}


static bool callback_timer_ms( struct repeating_timer *t )
{
   // data is not a pointer, but an uint32_t
   uint32_t id = (uint32_t)t->user_data;
   // bit 0 contains single shot=1 instead of repeat=0
   bool repeat = !(id & 1);

   set_state( id, true );
   if( repeat )
   {
      // make sure that the timer has the correct value (again)
      t->delay_us = -100 * timer_cache[id];
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


void timers_reset()
{
   timer_triggered = 0x00;
   memset( &timer_cache[0],       0, sizeof( timer_cache ) );
   memset( &timer_ms[0],          0, sizeof( timer_ms ) );
}


void timer_stop()
{
   for( int id = 4; id < 8; ++id )
   {
      struct repeating_timer *t = &timer_ms[id & 3];
      cancel_repeating_timer( t );
   }
}


void timer_restart()
{
   for( int id = 4; id < 8; ++id )
   {
      struct repeating_timer *t = &timer_ms[id & 3];
      if( t->user_data )
      {
         add_repeating_timer_us( -100 * timer_cache[id], callback_timer_ms, t->user_data, t );
      }
   }
}


void event_timer_cycle( uint32_t value )
{
   uint8_t id = value & 0x07;
   set_state( id, true );
   if( !(id & 1) )
   {
      // repeat timer
      queue_event_add( timer_cache[id], value );
   }
}


void io_post_timer( bool rw, uint8_t data, uint16_t address )
{
   // config bits (decoded from address)
   //           0        1
   // bit 0: lobyte   hibyte
   // bit 1: repeat   oneshot
   // bit 2: irq      nmi
   // bit 3: cycle    ms/10
   uint8_t config = address & 0x0f;
   bool highbyte  = address & 0x01;
   uint32_t id    = config >> 1;
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
   if( id & 0x04 )
   {
      // ms
      struct repeating_timer *t = &timer_ms[id & 3];

      cancel_repeating_timer( t );
      t->user_data = 0; // indicate disabled timer
      if( timer_cache[id] && highbyte )
      {
         add_repeating_timer_us( -100 * timer_cache[id], callback_timer_ms, (void*)id, t );
      }

      // clear flag for later read
      set_state( address, false );
   }
   else
   {
      // cycle
      if( queue_event_contains( BUSMSG_EVENT_TIMER | id ) )
      {
         // when timer is running cancel it and clear cached value
         queue_event_cancel( BUSMSG_EVENT_TIMER | id );
         timer_cache[id] = 0;
      }
      if( highbyte )
      {
         // highbyte: store and start timer
         queue_event_add( timer_cache[id], BUSMSG_EVENT_TIMER | id );
      }
   }
}


int info_timers( char *b, size_t bsize )
{
   int   used = 0;
   int   i;

   used += snprintf( b+used, bsize-used, "ID | <CFG> | value\n" );
   for( i = 0; i < 8; ++i )
   {
      if( (used+1) >= bsize )
      {
         break;
      }
      used += snprintf( b+used, bsize-used,
                        "%2d | %c %c %c | %5d\n",
                        i, i&1 ? '1':'R', i&2 ? 'N':'I', i&4 ? 'T':'C',
                        timer_cache[i] );
   }
   return used;
}
