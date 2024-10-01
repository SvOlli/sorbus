
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <string.h>

int main()
{
   char buffer[81] =
      "\nThis is a test of all custom functions implemented in sorbus.lib\n";
   int dr;
   write( 1, buffer, strlen(buffer) );
   printf( "Enter something:\n" );
   dr = read( 0, buffer, 81 );
   if( !strcmp( buffer, "something" ) )
   {
      printf( "Good job! Bytes entered in hex: $%x\n", dr );
   }
   else
   {
      printf( "%d bytes entered:\n%s\n", dr, buffer );
   }
   return 0;
}
