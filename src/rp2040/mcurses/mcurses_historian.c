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


static bool mcurses_historian_move( int32_t move )
{
   switch( move )
   {
      case LINEVIEW_FIRSTLINE:
         _current = 0;
         break;
      case LINEVIEW_LASTLINE:
         _current = _entries - screen_get_rows() - 2;
         break;
      default:
         _current += move;
         if( (_current < 0) || (_current > (_entries - screen_get_rows() - 2)) )
         {
            _current -= move;
            return false;
         }
         break;
   }
   return true;
}


const char* mcurses_historian_data( int32_t offset )
{
   static char buffer[80] = { 0 };
   switch( offset )
   {
      case LINEVIEW_FIRSTLINE:
         return "steps";
      case LINEVIEW_LASTLINE:
         return "Backtrace Viewer";
   }
   snprintf( &buffer[0], sizeof(buffer)-1,
             "%5d:%s", _current + offset, disass_historian_entry( historian, _current + offset ) );
fprintf( stderr, "%s\n", &buffer[0] );
   return &buffer[0];
}


void mcurses_historian( cputype_t cpu, uint32_t *trace, uint32_t entries, uint32_t start )
{
   lineview_t config;
   config.nextbank   = 0;
   config.move       = mcurses_historian_move;
   config.data       = mcurses_historian_data;
   config.attributes = F_WHITE | B_BLUE;
   config.bank       = 0;
   config.current    = entries - (screen_get_rows() - 2);
   _entries          = entries;
   _current          = 0;

   historian = disass_historian_init( cpu, trace, entries, start );
   lineview( &config );
   disass_historian_done( historian );
}
