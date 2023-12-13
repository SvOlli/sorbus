
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
mutex_t       _queue_event_mutex;

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
   queue_event_t *retval = 0;

   QUEUE_EVENT_MUTEX_LOCK();
   for( int i = 0; i < count_of(queue_events); ++i )
   {
      if( !(queue_events[i].timestamp) )
      {
         retval = &queue_events[i];
         break;
      }
   }
   QUEUE_EVENT_MUTEX_UNLOCK();
   return retval;
}


// clean out queue totally
void queue_event_reset()
{
   QUEUE_EVENT_MUTEX_LOCK();
   memset( &queue_events[0], 0x00, sizeof(queue_events) );
   _queue_next_timestamp = 0;
   QUEUE_EVENT_MUTEX_UNLOCK();
}


// clean out queue totally
void queue_event_init()
{
   // TODO: check if mutex is already initialized
   mutex_init( &_queue_event_mutex );
   queue_event_reset();
}


void queue_event_add( uint32_t when, queue_event_handler_t handler, void *data )
{
   uint64_t timestamp = _queue_cycle_counter + when;

   queue_event_t *newevent = queue_event_get();
   if( !newevent )
   {
      printf( "Could not find place in event queue:\n" );
      printf( "addr    |timestamp       |handler |data    |next\n" );
      for( int i = 0; i < count_of(queue_events); ++i )
      {
         printf( "%08x|%16llx|%08x|%08x|%08x\n",
                 (uint32_t)&queue_events[i],
                 (uint64_t)queue_events[i].timestamp,
                 (uint32_t)queue_events[i].handler,
                 (uint32_t)queue_events[i].data,
                 (uint32_t)queue_events[i].next );
      }
      assert( newevent );
   }

   // fill in data
   newevent->timestamp = timestamp;
   newevent->handler   = handler;
   newevent->data      = data;
   newevent->next      = 0;

   QUEUE_EVENT_MUTEX_LOCK();
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
      }
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
            // due to speed constrains, only one event per timestamp is allowed
            if( newevent->timestamp == _queue_next_event->timestamp )
            {
               (newevent->timestamp)++;
            }
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
   QUEUE_EVENT_MUTEX_UNLOCK();
}


void queue_event_cancel( queue_event_handler_t handler )
{
   queue_event_t *current  = 0;
   queue_event_t *previous = 0;

   QUEUE_EVENT_MUTEX_LOCK();
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

         break; // canceling first only, remove break to cancel all
                // due to implementation logic, NEVER cancel all:
                // when handling the event, a new one might be created within
                // the handler, before the current one is removed by
                // queue_event_handler()
      }
      previous = current;
   }
   QUEUE_EVENT_MUTEX_UNLOCK();
}


bool queue_event_contains( queue_event_handler_t handler )
{
   bool retval = false;
   queue_event_t *current  = 0;

   QUEUE_EVENT_MUTEX_LOCK();
   for( current = _queue_next_event; current; current = current->next )
   {
      if( current->handler == handler )
      {
         retval = true;
         break; // found one, no need to search further
      }
   }
   QUEUE_EVENT_MUTEX_UNLOCK();

   return retval;
}
