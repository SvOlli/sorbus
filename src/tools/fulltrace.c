
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../rp2040/disassemble/disassemble.c"
#include "../rp2040/disassemble/disassemble_beancounter.c"
#include "../rp2040/disassemble/disassemble_fulltrace.c"
#include "../rp2040/disassemble/disassemble_historian.c"

uint8_t *loadfile( const char *filename, ssize_t *filesize );

//#include "loadfile.c"

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
     , progname );
   exit( retval );
}


uint32_t next_pow2( uint32_t in )
{
   for( uint32_t i = 1; i; i <<= 1 )
   {
      if( i >= in )
      {
         return i;
      }
   }
   return 0;
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


uint64_t *get_fulltrace( const uint8_t *start, const uint8_t *end, uint32_t *size )
{
   const uint8_t *c;
   uint32_t entries = 0;
   uint64_t *sample = 0, *s;

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
   sample = (uint64_t*)calloc( next_pow2(entries), sizeof(uint64_t) );

   s = sample;
   for( c = start; c < end; ++c )
   {
      errno = 0;
      *(s++) = strtoll( (const char*)c, 0, 16 );
      if( errno )
      {
         perror( "strtoll" );
         exit(30);
      }
      while( *c != '\n' )
      {
         ++c;
      }
   }
   if( size )
   {
      *size = next_pow2(entries);
   }
   return sample;
}


uint32_t *fulltrace2trace( uint64_t *refbuffer, uint32_t size )
{
   int i;
   uint32_t *buffer = (uint32_t*)malloc( sizeof(uint32_t) * size );
   uint32_t *u32 = buffer;
   uint64_t *u64 = refbuffer;

   for( i = 0; i < size; ++i )
   {
      *(u32++) = (uint32_t)*(u64++);
   }
   return buffer;
}


void print_result( uint64_t *refbuffer, uint64_t *checkbuffer, uint32_t size )
{
   uint32_t i;
   uint64_t *r = refbuffer;
   uint64_t *c = checkbuffer;
   fullinfo_t fi;
   for( i = 0; i < size; ++i )
   {
      fi.raw = *c;
      printf( "%016lx %016lx %c %s\n", *r, *c, *r != *c ? '!' : ' ',
              disass_trace( fi ) );
      ++r;
      ++c;
   }
}


int main( int argc, char* argv[] )
{
   const char *progname = argv[0];

   cputype_t cpu = CPU_ERROR;
   disass_fulltrace_t dah;

   int opt;
   bool fail = false;

   const char *filename = 0;
   uint8_t *filedata = 0;
   ssize_t filesize;

   const uint8_t *start, *end;
   uint32_t size;
   uint64_t *refbuffer = 0;
   uint32_t *buffer = 0;

   while ((opt = getopt(argc, argv, "ac:df:hp")) != -1)
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
      filedata  = loadfile( filename, &filesize );
      start     = get_start( filedata, filedata + filesize, &cpu );
      end       = get_end( start, filedata + filesize );
      refbuffer = get_fulltrace( start, end, &size );
      if( filedata )
      {
         free( filedata );
      }
   }
   if( fail )
   {
      return 1;
   }
   buffer = fulltrace2trace( refbuffer, size );
   dah = disass_fulltrace_init( cpu, buffer, size, 0 );

   print_result( refbuffer, (uint64_t*)(dah->fullinfo)+BOUNDSBUFFER, size );
   
   disass_fulltrace_done( dah );
   free( buffer );
   free( refbuffer );

   return 0;
}
