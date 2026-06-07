
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define SHOW_CONFIDENCE 1

#include "sorbus_rte.h"

#include "da_base.h"
#include "da_trace.h"

#include "mcurses.h"
#include "mc_trace.c"


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
     "%s: test tool for historian disassembler\n"
     "\t-c cpu:\tcputype (mandatory)\n"
     "\t-f file:\ttrace file (mandatory)\n"
     "\t-h:\tshow help\n"
     , progname );
   exit( retval );
}


const uint8_t *get_start( const uint8_t *start, const uint8_t *end, cputype_t *cputype )
{
   const uint8_t *c;
   const uint8_t magic[] = "TRACE_START";

   for( c = start; c < end - sizeof(magic)-1; ++c )
   {
      if( !memcmp( c, &magic[0], sizeof(magic)-1 ) )
      {
         break;
      }
   }
   /* nothing found: assume data start at file start */
   if( c == (end - sizeof(magic)-1) )
   {
      return start;
   }

   c += sizeof(magic)-1;
   switch( *c )
   {
      case ' ':
         if( cputype )
         {
            if( *cputype == CPU_ERROR )
            {
               if( (end - c) > 6 )
               {
                  *cputype = getcputype( (const char*)(c+1) );
               }
            }
         }
         while( *(c++) != '\n' )
         {
            if( c >= end )
            {
               return end;
            }
         }
         return c;
      case '\n':
         return c+1;
      default:
         return end;
   }
}


const uint8_t *get_end( const uint8_t *start, const uint8_t *end )
{
   const uint8_t *c;
   const uint8_t magic[] = "TRACE_END";

   for( c = start; c < end - sizeof(magic)-1; ++c )
   {
      if( !memcmp( c, &magic[0], sizeof(magic)-1 ) )
      {
         return c;
      }
   }
   return end;
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

   const uint8_t *start, *end;
   uint32_t size;
   uint32_t *buffer = 0;

   int count = 0;
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
            cpu = getcputype( optarg );
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
      start    = get_start( filedata, filedata + filesize, &cpu );
      end      = get_end( start, filedata + filesize );
      buffer   = get_trace( start, end, &size );
      if( filedata )
      {
         free( filedata );
      }
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
