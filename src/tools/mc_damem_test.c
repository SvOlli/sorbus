
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "sorbus_rte.h"
#include "da_memory.h"

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
     "%s: test tool for testing disassembler\n"
     "\t-c cpu:\tcputype (mandatory)\n"
     "\t-f file:\tbinary file (mandatory)\n"
     "\t-a addr:\tstart addresses of file in memory\n"
     , progname );
   exit( retval );
}




int main( int argc, char *argv[] )
{
   const char *progname = argv[0];
   cputype_t cpu = CPU_ERROR;
   mc_damem_t  *mcd = (mc_damem_t*)malloc( sizeof( mc_damem_t ) );
   da_memory_t dam;

   const char *filename = 0;
   uint16_t address = 0;

   int opt;
   bool fail = false;

   struct termios oldt, newt;

   while ((opt = getopt(argc, argv, "a:c:f:h")) != -1)
   {
      switch( opt )
      {
         case 'a':
            address = strtol( optarg, 0, 0 );
            break;
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
      fail = !memsim_loadfile( address, filename );
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

   mcd->banks        = memsim_banks();
   mcd->dam          = da_memory_init();
   mcd->show_hex     = true;
   mcd->show_text    = true;
   mcd->show_cycles  = true;

   //screen_get_size( 0, 0 );
   //da_memory_linecache( mcd->dam, screen_get_lines() );
   da_memory_linecache( mcd->dam, 24 );
   dam = mcd->dam;
   dam->address = address;
   dam->bank    = 0;
   dam->peek    = memsim_peek;
   dam->cpu     = cpu;

   mcurses_damem( mcd );

   endwin();
   screen_restore();

   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

   da_memory_done( dam );
   free( mcd );
   return 0;
}
