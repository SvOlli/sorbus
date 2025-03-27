
#ifndef SOUND_BUS_H
#define SOUND_BUS_H SOUND_BUS_H

#include <hardware/clocks.h>
#include <pico/util/queue.h>
#include <sound_bus.pio.h>

// might replace arguments
extern uint8_t sound_bus_addr;
extern uint8_t sound_bus_data;

// CPU writes call this handler
typedef void (*sound_bus_write_handler_t)( uint8_t addr, uint8_t data );
void sound_bus_write_handler_set( sound_bus_write_handler_t );
void sound_bus_write_handler_default( uint8_t addr, uint8_t data );

// CPU read values need to be prepared in this array
extern uint8_t sound_bus_read_values[0x100];
// after read, the set read_handler is called for notification
typedef void (*sound_bus_read_handler_t)( uint8_t addr );
void sound_bus_read_handler_set( sound_bus_read_handler_t );
void sound_bus_read_handler_default( uint8_t addr );


void sound_bus_init();
void sound_bus_loop();

#endif

