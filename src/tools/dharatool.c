/*
 * This tool is based on mkftl.c from the FUZIX port for ESP32 and Pi Pico:
 * https://github.com/davidgiven/FUZIX/blob/rpipico/Standalone/mkftl.c
 *
 * Since the original file has no explizit license, it should be covered
 * by the GNU GENERAL PUBLIC LICENSE Version 2 of the project.
 *
 * Thanks to David Given for pointing me out to his code.
 *
 * However it has been modified a lot and taylored for the use with the
 * Sorbus Computer's Native Core
 */

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
#define XIP_BASE (0x10000000)
#include "../rp2040/native/flash_config.h"


static const uint32_t flashsize = FLASH_DRIVE_END - FLASH_DRIVE_START;
static uint8_t gcratio = GC_RATIO;
static bool debug_info = false;

struct dhara_map dhara;
static uint8_t* dhara_data;

static void debugf( const char* s, ... )
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


long loadfile( uint8_t* buffer, const uint32_t buffer_size, const char *filename )
{
   FILE *f;
   long file_size = -1;

   f = fopen( filename, "rb" );
   if( !f )
   {
      fprintf( stderr, "opening '%s' failed: %s\n", filename, strerror( errno ) );
      exit( 21 );
   }
   // check if buffer fits
   fseek( f, 0, SEEK_END );
   file_size = ftell( f );
   fseek( f, 0, SEEK_SET );

   memset( buffer, 0, buffer_size );

   if( (file_size < 0) && (file_size > buffer_size) )
   {
      fprintf( stderr, "a file size of %ld for '%s' cannot be loaded\n",
               file_size, filename );
      exit( 21 );
   }

   if( !fread( buffer, file_size, 1, f ) )
   {
      fprintf( stderr, "file load of '%s' incomplete\n", filename );
      exit( 21 );
   }

   fclose( f );

   printf( "file '%s' loaded, %d (0x%08x) bytes\n", filename, file_size, file_size );
   return file_size;
}


void savefile( const uint8_t* buffer, const uint32_t buffer_size, const char *filename )
{
   FILE *f;

   f = fopen( filename, "wb" );

   if( !f )
   {
      fprintf( stderr, "opening '%s' failed: %s\n", filename, strerror(errno) );
      exit( 22 );
   }
   if( !fwrite( buffer, buffer_size, 1, f ) )
   {
      fprintf( stderr, "file save of '%s' incomplete\n", filename );
      exit( 22 );
   }

   printf( "file '%s' saved, %d (0x%08x) bytes\n", filename, buffer_size, buffer_size );
   fclose( f );
}


int dhara_nand_erase( const struct dhara_nand *n, dhara_block_t b,
                      dhara_error_t *err )
{
   debugf( "  dhara_nand_erase(b=%08x)\n", b );
   memset(dhara_data + (b*BLOCK_SIZE), 0xff, BLOCK_SIZE);
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
   memcpy(dhara_data + (p*PAGE_SIZE), data, PAGE_SIZE);
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
   if (p >= (flashsize/PAGE_SIZE))
   {
      if( err )
      {
         *err = DHARA_E_BAD_BLOCK;
      }
      debugf( "< dhara_nand_read = %s\n", dhara_strerror( DHARA_E_BAD_BLOCK ) );
      return -1;
   }

   memcpy(data, dhara_data + (p*PAGE_SIZE) + offset, length);
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
   const uint8_t* ptr = dhara_data + (p*PAGE_SIZE);
   for( int i=0; i<PAGE_SIZE; i++ )
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
   const uint8_t* psrc = dhara_data + (src*PAGE_SIZE);
   uint8_t* pdst = dhara_data + (dst*PAGE_SIZE);
   memcpy(pdst, psrc, PAGE_SIZE);
   if( err )
   {
      *err = DHARA_E_NONE;
   }
   debugf( "< dhara_nand_copy\n" );
   return 0;
}


static int ilog2(uint32_t i)
{
   return ffsl(i) - 1;
}


void binary2dhara( uint8_t *inbuffer, uint32_t inbuffer_size,
                   uint8_t *outbuffer )
{
   int sectors = (inbuffer_size + PAGE_SIZE - 1) / PAGE_SIZE;
   int s;
   dhara_error_t err = DHARA_E_NONE;

   for( s = 0; s < sectors; ++s )
   {
      dhara_map_write( &dhara, s, inbuffer + (s * PAGE_SIZE), &err );

      if( err != DHARA_E_NONE )
      {
         fprintf( stderr, "dhara error writing sector %d: %s\n", s, dhara_strerror( err ) );
         exit( 1 );
      }
   }

   dhara_map_sync( &dhara, &err );
   if( err != DHARA_E_NONE )
   {
      fprintf( stderr, "dhara error syncing after write: %s\n", dhara_strerror( err ) );
      exit( 1 );
   }
}


int dhara2binary( uint8_t *inbuffer, uint8_t *outbuffer, uint32_t outbuffer_size )
{
   dhara_page_t page;
   int sectors = outbuffer_size / PAGE_SIZE;
   int lastusedpage = 0;
   int s;
   dhara_error_t err = DHARA_E_NONE;

   for( s = 0; s < sectors; ++s )
   {
      if( dhara_map_find( &dhara, s, &page, &err) == 0 )
      {
         lastusedpage = s;
      }

      err = DHARA_E_NONE;
      dhara_map_read( &dhara, s, outbuffer + (s * PAGE_SIZE), &err );

      if( err != DHARA_E_NONE )
      {
         fprintf( stderr, "dhara error reading sector %d: %s\n", s, dhara_strerror( err ) );
         exit( 1 );
      }
   }
   return lastusedpage;
}


void usage( const char *progname, int exitcode )
{
   fprintf( exitcode == 0 ? stdout : stderr,
            "%s <dhararead|dharawrite> <infile> <outfile>\n",
            progname );
   exit( exitcode );
}


int main(int argc, char* const* argv)
{
   uint8_t  *binary_data;
   uint32_t binary_pages;
   uint8_t  *page_buf;
   dhara_error_t err = DHARA_E_NONE;

   // prepare data buffers
   struct dhara_nand nand;
   nand.log2_page_size = ilog2(PAGE_SIZE);
   nand.log2_ppb = ilog2(BLOCK_SIZE) - nand.log2_page_size;
   nand.num_blocks = flashsize / BLOCK_SIZE;

   // check parameters
   if( argc != 4 )
   {
      usage( argv[0], argc == 1 ? 0 : 1 );
   }

   page_buf = (uint8_t*)malloc( PAGE_SIZE );
   dhara_data = (uint8_t*)malloc( flashsize );
   memset( dhara_data, 0xff, flashsize );

   dhara_map_init( &dhara, &nand, page_buf, gcratio );
   dhara_map_resume( &dhara, &err );
   debugf( "dhara_map_resume()=%s\n", dhara_strerror( err ) );
   printf( "Number of physical erase blocks: %d\n", nand.num_blocks );
   binary_pages = dhara_map_capacity( &dhara );
   printf( "Maximum logical size: %d sectors (%dkB)\n", binary_pages, binary_pages * PAGE_SIZE / 1024 );
   binary_data = (uint8_t*)malloc( binary_pages * PAGE_SIZE );

   if( !strcmp( argv[1], "dhararead" ) )
   {
      long loaded;
      int lastusedpage;

      loaded = loadfile( dhara_data, flashsize, argv[2] );
      dhara_map_init( &dhara, &nand, page_buf, gcratio );
      dhara_map_resume( &dhara, &err );
      if( loaded != flashsize )
      {
         fprintf( stderr, "'%s' too short: got %ld, expected %d.\n",
                  argv[2], loaded, flashsize );
         exit( 21 );
      }
      lastusedpage = dhara2binary( dhara_data, binary_data, binary_pages * PAGE_SIZE );
      savefile( binary_data, (lastusedpage+1) * PAGE_SIZE, argv[3] );
   }
   else if ( !strcmp( argv[1], "dharawrite" ) )
   {
      long loaded;
      loaded = loadfile( binary_data, binary_pages * PAGE_SIZE, argv[2] );
      binary2dhara( binary_data, loaded, dhara_data );
      savefile( dhara_data, flashsize, argv[3] );
   }
   else
   {
      fprintf( stderr, "don't know how to '%s'\n", argv[1] );
      usage( argv[0], 1 );
   }

   free( page_buf );
   free( dhara_data );
   free( binary_data );
   return 0;
}
