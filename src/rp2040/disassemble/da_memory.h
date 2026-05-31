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


typedef enum {
   TYPE_UNCHECKED = 0,
   TYPE_GUESSED_DATA,
   TYPE_GUESSED_CODE,
   TYPE_MANUAL_DATA,
   TYPE_MANUAL_CODE
} da_data_type_t;


struct da_memory_s {
   cputype_t      cpu;
   const uint32_t *opcodes;
   peek_t         peek;
   uint8_t        bank;
   uint16_t       address;
   bool           bytemode;
   bool           m;
   bool           x;
   da_data_type_t type[0x10000];
};
typedef struct da_memory_s *da_memory_t;


da_memory_t da_memory_init();
void da_memory_done( da_memory_t d );

void da_memory_set_cpu( da_memory_t d, cputype_t cpu );
void da_memoty_set_bytemode( da_memory_t d, bool enable );
void da_memory_set_mx816( da_memory_t d, bool m, bool x );
void da_memory_set_address( da_memory_t d, uint8_t bank, uint16_t address );
void da_memory_set_data( da_memory_t d, da_data_type_t t );
uint16_t da_memory_next( da_memory_t d );
uint16_t da_memory_prev( da_memory_t d );
da_fullinfo_t da_memory_fullinfo( da_memory_t d, uint16_t address );

#endif

