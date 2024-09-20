
/*
 * wozcat.c: helper tool to get binary data in RAM using WozMon
 *
 * usage example:
 * build/tools/wozcat.exe 0x400 <build/65c02/bcdcheck.sx4 >/dev/ttyACM0
 */

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

   /* if we are in menu drop to WozMon */
   /* if we are already in WozMon this does nothing, due to ESC */
   putchar( 'w' );
   fflush( stdout );
   usleep( 100000 );

   /* make sure we're at a defined point */
   putchar( 0x1b ); /* $1b = ESC */
   fflush( stdout );

   /* when we're not starting at an 8 byte boundry, write address */
   if( addr & 7 )
   {
      printf( "%04lx:", addr );
   }

   while( (c = getchar()) != EOF )
   {
      if( !(addr & 7) )
      {
         /* print address at 8 byte boundry */
         printf( "%04lx:", addr );
      }

      /* print data, at end of line with return */
      printf( "%02x%c", c,
              ( (addr & 7) == 7 ) ? '\r' : ' ' );
      fflush( stdout );
      if( (addr & 7) == 7 )
      {
         /* give WozMon some time to prozess line */
         usleep( 50000 );
      }
      ++addr;
   }

   /* make sure that last line is entered */
   putchar( '\r' );

   return 0;
}
