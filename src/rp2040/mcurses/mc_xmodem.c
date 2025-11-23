/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"

#include "generic_helper.h"


bool mc_xmodem_upload( poke_t poke )
{
   static uint16_t addr = 0x400;
   int ret;

   clear();
   attrset( MC_ATTRIBUTES_XMODEM );
   mcurses_textline(       0, 0, COLS-1, "?" );
   mcurses_textline( LINES-1, 0, COLS-1, "  XModem Upload" );
   attrset( MC_ATTRIBUTES_DEFAULT );

   move( 2, 0 );
   addstr( "Enter upload address: $" );
   if( !mcurses_get4hex( &addr ) )
   {
      /* entry canceled using Ctrl+C */
      return false;
   }
   if( addr < 0x0004 )
   {
      mcurses_textbox( false, MC_TEXT_CENTER, MC_TEXT_CENTER,
                       "Useful address ranges are:\n"
                       "$0004-$CFFF and $E000-$FFFF"
                      );
      getch();
      return false;
   }

   move( 4, 0 );
   addstr( "Start XModem Transfer now: " );
   ret = xmodem_receive( poke, addr );

   move( 8, 0 );

   if (ret<0)
   {
      addstr( "Failure in reception: " );
      switch( ret )
      {
         case -1:
            addstr( "canceled by remote" );
            break;
         case -2:
            addstr( "sync error" );
            break;
         case -3:
            addstr( "too many retry error" );
            break;
         case -4:
            addstr( "file too large error" );
            break;
         default:
            addstr( "unknown error" );
            break;
      }
   }
   else
   {
      addstr( "Reception successful: $" );
      mcurses_hexout4( addr );
      addstr( "-$" );
      mcurses_hexout4( addr + ret );
   }
   move( 10, 0 );
   addstr( "Press any key to continue " );
   getch();

   return (ret >= 0);
}
