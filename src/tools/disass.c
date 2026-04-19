
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "sorbus_rte.h"

#define SHOW_CONFIDENCE 1

typedef unsigned int uint;

#include "../rp2040/disassemble/disassemble.c"
#include "../rp2040/mcurses/mc_disass.c"

#include "../rp2040/mcurses/mcurses.h"


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
     "%s: test tool for testing disassembler\n"
     "\t-c cpu:\tcputype (mandatory)\n"
     "\t-f file:\ttrace file (mandatory)\n"
     "\t-a addr:\tstart addresses of file in memory\n"
     , progname );
   exit( retval );
}


void mcurses( cputype_t cpu, uint16_t address )
{
   struct termios oldt, newt;
   mc_disass_t dav   = { 0 };

   dav.banks      = debug_banks;
   dav.peek       = debug_peek;
   dav.charset    = 1;
   dav.cpu        = cpu;
   dav.bank       = 0;
   dav.address    = address;
   dav.m816       = false;
   dav.x816       = false;

   tcgetattr( STDIN_FILENO, &oldt );
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr( STDIN_FILENO, TCSANOW, &newt );

   screen_save();
   initscr();

   mcurses_disassemble( &dav );
   endwin();
   screen_restore();

   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
}


int main( int argc, char *argv[] )
{
   const char *progname = argv[0];
   cputype_t cpu = CPU_ERROR;

   const char *filename = 0;
   uint16_t address = 0;

   int opt;
   bool fail = false;

   while ((opt = getopt(argc, argv, "a:c:f:h")) != -1)
   {
      switch( opt )
      {
         case 'a':
            address = strtol( optarg, 0, 0 );
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

   if( cpu == CPU_ERROR )
   {
      fprintf( stderr, "CPU type not set\n" );
      fail = true;
   }
   if( !filename )
   {
      fprintf( stderr, "filename not set\n" );
      fail = true;
   }
   if( !fail )
   {
      fail = !debug_loadfile( address, filename );
   }

   if( fail )
   {
      help( progname, 1 );
   }

   mcurses( cpu, address );
   return 0;
}
