/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */

#ifndef __DA_MEMORY_H__
#define __DA_MEMORY_H__ __DA_MEMORY_H__

#include "da_base.h"

struct da_memory_s {
   cputype_t      cpu;
   peek_t         peek;
   uint8_t        bank;
   uint16_t       address;
   bool           bytemode;
   bool           m816;
   bool           x816;
   uint16_t       lines;
   uint16_t       *linecache;
};


da_memory_t da_memory_init();
void da_memory_done( da_memory_t d );
void da_memory_linecache( da_memory_t d, uint16_t lines );

da_fullinfo_t da_memory_fullinfo( da_memory_t d, int16_t offset );
void da_memory_next( da_memory_t d, uint16_t steps );
void da_memory_prev( da_memory_t d, uint16_t steps );
uint16_t da_memory_findprev( da_memory_t d, uint16_t address );

da_fullinfo_t da_memory_single( cputype_t cpu, peek_t peek,
                     uint8_t bank, uint16_t address, bool m, bool x );

#endif
