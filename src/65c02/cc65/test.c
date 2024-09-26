
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

int main()
{
   printf( "\n%s was %c%s%c here... $%04x\n", "SvOlli", 34, "fuckin'", 34, *((uint16_t*)0xfffc) );
   printf( "sizeof(char)=%d, sizeof(int)=%d, sizeof(long)=%d\n", sizeof(char), sizeof(int), sizeof(long) );
   return 0;
}

