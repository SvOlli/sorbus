
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define min(a,b) ((a) < (b) ? (a) : (b))

const uint8_t maxline = 0x20; // was originally 0x18 -> option?


/*
 * papertape format (line based, all data hex, except for start):
 * - start: 0x3A (":") or 0x3B (";") (in TIM 0x3B is written, but 0x3A is read)
 * - 1 hexbyte: datasize
 * - 2 hexbyte: address
 * - n hexbyte: data written to address (n=datasize)
 * - 2 hexbyte: checksum (added over all bytes read, including datasize & address)
 */
void writeppt( FILE *f, uint16_t addr, uint16_t size, uint8_t *data )
{
   uint16_t a = 0;
   uint8_t *d = data;
   uint8_t i;
   uint16_t chksum;

   //printf( "WH %04X %04X\n", addr, addr+size-1 );
   fprintf( f, "LH\r" );
   fflush( f );
   usleep( 1000 );

   for( a = 0; a < size; a += maxline )
   {
      uint8_t line = min( maxline, size - a );
      fprintf( f, ";%02X%04X", line, addr + a );
      chksum = line + ((addr + a) & 0xff) + ((addr + a) >> 8);
      for( i = 0; i < line; ++i )
      {
         fprintf( f, "%02X", *d );
         chksum += *d;
         ++d;
      }
      fprintf( f, "%04X\r", chksum );
      fflush( f );
      //usleep( 15000 );
   }
   fprintf( f, ";00" );
   fflush( f );
   usleep( 1000 );
}


void rebootinto( FILE *f, uint8_t choice )
{
   fputc( 0x1d, f ); // ctrl+] -> meta menu
   fflush( f );
   usleep( 10000 );
   fputc( 'r', f ); // reboot
   fflush( f );
   usleep( 10000 );
   fputc( choice, f );
   fflush( f );
   usleep( 20000 );
}


void smgoto( FILE *f, uint16_t addr )
{
   fprintf( f, "GM" );
   fflush( f );
   usleep( 250000 );
   fprintf( f, "G%04X\r", addr );
   fflush( f );
}


int main( int argc, char *argv[] )
{
   FILE *f;
   uint8_t mem[0x10000];
   size_t size;
   unsigned long addr;

   /*
    * TODO: implement arguments
    * - reboot into TIM optional
    * - unify wozcat and timcat?
    * - startaddress optional
    * - uart code?
    * - papertape chunk length
    */
   if( argc != 4 )
   {
      fprintf( stderr, "usage: %s <device> <startaddr> <filename>\n", argv[0] );
      return 1;
   }
   addr = strtoul( argv[2], 0, 0 );
   if( addr < 0x300 )
   {
      fprintf( stderr, "warning: addresses below 0x300 not supported\n" );
   }
   f = fopen( argv[3], "rb" );
   if( !f )
   {
      fprintf( stderr, "%s: %s\n", argv[2], strerror(errno) );
      return 10;
   }
   size = fread( &mem[0], 1, sizeof(mem), f );
   fclose( f );
   f = fopen( argv[1], "wb" );
   rebootinto( f, 't' );
   writeppt( f, addr, size, &mem[0] );
   smgoto( f, addr );
   fclose( f );
}

