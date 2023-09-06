/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pico/stdlib.h"

typedef struct {
   uint32_t mask_address;  // input:  up to 16 contiguous bits
   uint32_t mask_data;     // both:   8 contiguous bits
   uint32_t mask_rw;       // input:  1 bit
   uint32_t mask_clock;    // output: 1 bit
   uint32_t mask_rdy;      // output: 1 bit (could be also input in the future)
   uint32_t mask_irq;      // output: 1 bit (could be also input in the future)
   uint32_t mask_nmi;      // output: 1 bit (could be also input in the future)
   uint32_t mask_reset;    // output: 1 bit (could be also input in the future)
   uint32_t mask_input;    // convenience
   uint32_t mask_output;   // convenience
   uint     shift_data;
   uint     shift_address;
} bus_config_t;

#define BUSCONFIG_SHIFT_DATA 16
#define BUSCONFIG_MASK_DATA 0x00FF0000L
#define BUSCONFIG_SHIFT_ADDRESS 0
#define BUSCONFIG_MASK_ADDRESS 0x0000FFFFL
#define BUSCONFIG_MASK_CLOCK 0x02000000L
#define BUSCONFIG_MASK_RW 0x01000000L
#define BUSCONFIG_MASK_RDY 0x04000000L
#define BUSCONFIG_MASK_IRQ 0x08000000L
#define BUSCONFIG_MASK_NMI 0x10000000L
#define BUSCONFIG_MASK_RESET 0x20000000L


typedef struct {
   uint32_t bus_state;     // cache of GPIOs
   uint32_t reset_cycles;  //
   uint32_t nmi_cycles;    //
}
bus_state_t;

extern const bus_config_t bus_config;

void bus_init();
void bus_reset();

static inline void bus_clock_set( bool clk )
{
   if( clk )
   {
      gpio_set_mask( bus_config.mask_clock );
   }
   else
   {
      gpio_clr_mask( bus_config.mask_clock );
   }
}
