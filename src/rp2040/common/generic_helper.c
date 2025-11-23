
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
   "6502 RevA",
   "65SC02",
   NULL
};


const char *cputype_name( cputype_t cputype )
{
   if( cputype >= count_of(cputype_names) )
   {
      cputype = CPU_ERROR;
   }
   return cputype_names[cputype];
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
/* disable pedantic as it warns about non ISO C usage of nested functions */
void print_hexdump_buffer( uint8_t bank, const uint8_t *memory, uint32_t size,
                           bool showbank )
{
   /* I think that's the first time in anything I coded,
    * where a function within a function actually makes totally sense.
    * Today is a good day. */
   /* Note: nested functions are not C standard, but a GCC extension */
   uint8_t peek( uint8_t /*bank*/, uint16_t a )
   {
      return memory[a];
   }

   print_hexdump( peek, bank, 0, size, showbank );
}
#pragma GCC diagnostic pop


void print_hexdump( peek_t peek, uint8_t bank, uint16_t address, uint32_t size,
                    bool showbank )
{
   for( uint32_t i = 0; i < size; i += 0x10 )
   {
      if( showbank )
      {
         printf( "%02x:", bank );
      }
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
            printf( " %02x", peek(bank,a) );
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
         uint8_t v = peek(bank,a);
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

int32_t get_16bit_address( uint16_t lastaddr )
{
   uint16_t addr = 0;
   int count = 0;

   for(;;)
   {
      int c = getchar();
      if( (c >= '0') && (c <= '9') )
      {
         putchar( c );
         addr = (addr << 4) | (c & 0x0f);
         if( ++count >= 4 )
         {
            return addr;
         }
      }
      else if( ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')) )
      {
         putchar( toupper(c) );
         c = toupper(c) - 'A' + 10;
         addr = (addr << 4) | (c & 0x0f);
         if( ++count >= 4 )
         {
            return addr;
         }
      }
      else switch( c )
      {
         case 0x03: // CTRL+C
         case 0x1b: // ESC
         case 0x1d: // CTRL+]
         case 'q':
            puts( "quit" );
            return (-1);
         case ' ':
            while( count > 0 )
            {
               printf( "\b \b" );
               --count;
            }
            // slip through
         case 0x0d:
            if( !count )
            {
               addr = lastaddr;
               printf( "%04X", addr );
            }
            return addr;
            break;
         case 0x08:
         case 0x7f:
            if( count >= 0 )
            {
               printf( "\b \b" );
               addr >>= 4;
               --count;
            }
            break;
         default:
            break;
      }
   }
}
