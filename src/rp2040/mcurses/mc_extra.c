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


#define BORDER_TOP                  0x2500
#define BORDER_BOTTOM               0x2500
#define BORDER_LEFT                 0x2502
#define BORDER_RIGHT                0x2502
#define BORDER_TOP_LEFT             0x250c
#define BORDER_TOP_RIGHT            0x2510
#define BORDER_BOTTOM_LEFT          0x2514
#define BORDER_BOTTOM_RIGHT         0x2518
#define DOUBLE_BORDER_TOP           0x2550
#define DOUBLE_BORDER_BOTTOM        0x2550
#define DOUBLE_BORDER_LEFT          0x2551
#define DOUBLE_BORDER_RIGHT         0x2551
#define DOUBLE_BORDER_TOP_LEFT      0x2554
#define DOUBLE_BORDER_TOP_RIGHT     0x2557
#define DOUBLE_BORDER_BOTTOM_LEFT   0x255A
#define DOUBLE_BORDER_BOTTOM_RIGHT  0x255D


static struct {
   uint16_t columns;
   uint16_t lines;
} screen = { 0 };


static struct {
   uint16_t column;
   uint16_t line;
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


uint16_t screen_get_lines()
{
   return screen.lines;
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

   /* get old cursor position */
   if( !screen_get_cursor_pos( &sx, &sy ) )
   {
      return false;
   }

   /* set cursor to maximal bottom right */
   screen_set_cursor_pos( 0xFFFE, 0xFFFE );
   if( !screen_get_cursor_pos( &x, &y ) )
   {
      return false;
   }

   screen_set_cursor_pos( sx, sy );

   /* screen_get_cursor_pos() uses 0-offset */
   screen.columns = ++x;
   screen.lines   = ++y;
   if( columns )
   {
      *columns = x;
   }
   if( lines )
   {
      *lines = y;
   }
   if( !x || !y )
   {
      return false;
   }
   return true;
}


#if 0
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
#endif


void screen_save()
{
   screen_get_cursor_pos( &saved.column, &saved.line );
   screen_puts( "\033[?47h" );
}


void screen_restore()
{
   screen_puts( "\033[?47l" );
   screen_set_cursor_pos( saved.column, saved.line );
}


void screen_enable_alternative_buffer()
{
   screen_puts( "\033[?1049h" );
}


void screen_disable_alternative_buffer()
{
   screen_puts( "\033[?1049l" );
}


void mcurses_sorbus_logo( uint16_t line, uint16_t column )
{
   int i;
   uint16_t logo_data[192] = {
   0x2591, 0x2591, 0x2591, 0x2588, 0x2580, 0x2580, 0x2591, 0x2588,
   0x2580, 0x2588, 0x2591, 0x2588, 0x2580, 0x2584, 0x2591, 0x2588,
   0x2580, 0x2584, 0x2591, 0x2588, 0x2591, 0x2588, 0x2591, 0x2588,
   0x2580, 0x2580, 0x2591, 0x2591, 0x2591, 0x2588, 0x2580, 0x2580,
   0x2591, 0x2588, 0x2580, 0x2588, 0x2591, 0x2588, 0x2584, 0x2588,
   0x2591, 0x2588, 0x2580, 0x2588, 0x2591, 0x2588, 0x2591, 0x2588,
   0x2591, 0x2580, 0x2588, 0x2580, 0x2591, 0x2588, 0x2580, 0x2580,
   0x2591, 0x2588, 0x2580, 0x2584, 0x2591, 0x2591, 0x2591, 0x2591,

   0x2591, 0x2591, 0x2591, 0x2580, 0x2580, 0x2588, 0x2591, 0x2588,
   0x2591, 0x2588, 0x2591, 0x2588, 0x2580, 0x2584, 0x2591, 0x2588,
   0x2580, 0x2584, 0x2591, 0x2588, 0x2591, 0x2588, 0x2591, 0x2580,
   0x2580, 0x2588, 0x2591, 0x2591, 0x2591, 0x2588, 0x2591, 0x2591,
   0x2591, 0x2588, 0x2591, 0x2588, 0x2591, 0x2588, 0x2591, 0x2588,
   0x2591, 0x2588, 0x2580, 0x2580, 0x2591, 0x2588, 0x2591, 0x2588,
   0x2591, 0x2591, 0x2588, 0x2591, 0x2591, 0x2588, 0x2580, 0x2580,
   0x2591, 0x2588, 0x2580, 0x2584, 0x2591, 0x2591, 0x2591, 0x2591,

   0x2591, 0x2591, 0x2591, 0x2580, 0x2580, 0x2580, 0x2591, 0x2580,
   0x2580, 0x2580, 0x2591, 0x2580, 0x2591, 0x2580, 0x2591, 0x2580,
   0x2580, 0x2591, 0x2591, 0x2580, 0x2580, 0x2580, 0x2591, 0x2580,
   0x2580, 0x2580, 0x2591, 0x2591, 0x2591, 0x2580, 0x2580, 0x2580,
   0x2591, 0x2580, 0x2580, 0x2580, 0x2591, 0x2580, 0x2591, 0x2580,
   0x2591, 0x2580, 0x2591, 0x2591, 0x2591, 0x2580, 0x2580, 0x2580,
   0x2591, 0x2591, 0x2580, 0x2591, 0x2591, 0x2580, 0x2580, 0x2580,
   0x2591, 0x2580, 0x2591, 0x2580, 0x2591, 0x2591, 0x2591, 0x2591 };

   move( line+0, column );
   for( i = 0x00; i < 0x40; ++i )
   {
      addch( logo_data[i] );
   }
   move( line+1, column );
   for( i = 0x40; i < 0x80; ++i )
   {
      addch( logo_data[i] );
   }
   move( line+2, column );
   for( i = 0x80; i < 0xc0; ++i )
   {
      addch( logo_data[i] );
   }
}


void mcurses_border( bool dframe, uint16_t top, uint16_t left,
                    uint16_t bottom, uint16_t right )
{
   int i;

   move( top, left );
   addch( dframe ? DOUBLE_BORDER_TOP_LEFT : BORDER_TOP_LEFT );
   for( i = left+1; i < right; ++i )
   {
      addch( dframe ? DOUBLE_BORDER_TOP : BORDER_TOP );
   }
   addch( dframe ? DOUBLE_BORDER_TOP_RIGHT : BORDER_TOP_RIGHT );

   for( i = top+1; i < bottom; ++i )
   {
      move( i, left );
      addch( dframe ? DOUBLE_BORDER_LEFT : BORDER_LEFT );
      move( i, right );
      addch( dframe ? DOUBLE_BORDER_RIGHT : BORDER_RIGHT );
   }

   move( bottom, left );
   addch( dframe ? DOUBLE_BORDER_BOTTOM_LEFT : BORDER_BOTTOM_LEFT );
   for( i = left+1; i < right; ++i )
   {
      addch( dframe ? DOUBLE_BORDER_BOTTOM : BORDER_BOTTOM );
   }
   addch( dframe ? DOUBLE_BORDER_BOTTOM_RIGHT : BORDER_BOTTOM_RIGHT );
}


void mcurses_line_horizontal( bool dframe, uint16_t top, uint16_t left,
                              uint16_t right )
{
   uint16_t i;
   move( top, left );
   addch( dframe ? 0x2560 : 0x251c );
   for( i = left+1; i < right; ++i )
   {
      addch( dframe ? 0x2550 : 0x2500 );
   }
   addch( dframe ? 0x2563 : 0x2524 );
}


void mcurses_line_vertical( bool dframe, uint16_t top, uint16_t left,
                            uint16_t bottom )
{
   uint16_t i;
   move( top, left );
   addch( dframe ? 0x2566 : 0x252c );
   for( i = top+1; i < bottom; ++i )
   {
      move( i, left );
      addch( dframe ? 0x2551 : 0x2502 );
   }
   move( bottom, left );
   addch( dframe ? 0x2569 : 0x2534 );
}


void mcurses_textline( uint16_t line, uint16_t left, uint16_t right,
                       const char *text )
{
   const uint8_t *c = (const uint8_t*)text;
   uint16_t i;
   move( line, left );
   for( i = left; i <= right; ++i )
   {
      addch( *c ? *(c++) : ' ' );
   }
}


void mcurses_textsize( uint16_t *lines, uint16_t *columns, const char *text )
{
   uint16_t len   = 0;
   uint16_t width = 0;
   uint16_t rows  = 1;
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

   /* evaluate last line */
   if( len > width )
   {
      width = len;
   }
   /* new empty line caused by newline at end will not get added */
   if( !len )
   {
      --rows;
   }

   /* check if return values */
   if( lines )
   {
      *lines = rows;
   }
   if( columns )
   {
      *columns = width;
   }
}


static void screen_printtext( uint16_t line, uint16_t column,
                              uint16_t columns, const char *text )
{
   const char *c;
   uint16_t col = 0;

   move( line, column );
   for( c = text; *c; ++c )
   {
      if( *c == '\n' )
      {
         ++line;
         while( col++ < columns )
         {
            addch( ' ' );
         }
         col = 0;
         move( line, column );
      }
      else
      {
         if( col++ < columns )
         {
            addch( *c );
         }
      }
   }
   if( *(c-1) == '\n' )
   {
      --line;
   }
   move( line, column );
}


void mcurses_titlebox( bool dframe, uint16_t line, uint16_t column,
                       const char *title, const char *text )
{
   uint16_t rows, width, row = 3;
   const char *c;
   uint16_t i;

   mcurses_textsize( &rows, &width, text );

   if( line == MC_TEXT_CENTER )
   {
      line = ((screen_get_lines() - rows) >> 1) - 2;
   }
   if( column == MC_TEXT_CENTER )
   {
      column = ((screen_get_columns() - width) >> 1) - 1;
   }

   mcurses_border( dframe, line, column, line + rows + 3, column + width + 1 );
   mcurses_line_horizontal( dframe, line+2, column, column + width + 1 );

   move( line+1, column+1 );
   c = title;
   for( i = 0; i < width; ++i )
   {
      if( *c )
      {
         addch( *(c++) );
      }
      else
      {
         addch( ' ' );
      }
   }

   screen_printtext( line + row, column + 1, width, text );
}


void mcurses_textbox( bool dframe, uint16_t line, uint16_t column,
                     const char *text )
{
   uint16_t rows, width, row = 1;

   mcurses_textsize( &rows, &width, text );

   if( line == MC_TEXT_CENTER )
   {
      line = ((screen_get_lines() - rows) >> 1) - 1;
   }
   if( column == MC_TEXT_CENTER )
   {
      column = ((screen_get_columns() - width) >> 1) - 1;
   }

   mcurses_border( dframe, line, column, line + rows + 1, column + width + 1 );
   screen_printtext( line + row, column + 1, width, text );
}


static void hex1ascii( uint8_t val )
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


void mcurses_hexout( uint64_t value, uint8_t digits )
{
   uint8_t outchar = '0' + (value & 0xF);
   if( outchar > '9' )
   {
      outchar += 'A' - '9';
   }
   if( digits > 1 )
   {
      mcurses_hexout( value >> 4, digits - 1 );
   }
   addch( outchar );
}


bool mcurses_get4hex( uint16_t *value )
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
