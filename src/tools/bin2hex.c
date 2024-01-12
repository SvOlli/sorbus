
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
   uint8_t *buffer;
   uint32_t addr;
   long filesize, i;
   FILE *f;

   if( argc < 3 )
   {
      fprintf( stderr, "usage: %s <infile> <startaddr> (<outfile>)\n", argv[0] );
      return 0;
   }
   errno = 0;
   addr = strtoul( argv[2], (char**)&buffer, 0 );
   if( errno )
   {
      perror( "start address" );
      return 9;
   }
   if( buffer && strlen((char*)buffer) )
   {
      fprintf( stderr, "start address not a number\n" );
      return 9;
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
   if( (addr + filesize) > 0x10000 )
   {
      fprintf( stderr, "out of address space\n" );
      return 8;
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

   if( argc > 3 )
   {
      f = fopen( argv[3], "wb" );
   }
   else
   {
      f = stdout;
   }
   if( !f )
   {
      perror( "open for write failed" );
      return 10;
   }

   if( addr & 0x07 )
   {
      fprintf( f, "%04x:", addr );
   }
   for( i = 0; i < filesize; ++i )
   {
      if( (addr & 0x07) == 0 )
      {
         fprintf( f, "%04x:", addr );
      }
      fprintf( f, " %02x", buffer[i] );
      usleep( 150 );
      if( (addr & 0x07) == 0x07 )
      {
         if( f == stdout )
         {
            fprintf( f, "\r\n" );
            usleep( 15000 );
         }
         else
         {
            fprintf( f, "\n" );
         }
      }
      addr++;
   }
   if( addr & 0x07 )
   {
      if( f == stdout )
      {
         fprintf( f, "\r\n" );
      }
      else
      {
         fprintf( f, "\n" );
      }
   }
   fclose( f );

   if( buffer )
   {
      free( buffer );
   }

   return 0;
}
