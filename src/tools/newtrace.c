
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "da_trace.h"
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
     "%s: test tool for historian disassembler\n"
     "\t-c cpu:\tcputype (mandatory, if not specified in file)\n"
     "\t-f file:\ttrace file (mandatory)\n"
     , progname );
   exit( retval );
}


void print_result( const uint32_t *opcodes, da_fullinfo_t *refbuffer,
                   da_trace_t dah, uint32_t size )
{
   static char buffer[128] = { 0 };
   char *b = &buffer[0];
   size_t bsize = sizeof(buffer) - 1;
   size_t used  = 0;
   uint32_t i;
   da_fullinfo_t *r = refbuffer;
   da_fullinfo_t *c = dah->fullinfo;

   for( i = 0; i < size; ++i )
   {
      if( (!c->raw) || (!r->raw) )
      {
         break;
      }
      used  = 0;
      used += snprintf( b+used, bsize-used, "%4d:", i );
      used += da_snf_fullinfo( b+used, bsize-used, dah->cpu, "[a w d RNIS] ",  *c, DA_FLAG_NONE );
      used += da_snf_fullinfo( b+used, bsize-used, dah->cpu, "e:[dx] ",        *r, DA_FLAG_NONE );
      used += da_snf_fullinfo( b+used, bsize-used, dah->cpu, "e:[dx]E n cC y", *c, DA_FLAG_NONE );
      puts( b );
      ++r;
      ++c;
   }
}


int main( int argc, char* argv[] )
{
   const char *progname = argv[0];

   cputype_t cpu = CPU_ERROR;
   da_trace_t dah;

   int opt;
   bool fail = false;

   const char *filename = 0;
   uint8_t *filedata = 0;
   ssize_t filesize;

   char *start;
   uint32_t size;
   da_fullinfo_t *refbuffer = 0;
   uint32_t *buffer = 0;

   while ((opt = getopt(argc, argv, "ac:df:hp")) != -1)
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
      start     = trace_get_start( (char*)filedata, &cpu );
      refbuffer = trace_get_fulltrace( start, &size );
      free( filedata );
   }
   if( fail )
   {
      return 1;
   }
   buffer = fulltrace2trace( refbuffer, size );
   dah = da_trace_init( cpu, buffer, size, 0 );

   if( da_cc_start( dah, 0 ) )
   {
      print_result( dah->opcodes,
                    refbuffer,
                    dah,
                    size );
   }
   else
   {
      printf( "da_cc_trace() failed\n" );

      print_result( dah->opcodes,
                    refbuffer,
                    dah,
                    size );
   }

   da_trace_done( dah );
   free( buffer );
   free( refbuffer );

   return 0;
}
