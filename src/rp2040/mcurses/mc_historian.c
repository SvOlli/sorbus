/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"
#include "../common/disassemble.h"


struct mc_historian {
   disass_fulltrace_t historian;
   uint32_t entries;
   uint32_t current;
   uint16_t datalines;
};


static int32_t mcurses_historian_move( void *d, int32_t movelines )
{
   int32_t retval = 0;
   struct mc_historian *mch = (struct mc_historian *)d;

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


const char* mcurses_historian_data( void *d, int32_t offset )
{
   struct mc_historian *mch = (struct mc_historian *)d;
   disass_fulltrace_t   dah = (disass_fulltrace_t)(mch->historian);

   switch( offset )
   {
      case MC_LINEVIEW_FIRSTLINE:
         return "cycle:addr r da flg:C:disassembly";
      case MC_LINEVIEW_LASTLINE:
         return "  Backtrace Viewer  (Ctrl+C to leave)";
      default:
         break;
   }

   return disass_fulltrace_entry( dah, mch->current + offset );
}


int32_t mcurses_historian_keypress( void *d, uint8_t *ch )
{
   int32_t              retval = 0;
   struct mc_historian *mch = (struct mc_historian *)d;
   disass_fulltrace_t   dah = (disass_fulltrace_t)(mch->historian);

   switch( (*ch) )
   {
      case 'S':
         (void)disassemble_beancounter_single( dah, mch->current + BOUNDSBUFFER );
         retval = MC_LINEVIEW_REDRAWALL;
         break;
      case 's':
         disassemble_beancounter( dah, mch->current + BOUNDSBUFFER );
         retval = MC_LINEVIEW_REDRAWALL;
         break;
      default:
         break;
   }
   return retval;
}


void mcurses_historian( cputype_t cpu, uint32_t *trace, uint32_t entries, uint32_t start )
{
   lineview_t config       = { 0 };
   struct mc_historian mch = { 0 };

   mch.historian     = disass_fulltrace_init( cpu, trace, entries, start );
   mch.entries       = entries;
   mch.current       = entries - (screen_get_lines() - 2);
   mch.datalines     = screen_get_lines()-2;

   config.data       = mcurses_historian_data;
   config.move       = mcurses_historian_move;
   config.cpos       = 0;
   config.keypress   = mcurses_historian_keypress;
   config.d          = (void*)(&mch);
   config.attributes = MC_ATTRIBUTES_BACKTRACE;
   config.charset    = 0;

   disass_historian_assumptions( mch.historian );
   mcurses_historian_move( config.d, MC_LINEVIEW_LASTLINE );
   disass_show( DISASS_SHOW_NOTHING );
   lineview( &config );
   disass_fulltrace_done( mch.historian );
}
