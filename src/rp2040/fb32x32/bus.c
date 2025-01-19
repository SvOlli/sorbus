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

#include "common/bus.h"
#include "fb32x32.h"
#include "bus.pio.h"

bi_decl(bi_program_url("https://xayax.net/sorbus/"))

bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_address, "A0-A15"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_data,    "D0-D7"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_rw,      "R/!W"));
bi_decl(bi_pin_mask_with_name(BUS_CONFIG_mask_clock,   "CLK"));

uint8_t mem_cache[0x12000];

void bus_init()
{
   const float freq = 1150000;
   uint offset = pio_add_program( pio1, &bus_wait_program );
   bus_wait_program_init( pio1, 0, offset, freq );
}


void bus_loop()
{
   uint32_t bus;
   uint16_t address;

   for(;;)
   {
      /* wait in tight loop for a write to bus to check address and data */
      bus = pio_sm_get_blocking( pio1, 0 );
      address = bus >> BUS_CONFIG_shift_address;

      // cache every write access
      mem_cache[address] = bus >> BUS_CONFIG_shift_data;

      // handle control registers
      if( (address >> 8) == 0xD3 )
      {
         if( multicore_fifo_wready() )
         {
            multicore_fifo_push_blocking( bus );
         }
      }
   }
}

