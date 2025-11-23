
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define SHOW_CONFIDENCE 1

typedef unsigned int uint;

#define BUS_H BUS_H

typedef struct {
   uint32_t mask_address;  // input:  up to 16 contiguous bits
   uint32_t mask_data;     // both:   8 contiguous bits
   uint32_t mask_rw;       // input:  1 bit
   uint32_t mask_clock;    // output: 1 bit
   uint32_t mask_rdy;      // output: 1 bit (could be also input in the future)
   uint32_t mask_irq;      // output: 1 bit (could be also input in the future)
   uint32_t mask_nmi;      // output: 1 bit (could be also input in the future)
   uint32_t mask_reset;    // output: 1 bit (could be also input in the future)
   uint32_t mask_input;    // convenience
   uint32_t mask_output;   // convenience
   uint     shift_data;
   uint     shift_address;
} bus_config_t;

const bus_config_t bus_config = {
   .mask_address  = 0x0000FFFF,  // input:  16 contiguous bits
   .mask_data     = 0x00FF0000,  // both:   8 contiguous bits
   .mask_rw       = 0x01000000,  // input:  1 bit
   .mask_clock    = 0x02000000,  // output: 1 bit
   .mask_rdy      = 0x04000000,  // output: 1 bit (could be also input in the future)
   .mask_irq      = 0x08000000,  // output: 1 bit (could be also input in the future)
   .mask_nmi      = 0x10000000,  // output: 1 bit (could be also input in the future)
   .mask_reset    = 0x20000000,  // output: 1 bit (could be also input in the future)
   .mask_input    = 0x01FFFFFF,  // convenience
   .mask_output   = 0x3EFF0000,  // convenience
   .shift_data    = 16,
   .shift_address = 0
};

/* alternative rp2040_purple notations for faster access (gains ~7%) */
#define BUS_CONFIG_mask_address  (0x0000FFFF)
#define BUS_CONFIG_mask_data     (0x00FF0000)
#define BUS_CONFIG_mask_rw       (0x01000000)
#define BUS_CONFIG_mask_clock    (0x02000000)
#define BUS_CONFIG_mask_rdy      (0x04000000)
#define BUS_CONFIG_mask_irq      (0x08000000)
#define BUS_CONFIG_mask_nmi      (0x10000000)
#define BUS_CONFIG_mask_reset    (0x20000000)
#define BUS_CONFIG_mask_input    (0x01FFFFFF)
#define BUS_CONFIG_mask_output   (0x3EFF0000)
#define BUS_CONFIG_shift_data    (16)
#define BUS_CONFIG_shift_address (0)

#include "../rp2040/common/generic_helper.c"
#include "../rp2040/common/disassemble.c"
#include "../rp2040/common/disassemble_historian.c"
#include "../rp2040/mcurses/mc_historian.c"
#include "loadfile.c"

#include "../rp2040/mcurses/mcurses.h"


uint32_t mf_checkheap()
{
   return 0;
}


cputype_t getcputype( const char *argi )
{
   cputype_t retval = CPU_ERROR;
   char *arg = strdup( argi );
   char *c;

   for( c = arg; *c; ++c )
   {
      *c = toupper( *c );
   }

   if( !strncasecmp( arg, "6502", 4 ) )
   {
      retval = CPU_6502;
   }
   else if( !strncasecmp( arg, "65C02", 5 ) )
   {
      retval = CPU_65C02;
   }
   else if( !strncasecmp( arg, "65SC02", 6 ) )
   {
      retval = CPU_65SC02;
   }
   else if( !strncasecmp( arg, "65816", 5) )
   {
      retval = CPU_65816;
   }
   else if( !strncasecmp( arg, "65CE02", 6 ) )
   {
      retval = CPU_65CE02;
   }

   free( arg );
   return retval;
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
   disass_historian_t dah;

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
      dah = disass_historian_init( cpu, buffer, size, 0 );
      for( count = 0; count < size; ++count )
      {
         if( ! *(buffer + count) )
         {
            /* empty entry -> end of input */
            break;
         }
         printf( "%5d:", count );
         if( disass_historian_entry( dah, count ) )
         {
            puts( disass_historian_entry( dah, count ) );
         }
      }
      disass_historian_done( dah );
   }
   free( buffer );

   return 0;
}
