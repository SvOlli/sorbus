
#ifndef _32X32LEDS_H
#define _32X32LEDS_H _32X32LEDS_H

#include <stdint.h>

/* framebuffer as sniffed from the bus */
extern uint8_t led_framebuffer[0x400];

/* macro to for setting pixel */
#define led_pixel( pos, color ) \
do { led_framebuffer[pos & 0x3FF] = color; } while(0)

/* initialize PIO machine, must be run after bus_init() */
void led_init();
/* tab is 0x00 to 0x0F, bright is 0x00 to 0x03, 0 being brightest */
void led_setcolors( uint8_t tab, uint8_t bright );
/* write framebuffer to LEDs */
void led_flush();

#endif
