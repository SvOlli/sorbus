
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generic_helper.h"
#include "sorbus_rte.h"


FILE *in  = NULL;
FILE *out = NULL;

uint16_t min_addr = 0xFFFF;
uint16_t max_addr = 0x0000;


#if 0
bool papertape_read( poke_t poke );
void papertape_write( peek_t peek, uint16_t start, uint16_t end, uint8_t step );
#endif


int16_t readchar( uint32_t timeout )
{
   return (int16_t)getc( in );
}

void writechar( uint8_t c )
{
   putc( c, out );
}

void trace_poke( uint8_t bank, uint16_t addr, uint8_t value )
{
   if( addr < min_addr )
   {
      min_addr = addr;
   }
   if( addr > max_addr )
   {
      max_addr = addr;
   }
   memsim_poke( bank, addr, value );
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
     "%s: test tool for converting from to papertape\n"
     "\t-p file:\tpapertape file\n"
     "\t-f file:\tbinary file (mandatory)\n"
     "\t-a addr:\tstart addresses of file in memory (default: $0400)\n"
     "\t-b 1-32:\tbytes per line\n"
     "\t-v:\t\tverbose output\n"
     , progname );
   exit( retval );
}


int main( int argc, char *argv[] )
{
   const char *progname = argv[0];

   const char *binname  = 0;
   const char *ptpname  = 0;
   int mode = 0;
   bool verbose = false;

   uint16_t address = 0x0400;
   uint16_t bytesloaded = 0;
   uint8_t  bytesperline = 32;

   int opt;
   bool fail = false;
   in  = stdin;
   out = stdout;

   while ((opt = getopt(argc, argv, "a:b:f:p:vh")) != -1)
   {
      switch( opt )
      {
         case 'a':
            address = strtol( optarg, 0, 0 );
            break;
         case 'b':
            bytesperline = (uint8_t)strtoul( optarg, 0, 0 );
            if( (bytesperline < 1) || (bytesperline > 32) )
            {
               bytesperline = 32;
            }
            break;
         case 'f':
            binname = optarg;
            break;
         case 'p':
            ptpname = optarg;
            break;
         case 'h':
            help( progname, 0 );
            break;
         case 'v':
            verbose = true;
            break;
         default:
            fail = true;
            break;
      }
   }

   if( !binname )
   {
      fprintf( stderr, "binary binname not set\n" );
      fail = true;
   }
   if( (optind+1) == argc )
   {
      if( !strcasecmp(argv[optind],"read") )
      {
         printf( "read mode\n" );
         mode = 1;
      }
      if( !strcasecmp(argv[optind],"write") )
      {
         printf( "write mode\n" );
         mode = 2;
      }
   }
   if( !mode )
   {
      fprintf( stderr, "need to specify 'read' or 'write' papertape\n" );
      fail = true;
   }

   if( fail )
   {
      help( progname, 1 );
   }

   switch( mode )
   {
      case 1:
      {
         if( ptpname )
         {
            in = fopen( ptpname, "rb" );
            if( !in )
            {
               fprintf( stderr, "opening ptp file '%s' failed: %s\n",
                        ptpname, strerror(errno) );
               return 10;
            }
         }
         fail = !papertape_read( trace_poke, 0 );
         if( ptpname )
         {
            fclose( in );
         }
         if( fail )
         {
            fprintf( stderr, "parsing ptp file '%s' failed: %s\n",
                     ptpname, strerror(errno) );
            return 20;
         }
         if( verbose )
         {
            fprintf( stderr, "ptp file contained data from $%04X to $%04X\n",
                     min_addr, max_addr );
         }
         out = fopen( binname, "wb" );
         if( !out )
         {
            fprintf( stderr, "create binary file '%s' failed: %s\n",
                     ptpname, strerror(errno) );
            return 11;
         }
         if( !fwrite( getram(min_addr), max_addr-min_addr+1, 1, out ) )
         {
            fprintf( stderr, "write binary file '%s' failed: %s\n",
                     ptpname, strerror(errno) );
            fclose( out );
            return 12;
         }
         fclose( out );
         break;
      }
      case 2:
      {
         bytesloaded = memsim_loadfile( address, binname );
         if( !bytesloaded )
         {
            fprintf( stderr, "loading binary file '%s' failed: %s\n",
                     binname, strerror(errno) );
            return 30;
         }
         if( verbose )
         {
            fprintf( stderr, "binary file contained data from $%04X to $%04X\n",
                     address, address + bytesloaded - 1 );
         }
         if( ptpname )
         {
            out = fopen( ptpname, "wb" );
            if( !out )
            {
               fprintf( stderr, "opening ptp file '%s' failed: %s\n",
                        ptpname, strerror(errno) );
               return 31;
            }
         }

         papertape_write( out, memsim_peek, 0, 0,
                          address, address+bytesloaded, bytesperline );

         if( ptpname )
         {
            fclose( out );
         }
         break;
      }
      default:
         fprintf( stderr, __FILE__ "(%d): internal error: %d\n", __LINE__, mode );
         break;
   }

   return 0;
}
