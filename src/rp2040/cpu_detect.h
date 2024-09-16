/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPUDETECT_H
#define CPUDETECT_H CPUDETECT_H

typedef enum {
   CPU_ERROR, CPU_6502, CPU_65C02, CPU_65816, CPU_65CE02, CPU_65SC02, CPU_UNDEF
} cputype_t;

cputype_t cpu_detect();
const char *cputype_name( cputype_t cputype );
unsigned char cpu_detect_raw();

#endif

