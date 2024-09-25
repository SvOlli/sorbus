
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <string.h>

int main()
{
   char buffer[81] = "SvOlli was here...";
   int dr;
   printf( "\n%04X\n", buffer );
   write( 1, buffer, strlen(buffer) );
   printf( "\n" );
   dr = read( 0, buffer, 81 );
   printf( "\n" );
   write( 1, buffer, strlen(buffer) );
   printf( "\n" );
   return 0;
}

