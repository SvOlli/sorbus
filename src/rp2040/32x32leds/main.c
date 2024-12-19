/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements JAM (Just Another Machine), a custom Sorbus core
 * for the Sorbus Computer
 */

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "rp2040_purple.h"
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/binary_info.h>
#include <hardware/clocks.h>

#include "common/bus.h"
#include "32x32leds.h"

#include "bus.pio.h"


bi_decl(bi_program_name("Sorbus Computer 32x32 LEDs"))
bi_decl(bi_program_description("Driver for a 6502asm style LED matrix"))
bi_decl(bi_program_url("https://xayax.net/sorbus/"))


bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_address, "A0-A15"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_data,    "D0-D7"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_rw,      "R/!W"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_clock,   "CLK"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_reset,   "WS2812_DATA"));


void bus_init()
{
   uint offset = pio_add_program( pio1, &bus_wait_program );
   bus_wait_program_init( pio1, 0, offset );
}


static inline void bus_sniffer()
{
   uint32_t bus;
   uint16_t address;

   for(;;)
   {
      /* wait in tight loop for a write to bus to check address and data */
      bus = pio_sm_get_blocking( pio1, 0 );
      address = bus >> BUS_CONFIG_shift_address;

      if( (address >= 0xCC00) && (address < 0xD000) )
      {
         led_pixel( address, bus >> BUS_CONFIG_shift_data );
      }
      else if( address == 0xDF04 )
      {
         if( multicore_fifo_wready() )
         {
            multicore_fifo_push_blocking( bus );
         }
      }
   }
}


void led_handler()
{
   static uint8_t brightness;
   static uint8_t colors;
   uint8_t  data;

   for(;;)
   {
      data = multicore_fifo_pop_blocking() >> BUS_CONFIG_shift_data;

      if( data == 0x00 )
      {
         led_flush();
      }
      else if( (data >= 0x80) && (data <= 0x8F) )
      {
         colors = data & 0x0F;
         led_setcolors( colors, brightness );
      }
      else if( (data >= 0x90) && (data <= 0x9F) )
      {
         brightness = data & 0x0F;
         led_setcolors( colors, brightness );
      }
   }
}


int main()
{
   // setup UART
   stdio_init_all();

   // for toying with overclocking
#if 0
   set_sys_clock_khz( 240000, false );
#else
   set_sys_clock_khz( 133000, false );
#endif

   bus_init();
   led_init();
   // switch to RGB palette
   led_setcolors( 0, 2 );
   for( int c0 = 0; c0 < 0x100; ++c0 )
   {
      int c1 = ((c0 & 0x3C) << 2) | ((c0 & 0xC0) >> 4) | (c0 & 0x03);
      int c2 = ((c1 & 0x3C) << 2) | ((c1 & 0xC0) >> 4) | (c1 & 0x03);
      int x0 = (c0 & 0x0F);
      int x1 = (c0 & 0x0F) + 16;
      int y0 = (c0 >> 4);
      int y1 = (c0 >> 4) + 16;
      led_pixel( x0+y0*32, c0 );
      led_pixel( x1+y0*32, c1 );
      led_pixel( x0+y1*32, c2 );
      led_pixel( x1+y1*32, c0 );
   }
   led_flush();
   // switch to C64 palette
   led_setcolors( 6, 2 );

#if 1
   // core1 updates the LEDs
   multicore_launch_core1( led_handler );

   // core0 waits for data
   bus_sniffer();
#else
   for(;;)
   {
      int in = getchar_timeout_us(33);
      if( in != PICO_ERROR_TIMEOUT )
      {
         putchar( in );
         fflush( stdout );
      }
   }
#endif

   // keep the compiler happy
   return 0;
}
