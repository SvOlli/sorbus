
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
   void                   *data;
   struct queue_event_s   *next;
 */
   queue_event_t *event;
   int i = 0;
   printf( "%s: %016llx:%016llx\n", text, _queue_cycle_counter, _queue_next_timestamp );
   for( event = _queue_next_event; event; event = event->next )
   {
      printf( "%02d:%016llx:%p:%p\n",
              i++,
              event->timestamp,
              event->handler,
              event->data );
   }
   printf( "done.\n" );
}


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


void queue_event_add( uint32_t when, queue_event_handler_t handler, void *data )
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
      bool insert = false;
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
               insert = true;
               break;
            }
            previous = current;
         }
         assert( insert );
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

         break;
      }
      previous = current;
   }
}


bool queue_event_contains( queue_event_handler_t handler )
{
   bool retval = false;
   queue_event_t *current  = 0;

   for( current = _queue_next_event; current; current = current->next )
   {
      if( current->handler == handler )
      {
         retval = true;
         break; // found one, no need to search further
      }
   }

   return retval;
}
