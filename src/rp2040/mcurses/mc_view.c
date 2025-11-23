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

   for( l = 0; l < (LAST_LINE - FIRST_LINE); ++l )
   {
      move( l+FIRST_LINE, 0 );
      printline( config->data( config->d, l ), len );
   }
}


static void header_footer( const lineview_t *config )
{
   attrset( config->attributes );
   move( 0, 0 );
   printline( config->data( config->d, LINEVIEW_FIRSTLINE ), COLS );
   move( LAST_LINE+1, 0 );
   printline( config->data( config->d, LINEVIEW_LASTLINE  ), COLS );
   attrset( A_NORMAL | F_DEFAULT | B_DEFAULT );
}


void lineview( lineview_t *config )
{
   uint16_t       line, column;
   uint8_t        ch    = 0;
   const int32_t  page  = LAST_LINE - FIRST_LINE - 1;
   int32_t        step;

   clear();
   setscrreg( FIRST_LINE, LAST_LINE-1 );

   header_footer( config );

   printlines( config, COLS );

   do
   {
      line = LINES-1;
      column = COLS-1;
      if( config->cpos )
      {
         config->cpos( config->d, &line, &column );
      }
      move( line, column );
      ch = getch();

      if( config->keypress )
      {
         int32_t keyaction = config->keypress( config->d, &ch );
         
         if( keyaction == LINEVIEW_LASTLINE )
         {
            header_footer( config );
         }

         if( keyaction )
         {
            printlines( config, COLS );
         }
      }

#if 0
      {
         char hex[3] = { 0 };
         move( LINES-1, COLS-3 );
         snprintf( hex, 3, "%02x", ch );
         addstr( hex );
      }
#endif
      step = 0;
      switch( ch )
      {
         case KEY_UP:
            step = -1;
            break;
         case KEY_DOWN:
            step = +1;
            break;
         case KEY_PPAGE:
            step = -page;
            break;
         case KEY_NPAGE:
            step = +page;
            break;
         case KEY_HOME:
            step = LINEVIEW_FIRSTLINE;
            break;
         case KEY_END:
            step = LINEVIEW_LASTLINE;
            break;
         default:
            break;
      }
      step = config->move( config->d, step );
      switch( step )
      {
         case 0:
            break;
         case -1:
            move( FIRST_LINE, 0 );
            insertln();
            printline( config->data( config->d, 0 ), COLS-1 );
            break;
         case +1:
            move( LAST_LINE-1, 0 );
            scroll();
            printline( config->data( config->d, LAST_LINE-FIRST_LINE-1 ), COLS-1 );
            break;
         default:
            printlines( config, COLS-1 );
            break;
      }
   }
   while( ch != 0x03 ); // quit with CTRL+C
}
