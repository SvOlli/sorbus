
#ifndef _EVENT_QUEUE_H_
#define _EVENT_QUEUE_H_ _EVENT_QUEUE_H_

#include "jam.h"

#define QUEUE_EVENT_SIZE (32)
#define QUEUE_EVENT_INLINE (0)

typedef enum {
   EVENT_NONE = 0,
   EVENT_CLEAR_RESET,
   EVENT_ESTIMATE_CPUFREQ,
   EVENT_WATCHDOG,
   EVENT_TIMER_CYCLE_IRQ,
   EVENT_TIMER_CYCLE_NMI,
   EVENT_FLASH_SYNC
} event_id_t;

#include <stdbool.h>
#include <stdint.h>

typedef void (*queue_event_handler_t)(void);

typedef struct queue_event_s {
   uint64_t                timestamp;
   struct queue_event_s    *next;
   uint32_t                full_id;
} queue_event_t;

extern uint64_t      _queue_cycle_counter;
extern uint64_t      _queue_next_timestamp;
extern queue_event_t *_queue_next_event;


// replacement for "delete": clear out data for memory block to be reused
static inline void queue_event_drop( queue_event_t *event )
{
   event->timestamp = 0;
   event->full_id   = 0;
   event->next      = 0;
}


// process event queue
#if QUEUE_EVENT_INLINE
   // code is part of loop
#else
#include <pico/multicore.h>
static inline void queue_event_process()
{
   if( _queue_next_timestamp == ++_queue_cycle_counter )
   {
      uint32_t              full_id;
      queue_event_t         *current;

      current               = _queue_next_event;
      _queue_next_event     = _queue_next_event->next;
      _queue_next_timestamp = _queue_next_event ? _queue_next_event->timestamp : 0;

      full_id = current->full_id;

      queue_event_drop( current );
      // full_id: 0x01eevvvv: 01: magic, ee: event number, vvvv: 16-bit data
      multicore_fifo_push_blocking( full_id );
   }
}
#endif

// add an event to the loop, executed at now + when clock cycles
// note: the handler is primary key
void queue_event_add( uint32_t when, uint32_t full_id );

// remove event from queue
void queue_event_cancel( uint32_t full_id );

// check if the event queue contains a specific event
bool queue_event_contains( uint32_t full_id );

// re-initialize the queue
void queue_event_reset();

// setup (and reset) the queue mutex
void queue_event_init();

// fill an snprintf-like buffer with the current state
int queue_event_info( char *b, size_t bsize );

#endif
