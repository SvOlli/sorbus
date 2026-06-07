
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>


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
