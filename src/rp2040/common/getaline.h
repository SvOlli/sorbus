/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GETALINE_H
#define GETALINE_H GETALINE_H

void getaline_init();
void getaline_prompt( const char *prompt );
void getaline_fatal( const char *fmt, ... );
const char *getaline();

#endif
