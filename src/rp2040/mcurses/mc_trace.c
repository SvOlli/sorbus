/**
 * Copyright (c) 2026 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"
#include "da_trace.h"

#include <stdio.h>
#include <string.h>

struct mc_trace_s {
   da_trace_t trace;
   uint32_t entries;
   uint32_t current;
   uint16_t datalines;
   uint8_t  center;
};


static int32_t mcurses_trace_move( void *d, int32_t movelines )
{
   int32_t retval = 0;
   struct mc_trace_s *mch = (struct mc_trace_s *)d;

   if( movelines == MC_LINEVIEW_FIRSTLINE )
   {
      movelines = -(mch->entries);
   }
   else if( movelines == MC_LINEVIEW_LASTLINE )
   {
      movelines = mch->entries;
   }

   if( (int32_t)(mch->current + movelines) < 0 )
   {
      retval = -(mch->current);
      mch->current = 0;
   }
   else if( (mch->current + movelines) > (mch->entries - mch->datalines) )
   {
      retval = (mch->entries - mch->datalines) - mch->current;
      mch->current = mch->entries - mch->datalines;
   }
   else
   {
      retval = movelines;
      mch->current += movelines;
   }

   return retval;
}


const char* mcurses_trace_data( void *d, int32_t offset )
{
   // offset is typicall 0 - #lines-2
   struct mc_trace_s   *mch = (struct mc_trace_s *)d;
   da_trace_t           dah = mch->trace;
   uint32_t             entry = mch->current + offset;
   static char          text[128];
   int                  pos = 0;

   memset( &text[0], 0, sizeof(text) );

   switch( offset )
   {
      case MC_LINEVIEW_FIRSTLINE:
         return "  cycle:addr r da flags  :C:disassembly";
      case MC_LINEVIEW_LASTLINE:
         return "  Backtrace Viewer  (Ctrl+C to leave)";
      default:
         break;
   }
   pos += snprintf( text+pos, sizeof(text)-1-pos,
                    "  %5d:", entry );
   da_snf_fullinfo( text+pos, sizeof(text)-1-pos, dah->cpu, "a w d RNISn:e:EcC y",
                    dah->fullinfo[entry], DA_FLAG_NONE );
   return &text[0];
}


int32_t mcurses_trace_keypress( void *d, uint8_t *ch )
{
   int32_t              retval = 0;
   struct mc_trace_s   *mch = (struct mc_trace_s *)d;
   da_trace_t           dah = mch->trace;

   switch( (*ch) )
   {
      case 'S':
      case 's':
      case 'D':
      case 'd':
         da_cc_start( dah, mch->current );
         retval = MC_LINEVIEW_REDRAWALL;
         break;
      default:
         break;
   }
   return retval;
}


void mcurses_trace( cputype_t cpu, uint32_t *trace, uint32_t entries, uint32_t start )
{
   lineview_t config       = { 0 };
   struct mc_trace_s mch   = { 0 };

   mch.trace         = da_trace_init( cpu, trace, entries, start );
   mch.entries       = mch.trace->entries;
   mch.current       = 0;
   mch.datalines     = screen_get_lines()-2;
   mch.center        = (mch.datalines-1) >> 1;

   config.data       = mcurses_trace_data;
   config.move       = mcurses_trace_move;
   config.cpos       = 0;
   config.keypress   = mcurses_trace_keypress;
   config.d          = (void*)(&mch);
   config.attributes = MC_ATTRIBUTES_BACKTRACE;
   config.charset    = 0;

   /* we're running a NULL-terminated list instead ringbuffer */
   if( !entries )
   {
      /* remove the NULL from output */
      --(mch.entries);
      /* assume that it starts at a valid point */
      da_cc_start( mch.trace, 0 );
   }

   mcurses_trace_move( config.d, MC_LINEVIEW_FIRSTLINE );
   lineview( &config );
   da_trace_done( mch.trace );
}
