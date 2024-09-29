
#include "disassemble.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

struct disass_historian_s
{
   uint32_t    entries;
   char        *text;
};


static void disass_historian_run( disass_historian_t d, uint32_t *trace, uint32_t entries )
{
   int index = 0, offset, i;
   uint16_t address, expectedaddress;
   uint8_t data[4];
   bool done;

   while( index < entries )
   {
      /* TODO: parameterize */
      address = trace[index] & 0xFFFF;
      data[0] = (trace[index] >> 16) & 0xFF;
      data[1] = 0x00;
      data[2] = 0x00;
      data[3] = 0x00;
      offset = 1;
      
      /* find arguments */
      expectedaddress = address+1;
      done = false;
      i = 1;
      offset = 0;
      while( !done ) // 12 clockcycles is too much, but who cares
      {
//         printf( "i=%d\n", i );
         if( (index+i) >= entries )
         {
//            printf( "(index+i) >= entries (%d+%d>%d)\n",index,i,entries );
            done = true;
         }
         else
         {
            uint32_t addr = trace[index+i] & 0xFFFF;
            if( addr == expectedaddress )
            {
               data[++offset] = (trace[index+i] >> 16) & 0xFF;
//                  printf( "offset > (count_of(data) - 1) (%d>%d)\n",offset,(count_of(data) - 1) );
               ++expectedaddress;
               if( offset > (count_of(data) - 1) )
               {
                  done = true;
               }
            }
         }
         if( ++i > 10 ) // longest 65xx instuction takes 8 clock cycles
         {
//            printf( "i > 10 (%d)\n",i );
            done = true;
         }
      }
#if 0
      printf( "%3d:%08x:disass(%04x,%02x,%02x,%02x,%02x):%s\n",
              index, trace[index],
              address, data[0], data[1], data[2], data[3],
              disass( address, data[0], data[1], data[2], data[3] ) );
#endif
      strncpy( d->text + (index * DISASS_MAX_STR_LENGTH),
               disass( address, data[0], data[1], data[2], data[3] ),
               DISASS_MAX_STR_LENGTH-1 );
      /* make sure string is null terminated */
      *(d->text + ((index + 1) * DISASS_MAX_STR_LENGTH - 1)) = 0;
      ++index;
   }
}


disass_historian_t disass_historian_init( uint32_t *trace, uint32_t entries )
{
   disass_historian_t d = (disass_historian_t)malloc( sizeof( struct disass_historian_s ) );

   d->entries = entries;
   d->text    = (char*)malloc( entries * DISASS_MAX_STR_LENGTH );

   memset( d->text, 0, entries * DISASS_MAX_STR_LENGTH );
   disass_historian_run( d, trace, entries );

   return d;
}


void disass_historian_done( disass_historian_t d )
{
   memset( d->text, 0, d->entries * DISASS_MAX_STR_LENGTH );
   free( d->text );
   free( d );
}


const char *disass_historian_entry( disass_historian_t d, uint32_t entry )
{
   if( entry >= d->entries )
   {
      return 0;
   }
   return &(d->text[entry * DISASS_MAX_STR_LENGTH]);
}
