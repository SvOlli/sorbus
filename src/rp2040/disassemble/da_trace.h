/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */

#ifndef __DA_TRACE_H__
#define __DA_TRACE_H__ __DA_TRACE_H__

#include "da_base.h"


struct da_trace_s {
   cputype_t      cpu;        // cpu traced
   uint32_t       entries;    // number of entries in list below
   da_fullinfo_t  *fullinfo;  // list of annotated trace
   const uint32_t *opcodes;   // pointer to opcode table for cpu
#if 0
   /* unused so far */
   da_cpu_flag_t  flag_n;     // $80 negaitve
   da_cpu_flag_t  flag_v;     // $40 overflow
                              // $20 -
   da_cpu_flag_t  flag_i;     // $10 brk
   da_cpu_flag_t  flag_d;     // $08 enable bcd mode
   da_cpu_flag_t  flag_b;     // $04 irq disable
   da_cpu_flag_t  flag_z;     // $02 zero
#endif
   da_cpu_flag_t  flag_c;     // $01 carry
   // 65816 only below
   da_cpu_flag_t  flag_e;     // $01 set/read with XCE
   da_cpu_flag_t  flag_m;     // $20 native: shared with unused flag
   da_cpu_flag_t  flag_x;     // $10 native: shared with brk
};
typedef struct da_trace_s *da_trace_t;

/*
 * create trace object and setup from ringbuffer
 */
da_trace_t da_trace_init( cputype_t cpu, uint32_t *ringbuffer,
                          uint32_t ringbuffersize, uint32_t start );
/*
 * destroy object created with function above
 */
void da_trace_done( da_trace_t d );

/*
 * start cyclecounting based disassembly at position
 */
bool da_cc_start( da_trace_t d, uint32_t start );

#endif
