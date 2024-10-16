
#include <stdint.h>
#include <stdio.h>

void main()
{
   printf( "\n" );
   printf( "sizeof(char)    = %d  sizeof(unsigned char) = %d\n",
            sizeof(char), sizeof(unsigned char) );
   printf( "sizeof(int)     = %d  sizeof(unsigned int)  = %d\n",
            sizeof(int), sizeof(unsigned int) );
   printf( "sizeof(long)    = %d  sizeof(unsigned long) = %d\n",
            sizeof(long), sizeof(unsigned long) );
   printf( "sizeof(int8_t)  = %d  sizeof(uint8_t)       = %d\n",
            sizeof(int8_t), sizeof(uint8_t) );
   printf( "sizeof(int16_t) = %d  sizeof(uint16_t)      = %d\n",
            sizeof(int16_t), sizeof(uint16_t) );
   printf( "sizeof(int32_t) = %d  sizeof(uint32_t)      = %d\n",
            sizeof(int32_t), sizeof(uint32_t) );
   printf( "sizeof(void*)   = %d\n",
            sizeof(void*) );
}

