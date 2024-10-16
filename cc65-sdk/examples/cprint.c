
#include <stdlib.h>
#include <unistd.h>

const char pieces[] = { 128,130,140,144,148,152,156,164,172,180,188 };

int main()
{
   char outbuffer[3] = { 226,148, 0 };
   for(;;)
   {
      outbuffer[2] = pieces[rand()%sizeof(pieces)];
      write( 1, outbuffer, sizeof(outbuffer) );
      if( (*(char*)(0xDF0D) > 0) && (*(char*)(0xDF0C) == 3) )
      {
         return 0;
      }
   }
}
