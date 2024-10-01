
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define STEP (16)

int main()
{
   uint8_t *addr = 0xFF00;
   uint8_t i;
   printf( "\nthis program written in C and compiled using the cc65-sdk\n" );
   printf( "hexdump of BIOS page:\n" );

   for( addr = 0xFF00; addr; addr += STEP )
   {
      printf( "%04x:", addr );
      for( i = 0; i < STEP; ++i )
      {
         printf( " %02x", *(addr + i) );
      }
      printf( " : " );
      for( i = 0; i < STEP; ++i )
      {
         uint8_t c = *(addr + i);
         if( (c >= ' ') && (c < 0x7f) )
         {
            putchar( c );
         }
         else
         {
            putchar( '.' );
         }
      }
      printf( "\n" );
   }

   return 0;
}

