
#include "generic_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"


static char* cputype_names[] =
{
   "NONE",
   "6502",
   "65C02",
   "65816",
   "65CE02",
   "6502 (Rev.A)",
   "65SC02"
};


const char *cputype_name( cputype_t cputype )
{
   if( cputype >= count_of(cputype_names) )
   {
      cputype = CPU_ERROR;
   }
   return cputype_names[cputype];
}


void hexdump_buffer( const uint8_t *memory, uint32_t size )
{
   /* I think that's the first time in anything I coded,
    * where a function within a function actually makes totally sense.
    * Today is a good day. */
   /* Note: nested functions are not C standard, but a GCC extension */
   uint8_t peek( uint16_t a )
   {
      return memory[a];
   }

   hexdump( peek, 0, size );
}


void hexdump( peek_t peek, uint16_t address, uint32_t size )
{
   for( uint32_t i = 0; i < size; i += 0x10 )
   {
      printf( "%04x:", address + i );

      for( uint8_t j = 0; j < 0x10; ++j )
      {
         uint16_t a = address + i + j;
         if( (i + j) > size )
         {
            printf( "   " );
         }
         else
         {
            printf( " %02x", peek(a) );
         }
         if( j == 7 )
         {
            printf( " " );
         }
      }
      printf( "  " );
      for( uint8_t j = 0; j < 0x10; ++j )
      {
         uint16_t a = address + i + j;
         if( (i + j) > size )
         {
            break;
         }
         uint8_t v = peek(a);
         if( (v >= 32) && (v <= 127) )
         {
            printf( "%c", v );
         }
         else
         {
            printf( "." );
         }
      }

      printf( "\n" );
   }
}


const char* decode_trace( uint32_t state, bool bank_enabled, uint8_t bank )
{
   static char buffer[32];
   int offset = 0;

   if( bank_enabled )
   {
      snprintf( &buffer[0], sizeof(buffer), "%02x:", bank );
   }
   else
   {
      buffer[0] = '\0';
   }

   offset = strlen( buffer );
   snprintf( &buffer[offset], sizeof(buffer)-offset,
             "%04x %c %02x %c%c%c",
             (state & bus_config.mask_address) >> (bus_config.shift_address),
             (state & bus_config.mask_rw) ? 'r' : 'w',
             (state & bus_config.mask_data) >> (bus_config.shift_data),
             (state & bus_config.mask_reset) ? ' ' : 'R',
             (state & bus_config.mask_nmi) ? ' ' : 'N',
             (state & bus_config.mask_irq) ? ' ' : 'I' );
   buffer[sizeof(buffer)-1] = '\0';

   return &buffer[0];
}
