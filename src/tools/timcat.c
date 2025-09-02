
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define min(a,b) ((a) < (b) ? (a) : (b))

const uint8_t maxline = 0x20; // was originally 0x18 -> option?


void writewoz( uint16_t addr, uint16_t size, uint8_t *data )
{
   int i;
   uint8_t *c;

   /* when we're not starting at an 8 byte boundry, write address */
   if( addr & 7 )
   {
      printf( "%04X:", addr );
   }

   for( i = 0, c = data; i < size; ++i, ++c )
   {
      if( !(addr & 7) )
      {
         /* print address at 8 byte boundry */
         printf( "%04X:", addr );
      }

      /* print data, at end of line with return */
      printf( "%02x%c", *c,
              ( (addr & 7) == 7 ) ? '\r' : ' ' );
      fflush( stdout );
      if( (addr & 7) == 7 )
      {
         /* give WozMon some time to prozess line */
         usleep( 10000 );
      }
      ++addr;
   }
}

/*
 * papertape format (line based, all data hex, except for start):
 * - start: 0x3A (":") or 0x3B (";") (in TIM 0x3B is written, but 0x3A is read)
 * - 1 hexbyte: datasize
 * - 2 hexbyte: address
 * - n hexbyte: data written to address (n=datasize)
 * - 2 hexbyte: checksum (added over all bytes read, including datasize & address)
 */
void writeppt( uint16_t addr, uint16_t size, uint8_t *data )
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
      //usleep( 15000 );
   }
   printf( ";00" );
   fflush( stdout );
   usleep( 1000 );
}


void rebootinto( uint8_t choice )
{
   putchar( 0x1d ); // ctrl+] -> meta menu
   fflush( stdout );
   usleep( 10000 );
   putchar( 'r' ); // reboot
   fflush( stdout );
   usleep( 10000 );
   putchar( choice ); // TIM
   fflush( stdout );
   usleep( 20000 );
}


void timgoto( uint16_t addr )
{
   printf( "R:%04X\rG", addr );
   fflush( stdout );
}


void sysmongoto( uint16_t addr )
{
   printf( "g%04X\r", addr );
   fflush( stdout );
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
   if( argc != 3 )
   {
      fprintf( stderr, "usage: %s <startaddr> <filename>\n", argv[0] );
      return 1;
   }
   addr = strtoul( argv[1], 0, 0 );
   if( addr < 0x300 )
   {
      fprintf( stderr, "warning: addresses below 0x300 not supported\n" );
   }
   f = fopen( argv[2], "rb" );
   if( !f )
   {
      fprintf( stderr, "%s: %s\n", argv[2], strerror(errno) );
      return 10;
   }
   size = fread( &mem[0], 1, sizeof(mem), f );
   fclose( f );
   rebootinto( 't' );
   writeppt( addr, size, &mem[0] );
   timgoto( addr );
}

