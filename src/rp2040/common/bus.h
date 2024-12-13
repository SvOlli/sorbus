/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BUS_H
#define BUS_H BUS_H

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

/* alternative rp2040_purple notations for faster access (gains ~7%) */
#define BUS_CONFIG_mask_address  (0x0000FFFF)
#define BUS_CONFIG_mask_data     (0x00FF0000)
#define BUS_CONFIG_mask_rw       (0x01000000)
#define BUS_CONFIG_mask_clock    (0x02000000)
#define BUS_CONFIG_mask_rdy      (0x04000000)
#define BUS_CONFIG_mask_irq      (0x08000000)
#define BUS_CONFIG_mask_nmi      (0x10000000)
#define BUS_CONFIG_mask_reset    (0x20000000)
#define BUS_CONFIG_mask_input    (0x01FFFFFF)
#define BUS_CONFIG_mask_output   (0x3EFF0000)
#define BUS_CONFIG_shift_data    (16)
#define BUS_CONFIG_shift_address (0)

extern const bus_config_t bus_config;

void bus_init();

#endif
