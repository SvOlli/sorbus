
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define min(a,b) ((a) < (b) ? (a) : (b))

const uint8_t maxline = 0x18;


void writeblock( uint16_t addr, uint16_t size, uint8_t *data )
{
   uint16_t a = 0;
   uint8_t *d = data;
   uint8_t i;
   uint16_t chksum;

   //printf( "WH %04X %04X\n", addr, addr+size-1 );
   printf( "LH\r" );
   fflush( stdout );
   usleep( 1000 );

   for( a = 0; a < size; a += maxline )
   {
      uint8_t line = min( maxline, size - a );
      printf( ";%02X%04X", line, addr + a );
      chksum = line + ((addr + a) & 0xff) + ((addr + a) >> 8);
      for( i = 0; i < line; ++i )
      {
         printf( "%02X", *d );
         chksum += *d;
         ++d;
      }
      printf( "%04X\r", chksum );
      fflush( stdout );
      usleep( 12500 );
   }
   printf( ";00" );
   fflush( stdout );
   usleep( 1000 );
}


void gototim()
{
   putchar( 0x1d ); // ctrl+] -> meta menu
   fflush( stdout );
   usleep( 10000 );
   putchar( 'r' ); // reboot
   fflush( stdout );
   usleep( 10000 );
   putchar( 't' ); // TIM
   fflush( stdout );
   usleep( 10000 );
}


void gotoaddr( uint16_t addr )
{
   printf( "R:%04X\rG", addr );
   fflush( stdout );
}


int main( int argc, char *argv[] )
{
   FILE *f;
   uint8_t mem[0x10000];
   size_t size;
   unsigned long addr;

   if( argc != 3 )
   {
      fprintf( stderr, "usage: %s <startaddr> <filename>\n", argv[0] );
      return 1;
   }
   addr = strtoul( argv[1], 0, 0 );
   f = fopen( argv[2], "rb" );
   if( !f )
   {
      fprintf( stderr, "%s: %s\n", argv[2], strerror(errno) );
      return 10;
   }
   size = fread( &mem[0], 1, sizeof(mem), f );
   fclose( f );
   gototim();
   writeblock( addr, size, &mem[0] );
   gotoaddr( addr );
}

