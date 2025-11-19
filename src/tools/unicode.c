
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int putchar_utf8( uint16_t ch )
{
   int retval = 0;

   if( ch < 0x80 )
   {
      retval += putchar( ch );
   }
   else
   {
      if( ch < 0x800 )
      {
         /* write UTF-8 2-byte sequence
          * 110y yyxx 10xx xxxx
          * ^^^---------------- 2-byte encoding
          *           ^^------- followup data
          *    ^ ^^^^   ^^ ^^^^ 11 bits of data
          */
         retval += putchar( 0xc0 | ((ch & 0x07c0) >> 6) );
         retval += putchar( 0x80 |  (ch & 0x003f) );
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
         retval += putchar( 0xe0 | ((ch & 0xf000) >> 12) );
         retval += putchar( 0x80 | ((ch & 0x0fc0) >> 6) );
         retval += putchar( 0x80 |  (ch & 0x003f) );
      }
   }
   return retval;
}

int main( int argc, char *argv[] )
{
   uint32_t min, max, c;

   if( argc != 3 )
   {
      fprintf( stderr, "usage: %s <min> <max>\n", argv[0] );
      return 11;
   }
   min = strtoul( argv[1], 0, 0 );
   max = strtoul( argv[2], 0, 0 );
   if( min > max )
   {
      fprintf( stderr, "min is larger than max\n" );
      return 12;
   }
   for( c = min;; ++c )
   {
      printf( " 0x%04x:", c );
      putchar_utf8( c );
      if( c == max )
      {
         break;
      }
   }
   printf( "\n" );
   return 0;
}

