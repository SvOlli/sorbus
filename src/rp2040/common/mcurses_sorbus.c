/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if 0
static const char save_sequence[] = "\33[?1049h\33[22;0;0t\33[1;24r\33[4l\33(B\33[m\33[39m\33[49m\33[1;24r\33[H\33[2J\33[1;1H";
static const char restore_sequence[] = "\33[24;1H\33(B\33[m\33[39;49m\r\33[K\r\33[?1049l\33[23;0;0t";
#endif

static struct {
   uint16_t columns;
   uint16_t rows;
} screen = { 0 };

static struct {
   uint16_t column;
   uint16_t row;
} saved = { 0 };


void screen_puts( const char *s )
{
   while( *s )
   {
      mcurses_phyio_putc( *(s++) );
   }
}


uint16_t screen_get_columns()
{
   return screen.columns;
}


uint16_t screen_get_rows()
{
   return screen.rows;
}


static bool screen_get_cursor_pos( uint16_t *column, uint16_t *line )
{
   char buffer[32] = { 0 };
   int c = 0, i = 0, x = 0, y = 0;

   printf( "\33[6n" );
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


bool screen_get_size( uint16_t *lines, uint16_t *columns )
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
         screen.columns = 0xFFFF;
         screen.rows    = 0xFFFF;
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

   screen.columns = (x+1);
   screen.rows    = (y+1);
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


void _screen_save( uint16_t lines )
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


void _screen_restore( uint16_t lines )
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


void screen_save()
{
   screen_get_cursor_pos( &saved.column, &saved.row );
   screen_puts( "\033[?47h" );
}


void screen_restore()
{
   screen_puts( "\033[?47l" );
   screen_set_cursor_pos( saved.column, saved.row );
}


void screen_enable_alternative_buffer()
{
   screen_puts( "\033[?1049h" );
}


void screen_disable_alternative_buffer()
{
   screen_puts( "\033[?1049l" );
}


void screen_border( uint16_t top, uint16_t left,
                    uint16_t bottom, uint16_t right )
{
   int i;

   move( top, left );
   addch( 0x8c );
   for( i = left+1; i < right; ++i )
   {
      addch( 0x91 );
   }
   addch( 0x8b );

   for( i = top+1; i < bottom; ++i )
   {
      move( i, left );
      addch( 0x98 );
      move( i, right );
      addch( 0x98 );
   }

   move( bottom, left );
   addch( 0x8d );
   for( i = left+1; i < right; ++i )
   {
      addch( 0x91 );
   }
   addch( 0x8a );
}


void screen_textbox( uint16_t line, uint16_t column, const char *text )
{
   int rows=1, row=1, width=0, len=0;
   const char *c;

   /* count rows and get max line size */
   for( c = text; *c; ++c )
   {
      if( *c == '\n' )
      {
         if( len > width )
         {
            width = len;
         }
         len = 0;
         ++rows;
      }
      else
      {
         ++len;
      }
   }
   if( !len )
   {
      --row;
   }

   screen_border( line, column, line + rows + 1, column + width + 1 );
   move( line + row, column + 1 );
   for( c = text; *c; ++c )
   {
      if( *c == '\n' )
      {
         ++row;
         move( line + row, column + 1 );
      }
      else
      {
         addch( *c );
      }
   }
}


static uint8_t hex1ascii( uint8_t val )
{
   uint8_t ch;
   val &= 0x0F;

   if (val <= 9)
   {
      ch = val + '0';
   }
   else
   {
      ch = val - 10 + 'A';
   }
   addch( ch );
}


bool screen_get4hex( uint16_t *value )
{
   uint16_t x, y;
   uint16_t v = *value;
   int8_t   pos = 0;
   uint8_t  digits[4];
   uint8_t  end = 0;
   uint8_t  ch;

   getyx( y, x );

   for( pos = 3; pos >= 0; --pos )
   {
      move( y, x + pos );
      digits[pos] = v & 0xF;
      hex1ascii( v & 0xF );
      v >>= 4;
   }

   pos = 0;
   while( !end )
   {
      move (y, x + pos);
      ch = getch();
      switch( ch )
      {
         case KEY_BACKSPACE:
         case KEY_LEFT:
            if (pos > 0)
            {
               pos--;
            }
            break;
         case KEY_RIGHT:
            if (pos < 4)
            {
               pos++;
            }
            break;
         case KEY_HOME:
            pos = 0;
            break;
         case KEY_END:
            pos = 4;
            break;
         case KEY_CR:
            end = 1;
            break;
         case KEY_ESCAPE:
         case 0x03:  // Ctrl+C
            end = 2;
            break;
         default:
            if (pos < 4)
            {
               int8_t hd = -1;
               if( (ch >= 'a') && (ch <= 'f') )
               {
                  ch &= ~0x20;
               }
               if( (ch >= 'A') && (ch <= 'F') )
               {
                  hd = 10 + ch - 'A';
               }
               else if( (ch >= '0') && (ch <= '9') )
               {
                  hd = ch - '0';
               }

               if( hd >= 0 )
               {
                  digits[pos] = hd;
                  pos++;
                  addch( ch );
               }
            }
      }
   }

   for( v = 0, pos = 0; pos < 4; ++pos )
   {
      v <<= 4;
      v |= digits[pos];
   }
   if( end == 1 )
   {
      *value = v;
   }
   return end == 1;
}


#if 0
void screen_table( uint16_t line, uint16_t column, const char *cells[] )
{
   int i, rows, max_t = 0, max_d = 0;

   for( rows = 0; cells[rows*2]; ++rows )
   {
      if( !cells[rows*2+1] )
      {
         break;
      }
   }

   for( i = 0; i < rows; ++i )
   {
      int len_t = 0, len_d = 0;
      len_t = strlen(cells[i*2+0]);
      len_d = strlen(cells[i*2+1]);
      if( len_t > max_t )
      {
         max_t = len_t;
      }
      if( len_d > max_d )
      {
         max_d = len_d;
      }
   }

   screen_border( line, column, line + rows + 1, column + max_t + max_d + 2 );
   for( i = 0; i < rows; ++i )
   {
      move( line + i + 1, column + 1 );
      addstr( cells[i*2+0] );
      move( line + i + 1, column + max_t + 2 );
      addstr( cells[i*2+1] );
   }
}
#endif
