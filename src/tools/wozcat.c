
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
   unsigned long addr;
   char *rest;
   int c;

   if( argc < 2 )
   {
      fprintf( stderr, "usage: %s <start_address>\n", argv[0] );
      return 1;
   }

   addr = strtoul( argv[1], &rest, 0 );
   if( (rest && *rest) || (addr > 0xFFFF) )
   {
      fprintf( stderr, "bad start address\n" );
      return 2;
   }

   putchar( 'w' );
   fflush( stdout );
   usleep( 100000 );
   putchar( 0x1b ); // ESC
   fflush( stdout );
   
   if( addr & 7 )
   {
      printf( "%04lx:", addr );
   }

   while( (c = getchar()) != EOF )
   {
      if( !(addr & 7) )
      {
         printf( "%04lx:", addr );
      }
      printf( "%02x%c", c,
              ( (addr & 7) == 7 ) ? '\r' : ' ' );
      fflush( stdout );
      if( (addr & 7) == 7 )
      {
         usleep( 50000 );
      }
      ++addr;
   }
   
   putchar( '\r' );
   return 0;
}
