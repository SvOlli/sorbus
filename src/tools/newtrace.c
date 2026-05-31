
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../rp2040/disassemble/da_base.c"
#include "../rp2040/disassemble/da_cyclecount.c"
#include "../rp2040/disassemble/da_generated.c"
//#include "../rp2040/disassemble/da_memory.c"
#include "../rp2040/disassemble/da_trace.c"

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


/*
 * This will only be used on the host machine
 */
cputype_t getcputype( const char *argi )
{
   cputype_t retval = CPU_ERROR;
   char arg[8] = { 0 };
   const char *c = argi;
   int i;

   /* strip off any leading spaces */
   while( *c == ' ' )
   {
      ++c;
   }

   for( i = 0; *c && (i < 7); ++c, ++i )
   {
      arg[i] = toupper( *c );
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

   return retval;
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
   sample = (uint64_t*)calloc( entries, sizeof(uint64_t) );

   s = sample;
   for( c = start; c < end; ++c )
   {
      /* make sure that right conversion function is used */
      assert( sizeof(unsigned long long int) == sizeof(uint64_t) );
      errno = 0;
      *(s++) = strtoull( (const char*)c, 0, 16 );
      if( errno )
      {
         perror( "strtoull" );
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


/* in library snprintf-style */
const char *fullinfo_base( da_fullinfo_t fi )
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

const char *fullinfo_extra( da_fullinfo_t fi )
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


void print_result( const uint32_t *opcodes, uint64_t *refbuffer,
                   da_trace_t dah, uint32_t size )
{
   static char buffer[80] = { 0 };
   char *b = &buffer[0];
   size_t bsize = sizeof(buffer) - 1;
   size_t used  = 0;
   uint32_t i;
   uint64_t *r = refbuffer;
   uint64_t *c = (uint64_t*)(dah->fullinfo);
   da_fullinfo_t fi1, fi2;
   for( i = 0; i < size; ++i )
   {
      fi1.raw = *r;
      fi2.raw = *c;
      if( (!fi1.raw) || (!fi2.raw) )
      {
         break;
      }
      used  = 0;
      used += snprintf( b+used, bsize-used, "%4d:", i );
      used += da_snf_fullinfo( b+used, bsize-used, dah->cpu, "[a w d RNIS] ",  fi2, DA_FLAG_NONE );
      used += da_snf_fullinfo( b+used, bsize-used, dah->cpu, "e:[dx] ",        fi1, DA_FLAG_NONE );
      used += da_snf_fullinfo( b+used, bsize-used, dah->cpu, "e:[dx]E n cC y", fi2, DA_FLAG_NONE );
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
