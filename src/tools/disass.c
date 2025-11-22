
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
#include "../rp2040/mcurses/mcurses_disassemble.c"

#include "../rp2040/mcurses/mcurses.h"


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
   dav.cpu        = cpu;
   dav.bank       = 0;
   dav.address    = address;

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
   uint16_t address;

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
