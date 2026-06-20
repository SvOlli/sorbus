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

#endif

