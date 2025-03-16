
#ifndef VGA_BUS_H
#define VGA_BUS_H VGA_BUS_H

#include <hardware/clocks.h>
#include <pico/util/queue.h>
#include <vga_bus.pio.h>

typedef void (*vga_bus_callback_t)();

void vga_bus_init();
void vga_bus_handler_program_init( PIO pio, uint sm, uint offset,
                                   vga_bus_callback_t callback, float freq );

uint32_t vga_bus_read();
void vga_bus_write( uint8_t data );

#endif

