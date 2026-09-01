
#include "event_queue.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

uint64_t      _queue_cycle_counter  = 0;
uint64_t      _queue_next_timestamp = 0;
queue_event_t *_queue_next_event    = 0;

static queue_event_t queue_events[QUEUE_EVENT_SIZE] = { 0 };

void debug_queue_event( const char *text );

static inline void queue_event_debug( const char *text )
{
/*
   uint64_t               timestamp;
   queue_event_handler_t  handler;
   struct queue_event_s   *next;
 */
   queue_event_t *event;
   int i = 0;
   printf( "%s: %016llx:%016llx\n", text, _queue_cycle_counter, _queue_next_timestamp );
   for( event = _queue_next_event; event; event = event->next )
   {
      printf( "%02d:%016llx:%08lx\n",
              i++,
              event->timestamp,
              event->full_id );
   }
   printf( "done.\n" );
}


// replacement for "new": get an empty event
static inline queue_event_t *queue_event_get()
{
   queue_event_t *retval = 0;

   for( int i = 0; i < count_of(queue_events); ++i )
   {
      if( !(queue_events[i].timestamp) )
      {
         retval = &queue_events[i];
         break;
      }
   }
   return retval;
}


// clean out queue totally
void queue_event_reset()
{
   memset( &queue_events[0], 0x00, sizeof(queue_events) );
   _queue_next_event = 0;
   _queue_next_timestamp = 0;
}


// clean out queue totally
void queue_event_init()
{
   queue_event_reset();
}


void queue_event_add( uint32_t when, uint32_t full_id )
{
   uint64_t timestamp = _queue_cycle_counter + when;

   queue_event_t *newevent = queue_event_get();
   if( !newevent )
   {
      queue_event_debug( "Could not find place in event queue:\n" );
      assert( newevent );
   }

   // fill in data
   newevent->timestamp = timestamp;
   newevent->full_id   = full_id;
   newevent->next      = 0;
   if( !_queue_next_event )
   {
      // that's easy: no next event means no events at all
      _queue_next_event = newevent;
   }
   else
   {
      // due to speed constrains, only one event per timestamp is allowed
      if( newevent->timestamp == _queue_next_event->timestamp )
      {
         (newevent->timestamp)++;
         // now it's larger and can't be processes in "<" case
      }
      // is it earlier than the first entry in queue?
      else if( newevent->timestamp < _queue_next_event->timestamp )
      {
         newevent->next    = _queue_next_event;
         _queue_next_event = newevent;
      }
      else
      {
         queue_event_t *current;
         queue_event_t *previous = _queue_next_event;
         for( current = _queue_next_event; current; current = current->next )
         {
            // due to speed constrains, only one event per timestamp is allowed
            if( newevent->timestamp == current->timestamp )
            {
               (newevent->timestamp)++;
               // now it's larger and can't be in "<" case
            }
            else if( newevent->timestamp < current->timestamp )
            {
               break;
            }
            previous = current;
         }
         previous->next = newevent;
         newevent->next = current;
      }
   }

   _queue_next_timestamp = _queue_next_event->timestamp;
}


void queue_event_cancel( uint32_t full_id )
{
   queue_event_t *current  = 0;
   queue_event_t *previous = 0;

   for( current = _queue_next_event; current; current = current->next )
   {
      if( current->full_id == full_id )
      {
         if( previous )
         {
            // not first entry
            previous->next = current->next;
         }
         else
         {
            // first entry
            _queue_next_event = _queue_next_event->next;
         }

         queue_event_drop( current );

         break;
      }
      previous = current;
   }
}


// check if the event queue contains a specific event
bool queue_event_contains( uint32_t full_id )
{
   queue_event_t *current  = 0;

   for( current = _queue_next_event; current; current = current->next )
   {
      if( current->full_id == full_id )
      {
         return true;
      }
   }

   return false;
}


int queue_event_info( char *b, size_t bsize )
{
   int used = 0;
   int i;
   queue_event_t *event;

   const char *class[0x10] = {
   "undefined",         // 0x00xxxxxx
   "reset",             // 0x01xxxxxx
   "io_write",          // 0x02xxxxxx
   "io_read",           // 0x03xxxxxx
   "meta",              // 0x04xxxxxx
   "cpufreq",           // 0x05xxxxxx
   "timer_cycle",       // 0x06xxxxxx
   "flash_sync",        // 0x07xxxxxx
   "undefined",         // 0x08xxxxxx
   "undefined",         // 0x09xxxxxx
   "undefined",         // 0x0axxxxxx
   "undefined",         // 0x0bxxxxxx
   "undefined",         // 0x0cxxxxxx
   "undefined",         // 0x0dxxxxxx
   "undefined",         // 0x0exxxxxx
   "undefined"          // 0x0fxxxxxx
   };

   used = snprintf( b+used, bsize-used,
                    "cycle counter:   %016llx\n"
                    "next timerstamp: %016llx\n\n"
                    "id|       timestamp|   full_id|class\n"
                    , _queue_cycle_counter
                    , _queue_next_timestamp
                   );

   for( i = 0, event = _queue_next_event; event; event = event->next )
   {
      if( (used+1) >= bsize )
      {
         break;
      }
      used += snprintf( b+used, bsize-used,
                          "%02x|%016llx|0x%08x|%s\n"
                          , i++
                          , event->timestamp
                          , event->full_id
                          , class[(event->full_id >> 24) & 0xF]
                        );
   }
   return used;
}
