/*
 * ----------------------------------------------------------------------------
 * @file hexedit.c - full screen hex editor
 *
 * Copyright (c) 2014 Frank Meyer - frank(at)uclock.de
 *
 * Revision History:
 * V1.0 2015 xx xx Frank Meyer, original version
 * V1.1 2017 01 14 ChrisMicro, converted to Arduino example
 * V1.2 2019 01 22 ChrisMicro, memory access functions can now be set
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Possible keys:
 *  LEFT              back one byte
 *  RIGHT             forward one byte
 *  DOWN              one line down
 *  UP                one line up
 *  TAB               toggle between hex and ascii columns
 *  any other         input as hex or ascii value
 *  2 x ESCAPE        exit
 * ----------------------------------------------------------------------------
 * This version has been heavily modified for use with the Sorbus Computer by
 * Sven Oliver Moll. It is intended to work only with the Raspberry Pi
 * PICO-SDK and test cases on the Linux development host.
 * ----------------------------------------------------------------------------
 */

#include "mcurses.h"

#define FIRST_LINE      1
#define LAST_LINE       (LINES - 1)

#define BYTES_PER_ROW   16
#define FIRST_HEX_COL   7
#define LAST_BYTE_COL   (FIRST_HEX_COL + 3 * BYTES_PER_ROW)
#define FIRST_ASCII_COL (LAST_BYTE_COL + 2)
#define IS_PRINT(ch)    (((ch) >= 32 &&( ch ) < 0x7F) || ((ch) >= 0xA0))

#define MODE_HEX        0
#define MODE_ASCII      1

#define MOVE_NONE       0
#define MOVE_RIGHT      1
#define MOVE_LEFT       2
#define MOVE_DOWN       3
#define MOVE_UP         4
#define MOVE_PAGEDOWN   5
#define MOVE_PAGEUP     6

#define IS_HEX(ch)     (((ch) >= 'A' &&( ch ) <= 'F') || ((ch) >= 'a' &&( ch ) <= 'f') || ((ch) >= '0' &&( ch ) <= '9'))


uint8_t hexedit_peek( mc_hexedit_t *config, uint16_t address )
{
   return config->peek( config->bank, address );
}


void hexedit_poke( mc_hexedit_t *config, uint16_t address, uint8_t data )
{
   config->poke( config->bank, address, data );
}


uint8_t chartohex( uint8_t ch )
{
   uint8_t val;

   if (ch >= 'A' && ch <= 'F')
   {
      val = (ch - 'A') + 10;
   }
   else if (ch >= 'a' && ch <= 'f')
   {
      val = (ch - 'a') + 10;
   }
   else
   {
      val = (ch - '0');
   }
   mcurses_hexout( val, 1 );    // print value in hex
   return val;
}

void print_hex_line( mc_hexedit_t *config, uint8_t line, uint16_t off )
{
   uint8_t       col;
   uint8_t       ch;

   move( line, 0 );
   mcurses_hexout4( off );
   addstr("   ");

   move( line, FIRST_HEX_COL );
   for (col = 0; col < BYTES_PER_ROW; col++)
   {
      mcurses_hexout2( hexedit_peek( config, off ) );
      addch( ' ' );
      off++;
   }

   off -= BYTES_PER_ROW;

   move( line, FIRST_ASCII_COL );

   for (col = 0; col < BYTES_PER_ROW; col++)
   {
      ch = hexedit_peek( config, off );

      mcurses_debug_byte( ch, config->charset );
      off++;
   }
}


static uint16_t fix_topleft( uint16_t topleft, uint16_t address )
{
   /* adjust top left if out of range */
   uint16_t tline = topleft / BYTES_PER_ROW;
   uint16_t aline = address / BYTES_PER_ROW;
   uint8_t line;

   for( line = 0; line <= (LAST_LINE-FIRST_LINE-1); ++line )
   {
      if( (tline + line) == aline )
      {
         /* line inside area, everything can stay */
         return topleft;
      }
   }

   address -= address % BYTES_PER_ROW;
   if( aline < tline )
   {
      /* address is smaller than topline -> address it on top */
      return address;
   }
   else
   {
      /* address is on bottom */
      return address - (LAST_LINE - FIRST_LINE - 1) * BYTES_PER_ROW;
   }
}


static uint16_t print_hex_page( mc_hexedit_t *config, uint16_t topleft, uint16_t address )
{
   uint8_t     line;
   uint16_t    off;

   topleft = fix_topleft( topleft, address );
   off = topleft;
   for (line = FIRST_LINE; line < LAST_LINE; line++)
   {
      print_hex_line( config, line, off );
      off += BYTES_PER_ROW;
   }

   return topleft;
}


/*-----------------------------------------------------------------------------
 * hexdit: hex editor
 *-----------------------------------------------------------------------------
 */
void hexedit( mc_hexedit_t *config )
{
   uint8_t     ch;
   uint8_t     line;
   uint8_t     col;
   uint16_t    address = config->address;
   uint16_t    topleft = config->topleft;
   uint8_t     byte;
   uint8_t     mode    = MODE_HEX;
   uint8_t     cmove   = MOVE_NONE;
   uint8_t     bank    = config->bank;
   bool        redraw  = true;

   clear();
   setscrreg( FIRST_LINE, LAST_LINE-1 );

   attrset( MC_ATTRIBUTES_HEXEDIT );

   move ( LINES-1, 0 );
   addstr( "  Hex Editor  (Ctrl+C to leave)" );
   //       01234567890123456789012345678901
   // col needs to start with number of chars in above
   for( col = 31; col < (FIRST_ASCII_COL + BYTES_PER_ROW); ++col )
   {
      addch( ' ' );
   }
   move (0, 0);
   for (col = 0; col < FIRST_HEX_COL; col++)
   {
      addch( ' ' );
   }

   for (byte = 0; byte < BYTES_PER_ROW; byte++)
   {
      if( byte >> 4 )
      {
         mcurses_hexout( byte, 1 );
      }
      else
      {
         addch( ' ' );
      }
      mcurses_hexout( byte, 1 );
      addch (' ');
      col += 3;
   }

   for ( ; col < FIRST_ASCII_COL; col++)
   {
      addch (' ');
   }

   for (byte = 0; byte < BYTES_PER_ROW; byte++)
   {
      mcurses_hexout( byte, 1 );
   }

   attrset( B_DEFAULT | F_DEFAULT );

   do
   {
      move( 0, 0 );
      attrset( B_RED | F_WHITE );

      mcurses_hexout2( address >> 8 );
      mcurses_hexout2( address & 0xFF );
      addch( ':' );
      mcurses_hexout( bank, 1 );

      attrset( B_DEFAULT | F_DEFAULT );

      if( redraw )
      {
         topleft = print_hex_page( config, topleft, address );
         redraw = false;
      }

      line = ((address - topleft) / BYTES_PER_ROW) + FIRST_LINE;
      byte = (address - topleft) % BYTES_PER_ROW;
      if( mode == MODE_HEX )
      {
         col = FIRST_HEX_COL + 3 * byte;
      }
      else
      {
         col = FIRST_ASCII_COL + byte;
      }

#if 0
      move( LINES-1, 0 );
      addstr( " t=" );
      ito4x( topleft );
      addstr( " a=" );
      ito4x( address );

      addstr( " l=" );
      mcurses_hexout2( line );
      addstr( " b=" );
      mcurses_hexout2( byte );
#endif

      move( line, col );
      ch = getch ();

      switch( ch )
      {
         case KEY_HOME:
         {
            address = address - (address % BYTES_PER_ROW);
            break;
         }
         case KEY_END:
         {
            address = address - (address % BYTES_PER_ROW) + BYTES_PER_ROW - 1;
            break;
         }
         case KEY_NPAGE:
         {
            address += 0x100;
            redraw = true;
            break;
         }
         case KEY_PPAGE:
         {
            address -= 0x100;
            redraw = true;
            break;
         }
         case 0x02: /* Ctrl+B: select bank */
         {
            bank = config->nextbank();
            redraw = true;
            break;
         }
         case 0x07: /* Ctrl+G: enter address */
         {
            move( 0, 0 );
            if( mcurses_get4hex( &address ) )
            {
               topleft = address;
            }
            redraw = true;
            break;
         }
         case KEY_RIGHT:
         {
            ++address;
            cmove = MOVE_RIGHT;
            break;
         }
         case KEY_LEFT:
         case KEY_BACKSPACE:
         {
            --address;
            cmove = MOVE_LEFT;
            break;
         }
         case KEY_DOWN:
         {
            address += BYTES_PER_ROW;
            cmove = MOVE_DOWN;
            break;
         }
         case KEY_UP:
         {
            address -= BYTES_PER_ROW;
            cmove = MOVE_UP;
            break;
         }
         case KEY_TAB:
         {
            if (mode == MODE_HEX)
            {
               mode = MODE_ASCII;
            }
            else
            {
               mode = MODE_HEX;
            }
         }   /* fall through */
         default:
         {
            uint8_t check;
            if (mode == MODE_HEX)
            {
               if (IS_HEX(ch))
               {
                  uint8_t value = chartohex( ch ) << 4;

                  /* wait for second hex character */
                  ch = getch ();

                  if (IS_HEX(ch))
                  {
                     value |= chartohex( ch );
                     hexedit_poke( config, address, value );
                     check = hexedit_peek( config, address );
                     if( check == value )
                     {
                        move( line, FIRST_ASCII_COL + byte );
                        mcurses_debug_byte( value, config->charset );
                     }
                     cmove = MOVE_RIGHT;
                  }
                  /* re-read */
                  move( line, FIRST_HEX_COL + 3 * byte );
                  mcurses_hexout2( hexedit_peek( config, address++ ) );
               }
            }
            else // MODE_ASCII
            {
               /* input is only possible in ASCII */
               if( (ch >= 0x20) && (ch < 0x7F) )
               {
                  hexedit_poke( config, address, ch );
                  check = hexedit_peek( config, address++ );
                  if( check == ch )
                  {
                     addch( ch );
                     move( line, FIRST_HEX_COL + 3 * byte );
                     mcurses_hexout2( ch );
                  }
                  cmove = MOVE_RIGHT;
               }
            }
         }
      }

      if( cmove == MOVE_RIGHT )
      {
         if( (address % BYTES_PER_ROW) == 0 )
         {
            cmove = MOVE_DOWN;
         }
      }
      else if( cmove == MOVE_LEFT )
      {
         if( (address % BYTES_PER_ROW) == (BYTES_PER_ROW-1) )
         {
            cmove = MOVE_UP;
         }
      }

      if( cmove == MOVE_UP )
      {
         if (line == FIRST_LINE)
         {
            insertln();
            print_hex_line( config, FIRST_LINE, address - (address % BYTES_PER_ROW) );
            topleft -= BYTES_PER_ROW;
         }
         else
         {
            line--;
         }
      }
      else if( cmove == MOVE_DOWN )
      {
         if (line == LAST_LINE - 1)
         {
            scroll();
            print_hex_line( config, line, address - (address % BYTES_PER_ROW) );
            topleft += BYTES_PER_ROW;
         }
         else
         {
            line++;
         }
      }
      cmove = MOVE_NONE;
   }
   while (ch != 0x03);  // Ctrl+C exits

   config->bank    = bank;
   config->address = address;
   config->topleft = topleft;
   return;
}

