
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
   char buffer[16384];
   FILE *fin = NULL ,*fout = NULL;
   int i;
   size_t dataread;

   if( argc < 3 )
   {
      fprintf( stderr, "usage: %s <outfile> <infile> (<infile...)\n", argv[0] );
      return 10;
   }

   fout = fopen( argv[1], "wb" );
   if( !fout )
   {
      fprintf( stderr, "could not open output file '%s': %s\n",
         argv[1], strerror( errno ) );
      return 1;
   }

   for( i = 2; i < argc; ++i )
   {
      fin = fopen( argv[i], "rb" );
      if( !fin )
      {
         fprintf( stderr, "could not open input file '%s': %s\n",
            argv[i], strerror( errno ) );
         fclose( fout );
         unlink( argv[1] );
         return 2;
      }
      while( (dataread = fread( buffer, 1, sizeof(buffer), fin )) )
      {
         fwrite( buffer, 1, dataread, fout );
      }
      fclose( fin );
   }
   fclose( fout );
   return 0;
}
