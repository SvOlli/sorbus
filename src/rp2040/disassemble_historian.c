
#include "disassemble.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

// just for debugging
#define SHOW_CONFIDENCE 0
#define ring(x,s) ((x) & (s-1))

struct disass_historian_s
{
   uint32_t    entries;
   char        *text;
};


static void disass_historian_run( disass_historian_t d, uint32_t *trace, uint32_t entries, uint32_t start )
{
   int index = 0, offset, i;
   uint16_t address, expectedaddress;
   uint8_t data[4];
   bool done;
   uint8_t *eval = calloc( entries, sizeof(uint8_t) );

   /* step 1: disassemble everything */
   for( index = 0; index < entries; ++index )
   {
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
      while( !done )
      {
         uint32_t addr = trace[ring(index+i,entries)] & 0xFFFF;
         if( addr == expectedaddress )
         {
            data[++offset] = (trace[ring(index+i,entries)] >> 16) & 0xFF;
            ++expectedaddress;
            if( offset > (count_of(data) - 1) )
            {
               // found all parameters
               done = true;
            }
         }

         if( ++i > 10 ) // longest 65xx instuction takes 8 clock cycles
         {
            // could not find all parameters, d'oh!
            done = true;
         }
      }
#if SHOW_CONFIDENCE
      strncpy( 2 + d->text + (index * DISASS_MAX_STR_LENGTH),
               disass( address, data[0], data[1], data[2], data[3] ),
               DISASS_MAX_STR_LENGTH-1 );
#else
      strncpy( d->text + (index * DISASS_MAX_STR_LENGTH),
               disass( address, data[0], data[1], data[2], data[3] ),
               DISASS_MAX_STR_LENGTH-1 );
#endif
      /* make sure string is null terminated */
      *(d->text + ((index + 1) * DISASS_MAX_STR_LENGTH - 1)) = '\0';
   }

   /* step 2: detect interruptions */
   for( index = start; index != ring(start-1,entries); index = ring(index+1,entries) )
   {
      uint32_t addr0 = trace[ring(index-5,entries)] & 0xFFFF;
      uint32_t addr1 = trace[ring(index-4,entries)] & 0xFFFF;
      uint32_t addr2 = trace[ring(index-3,entries)] & 0xFFFF;
      uint32_t addr3 = trace[ring(index-2,entries)] & 0xFFFF;
      uint32_t addr4 = trace[ring(index-1,entries)] & 0xFFFF;

      if( ((addr0 & 0xFF00) == 0x0100) && // check stack access
          ((addr1 & 0xFF00) == 0x0100) &&
          ((addr2 & 0xFF00) == 0x0100) &&
          ( addr3           >= 0xFFFA) &&
          ( addr4           >= 0xFFFB) )
      {
         eval[ring(index-5,entries)] = 0; // writing to stack
         eval[ring(index-4,entries)] = 0;
         eval[ring(index-3,entries)] = 0;
         eval[ring(index-2,entries)] = 0; // reading reset/nmi/irq vector
         eval[ring(index-1,entries)] = 0;
         eval[ring(index  ,entries)] = 9; // executing first instruction
      }
      else
      {
         eval[ring(index,entries)] = 3;
      }

      if( addr0 == addr1 )
      {
         eval[ring(index-5,entries)] = 0; // dummy read
         ++eval[ring(index-4,entries)];
      }
   }

   /* final step: clear unused */
   for( i = 0; i < entries; ++i )
   {
#if SHOW_CONFIDENCE
      *(d->text + (i * DISASS_MAX_STR_LENGTH) + 0) = '0' + eval[i];
      *(d->text + (i * DISASS_MAX_STR_LENGTH) + 1) = ':';
      if( eval[i] < 3 )
      {
         *(2 + d->text + (i * DISASS_MAX_STR_LENGTH)) = '\0';
      }
#else
      if( eval[i] < 3 )
      {
         *(d->text + (i * DISASS_MAX_STR_LENGTH)) = '\0';
      }
#endif
   }

   free( eval );
}


disass_historian_t disass_historian_init( uint32_t *trace, uint32_t entries, uint32_t start )
{
   disass_historian_t d = (disass_historian_t)malloc( sizeof( struct disass_historian_s ) );

   // TODO: find something better than assert (or somewhere else to check?)
   assert( (entries & (entries-1)) == 0 );

   d->entries = entries;
   d->text    = (char*)malloc( entries * DISASS_MAX_STR_LENGTH );

   memset( d->text, 0, entries * DISASS_MAX_STR_LENGTH );
   disass_historian_run( d, trace, entries, start );

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
