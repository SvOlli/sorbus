
#include "disassemble.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

// just for debugging
#ifndef SHOW_CONFIDENCE
#define SHOW_CONFIDENCE 0
#endif
#define ring(x,s) ((x) & (s-1))
#define re(x) ring(x,entries)

struct disass_historian_s
{
   uint32_t    entries;
   char        *text;
};


static uint16_t getcycles( uint32_t *trace, uint32_t entries, uint32_t index )
{
   uint16_t address  = trace_address( trace[index] );
   uint8_t  data     = trace_data( trace[index] );

   uint16_t nextaddr = address + disass_bytes( data );
   uint8_t  bc       = disass_basecycles( data );

   if( disass_isjump( data ) )
   {
      switch( disass_addrmode( data )  )
      {
         case REL: /* 8-bit branch */
            {
               int8_t offset = trace_data( trace[re(index+1)] );
               nextaddr = address + 2 + offset;
               if( trace_address( trace[re(index+2)] ) == nextaddr )
               {
                  /* 65CE02 branch taken */
                  return 2;
               }
               else if( trace_address( trace[re(index+3)] ) == nextaddr )
               {
                  /* branch taken */
                  return 3;
               }
               else if( trace_address( trace[re(index+4)] ) == nextaddr )
               {
                  /* branch taken crossing page */
                  return 4;
               }
               /* assume branch not taken */
               return 2;
            }
            break;
         case ZPNR:
            {
               int8_t offset = trace_data( trace[re(index+4)] );
               nextaddr = address + 3 + offset;
               if( trace_address( trace[re(index+4)] ) == nextaddr )
               {
                  /* 65CE02 branch taken... or not, who cares? */
                  return 4;
               }
               else if( trace_address( trace[re(index+6)] ) == nextaddr )
               {
                  /* branch taken */
                  return 6;
               }
               else if( trace_address( trace[re(index+6)] ) == nextaddr )
               {
                  /* branch taken crossing page */
                  return 7;
               }
               /* assume branch not taken */
               return 5;
            }
            /* on 65CE02 always 4 cycles, even if branch is taken */
            break;
         default:
            /* most jumps are handled by custom code */
            break;
      }
   }
   else
   {
      switch( disass_extracycles( data ) )
      {
         case 0:
            return bc;
            break;
         case 1:
         case 2:
            if( trace_address( trace[re(index+bc)] ) == nextaddr )
            {
               return bc;
            }
            else if( trace_address( trace[re(index+bc+1)] ) == nextaddr )
            {
               return bc+1;
            }
            else if( trace_address( trace[re(index+bc+2)] ) == nextaddr )
            {
               return bc+2;
            }
            break;
         default:
            printf( "internal error: extracycles > 2\n" );
            break;
      }
   }
   return 0;
}


static void disass_historian_run( disass_historian_t d, uint32_t *trace, uint32_t entries, uint32_t start )
{
   int index = 0, offset, i;
   uint16_t address, expectedaddress;
   uint8_t data[4];
   bool done;
   int8_t *eval = calloc( entries, sizeof(uint8_t) );

   /* step 1: disassemble everything */
   for( index = 0; index < entries; ++index )
   {
      /* set some sane confidence */
      eval[index] = 3;

      address = trace_address( trace[index] );
      data[0] = trace_data( trace[index] );
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
         uint32_t addr = trace_address( trace[re(index+i)] );
         if( addr == expectedaddress )
         {
            data[++offset] = trace_data( trace[re(index+i)] );
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

   /* step 2: run some assumptions */
   index = start;
   for( i = 0; i < entries; i++ )
   {
      uint32_t addr5 = trace_address( trace[re(index-5)] );
      uint32_t addr4 = trace_address( trace[re(index-4)] );
      uint32_t addr3 = trace_address( trace[re(index-3)] );
      uint32_t addr2 = trace_address( trace[re(index-2)] );
      uint32_t addr1 = trace_address( trace[re(index-1)] );

      uint16_t addr  = trace_address( trace[re(index)] );
      uint8_t  data  = trace_data( trace[re(index)] );

      /* detect processed interruptions */
      if( ((addr5 & 0xFF00) == 0x0100) && // check stack access
          ((addr4 & 0xFF00) == 0x0100) &&
          ((addr3 & 0xFF00) == 0x0100) &&
          ( addr2           >= 0xFFFA) &&
          ( addr1           >= 0xFFFB) )
      {
         eval[re(index-5)] = 0; // writing to stack
         eval[re(index-4)] = 0;
         eval[re(index-3)] = 0;
         eval[re(index-2)] = 0; // reading reset/nmi/irq vector
         eval[re(index-1)] = 0;
         eval[re(index  )] = 9; // executing first instruction
      }

      if( data == 0x20 )
      {
         /* JSR cycles are:
          * 0: opcode
          * 1: read low
          * 2: adjust stack (not on 65CE02)
          * 3: write stack high
          * 4: write stack low
          * 5: read high
          */
         int bc = disass_basecycles(data); /* can't be hardcoded due to 65CE02 */
         if( (trace_address( trace[re(index+1)] ) == addr+1) &&
             (trace_address( trace[re(index+bc-1)] ) == addr+2) &&
             trace_is_write( trace[re(index+bc-2)] ) &&
             trace_is_write( trace[re(index+bc-3)] )
            )
         {
            eval[re(index)] = 9;
            for( int n = 1; n < bc; ++n )
            {
               eval[re(index+n)] = 0;
            }
            eval[re(index+bc)] = 9;
         }
      }
      else if( data == 0x60 )
      {
         int bc = disass_basecycles(data); /* can't be hardcoded due to 65CE02 */
         uint16_t target;
         if( bc == 6 )
         {
            /* RTS cycles are:
             * 0: opcode
             * 1: dummy read
             * 2: stack+0 dummy (not on 65CE02)
             * 3: stack+1 target high
             * 4: stack+2 target low
             * 5: dummy read to inc PC (not on 65CE02)
             */
            target = trace_data( trace[re(index+3)] ) |
                    (trace_data( trace[re(index+4)] ) << 8);
            if( (trace_address( trace[re(index+1)] ) == addr+1) &&
                (trace_address( trace[re(index+2)] ) ==
                   (trace_address( trace[re(index+3)] ) - 1) ) &&
                (trace_address( trace[re(index+3)] ) ==
                   (trace_address( trace[re(index+4)] ) - 1) ) &&
                (trace_address( trace[re(index+5)] ) == target ) )
            {
               eval[re(index)] = 9;
               for( int n = 1; n < bc; ++n )
               {
                  eval[re(index)+n] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
         else if( bc == 4 )
         {
            /* RTS cycles on 65CE02 are:
             * 0: opcode
             * 1: dummy read
             * 2: stack+0 target high
             * 3: stack+1 target low
             */
            target = trace_data( trace[re(index+2)] ) |
                    (trace_data( trace[re(index+3)] ) << 8);
            if( (trace_address( trace[re(index+1)] ) == addr+1) &&
                (trace_address( trace[re(index+2)] ) ==
                   (trace_address( trace[re(index+3)] ) - 1) ) &&
                (trace_address( trace[re(index+4)] ) == target+1 ) )
            {
               eval[re(index)] = 9;
               for( int n = 1; n < bc; ++n )
               {
                  eval[re(index)+n] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
      }
      else if( data == 0x6c )
      {
         int bc = disass_basecycles(data); /* can't be hardcoded due to 65CE02 */
         /* JMP() cycles are:
          * 0: opcode
          * 1: read low addr
          * 2: read high addr
          * 3: dummy read (not on 65CE02)
          * 4: target low
          * 5: target high
          */
         uint16_t jmpaddr = trace_data( trace[re(index+1)] ) |
                           (trace_data( trace[re(index+2)] ) << 8);
         uint16_t target  = trace_data( trace[re(index+bc-2)] ) |
                           (trace_data( trace[re(index+bc-1)] ) << 8);
         if( ( trace_address( trace[re(index+bc-2)] ) == jmpaddr ) &&
             ( trace_address( trace[re(index+bc-1)] ) == jmpaddr+1 ) &&
             ( trace_address( trace[re(index+bc)] ) == target )
            )
         {
            eval[re(index)] = 9;
            for( int n = 1; n < bc; ++n )
            {
               eval[re(index)+n] = 0;
            }
            eval[re(index+bc)] = 9;
         }
      }

      /* prefetch */
      if( addr1 == addr )
      {
         eval[re(index-1)] = 0; // dummy read
         ++eval[re(index)];
      }

      /* follow sure tracks */
      if( eval[re(index)] >= 9 )
      {
         int ni = getcycles( trace, entries, index );
         if( ni > 0 )
         {
            for( int n = 1; n < ni; ++n )
            {
               eval[re(index+n)] = 0;
            }
            eval[re(index+ni)] = 9;
         }
      }

      if( trace_is_write( trace[re(index)] ) )
      {
         /* after a write it's more likely an opcode fetch */
         if( eval[re(index+1)] > 0 )
         {
            ++eval[re(index+1)];
         }
         /* writes can never be opcodes */
         eval[re(index)] = 0;
      }

      if( eval[re(index)] > 3 )
      {
         int ni = getcycles( trace, entries, index );
         if( ni > 0 )
         {
            for( int n = 1; n < ni; ++n )
            {
               --eval[re(index+n)];
            }
            ++eval[re(index+ni)];
         }
      }

      index = re(index+1);
   }

   /* final step: clear unused */
   for( i = 0; i < entries; ++i )
   {
      if( eval[i] < 0 )
      {
         eval[i] = 0;
      }
      else if( eval[i] > 9 )
      {
         eval[i] = 9;
      }
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
