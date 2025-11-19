
#include "sorbus_rte.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include "jam_kernel.h"
#include "jam_tools.h"
#include "jam_basic.h"


static uint8_t ram[0x10000];


uint8_t debug_banks()
{
   return 3;
}


void debug_poke( uint8_t bank, uint16_t addr, uint8_t value )
{
   if( !bank )
   {
      ram[addr] = value;
   }
}


uint8_t debug_peek( uint8_t bank, uint16_t addr )
{
   if( addr < 0xE000 )
   {
      return ram[addr];
   }
   switch( bank )
   {
      case 1:
         return jam_kernel[addr & 0x1FFF];
      case 2:
         return jam_tools[addr & 0x1FFF];
      case 3:
         return jam_basic[addr & 0x1FFF];
      default:
         return ram[addr];
   }
}


bool debug_loadfile( uint16_t addr, const char *filename )
{
   ssize_t filesize;
   uint8_t *filedata;

   filedata = loadfile( filename, &filesize );
   if( !filedata )
   {
      return false;
   }
   if( filesize > (sizeof(ram) - addr) )
   {
      filesize = sizeof(ram) - addr;
   }
   memcpy( &ram[addr], filedata, filesize );
   free( filedata );
   return true;
}


/* return null pointer on fail */
uint8_t *loadfile( const char *filename, ssize_t *filesize )
{
   ssize_t  dataread;
   ssize_t  datasize;
   uint8_t  *data = 0, *d;
   int      fd;

   if( filesize )
   {
      *filesize = -1;
   }
   fd = open( filename, O_RDONLY );
   {
      if( fd < 0 )
      {
         /* TODO: better error handling */
         perror( "open" );
         return 0;
      }
   }

   datasize = (ssize_t)lseek( fd, 0, SEEK_END );
   (void)lseek( fd, 0, SEEK_SET );
   if( datasize < 0 )
   {
      /* TODO: better error handling */
      perror( "seek" );
      close( fd );
      return 0;
   }

   data = malloc( (size_t)datasize );
   if( !data )
   {
      perror( "malloc" );
      close( fd );
      return 0;
   }

   for( d = data; d < (data + datasize); d += dataread )
   {
      dataread = read( fd, d, datasize - (d - data) );
      if( dataread < 0 )
      {
         perror( "read" );
         free( data );
         close( fd );
         return 0;
      }
   }

   if( filesize )
   {
      *filesize = datasize;
   }
   close( fd );
   return data;
}


uint32_t mf_checkheap()
{
   return 0;
}
