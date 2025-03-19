
#ifndef SOUND_BUS_H
#define SOUND_BUS_H SOUND_BUS_H

#include <hardware/clocks.h>
#include <pico/util/queue.h>
#include <sound_bus.pio.h>

typedef void (*sound_bus_callback_t)();

void sound_bus_init();
void sound_bus_handler_program_init( PIO pio, uint sm, uint offset,
                                     vga_bus_callback_t callback, float freq );

#endif

