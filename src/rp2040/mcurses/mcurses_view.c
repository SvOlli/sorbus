/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"

#include <string.h>

#define FIRST_LINE      1
#define LAST_LINE       (LINES - 1)


static void printline( const char *line, int len )
{
   int i;
   for( i = 0; i < len; ++i )
   {
      if( line && *line )
      {
         addch(*(line++));
      }
      else
      {
         addch(' ');
      }
   }
}


static void printlines( const lineview_t *config, int len )
{
   uint16_t l;
   int32_t  lines  = (LAST_LINE - FIRST_LINE);

   for( l = 0; l < lines; ++l )
   {
      move( l+FIRST_LINE, 0 );
      printline( config->data( config->d, l ), len );
   }
}


void lineview( lineview_t *config )
{
   uint8_t        ch    = 0;
   const int32_t  page  = (LAST_LINE - FIRST_LINE) / 2;

   clear();
   setscrreg( FIRST_LINE, LAST_LINE-1 );

   attrset( config->attributes );
   move( 0, 0 );
   printline( config->data( config->d, LINEVIEW_FIRSTLINE ), COLS );
   move( LAST_LINE+1, 0 );
   printline( config->data( config->d, LINEVIEW_LASTLINE  ), COLS );
   attrset( A_NORMAL | F_DEFAULT | B_DEFAULT );

   printlines( config, COLS );

   do
   {
      move( LINES-1, COLS-1 );
      ch = getch();
      switch( ch )
      {
         case KEY_UP:
            if( config->move( config->d, -1 ) )
            {
               move( FIRST_LINE, 0 );
               insertln();
               printline( config->data( config->d, 0 ), COLS-1 );
            }
            break;
         case KEY_DOWN:
            if( config->move( config->d, +1 ) )
            {
               move( LAST_LINE-1, 0 );
               scroll();
               printline( config->data( config->d, LAST_LINE-FIRST_LINE-1 ), COLS-1 );
            }
            break;
         case KEY_PPAGE:
            if( config->move( config->d, -page ) )
            {
               printlines( config, COLS );
            }
            break;
         case KEY_NPAGE:
            if( config->move( config->d, +page ) )
            {
               printlines( config, COLS );
            }
            break;
         case KEY_HOME:
            if( config->move( config->d, LINEVIEW_FIRSTLINE ) )
            {
               printlines( config, COLS );
            }
            break;
         case KEY_END:
            if( config->move( config->d, LINEVIEW_LASTLINE ) )
            {
               printlines( config, COLS );
            }
            break;
         case 0x02:     // CTRL+B
            if( config->nextbank )
            {
               config->nextbank( config->d );
            }
            break;
         default:
            break;
      }
   }
   while( ch != 0x03 ); // quit with CTRL+C
}
