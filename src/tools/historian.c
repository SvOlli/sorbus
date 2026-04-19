
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define SHOW_CONFIDENCE 1

#include "../rp2040/disassemble/disassemble.c"
#include "../rp2040/disassemble/disassemble_beancounter.c"
#include "../rp2040/disassemble/disassemble_fulltrace.c"
#include "../rp2040/disassemble/disassemble_historian.c"
#include "../rp2040/mcurses/mc_historian.c"
#include "loadfile.c"

#include "../rp2040/mcurses/mcurses.h"


uint32_t mf_checkheap()
{
   return 0;
}


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
     "\t-a:\tshow addresses\n"
     "\t-d:\tshow hexdump\n"
     "\t-p:\tinteractive pager\n"
     "\t-h:\tshow help\n"
     , progname );
   exit( retval );
}


void mcurses( cputype_t cpu, uint32_t *trace, uint32_t entries, uint32_t start )
{
   struct termios oldt, newt;
   uint32_t count;

   tcgetattr( STDIN_FILENO, &oldt );
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr( STDIN_FILENO, TCSANOW, &newt );

   screen_save();
   initscr();
   for( count = 0; count < entries; ++count )
   {
      if( ! *(trace + count) )
      {
         /* empty entry -> end of input */
         break;
      }
   }
   mcurses_historian( cpu, trace, count, start );
   endwin();
   screen_restore();

   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
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
   if( !(*end == '\n') && !(*(end-1) == '\n') )
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
   disass_fulltrace_t dah;

   const uint8_t *start, *end;
   uint32_t size;
   uint32_t *buffer = 0;

   int count = 0;
   const char *filename = 0;
   uint8_t *filedata = 0;
   ssize_t filesize;

   int opt;
   bool fail = false;
   bool pager = false;
   disass_show_t show_extra = DISASS_SHOW_NOTHING;

   while ((opt = getopt(argc, argv, "ac:df:hp")) != -1)
   {
      switch( opt )
      {
         case 'a':
            show_extra |= DISASS_SHOW_ADDRESS;
            break;
         case 'c':
            cpu = getcputype( optarg );
            if( cpu == CPU_ERROR )
            {
               fprintf( stderr, "unknown cpu: %s\n", optarg );
               fprintf( stderr, "known CPUs: 6502, 65C02, 65SC02, 65816, 65CE02\n" );
               fail = true;
            }
            break;
         case 'd':
            show_extra |= DISASS_SHOW_HEXDUMP;
            break;
         case 'f':
            filename = optarg;
            break;
         case 'p':
            pager = true;
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

   disass_show( show_extra );
   if( pager )
   {
      mcurses( cpu, buffer, size, 0 );
   }
   else
   {
      dah = disass_fulltrace_init( cpu, buffer, size, 0 );
      disass_historian_assumptions( dah );
      for( count = 0; count < size; ++count )
      {
         if( ! *(buffer + count) )
         {
            /* empty entry -> end of input */
            break;
         }
         printf( "%5d:", count );
         if( disass_fulltrace_entry( dah, count ) )
         {
            puts( disass_fulltrace_entry( dah, count ) );
         }
      }
      disass_fulltrace_done( dah );
   }
   free( buffer );

   return 0;
}
