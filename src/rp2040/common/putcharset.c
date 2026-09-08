
#include "generic_helper.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utf8codepages.h"


uint16_t cs_to_cs16( uint8_t ch, uint8_t cs )
{
   uint16_t ch16;

   if( (cs > 0) && (ch == 0x7f) )
   {
      ch16 = 0xFFFD;
   }
   else if( (ch < 0x80) || (cs == 0) )
   {
      ch16 = ch;
   }
   else
   {
      switch( cs )
      {
         default:
            ch16 = sorbus_codepage[ch & 0x7F];
            break;
      }
   }
   return ch16;
}


const char *cs_to_utf8( uint8_t ch, uint8_t cs )
{
   static char output[5] = { 0 };
   char *c = &output[0];

   if( (ch < 0x80) || (cs == 0) )
   {
      /* input data can be passed through */
      *(c++) = (char)(ch);
   }
   else
   {
      uint16_t ch16 = cs_to_cs16( ch, cs );

      /* now, output multibyte sequence */
      if( ch16 < 0x800 )
      {
         /* write UTF-8 2-byte sequence
          * 110y yyxx 10xx xxxx
          * ^^^---------------- 2-byte encoding
          *           ^^------- followup data
          *    ^ ^^^^   ^^ ^^^^ 11 bits of data
          */
         *(c++) = (char)(0xc0 | ((ch16 & 0x07c0) >> 6));
         *(c++) = (char)(0x80 |  (ch16 & 0x003f));
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
         *(c++) = (char)(0xe0 | ((ch16 & 0xf000) >> 12));
         *(c++) = (char)(0x80 | ((ch16 & 0x0fc0) >> 6));
         *(c++) = (char)(0x80 |  (ch16 & 0x003f));
      }
   }

// e295b1
// 1110 0010 1001 0101 1011 0001
//      0010   01 0101   11 0001
//      2571
   *c = (char)(0);
   return output;
}


int cs_sn_utf8( char *b, size_t bsize, uint8_t ch, uint8_t cs )
{
   const char *utf8 = cs_to_utf8( ch, cs );
   size_t utf8len = strlen( utf8 );

   if( utf8len > bsize )
   {
      /* better to write nothing at all instead of truncated utf8 */
      return 0;
   }
   return snprintf( b, bsize, "%s", utf8 );
}


int cs_sn_utf8_debug( char *b, size_t bsize, uint8_t ch, uint8_t cs )
{
   if( ch == 0x7F )
   {
      /* quickhack: add 0xFFFD as already encoded UTF-8 */
      if( bsize >= 3 )
      {
         *(b++) = (char)0xEF;
         *(b++) = (char)0xBF;
         *(b++) = (char)0xBD;
         return 3;
      }
   }
   if( ch < 0x20 )
   {
      /* another quickhack: insert inverse sequence */
      return snprintf( b, bsize, "%c[;7m%c%c[;m", 0x1b, ch | 0x40, 0x1b );
   }
   return cs_sn_utf8( b, bsize, ch, cs );
}


int cs_put_utf8( uint8_t ch, uint8_t cs )
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


int cs_textlen( const char *string )
{
   const char *c;
   int count = 0;
   int8_t skip_counter = 0;

   for( c = string; *c; ++c )
   {
      if( *c == 0x1b ) // ESC
      {
         skip_counter = -1;
      }
      else if( *c & 0x80 )
      {
         if( (*c & 0xe0) == 0xe0 )
         {
            skip_counter = 3;
         }
         else if( (*c & 0xc0) == 0xc0 )
         {
            skip_counter = 2;
         }
         ++count;
      }
      else
      {
         ++count;
      }

      if( skip_counter < 0 )
      {
         if( (toupper(*c) >= 'A') && (toupper(*c) <= 'Z') )
         {
            skip_counter = 0;
         }
      }
      else if( skip_counter > 0 )
      {
         --skip_counter;
      }
   }
   return count;
}
