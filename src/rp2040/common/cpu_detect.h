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
 * return value might be CPU_ERROR even though CPU is okay, retry suggested
 */
cputype_t cpu_detect( bool debug );

#endif

