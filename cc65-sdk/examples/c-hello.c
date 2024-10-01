
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define STEP (16)

int main()
{
   uint8_t *addr;
   uint8_t i;
   printf( "\nthis program was written in C and compiled using the cc65-sdk\n" );
   printf( "hexdump of BIOS page:\n" );

   for( addr = (uint8_t*)0xFF00; addr; addr += STEP )
   {
      printf( "%04x:", addr );
      for( i = 0; i < STEP; ++i )
      {
         printf( "%s%02x", i == 8 ? "  " : " ", *(addr + i) );
      }
      printf( "  " );
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

