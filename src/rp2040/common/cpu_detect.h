/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPUDETECT_H
#define CPUDETECT_H CPUDETECT_H

#include <stdbool.h>

#include "generic_helper.h"

/*
 * run a CPU detection in a small confined extra environment
 */
cputype_t cpu_detect( bool debug );

/*
 * return raw trace log of last cpu_detect run
 * always terminated with an 0x00000000 entry
 */
uint32_t *cpu_detect_trace();

#endif

