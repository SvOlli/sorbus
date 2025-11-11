/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses_sorbus.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if 0
static const char save_sequence[] = "\33[?1049h\33[22;0;0t\33[1;24r\33[4l\33(B\33[m\33[39m\33[49m\33[1;24r\33[H\33[2J\33[1;1H";
static const char restore_sequence[] = "\33[24;1H\33(B\33[m\33[39;49m\r\33[K\r\33[?1049l\33[23;0;0t";
#endif

static uint16_t screen_columns = 0;
static uint16_t screen_rows = 0;


uint16_t screen_get_columns()
{
   return screen_columns;
}


uint16_t screen_get_rows()
{
   return screen_rows;
}


static bool screen_get_cursor_pos( uint16_t *column, uint16_t *line )
{
   char buffer[32] = { 0 };
   int c = 0, i = 0, x = 0, y = 0;

   puts( "\33[6n" );
   /* returns: \33[yyy;xxxR */
   while( c != 'R' )
   {
      c = getchar();
      buffer[i++] = c;
      if( c == '[' )
      {
         y = i;
      }
      if( c == ';' )
      {
         x = i;
      }
   }

   if( column )
   {
      *column = strtol( &buffer[x], 0, 10 ) - 1;
   }
   if( line )
   {
      *line = strtol( &buffer[y], 0, 10 ) - 1;
   }
   return true;
}


static void screen_set_cursor_pos( uint16_t column, uint16_t line )
{
   printf( "\33[%d;%dH", line+1, column+1 );
}


bool screen_get_size( uint16_t *columns, uint16_t *lines )
{
   /* mechanism adopted from src/65c02/jam/rom/vt100.s:
    * 1) save current cursor position
    * 2) set position to huge bottom right
    * 3) read back position to get screen size
    * 4) restore cursor position
    */

   uint16_t i=32, x=i, y=i, sx, sy;

   if( !screen_get_cursor_pos( &sx, &sy ) )
   {
      return false;
   }
   do
   {
      i <<= 1;
      if( !i ) /* bit shifted out of range */
      {
         /* consider a screen of 32768x32768 end of line */
         screen_columns = 0xFFFF;
         screen_rows    = 0xFFFF;
         return false;
      }
      screen_set_cursor_pos( i, i );
      if( !screen_get_cursor_pos( &x, &y ) )
      {
         return false;
      }
   }
   while( (x == i) || (y == i) );
   screen_set_cursor_pos( sx, sy );

   screen_columns = (x+1);
   screen_rows    = (y+1);
   if( columns )
   {
      *columns = x+1;
   }
   if( lines )
   {
      *lines = y+1;
   }
   return true;
}


void screen_save( uint16_t lines )
{
   /* this is a very evil hack:
    * straced the output from most to save screen for 80x24 and 81x25
    * adjusted output with parameters where things changed
    * please give me something better, but works for now */
   printf( "\033[?1049h"    // enable the alternative buffer
           "\033[22;0;0t"
           "\033[1;%dr"
           "\033[4l"
           "\033(B"
           "\033[m"         // set default color/graphics mode ?
           "\033[39m"       // default foreground color
           "\033[49m"       // default background color
           "\033[1;%dr"
           "\033[H"         // move cursor home
           "\033[2J"
           "\033[1;1H", lines, lines );
}


void screen_restore( uint16_t lines )
{
   /* this is a very evil hack:
    * straced the output from most to save screen for 80x24 and 81x25
    * adjusted output with parameters where things changed
    * please give me something better, but works for now */
   printf( "\033[%d;1H"
           "\033(B"
           "\033[m"         // set default color/graphics mode ?
           "\033[39;49m\r"  // default foreground/background color
           "\033[K\r"
           "\033[?1049l"    // disable the alternative buffer
           "\033[23;0;0t", lines );
}
