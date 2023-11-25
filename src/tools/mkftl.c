#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include "../rp2040/dhara/error.h"
#include "../rp2040/dhara/map.h"
#include "../rp2040/dhara/nand.h"

static uint32_t pagesize = 512;
static uint32_t erasesize = 4096;
static uint32_t flashsize = 12*1024*1024; /* bytes */
static uint8_t gcratio = 2;
static bool debug_info = false;

static uint8_t* flashdata;

static void debugf(const char* s, ...)
{
   va_list ap;
   va_start(ap, s);
   if( !debug_info )
   {
      return;
   }
   fprintf(stderr, "Debug: ");
   vfprintf(stderr, s, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

int dhara_nand_erase( const struct dhara_nand *n, dhara_block_t b,
                      dhara_error_t *err )
{
   debugf( "  dhara_nand_erase(b=%08x)\n", b );
   memset(flashdata + (b*erasesize), 0xff, erasesize);
   if( err )
   {
      *err = DHARA_E_NONE;
   }
   return 0;
}

int dhara_nand_prog( const struct dhara_nand *n, dhara_page_t p,
                     const uint8_t *data,
                     dhara_error_t *err )
{
   memcpy(flashdata + (p*pagesize), data, pagesize);
   if( err )
   {
      *err = DHARA_E_NONE;
   }
   return 0;
}

int dhara_nand_read( const struct dhara_nand *n, dhara_page_t p,
                     size_t offset, size_t length,
                     uint8_t *data,
                     dhara_error_t *err )
{
   debugf( "> dhara_nand_read(p=%08x,o=%08x,l=%08x)\n", p, offset, length );
   if (p >= (flashsize/pagesize))
   {
      if( err )
      {
         *err = DHARA_E_BAD_BLOCK;
      }
      debugf( "< dhara_nand_read = %s\n", dhara_strerror( DHARA_E_BAD_BLOCK ) );
      return -1;
   }

   memcpy(data, flashdata + (p*pagesize) + offset, length);
   if( err )
   {
      *err = DHARA_E_NONE;
   }
   debugf( "< dhara_nand_read\n" );
   return 0;
}

int dhara_nand_is_bad( const struct dhara_nand* n, dhara_block_t b )
{
   debugf( "  dhara_nand_is_bad(b=%04x)\n", b );
   return 0;
}

void dhara_nand_mark_bad( const struct dhara_nand *n, dhara_block_t b )
{
   debugf( "  dhara_nand_mark_bad(b=%04x)\n", b );
}

int dhara_nand_is_free( const struct dhara_nand *n, dhara_page_t p )
{
   debugf( "> dhara_nand_is_free(p=%04x)\n", p );
   const uint8_t* ptr = flashdata + (p*pagesize);
   for( int i=0; i<pagesize; i++ )
   {
      if( ptr[i] != 0xff )
      {
         debugf( "< dhara_nand_is_free = used\n" );
         return 0;
      }
   }
   debugf( "< dhara_nand_is_free = free\n" );
   return 1;
}

int dhara_nand_copy( const struct dhara_nand *n,
                     dhara_page_t src, dhara_page_t dst,
                     dhara_error_t *err )
{
   debugf( "> dhara_nand_copy(src=%04x,dst=%04x)\n", src, dst );
   const uint8_t* psrc = flashdata + (src*pagesize);
   uint8_t* pdst = flashdata + (dst*pagesize);
   memcpy(pdst, psrc, pagesize);
   if( err )
   {
      *err = DHARA_E_NONE;
   }
   debugf( "< dhara_nand_copy\n" );
   return 0;
}

static void panic(const char* s, ...)
{
   va_list ap;
   va_start(ap, s);
   fprintf(stderr, "Fatal: ");
   vfprintf(stderr, s, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   exit(1);
}

static bool is_power_of_2(uint32_t i)
{
   return i && !((i-1) & i);
}

static int ilog2(uint32_t i)
{
   return ffsl(i) - 1;
}

static void syntax_error(void)
{
   fprintf(stderr, "Syntax: mkftl [options] <inputfile> -o <outputfile>\n");
   fprintf(stderr, "Options:\n");
   fprintf(stderr, "  -p <number>   set page size, in bytes (default: %d bytes)\n", pagesize);
   fprintf(stderr, "  -e <number>   set erase block size, in bytes (default: %d bytes)\n", erasesize);
   fprintf(stderr, "  -s <number>   set image size, in kilobytes (default: %d kB)\n", flashsize/1024);
   fprintf(stderr, "  -g <number>   set garbage collection ratio (default: %d)\n", gcratio);
   exit(1);
}

int main(int argc, char* const* argv)
{
   const char* outputfilename = NULL;
   const char* inputfilename = NULL;
   int sectorno;
   int opt;

   while( (opt = getopt(argc, argv, "o:p:e:s:g:d")) >= 0 )
   {
      switch (opt)
      {
         case 'd':
            debug_info = true;
            break;

         case 'o':
            outputfilename = optarg;
            break;

         case 'p':
            pagesize = strtoul( optarg, NULL, 0 );
            break;

         case 'e':
            erasesize = strtoul( optarg, NULL, 0 );
            break;

         case 's':
            flashsize = strtoul( optarg, NULL, 0 ) * 1024;
            break;

         case 'g':
            gcratio = strtoul( optarg, NULL, 0 );
            break;

         default:
            syntax_error();
      }
   }

   if( argc != (optind+1) )
   {
      syntax_error();
   }
   inputfilename = argv[optind];

   if( !outputfilename )
   {
      panic("output filename must be supplied");
   }

   if( !is_power_of_2(pagesize) )
   {
      panic("page size %d is not a power of two", pagesize);
   }

   if( !is_power_of_2(erasesize) )
   {
      panic("erase size %d is not a power of two", erasesize);
   }

   if( erasesize % pagesize )
   {
      panic("erase size is not a multiple of page size");
   }

   if( flashsize % erasesize )
   {
      panic("flash size is not a multiple of erase size");
   }

   if( pagesize < 512 )
   {
      fprintf( stderr, "warning: pagesize of %d is below 512, dhara will perform sub par\n", pagesize );
   }

   uint8_t *buffer   = (uint8_t*)malloc(pagesize);
   uint8_t *page_buf = (uint8_t*)malloc(/*erasesize*/ 2*pagesize);

   struct dhara_nand nand;
   nand.log2_page_size = ilog2(pagesize);
   nand.log2_ppb = ilog2(erasesize) - nand.log2_page_size;
   nand.num_blocks = flashsize / erasesize;

   flashdata = malloc(flashsize);
   memset(flashdata, 0xff, flashsize);
   memset(page_buf,0xBD,2*pagesize);

   struct dhara_map dhara;
   dhara_map_init(&dhara, &nand, page_buf, gcratio);
   dhara_error_t err = DHARA_E_NONE;
   dhara_map_resume(&dhara, &err);
   debugf( "dhara_map_resume()=%s\n", dhara_strerror( err ) );
   printf( "Number of physical erase blocks: %d\n", nand.num_blocks );

   uint32_t lba = dhara_map_capacity( &dhara );
   printf( "Maximum logical size: %d sectors (%dkB)\n", lba, lba * pagesize / 1024 );

   FILE* inf = fopen(inputfilename, "rb");
   if( !inf )
   {
      panic("cannot open input file: %s", strerror(errno));
   }
   fseek( inf, 0, SEEK_END );
   int sectors = ftell(inf) / pagesize;
   fseek( inf, 0, SEEK_SET );
   if( sectors > lba )
   {
      fprintf(stderr, "Logical image too big (%d > %d)\n",
         sectors, lba);
      exit(1);
   }
   if( sectors == lba )
   {
      fprintf( stderr, "WARNING: logical image completely fills the FTL filesystem; you\n" );
      fprintf( stderr, "will have GC thrash if you try to write to it\n" );
   }

   for( sectorno=0; sectorno<sectors; sectorno++ )
   {
      memset( buffer, 0, pagesize );
      fread( buffer, pagesize, 1, inf );

      err = DHARA_E_NONE;
      dhara_map_write( &dhara, sectorno, buffer, &err );

      if( err != DHARA_E_NONE )
      {
         fprintf( stderr, "FTL error: %s\n", dhara_strerror( err ) );
         exit( 1 );
      }
   }
   fclose( inf );
   dhara_map_sync( &dhara, &err );
   if( err != DHARA_E_NONE )
   {
      fprintf( stderr, "FTL error: %s\n", dhara_strerror( err ) );
      exit( 1 );
   }
   printf( "%d sectors of %d bytes read from '%s'\n", sectorno, pagesize, inputfilename );

   err = DHARA_E_NONE;
   dhara_map_gc( &dhara, &err );
   uint32_t used = dhara_map_size( &dhara );
   printf( "Used space: %d sectors (%dkB)\n", used, used * pagesize / 1024 );

   printf( "Restarting dhara for verification\n" );
   dhara_map_init( &dhara, &nand, page_buf, gcratio );
   err = DHARA_E_NONE;
   dhara_map_resume( &dhara, &err );
   debugf( "dhara_map_resume()=%s\n", dhara_strerror( err ) );

   FILE* outf = fopen( outputfilename, "wb" );
   if( !outf )
   {
      panic( "cannot open output file '%s': %s", outputfilename, strerror(errno) );
   }
   fwrite( flashdata, 1, flashsize, outf );
   fclose( outf );

   free( buffer );
   if( page_buf[pagesize] != 0xBD )
   {
      printf( "warning: dhara has exceeded page_buffer size\n" );
   }
   free( page_buf );
   free( flashdata );
   return 0;
}

// vim: sw=4 ts=4 et:

