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
#include "mcurses_sorbus.h"

static hexedit_handler_bank_t hexedit_bank;
static hexedit_handler_peek_t hexedit_peek;
static hexedit_handler_poke_t hexedit_poke;

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

/*-----------------------------------------------------------------------------
 * itox: convert a decimal value 0-15 into hexadecimal digit
 *-----------------------------------------------------------------------------
 */
void itox (uint8_t val)
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


/*-----------------------------------------------------------------------------
 * itoxx: convert a decimal value 0-255 into 2 hexadecimal digits
 *-----------------------------------------------------------------------------
 */
void itoxx (unsigned char i)
{
   itox (i >> 4);
   itox (i & 0x0F);
}


uint8_t xtoi (uint8_t ch)
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
   itox (val);    // print value in hex
   return val;
}

void print_hex_line (uint8_t line, uint16_t off)
{
   uint8_t       col;
   uint8_t       ch;

   move (line, 0);
   itoxx (off >> 8);
   itoxx (off & 0xFF);
   addstr("   ");

   move (line, FIRST_HEX_COL);
   for (col = 0; col < BYTES_PER_ROW; col++)
   {
      itoxx (hexedit_peek(off));
      addch (' ');
      off++;
   }

   off -= BYTES_PER_ROW;

   move (line, FIRST_ASCII_COL);

   for (col = 0; col < BYTES_PER_ROW; col++)
   {
      ch = hexedit_peek(off);

      if (IS_PRINT(ch))
      {
         addch( ch );
      }
      else
      {
         addch ('.');
      }
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


static uint16_t print_hex_page( uint16_t topleft, uint16_t address )
{
   uint8_t     line;
   uint16_t    off;

   topleft = fix_topleft( topleft, address );
   off = topleft;
   for (line = FIRST_LINE; line < LAST_LINE; line++)
   {
      print_hex_line (line, off);
      off += BYTES_PER_ROW;
   }

   return topleft;
}


#if 0
void ito4x( uint16_t address )
{
   itoxx(address >> 8);
   itoxx(address & 0xFF);
}
#endif

/*-----------------------------------------------------------------------------
 * hexdit: hex editor
 *-----------------------------------------------------------------------------
 */
void hexedit( hexedit_t *config )
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

   hexedit_bank = config->nextbank;
   hexedit_peek = config->peek;
   hexedit_poke = config->poke;

   clear();
   setscrreg( FIRST_LINE, LAST_LINE-1 );

   attrset( F_WHITE | B_RED );

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
         itox( byte );
      }
      else
      {
         addch( ' ' );
      }
      itox (byte);
      addch (' ');
      col += 3;
   }

   for ( ; col < FIRST_ASCII_COL; col++)
   {
      addch (' ');
   }

   for (byte = 0; byte < BYTES_PER_ROW; byte++)
   {
      itox (byte);
   }

   attrset( B_DEFAULT | F_DEFAULT );

   do
   {
      move( 0, 0 );
      attrset( B_RED | F_WHITE );

      itoxx( address >> 8 );
      itoxx( address & 0xFF );
      addch( ':' );
      itox( bank );

      attrset( B_DEFAULT | F_DEFAULT );

      if( redraw )
      {
         topleft = print_hex_page( topleft, address );
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
      itoxx( line );
      addstr( " b=" );
      itoxx( byte );
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
            bank = hexedit_bank();
            redraw = true;
            break;
         }
         case 0x07: /* Ctrl+G: enter address */
         {
            move( 0, 0 );
            if( screen_get4hex( &address ) )
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
                  uint8_t value = xtoi( ch ) << 4;

                  /* wait for second hex character */
                  ch = getch ();

                  if (IS_HEX(ch))
                  {
                     value |= xtoi( ch );
                     hexedit_poke( address, value );
                     check = hexedit_peek( address );
                     if( check == value )
                     {
                        move (line, FIRST_ASCII_COL + byte);

                        if (IS_PRINT(value))
                        {
                           addch (value);
                        }
                        else
                        {
                           addch ('.');
                        }
                     }
                     cmove = MOVE_RIGHT;
                  }
                  /* re-read */
                  move( line, FIRST_HEX_COL + 3 * byte );
                  itoxx( hexedit_peek( address++ ) );
               }
            }
            else // MODE_ASCII
            {
               if (IS_PRINT(ch))
               {
                  hexedit_poke(address, ch);
                  check = hexedit_peek( address++ );
                  if( check == ch )
                  {
                     addch( ch );
                     move (line, FIRST_HEX_COL + 3 * byte);
                     itoxx( ch );
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
            print_hex_line( FIRST_LINE, address - (address % BYTES_PER_ROW) );
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
            print_hex_line( line, address - (address % BYTES_PER_ROW) );
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

