
#ifndef _NATIVE_EVENT_QUEUE_H_
#define _NATIVE_EVENT_QUEUE_H_ _NATIVE_EVENT_QUEUE_H_

#define QUEUE_EVENT_SIZE (32)

#include <stdint.h>

typedef void (*queue_event_handler_t)(void *data);

typedef struct queue_event_s {
   uint64_t               timestamp;
   queue_event_handler_t  handler;
   void                   *data;
   struct queue_event_s   *next;
} queue_event_t;

extern uint64_t _queue_cycle_counter;
extern uint64_t _queue_next_timestamp;
extern volatile queue_event_t *_queue_next_event;


// replacement for "delete": clear out data for memory block to be reused
static inline void queue_event_drop( queue_event_t *event )
{
   event->timestamp = 0;
   event->handler   = 0;
   event->data      = 0;
   event->next      = 0;
}


// process event queue
static inline void queue_event_process()
{
   ++_queue_cycle_counter;
   // while instead of if, because more than one entry may have same timestamp
   while( _queue_cycle_counter == _queue_next_timestamp )
   {
      queue_event_t *current = _queue_next_event;
      _queue_next_event      = _queue_next_event->next;
      _queue_next_timestamp  = _queue_next_event ? _queue_next_event->timestamp : 0;

      current->handler( current->data );
      queue_event_drop( current );
   }
}

// add an event to the loop, executed at now + when clock cycles
// note: the handler is primary key
void queue_event_add( uint32_t when, queue_event_handler_t handler, void *data );

// remove event from queue, identified by the handler
void queue_event_cancel( queue_event_handler_t handler );

// re-initialize the queue
void queue_event_init();

#endif

