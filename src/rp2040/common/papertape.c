
#include "generic_helper.h"

#define TIMEOUT (1000)

extern int16_t readchar( uint32_t timeout );

extern void writechar( uint8_t c );


int16_t nib2bin( int16_t nib )
{
   if( (nib >= '0') && (nib <= '9') )
   {
      return nib & 0xF;
   }

   if( (nib >= 'A') && (nib <= 'Z') )
   {
      return (nib - 'A') + 10;
   }

   if( (nib >= 'a') && (nib <= 'z') )
   {
      return (nib - 'a') + 10;
   }

   return -1;
}


static int16_t getbyte()
{
   int16_t lownib;
   int16_t highnib;

   highnib = nib2bin( readchar( TIMEOUT ) );
   if( highnib < 0 )
   {
      return -1;
   }
   lownib = nib2bin( readchar( TIMEOUT ) );
   if( lownib < 0 )
   {
      return -1;
   }

   return (highnib << 4) | lownib;
}


static bool waitcolon()
{
   int16_t c = 0;
   for(;;)
   {
      c = readchar( TIMEOUT );
      switch( c )
      {
         case '\r':
         case '\n':
         case ' ':
            continue;
         case ':':
         case ';':
            return true;
         default:
            fprintf( stderr, "waitcolon() got '%c'\n", c );
            return false;
      }
   }
   return true;
}


void papertape_write( FILE *f, peek_t peek, ptp_callback_t cb, uint8_t bank,
                      uint16_t start, uint16_t end, uint8_t step )
{
   uint16_t addr   = start;
   uint16_t chksum = 0;
   uint16_t cstep  = 0;
   uint8_t  count  = 0;
   uint8_t  data   = 0;

   /*
    * format: ;CCAAAADD[DD..]SSSS
    * ; 	indication of papertape data
    * CC 	number of databytes sent later on, 00 indicates end of transmission
    * AAAA 	address to write to
    * DD 	a byte of data, number of bytes must match the number specified as CC
    * SSSS 	checksum, every byte of CC, AA (2 bytes) and DD are added up and must match SSSS
    */
   for( addr = start; addr <= end; ++addr )
   {
      if( !cstep )
      {
         /* print line header */
         count = min( step, end - addr );
         fprintf( f, ";%02X%04X", count, addr );
         if( cb ) cb( false, addr );
         chksum = count + (addr >> 8) + (addr & 0xFF);
      }
      data = peek( bank, addr );
      fprintf( f, "%02X", data );
      if( cb ) cb( false, addr );
      chksum += data;
      if( (++cstep) >= count )
      {
         fprintf( f, "%04X\n", chksum );
         if( cb ) cb( true, addr );
         cstep = 0;
      }
   }
   fprintf( f, ";00%04X\n", addr );
}


bool papertape_read( poke_t poke, uint8_t bank )
{
   uint16_t addr   = 0;
   uint16_t chksum = 0;
   int16_t data    = 0;
   uint8_t len     = 0;
   uint8_t pos     = 0;

   for(;;)
   {
      if( !waitcolon() )
      {
         return false;
      }

      /* get data len */
      len = getbyte();
      chksum = (uint16_t)len;
      if( len == 0 )
      {
         break;
      }

      /* get highbyte of address */
      data = getbyte();
      if( data < 0 )
      {
         return false;
      }
      chksum += data;
      addr = data << 8;

      /* get lowbyte of address */
      data = getbyte();
      if( data < 0 )
      {
         return false;
      }
      chksum += data;
      addr |= data;

      /* get data bytes */
      for( pos = 0; pos < len; ++pos )
      {
         data = getbyte();
         if( data < 0 )
         {
            return false;
         }
         chksum += data;
         poke( bank, addr + pos, data & 0xFF );
      }

      /* verify checksum */
      data = getbyte();
      if( (chksum >> 8) != data )
      {
         fprintf( stderr, "chksum hi error $%02X != $%02X\n",
                  data, chksum >> 8 );
         return false;
      }
      data = getbyte();
      if( (chksum & 0xFF) != data )
      {
         fprintf( stderr, "chksum lo error $%02X != $%02X\n",
                  data, chksum & 0xFF );
         return false;
      }
   }
   return true;
}
