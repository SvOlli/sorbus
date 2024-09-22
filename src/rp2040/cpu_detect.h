/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPUDETECT_H
#define CPUDETECT_H CPUDETECT_H

#include <stdbool.h>

/*
 * all detectable CPU instruction sets
 */
typedef enum {
   CPU_ERROR, CPU_6502, CPU_65C02, CPU_65816, CPU_65CE02, CPU_65SC02, CPU_UNDEF
} cputype_t;

/*
 * run a CPU detection in a small confined extra environment
 * return value might be CPU_ERROR even though CPU is okay, retry suggested
 */
cputype_t cpu_detect( bool debug );

/*
 * convert enum to a string for display purposes
 */
const char *cputype_name( cputype_t cputype );

#endif

