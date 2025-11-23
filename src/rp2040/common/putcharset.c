
#include "generic_helper.h"

#include <stdint.h>
#include <stdio.h>

/* gaps still need to be filled, expect things to move */
uint16_t sorbus_codepage[0x80] = {

// 0x80-0x8A:
   0x2500, // ─ BOX DRAWINGS LIGHT HORIZONTAL
   0x2502, // │ BOX DRAWINGS LIGHT VERTICAL
   0x250C, // ┌ BOX DRAWINGS LIGHT DOWN AND RIGHT
   0x2510, // ┐ BOX DRAWINGS LIGHT DOWN AND LEFT
   0x2514, // └ BOX DRAWINGS LIGHT UP AND RIGHT
   0x2518, // ┘ BOX DRAWINGS LIGHT UP AND LEFT
   0x251C, // ├ BOX DRAWINGS LIGHT VERTICAL AND RIGHT
   0x2524, // ┤ BOX DRAWINGS LIGHT VERTICAL AND LEFT
   0x252C, // ┬ BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
   0x2534, // ┴ BOX DRAWINGS LIGHT UP AND HORIZONTAL
   0x253C, // ┼ BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
// 0x8B-0x8F:
   0x2600, // ☀ BLACK SUN WITH RAYS
   0x263C, // ☼ WHITE SUN WITH RAYS
   0x2605, // ★ BLACK STAR
   0x2606, // ☆ WHITE STAR
   0x2601, // ☁ CLOUD

// 0x90-0x9A:
   0x2550, // ═ BOX DRAWINGS DOUBLE HORIZONTAL
   0x2551, // ║ BOX DRAWINGS DOUBLE VERTICAL
   0x2554, // ╔ BOX DRAWINGS DOUBLE DOWN AND RIGHT
   0x2557, // ╗ BOX DRAWINGS DOUBLE DOWN AND LEFT
   0x255A, // ╚ BOX DRAWINGS DOUBLE UP AND RIGHT
   0x255D, // ╝ BOX DRAWINGS DOUBLE UP AND LEFT
   0x2560, // ╠ BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
   0x2563, // ╣ BOX DRAWINGS DOUBLE VERTICAL AND LEFT
   0x2566, // ╦ BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
   0x2569, // ╩ BOX DRAWINGS DOUBLE UP AND HORIZONTAL
   0x256C, // ╬ BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
// 0x9B-0x9F:
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,

// 0xA0-0xAF:
   0xFFFF,
   0xFFFF,
   0x00A2, // ¢ CENT SIGN
   0x00A3, // £ POUND SIGN
   0xFFFF,
   0x00A5, // ¥ YEN SIGN
   0xFFFF,
   0x00A7, // § SECTION SIGN
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0x20AC, // € Euro Sign
   0xFFFF,
   0xFFFF,
   0xFFFF,

// 0xB0-0xB5:
   0x00B0, // ° DEGREE SIGN
   0x00B9, // ¹ SUPERSCRIPT ONE
   0x00B2, // ² SUPERSCRIPT TWO
   0x00B3, // ³ SUPERSCRIPT THREE
   0x00B1, // ± PLUS-MINUS SIGN
   0x00B5, // µ MICRO SIGN

// 0xB6-0xBB:
   0x00B7, // · MIDDLE DOT
   0x2022, // • BULLET
   0x25CB, // ○ WHITE CIRCLE
   0x25CF, // ● BLACK CIRCLE
   0x25A1, // □ WHITE SQUARE
   0x25A0, // ■ BLACK SQUARE
// 0xBC-0xBE:
   0x00BC, // ¼ VULGAR FRACTION ONE QUARTER
   0x00BD, // ½ VULGAR FRACTION ONE HALF
   0x00BE, // ¾ VULGAR FRACTION THREE QUARTERS
// 0xBF:
   0xFFFF,

// 0xC0-0xC7:
   0x25B2, // ▲ BLACK UP-POINTING TRIANGLE
   0x25BC, // ▼ BLACK DOWN-POINTING TRIANGLE
   0x25C4, // ◄ BLACK LEFT-POINTING POINTER
   0x25BA, // ► BLACK RIGHT-POINTING POINTER

// 0xC8-0xCF:
   0x263A, // ☺ WHITE SMILING FACE
   0x263B, // ☻ BLACK SMILING FACE
   0x2020, // † Dagger
   0x2021, // ‡ Double Dagger
   0x2190, // ← LEFTWARDS ARROW
   0x2191, // ↑ UPWARDS ARROW
   0x2192, // → RIGHTWARDS ARROW
   0x2193, // ↓ DOWNWARDS ARROW
   0x2194, // ↔ LEFT RIGHT ARROW
   0x2195, // ↕ UP DOWN ARROW
   0x21B5, // ↵ DOWNWARDS ARROW WITH CORNER LEFTWARDS
   0xFFFF,

// 0xD0-0xD3:
   0x2669, // ♩ Quarter note
   0x266a, // ♪ Eighth note
   0x266b, // ♫ Beamed eighth notes
   0x266c, // ♬ Beamed sixteenth notes
// 0xD4-0xDF:
   0x2654, // ♔ WHITE CHESS KING
   0x2655, // ♕ WHITE CHESS QUEEN
   0x2656, // ♖ WHITE CHESS ROOK
   0x2657, // ♗ WHITE CHESS BISHOP
   0x2658, // ♘ WHITE CHESS KNIGHT
   0x2659, // ♙ WHITE CHESS PAWN
   0x265A, // ♚ BLACK CHESS KING
   0x265B, // ♛ BLACK CHESS QUEEN
   0x265C, // ♜ BLACK CHESS ROOK
   0x265D, // ♝ BLACK CHESS BISHOP
   0x265E, // ♞ BLACK CHESS KNIGHT
   0x265F, // ♟ BLACK CHESS PAWN

// 0xE0-0xE7:
   0x2660, // ♠ BLACK SPADE SUIT
   0x2661, // ♡ WHITE HEART SUIT
   0x2662, // ♢ WHITE DIAMOND SUIT
   0x2663, // ♣ BLACK CLUB SUIT
   0x2664, // ♤ WHITE SPADE SUIT
   0x2665, // ♥ BLACK HEART SUIT
   0x2666, // ♦ BLACK DIAMOND SUIT
   0x2667, // ♧ WHITE CLUB SUIT
// 0xE8-0xEA:
   0x2122, // ™ Trade Mark Sign
   0x00A9, // © COPYRIGHT SIGN
   0x00AE, // ® REGISTERED SIGN
// 0xEB-0xED:
   0x2610, // ☐ BALLOT BOX
   0x2611, // ☑ BALLOT BOX WITH CHECK
   0x2612, // ☒ BALLOT BOX WITH X
// 0xEE-0xF0:
   0x2591, // ░ LIGHT SHADE
   0x2592, // ▒ MEDIUM SHADE
   0x2593, // ▓ DARK SHADE

// 0xF1-0xFF:
   0x259D, // ▝ QUADRANT UPPER RIGHT
   0x2597, // ▗ QUADRANT LOWER RIGHT
   0x2596, // ▖ QUADRANT LOWER LEFT
   0x2598, // ▘ QUADRANT UPPER LEFT
   0x259A, // ▚ QUADRANT UPPER LEFT AND LOWER RIGHT
   0x259E, // ▞ QUADRANT UPPER RIGHT AND LOWER LEFT
   0x2580, // ▀ UPPER HALF BLOCK
   0x2584, // ▄ LOWER HALF BLOCK
   0x258C, // ▌ LEFT HALF BLOCK
   0x2590, // ▐ RIGHT HALF BLOCK
   0x2599, // ▙ QUADRANT UPPER LEFT AND LOWER LEFT AND LOWER RIGHT
   0x259B, // ▛ QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER LEFT
   0x259C, // ▜ QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT
   0x259F, // ▟ QUADRANT UPPER RIGHT AND LOWER LEFT AND LOWER RIGHT
   0x2588, // █ FULL BLOCK

// END!


};


int putcharset( uint8_t ch, uint8_t cs )
{
   int retval = 0;

   if( (ch < 0x80) || (cs == 0) )
   {
      /* input data can be passed through */
      retval += putchar( ch );
   }
   else
   {
      uint16_t ch16;

      switch( cs )
      {
         default:
            ch16 = sorbus_codepage[ch & 0x7F];
            break;
      }

      /* now, output multibyte sequence */
      if( ch16 < 0x800 )
      {
         /* write UTF-8 2-byte sequence
          * 110y yyxx 10xx xxxx
          * ^^^---------------- 2-byte encoding
          *           ^^------- followup data
          *    ^ ^^^^   ^^ ^^^^ 11 bits of data
          */
         retval += putchar( 0xc0 | ((ch16 & 0x07c0) >> 6) );
         retval += putchar( 0x80 |  (ch16 & 0x003f) );
      }
      else
      {
         /* write UTF-8 3-byte sequence
          * 1110 yyyy 10yy yyxx 10xx xxxx
          * ^^^-------------------------- 3-byte encoding
          *           ^^----------------- followup data
          *                     ^^------- followup data
          *      ^^^^   ^^ ^^^^   ^^ ^^^^ 16 bits of data
          */
         retval += putchar( 0xe0 | ((ch16 & 0xf000) >> 12) );
         retval += putchar( 0x80 | ((ch16 & 0x0fc0) >> 6) );
         retval += putchar( 0x80 |  (ch16 & 0x003f) );
      }
   }
   return retval;
}

