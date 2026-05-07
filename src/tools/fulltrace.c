
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../rp2040/disassemble/beancounter.c"
#include "../rp2040/disassemble/beancounter16.c"
#include "../rp2040/disassemble/disassemble.c"
#include "../rp2040/disassemble/fulltrace.c"
#include "../rp2040/disassemble/historian.c"

/* todo: find header */
uint8_t *loadfile( const char *filename, ssize_t *filesize );


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

#if 0
typedef union
{
   uint64_t       raw      :64;
   struct {
      /* lower 32bit are the same as trace from GPIOs */
      uint16_t    address  :16;
      uint8_t     data     : 8;
      bool        rw       : 1;
      bool        clock    : 1;
      bool        rdy      : 1;
      bool        irq      : 1;
      bool        nmi      : 1;
      bool        reset    : 1;
      uint8_t     bits30_31: 2;

      /* upper 32bit contain additional data for disassembly */
      uint8_t     data1    : 8;
      uint8_t     data2    : 8;
      uint8_t     data3    : 8;
      uint8_t     dataused : 2;
      bool        m816     : 1; /* reverse meaning from CPU flag: 1=16 bit */
      bool        x816     : 1; /* reverse meaning from CPU flag: 1=16 bit */
      uint8_t     eval     : 3;
      bool        n816     : 1; /* reverse meaning from CPU flag: 1=native */
   };
} fullinfo_t;
#endif

const char *fullinfo_base( fullinfo_t fi )
{
   static char text[32] = { 0 };
   // the following data is not printed
   // clock: as sampling is always done at the low phase
   // ADDR r D0 RNIR <- 14 chars Reset NMI IRQ RDY
   snprintf( text, sizeof(text)-1,
             "[%04X %c %02X %c%c%c%c]",
             fi.address,
             fi.rw ? 'r' : 'w',
             fi.data,
             fi.reset ? ' ' : 'R',
             fi.nmi   ? ' ' : 'N',
             fi.irq   ? ' ' : 'I',
             fi.rdy   ? ' ' : '_'
             );
   return &text[0];
}

const char *fullinfo_extra( fullinfo_t fi )
{
   const int chunk = 32;
   static char text[2*32] = { 0 };
   static int pos = 2*chunk;
   pos += chunk;
   if( pos >= sizeof(text) )
   {
      pos = 0;
   }
   // E U D1 D2 D3 EMX <- 16 chars
   snprintf( &text[pos], chunk-1,
             "[%d %d %02X %02X %02X %02X %c%c%c]",
             fi.eval,
             fi.dataused + 1,
             fi.data,
             fi.data1,
             fi.data2,
             fi.data3,
             fi.n816 ? 'N' : 'E',
             fi.m816 ? 'M' : ' ',
             fi.x816 ? 'X' : ' '
            );
   //printf( "%16lx -> %s\n", fi.raw, &text[0] );
   return &text[pos];
}


void print_result( uint32_t *opcodes, uint64_t *refbuffer,
                   disass_fulltrace_t dah, uint32_t size )
{
   uint32_t i;
   uint64_t *r = refbuffer;
   uint64_t *c = (uint64_t*)(dah->fullinfo)+BOUNDSBUFFER;
   fullinfo_t fi1, fi2;
   char fie1[32], fie2[32];
   for( i = 0; i < size; ++i )
   {
      fi1.raw = *r;
      fi2.raw = *c;
      strcpy( &fie1[0], fullinfo_extra( fi1 ) );
      strcpy( &fie2[0], fullinfo_extra( fi2 ) );
#if 1
      printf( "%s %s %s %c %x %x %s\n",
              fullinfo_base( fi1 ),
              fullinfo_extra( fi1 ),
              fullinfo_extra( fi2 ),
              disass_fullinfo_isequal( opcodes, fi1, fi2 ),
              pick_bytes( dah, i+BOUNDSBUFFER ),
              pick_cycles( dah, i+BOUNDSBUFFER ),
              (fi2.eval > 3) ? disass_fullinfo_inbounds( dah, i ) : "" );
#else
      printf( "%016lx %016lx %c %s: %s\n", *r, *c,
              disass_fullinfo_isequal( opcodes, fi1, fi2 ),
              decode_trace( (uint32_t)fi1.raw, false, 0 ),
              (fi2.eval > 3) ? disass_fullinfo_inbounds( dah, i ) : "" );
#endif
      ++r;
      ++c;
      if( !*r || !*c )
      {
         break;
      }
   }
}


int main( int argc, char* argv[] )
{
   const char *progname = argv[0];

   cputype_t cpu = CPU_ERROR;
   uint32_t *opcodes = 0;
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
   opcodes = disass_set_cpu( cpu );
   dah = disass_fulltrace_init( cpu, buffer, size, 0 );

   disassemble_beancounter( dah, BOUNDSBUFFER );
   print_result( opcodes,
                 refbuffer,
                 dah,
                 size );

   disass_fulltrace_done( dah );
   free( buffer );
   free( refbuffer );

   return 0;
}
