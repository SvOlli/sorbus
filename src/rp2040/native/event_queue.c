
#include "event_queue.h"

#include <assert.h>
#include <string.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

uint64_t      _queue_cycle_counter  = 0;
uint64_t      _queue_next_timestamp = 0;
volatile queue_event_t *_queue_next_event    = 0;

static queue_event_t queue_events[QUEUE_EVENT_SIZE] = { 0 };


#if 0
static inline uint32_t cycles_until( uint64_t start, uint64_t end )
{
   if( end >= start )
   {
      return (end - start); // truncate is okay since number are close by
   }
   else
   {
      return UINT32_MAX;
   }
}
#endif


// replacement for "new": get an empty event
static inline queue_event_t *queue_event_get()
{
   for( int i = 0; i < count_of(queue_events); ++i )
   {
      if( !(queue_events[i].timestamp) )
      {
         return &queue_events[i];
      }
   }
   return 0;
}


// clean out queue totally
void queue_event_init()
{
   memset( &queue_events[0], 0x00, sizeof(queue_events) );
   _queue_next_timestamp = 0;
}


void queue_event_add( uint32_t when, queue_event_handler_t handler, void *data )
{
   uint64_t timestamp = _queue_cycle_counter + when;

   queue_event_t *newevent = queue_event_get();
   assert( newevent );

   // fill in data
   newevent->timestamp = timestamp;
   newevent->handler   = handler;
   newevent->data      = data;
   newevent->next      = 0;

   if( !_queue_next_event )
   {
      // that's easy: no next event means no events at all
      _queue_next_event = newevent;
   }
   else
   {
      // is it earlier than the first entry in queue?
      if( newevent->timestamp < _queue_next_event->timestamp )
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
            if( newevent->timestamp < current->timestamp )
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


void queue_event_cancel( queue_event_handler_t handler )
{
   queue_event_t *current  = 0;
   queue_event_t *previous = 0;

   for( current = _queue_next_event; current; current = current->next )
   {
      if( current->handler == handler )
      {
         if( previous )
         {
            previous->next = current->next;
         }
         else
         {
            _queue_next_event = _queue_next_event->next;
         }

         queue_event_drop( current );

         return; // canceling first only, remove return to cancel all
      }
      previous = current;
   }
}
