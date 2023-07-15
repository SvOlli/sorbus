
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char *argv[] )
{
   uint8_t *buffer;
   long filesize, i;
   FILE *f;

   if( argc < 4 )
   {
      fprintf( stderr, "usage: %s <infile> <outfile> <label>\n", argv[0] );
      return 0;
   }

   f = fopen( argv[1], "rb" );
   if( !f )
   {
      perror( "open for read failed" );
      return 10;
   }
   if( fseek( f, 0, SEEK_END ) )
   {
      perror( "fseek failed" );
      return 11;
   }
   if( (filesize = ftell( f )) < 0 )
   {
      perror( "ftell failed" );
      return 12;
   }
   if( !filesize )
   {
      fprintf( stderr, "file has zero size\n" );
      return 13;
   }
   fseek( f, 0, SEEK_SET );
   buffer = (uint8_t*)malloc( filesize );
   if( !buffer )
   {
      perror( "malloc failed" );
      return 14;
   }
   if( !fread( buffer, filesize, 1, f ) )
   {
      perror( "fread failed" );
      return 15;
   }
   fclose( f );

   f = fopen( argv[2], "wb" );
   if( !f )
   {
      perror( "open for write failed" );
      return 10;
   }
   fprintf( f, "const uint8_t %s[] = {\n", argv[3] );
   for( i = 0; i < filesize; ++i )
   {
      if( (i & 0x07) == 0 )
      {
         fprintf( f, "   0x%02x%s", buffer[i], i < filesize-1 ? "," : "\n" );
      }
      else if( (i & 0x07) == 0x07 )
      {
         fprintf( f, "0x%02x%s\n", buffer[i], i < filesize-1 ? "," : "" );
      }
      else
      {
         fprintf( f, "0x%02x%s", buffer[i], i < filesize-1 ? "," : "\n" );
      }
   }
   fprintf( f, "};\n" );
   fclose( f );

   if( buffer )
   {
      free( buffer );
   }

   return 0;
}

