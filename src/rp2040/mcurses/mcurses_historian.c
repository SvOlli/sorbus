/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"
#include "../common/disassemble.h"

#include <stdio.h>


static disass_historian_t historian;
static uint32_t _entries;
static uint32_t _current;
static uint16_t _datalines;


static int32_t mcurses_historian_move( int32_t movelines )
{
   int32_t retval = 0;
   switch( movelines )
   {
      case LINEVIEW_FIRSTLINE:
         retval = -_current;
         _current = 0;
         break;
      case LINEVIEW_LASTLINE:
         retval = (_entries - _datalines) - _current;
         _current = _entries - _datalines;
         break;
      default:
         _current += movelines;
         retval = movelines;
         if( (_current < 0) || (_current > (_entries - _datalines)) )
         {
            _current -= movelines;
            retval = 0;
         }
         break;
   }
   return retval;
}


const char* mcurses_historian_data( int32_t offset )
{
   switch( offset )
   {
      case LINEVIEW_FIRSTLINE:
         return "cycle:addr r da flg:C:disassembly";
      case LINEVIEW_LASTLINE:
         return "Backtrace Viewer";
   }
   return disass_historian_entry( historian, _current + offset );
}


void mcurses_historian( cputype_t cpu, uint32_t *trace, uint32_t entries, uint32_t start )
{
   lineview_t config;
   config.nextbank   = 0;
   config.move       = mcurses_historian_move;
   config.data       = mcurses_historian_data;
   config.attributes = F_WHITE | B_YELLOW;
   config.bank       = 0;
   config.current    = entries - (screen_get_rows() - 2);
   _entries          = entries;
   _current          = 0;
   _datalines        = screen_get_rows()-2;

   historian = disass_historian_init( cpu, trace, entries, start );
   mcurses_historian_move( LINEVIEW_LASTLINE );
   lineview( &config );
   disass_historian_done( historian );
}
