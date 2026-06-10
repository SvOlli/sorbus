
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "sorbus_rte.h"
#include "da_trace.h"

#include "mcurses.h"


void help( const char *progname, int retval )
{
   FILE *f = stdout;
   const char *c;

   for( c = progname; *c; ++c )
   {
      if( (*(c) == '/') && (*(c+1)) )
      {
         progname = c+1;
      }
   }

   if( retval )
   {
      f = stderr;
   }

   fprintf( f,
     "%s: test tool for tracing disassembler\n"
     "\t-c cpu:\t\tcputype (mandatory, if not specified in file)\n"
     "\t-f file:\ttrace file (mandatory)\n"
     "\t-h:\t\tshow help\n"
     , progname );
   exit( retval );
}


uint32_t *get_trace( const uint8_t *start, const uint8_t *end, uint32_t *size )
{
   const uint8_t *c;
   uint32_t entries = 0;
   uint32_t *sample = 0, *s;

   for( c = start; c < end; ++c )
   {
      if( *c == '\n' )
      {
         ++entries;
      }
   }
   if( !(*(end-1) == '\n') && !(*(end-2) == '\n') )
   {
      ++entries;
   }
   /* entries needs to be power of 2 */
   sample = (uint32_t*)calloc( entries, sizeof(uint32_t) );

   s = sample;
   for( c = start; c < end; ++c )
   {
      errno = 0;
      *(s++) = strtol( (const char*)c, 0, 16 );
      if( errno )
      {
         perror( "strtol" );
         exit(30);
      }
      while( *c != '\n' )
      {
         ++c;
      }
   }
   if( size )
   {
      *size = entries;
   }
   return sample;
}


int main( int argc, char *argv[] )
{
   const char *progname = argv[0];
   cputype_t cpu = CPU_ERROR;

   const char *start;
   uint32_t size;
   uint32_t *buffer = 0;

   const char *filename = 0;
   uint8_t *filedata = 0;
   ssize_t filesize;

   struct termios oldt, newt;

   int opt;
   bool fail = false;

   while ((opt = getopt(argc, argv, "c:f:h")) != -1)
   {
      switch( opt )
      {
         case 'c':
            cpu = text2cputype( optarg );
            if( cpu == CPU_ERROR )
            {
               fprintf( stderr, "unknown cpu: %s\n", optarg );
               fprintf( stderr, "known CPUs: 6502, 65C02, 65SC02, 65816, 65CE02\n" );
               fail = true;
            }
            break;
         case 'f':
            filename = optarg;
            break;
         case 'h':
            help( progname, 0 );
            break;
         default:
            fail = true;
            break;
      }
   }

   if( !filename )
   {
      fprintf( stderr, "filename not set\n" );
      fail = true;
   }
   if( !fail )
   {
      filedata = loadfile( filename, &filesize );
      if( !filedata )
      {
         fprintf( stderr, "error loading '%s': %s\n", filename, strerror(errno) );
         exit( 10 );
      }
      start    = trace_get_start( (char*)filedata, &cpu );
      buffer   = trace_get_trace( start, &size );
      free( filedata );
   }

   if( cpu == CPU_ERROR )
   {
      fprintf( stderr, "CPU type not set\n" );
      fail = true;
   }

   if( fail )
   {
      help( progname, 1 );
   }

   tcgetattr( STDIN_FILENO, &oldt );
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr( STDIN_FILENO, TCSANOW, &newt );

   screen_save();
   initscr();

   mcurses_trace( cpu, buffer, size, 0 );

   endwin();
   screen_restore();

   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

   free( buffer );

   return 0;
}
