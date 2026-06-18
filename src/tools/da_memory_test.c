
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "da_memory.h"

#include "sorbus_rte.h"


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


void disassemble( cputype_t cpu, uint16_t address, uint16_t size )
{
   da_fullinfo_t fullinfo;
   char buffer[256];
   uint16_t a = address;
   da_memory_t dam;
   
   dam = da_memory_init( memsim_peek );
   dam->cpu = cpu;

   while( a < (address+size) )
   {
      char *b = &buffer[0];
      int bsize = sizeof(buffer)-1;
      int used = 0;
      memset( b, 0, sizeof(buffer) );

#if 1
      fullinfo = da_memory_single( cpu, memsim_peek, 0, a, false, false );
#else
      fullinfo.raw      = 0x3F000000;
      fullinfo.address  = a;
      fullinfo.data     = memsim_peek( 0, a   );
      fullinfo.data1    = memsim_peek( 0, a+1 );
      fullinfo.data2    = memsim_peek( 0, a+2 );
      fullinfo.data3    = memsim_peek( 0, a+3 );
      fullinfo.dataused = 3;
      fullinfo.eval     = DA_EVAL_MAX;
#endif

      used += snprintf( b+used, bsize-used, "%04X ",
                        da_memory_findprev( dam, a ) );
      used += da_snf_fullinfo( b+used, bsize-used, cpu, "A: DO tT cC y",
                               fullinfo, DA_FLAG_NONE );
      printf( "%s\n", &buffer[0] );
      a += da_pick_bytes( cpu, memsim_peek( 0, a ) );
   }
   
   da_memory_done( dam );
}


int main( int argc, char *argv[] )
{
   const char *progname = argv[0];
   cputype_t cpu = CPU_ERROR;

   const char *filename = 0;
   uint16_t address = 0;
   uint16_t filesize;

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

   filesize = memsim_filesize( filename );
   printf( "disassembly: $%04x-$%04x\n", address, address+filesize );
   disassemble( cpu, address, filesize );
   return 0;
}
