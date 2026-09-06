/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"
#include "event_queue.h"

#include <pico/stdio.h>

static const char *_file = "";
static int _line = 0;
static const char *_message = "";
static uint32_t _value = 0;


void internal_error( const char *file, int line,
                     const char *message, uint32_t value )
{
   _file = file;
   _line = line;
   _message = message;
   _value = value;
   bus_stop_cause = BUSMSG_META_INTERROR;
}


int internal_error_info( char *b, size_t bsize )
{
   return snprintf( b, bsize, "%s(%d):\n%s (0x%08x)\n",
                    _file, _line, _message, _value );
}
