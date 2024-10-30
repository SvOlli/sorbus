
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#include "../rp2040/generic_helper.c"
#include "../rp2040/disassemble.c"
#include "../rp2040/disassemble_historian.c"


cputype_t getcputype( const char *argi )
{
   cputype_t retval = CPU_ERROR;
   char *arg = strdup( argi );
   char *c;

   for( c = arg; *c; ++c )
   {
      *c = toupper( *c );
   }
   
   if( !strcmp( arg, "6502" ) )
   {
      retval = CPU_6502;
   }
   else if( !strcmp( arg, "65C02" ) )
   {
      retval = CPU_65C02;
   }
   else if( !strcmp( arg, "65SC02" ) )
   {
      retval = CPU_65SC02;
   }
   else if( !strcmp( arg, "65816" ) )
   {
      retval = CPU_65816;
   }
   else if( !strcmp( arg, "65CE02" ) )
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
     "\t-e:\tshow estimated confidence\n"
     "\t-t:\tshow trace info\n"
     "\t-h:\tshow help\n"
     , progname );
   exit( retval );
}

int main( int argc, char *argv[] )
{
   const char *progname = argv[0];
   cputype_t cpu = CPU_ERROR;
   disass_historian_t dah;
   int size = 16;
   int count = 0;
   int elements;
   const char *filename = 0;
   FILE *f;
   uint32_t *buffer = malloc( sizeof(uint32_t) * size );

   int opt;
   bool fail = false;
   bool show_trace = false;
   bool show_confidence = false;
   disass_show_t show_extra = DISASS_SHOW_NOTHING;

   while ((opt = getopt(argc, argv, "ac:def:ht")) != -1)
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
         case 'e':
            show_confidence = true;
            break;
         case 'f':
            filename = optarg;
            break;
         case 'h':
            help( progname, 0 );
            break;
         case 't':
            show_trace = true;
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

   if( fail )
   {
      help( progname, 1 );
   }
   
   f = fopen( filename, "rb" );
   if( !f )
   {
      fprintf( stderr, "could not read file '%s': %s\n",
               filename, strerror(errno) );
      return 10;
   }
   while(!feof(f))
   {
      if( count >= size )
      {
         size <<= 1;
         buffer = realloc( buffer, sizeof(uint32_t) * size );
      }
      elements = fscanf( f, "%x\n", buffer + count );
      if( elements < 1 )
      {
         fprintf( stderr, "could not scan file '%s' (line %d): %s\n",
                  filename, count+1, strerror(errno) );
         fclose( f );
         return 10;
      }
      ++count;
   }
   fclose( f );

   disass_show( show_extra );
   dah = disass_historian_init( cpu, buffer, size, 0 );
   for( count = 0; count < size; ++count )
   {
      printf( "%5d:", count );
      if( show_trace )
      {
         printf( "%s:",
                 decode_trace( *(buffer + count), false, 0 ) );
      }
      if( disass_historian_entry( dah, count ) )
      {
         printf( "%s\n", show_confidence ?
                 disass_historian_entry( dah, count ) :
                 disass_historian_entry( dah, count ) + 2 );
      }
   }
   disass_historian_done( dah );
   free( buffer );
   
   return 0;
}
