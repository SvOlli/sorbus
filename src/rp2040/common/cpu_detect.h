/**
 * Copyright (c) 2023-2026 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPUDETECT_H
#define CPUDETECT_H CPUDETECT_H

#include <stdbool.h>

#include "generic_helper.h"


/*
 * run a CPU detection in a small confined extra environment
 * sets cputype, not a return value, since it is typically used more than once
 */
extern cputype_t cputype;
void cpu_detect( bool debug );

/*
 * return raw trace log of last cpu_detect run
 * always terminated with an 0x00000000 entry
 */
uint32_t *cpu_detect_trace();
void cpu_detect_trace_free();

#endif

