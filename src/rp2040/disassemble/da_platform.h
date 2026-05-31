/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */

#ifndef __DA_PLATFORM_H__
#define __DA_PLATFORM_H__ __DA_PLATFORM_H__

#if PICO_ON_DEVICE

#include "bus.h"
#include "../common/generic_helper.h"

#else

/* host doesn't need tracking heap functions */
#define ht_malloc(s)    malloc(s)
#define ht_calloc(n,s)  calloc(n,s)
#define ht_realloc(p,s) realloc(p,s)
#define ht_free(p)      free(p)

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

#endif

#endif
