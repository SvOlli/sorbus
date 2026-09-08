
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DOCUMENTATION   "doc/site/jam/utf8codepages.md"
#define SOURCECODE      "src/rp2040/common/utf8codepages.h"

typedef struct {
   uint16_t   glyph;
   const char *description;
} charset_t;

charset_t codepage1[0x80] = {
// 0x80-0x8A:
   { 0x2500, "BOX DRAWINGS LIGHT HORIZONTAL" },
   { 0x2502, "BOX DRAWINGS LIGHT VERTICAL" },
   { 0x250C, "BOX DRAWINGS LIGHT DOWN AND RIGHT" },
   { 0x2510, "BOX DRAWINGS LIGHT DOWN AND LEFT" },
   { 0x2514, "BOX DRAWINGS LIGHT UP AND RIGHT" },
   { 0x2518, "BOX DRAWINGS LIGHT UP AND LEFT" },
   { 0x251C, "BOX DRAWINGS LIGHT VERTICAL AND RIGHT" },
   { 0x2524, "BOX DRAWINGS LIGHT VERTICAL AND LEFT" },
   { 0x252C, "BOX DRAWINGS LIGHT DOWN AND HORIZONTAL" },
   { 0x2534, "BOX DRAWINGS LIGHT UP AND HORIZONTAL" },
   { 0x253C, "BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL" },
// 0x8B-0x8F:
   { 0x2600, "BLACK SUN WITH RAYS" },
   { 0x263C, "WHITE SUN WITH RAYS" },
   { 0x2605, "BLACK STAR" },
   { 0x2606, "WHITE STAR" },
   { 0x2601, "CLOUD" },

// 0x90-0x9A:
   { 0x2550, "BOX DRAWINGS DOUBLE HORIZONTAL" },
   { 0x2551, "BOX DRAWINGS DOUBLE VERTICAL" },
   { 0x2554, "BOX DRAWINGS DOUBLE DOWN AND RIGHT" },
   { 0x2557, "BOX DRAWINGS DOUBLE DOWN AND LEFT" },
   { 0x255A, "BOX DRAWINGS DOUBLE UP AND RIGHT" },
   { 0x255D, "BOX DRAWINGS DOUBLE UP AND LEFT" },
   { 0x2560, "BOX DRAWINGS DOUBLE VERTICAL AND RIGHT" },
   { 0x2563, "BOX DRAWINGS DOUBLE VERTICAL AND LEFT" },
   { 0x2566, "BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL" },
   { 0x2569, "BOX DRAWINGS DOUBLE UP AND HORIZONTAL" },
   { 0x256C, "BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL" },
// 0x9B-0x9F:
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },

// 0xA0-0xAF:
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },
   { 0x00A2, "CENT SIGN" },
   { 0x00A3, "POUND SIGN" },
   { 0xFFFD, "(to be defined)" },
   { 0x00A5, "YEN SIGN" },
   { 0xFFFD, "(to be defined)" },
   { 0x00A7, "SECTION SIGN" },
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },
   { 0xFFFD, "(to be defined)" },
   { 0x20AC, "Euro Sign" },
   { 0x2571, "BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT" },
   { 0x2572, "BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT" },
   { 0x2573, "BOX DRAWINGS LIGHT DIAGONAL CROSS" },

// 0xB0-0xB5:
   { 0x00B0, "DEGREE SIGN" },
   { 0x00B9, "SUPERSCRIPT ONE" },
   { 0x00B2, "SUPERSCRIPT TWO" },
   { 0x00B3, "SUPERSCRIPT THREE" },
   { 0x00B1, "PLUS-MINUS SIGN" },
   { 0x00B5, "MICRO SIGN" },

// 0xB6-0xBB:
   { 0x00B7, "MIDDLE DOT" },
   { 0x2022, "BULLET" },
   { 0x25CB, "WHITE CIRCLE" },
   { 0x25CF, "BLACK CIRCLE" },
   { 0x25A1, "WHITE SQUARE" },
   { 0x25A0, "BLACK SQUARE" },
// 0xBC-0xBE:
   { 0x00BC, "VULGAR FRACTION ONE QUARTER" },
   { 0x00BD, "VULGAR FRACTION ONE HALF" },
   { 0x00BE, "VULGAR FRACTION THREE QUARTERS" },
// 0xBF:
   { 0xFFFD, "(to be defined)" },

// 0xC0-0xC3:
   { 0x25B2, "BLACK UP-POINTING TRIANGLE" },
   { 0x25BC, "BLACK DOWN-POINTING TRIANGLE" },
   { 0x25C4, "BLACK LEFT-POINTING POINTER" },
   { 0x25BA, "BLACK RIGHT-POINTING POINTER" },

// 0xC4-0xCF:
   { 0x263A, "WHITE SMILING FACE" },
   { 0x263B, "BLACK SMILING FACE" },
   { 0x2020, "Dagger" },
   { 0x2021, "Double Dagger" },
   { 0x2190, "LEFTWARDS ARROW" },
   { 0x2191, "UPWARDS ARROW" },
   { 0x2192, "RIGHTWARDS ARROW" },
   { 0x2193, "DOWNWARDS ARROW" },
   { 0x2194, "LEFT RIGHT ARROW" },
   { 0x2195, "UP DOWN ARROW" },
   { 0x21B5, "DOWNWARDS ARROW WITH CORNER LEFTWARDS" },
   { 0xFFFD, "(to be defined)" },

// 0xD0-0xD3:
   { 0x2669, "Quarter note" },
   { 0x266a, "Eighth note" },
   { 0x266b, "Beamed eighth notes" },
   { 0x266c, "Beamed sixteenth notes" },
// 0xD4-0xDF:
   { 0x2654, "WHITE CHESS KING" },
   { 0x2655, "WHITE CHESS QUEEN" },
   { 0x2656, "WHITE CHESS ROOK" },
   { 0x2657, "WHITE CHESS BISHOP" },
   { 0x2658, "WHITE CHESS KNIGHT" },
   { 0x2659, "WHITE CHESS PAWN" },
   { 0x265A, "BLACK CHESS KING" },
   { 0x265B, "BLACK CHESS QUEEN" },
   { 0x265C, "BLACK CHESS ROOK" },
   { 0x265D, "BLACK CHESS BISHOP" },
   { 0x265E, "BLACK CHESS KNIGHT" },
   { 0x265F, "BLACK CHESS PAWN" },

// 0xE0-0xE7:
   { 0x2660, "BLACK SPADE SUIT" },
   { 0x2661, "WHITE HEART SUIT" },
   { 0x2662, "WHITE DIAMOND SUIT" },
   { 0x2663, "BLACK CLUB SUIT" },
   { 0x2664, "WHITE SPADE SUIT" },
   { 0x2665, "BLACK HEART SUIT" },
   { 0x2666, "BLACK DIAMOND SUIT" },
   { 0x2667, "WHITE CLUB SUIT" },
// 0xE8-0xEA:
   { 0x2122, "Trade Mark Sign" },
   { 0x00A9, "COPYRIGHT SIGN" },
   { 0x00AE, "REGISTERED SIGN" },
// 0xEB-0xED:
   { 0x2610, "BALLOT BOX" },
   { 0x2611, "BALLOT BOX WITH CHECK" },
   { 0x2612, "BALLOT BOX WITH X" },
// 0xEE-0xF0:
   { 0x2591, "LIGHT SHADE" },
   { 0x2592, "MEDIUM SHADE" },
   { 0x2593, "DARK SHADE" },

// 0xF1-0xFF:
   { 0x259D, "QUADRANT UPPER RIGHT" },
   { 0x2597, "QUADRANT LOWER RIGHT" },
   { 0x2596, "QUADRANT LOWER LEFT" },
   { 0x2598, "QUADRANT UPPER LEFT" },
   { 0x259A, "QUADRANT UPPER LEFT AND LOWER RIGHT" },
   { 0x259E, "QUADRANT UPPER RIGHT AND LOWER LEFT" },
   { 0x2580, "UPPER HALF BLOCK" },
   { 0x2584, "LOWER HALF BLOCK" },
   { 0x258C, "LEFT HALF BLOCK" },
   { 0x2590, "RIGHT HALF BLOCK" },
   { 0x2599, "QUADRANT UPPER LEFT AND LOWER LEFT AND LOWER RIGHT" },
   { 0x259B, "QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER LEFT" },
   { 0x259C, "QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT" },
   { 0x259F, "QUADRANT UPPER RIGHT AND LOWER LEFT AND LOWER RIGHT" },
   { 0x2588, "FULL BLOCK" }
};


bool isdir( const char *path )
{
   struct stat st;

   if( stat( path, &st ) )
   {
      return false;
   }

   return S_ISDIR( st.st_mode );
}


bool cdroot()
{
   char cwd[4096] = "";

   while( strcmp( cwd, "/" ) )
   {
      getcwd( &cwd[0], sizeof(cwd) );
      if( isdir( "doc" ) && isdir( "src" ) )
      {
         fprintf( stderr, "'%s'\n", cwd );;
         return true;
      }
      chdir( ".." );
   }

   return false;
}


int doc( FILE *f )
{
   int i;
   const char *c;

   fprintf( f,
"\n# UTF-8 Codepages\n\n"
"For output the UART config register (UARTCF/$DF0B) has two bits (2 & 3)\n"
"that allow for selecting an output \"codepage\". For every codepage, the\n"
"lower 128 characters are the same (the ASCII table). Codepage 0 is raw\n"
"output, so the software has to create the multibyte sequences itself.\n"
"The other codepages can output a multibyte character from just a single\n"
"byte. The association of bytes to characters is stated below.\n"
);


   fprintf( f, "## Codepage 1\n\n" );
   fprintf( f, "| Char | UTF-8 | Glyph | Description |\n" );
   fprintf( f, "| ---- | ----- | ----- | ----------- |\n" );
   for( i = 0; i < 0x80; ++i )
   {
      fprintf( f, "| $%02x | 0x%04x | &#x%04x; | ",
               i + 0x80, codepage1[i].glyph, codepage1[i].glyph );
      for( c = codepage1[i].description; *c; ++c )
      {
         fputc( tolower(*c), f );
      }
      fprintf( f, " |\n" );
   }

   fprintf( f, "\n## Codepage 2\n\nRight now it is the same as codepage 1.\n" );
   fprintf( f, "\n## Codepage 3\n\nRight now it is the same as codepage 1.\n" );

   return 0;
}


int code( FILE *f )
{
   int i;
   fprintf( f, "/*\n * This code was automatically generated\n */\n\n" );
   fprintf( f, "#ifndef _CHARSET_H_\n#define _CHARSET_H_ _CHARSET_H_\n" );

   fprintf( f, "\n#include <stdint.h>\n\n" );
   fprintf( f, "uint16_t sorbus_codepage[0x80] = {\n" );

   for( i = 0; i < 0x80; ++i )
   {
      if( (i & 7) == 0 )
      {
         fprintf( f, "  " );
      }
      fprintf( f, " 0x%04x", codepage1[i].glyph );
      if( i != 0x7f )
      {
         fprintf( f, "," );
      }
      if( (i & 7) == 7 )
      {
         fprintf( f, "\n" );
      }
   }

   fprintf( f, "};\n" );
   fprintf( f, "#endif\n" );

   return 0;
}


int main( int argc, char *argv[] )
{
   FILE *f;

   fprintf( stderr, "searching for build root: " );
   if( !cdroot() )
   {
      fprintf( stderr, "not found!\n" );
      return 10;
   }
   
   fprintf( stderr, "creating '" DOCUMENTATION "'\n"  );
   f = fopen( DOCUMENTATION, "wb" );
   if( f )
   {
      doc( f );
      fclose( f );
   }
   else
   {
      return 11;
   }

   fprintf( stderr, "creating '" SOURCECODE "'\n" );
   f = fopen( SOURCECODE, "wb" );
   if( f )
   {
      code( f );
      fclose( f );
   }
   else
   {
      return 12;
   }

   return 0;
}

