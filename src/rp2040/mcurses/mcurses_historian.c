/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"
#include "../common/disassemble.h"

#include <stdio.h>


static struct mc_historian {
   disass_historian_t historian;
   uint32_t entries;
   uint32_t current;
   uint16_t datalines;
};


static int32_t mcurses_historian_move( void *d, int32_t movelines )
{
   int32_t retval = 0;
   struct mc_historian *mch = (struct mc_historian *)d;

   switch( movelines )
   {
      case LINEVIEW_FIRSTLINE:
         retval = -(mch->current);
         mch->current = 0;
         break;
      case LINEVIEW_LASTLINE:
         retval = (mch->entries - mch->datalines) - mch->current;
         mch->current = mch->entries - mch->datalines;
         break;
      default:
         mch->current += movelines;
         retval = movelines;
         if( (mch->current < 0) || (mch->current > (mch->entries - mch->datalines)) )
         {
            mch->current -= movelines;
            retval = 0;
         }
         break;
   }
   return retval;
}


const char* mcurses_historian_data( void *d, int32_t offset )
{
   struct mc_historian *mch = (struct mc_historian *)d;

   switch( offset )
   {
      case LINEVIEW_FIRSTLINE:
         return "cycle:addr r da flg:C:disassembly";
      case LINEVIEW_LASTLINE:
         return "Backtrace Viewer";
   }

   return disass_historian_entry( mch->historian, mch->current + offset );
}


void mcurses_historian( cputype_t cpu, uint32_t *trace, uint32_t entries, uint32_t start )
{
   lineview_t config;
   struct mc_historian mch;

   mch.historian     = disass_historian_init( cpu, trace, entries, start );
   mch.entries       = entries;
   mch.current       = entries - (screen_get_rows() - 2);
   mch.datalines     = screen_get_rows()-2;

   config.nextbank   = 0;
   config.move       = mcurses_historian_move;
   config.data       = mcurses_historian_data;
   config.d          = (void*)(&mch);
   config.attributes = F_WHITE | B_YELLOW;

   mcurses_historian_move( config.d, LINEVIEW_LASTLINE );
   lineview( &config );
   disass_historian_done( mch.historian );
}
