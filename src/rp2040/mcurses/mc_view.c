/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"

#include <string.h>

#define FIRST_LINE      1
#define LAST_LINE       (LINES - 1)


static void printline( const char *line, uint8_t charset, int len )
{
   int i;
   uint8_t *l = (uint8_t*)line;
   for( i = 0; i < len; ++i )
   {
      if( l && *l )
      {
         if( *l == 0x7f )
         {
            ++l;
            mcurses_debug_byte( *(l++), charset );
         }
         else
         {
            putcharset( *(l++), charset );
         }
      }
      else
      {
         addch( ' ' );
      }
   }
}


static void printlines( const lineview_t *config, int len )
{
   uint16_t l;

   for( l = 0; l < (LAST_LINE - FIRST_LINE); ++l )
   {
      move( l+FIRST_LINE, 0 );
      printline( config->data( config->d, l ), config->charset, len );
   }
}


static void header_footer( const lineview_t *config )
{
   attrset( config->attributes );
   move( 0, 0 );
   printline( config->data( config->d, MC_LINEVIEW_FIRSTLINE ), 0, COLS );
   move( LAST_LINE+1, 0 );
   printline( config->data( config->d, MC_LINEVIEW_LASTLINE  ), 0, COLS );
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

         if( keyaction == MC_LINEVIEW_REDRAWALL )
         {
            header_footer( config );
         }

         if( keyaction != 0 )
         {
            printlines( config, COLS );
         }
      }

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
         case ' ':
         case KEY_NPAGE:
            step = +page;
            break;
         case KEY_HOME:
            step = MC_LINEVIEW_FIRSTLINE;
            break;
         case KEY_END:
            step = MC_LINEVIEW_LASTLINE;
            break;
         case 'Q':
         case 'q':
            ch = 0x03;  // also allow 'Q' to quit
            break;      // can be overriden by config->keypress
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
            printline( config->data( config->d, 0 ), config->charset, COLS-1 );
            break;
         case +1:
            move( LAST_LINE-1, 0 );
            scroll();
            printline( config->data( config->d, LAST_LINE-FIRST_LINE-1 ), config->charset, COLS-1 );
            break;
         default:
            printlines( config, COLS-1 );
            break;
      }
   }
   while( ch != 0x03 ); // quit with CTRL+C
}
