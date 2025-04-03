/**
 * Copyright (c) 2025 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a 32x32 pixel framebuffer
 * for the Sorbus Computer
 */

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <hardware/clocks.h>

/* bus config is shared, because hardware is shared */
#include "common/bus.h"
#include "fb32x32.h"
#include <fb32x32bus.pio.h>

#define FB32X32_BUS_PIO pio1
#define FB32X32_BUS_SM  0

bi_decl(bi_program_url("https://xayax.net/sorbus/"))

bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_address, "A0-A15"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_data,    "D0-D7"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_rw,      "R/!W"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_clock,   "CLK"));

uint8_t mem_cache[0x12000];


void bus_init()
{
   uint pin;
   const float freq = 1150000;
   uint offset = pio_add_program( pio1, &bus_wait_program );

   // setup pins
   for( pin = 0; pin < INPINS+2; ++pin )
   {
      pio_gpio_init( FB32X32_BUS_PIO, pin );
   }

   // listen on the pins 0-25 of the bus
   pio_sm_config c = bus_wait_program_get_default_config( offset );
   sm_config_set_in_pin_base( &c, STARTPIN );
   sm_config_set_in_pin_count( &c, INPINS );
   pio_sm_set_consecutive_pindirs( FB32X32_BUS_PIO, FB32X32_BUS_SM, STARTPIN, INPINS+2, false );

   sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_RX );
   sm_config_set_in_shift( &c, false, true, INPINS ); // address+data bus

   // running the PIO state machine 20 times as fast as expected CPU freq
   float div = clock_get_hz( clk_sys ) / (freq * 20);
   sm_config_set_clkdiv( &c, div );

   pio_sm_init( FB32X32_BUS_PIO, FB32X32_BUS_SM, offset, &c );
   pio_sm_set_enabled( FB32X32_BUS_PIO, FB32X32_BUS_SM, true );
}


void bus_loop()
{
   uint32_t bus;
   uint16_t address;

   for(;;)
   {
      /* wait in tight loop for a write to bus to check address and data */
      bus = pio_sm_get_blocking( FB32X32_BUS_PIO, FB32X32_BUS_SM );
      address = bus >> BUS_CONFIG_shift_address;

      // cache every write access
      mem_cache[address] = bus >> BUS_CONFIG_shift_data;

      // handle control registers
      if( (address >> 8) == 0xD3 )
      {
#if 1
         // letting it block would fill up the pio cue before overflow
         multicore_fifo_push_blocking( bus );
#else
         if( multicore_fifo_wready() )
         {
            multicore_fifo_push_blocking( bus );
         }
#endif
      }
   }
}

