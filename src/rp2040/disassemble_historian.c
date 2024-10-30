
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
   cputype_t   cpu;
   uint32_t    entries;
   char        *text;
};


static uint16_t getcycles( uint32_t *trace, uint32_t entries, uint32_t index )
{
   uint16_t address  = trace_address( trace[index] );
   uint8_t  data     = trace_data( trace[index] );

   uint16_t nextaddr = address + disass_bytes( data );
   uint8_t  bc       = disass_basecycles( data );

   if( disass_is_jump( data ) )
   {
      switch( disass_addrmode( data )  )
      {
         int8_t offset;
         case REL: /* 8-bit branch */
            offset = trace_data( trace[re(index+1)] );
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
            break;
         case ZPNR:
            offset = trace_data( trace[re(index+4)] );
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
   return bc;
}


static void disass_historian_run( disass_historian_t d, uint32_t *trace, uint32_t start )
{
   int index = 0, offset, i, n;
   uint16_t address, expectedaddress;
   uint8_t data[4];
   bool done;
   const uint32_t entries = d->entries;
   const cputype_t cpu = d->cpu;
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
   for( i = 0; i < entries; ++i )
   {
      uint32_t addr5 = trace_address( trace[re(index-5)] );
      uint32_t addr4 = trace_address( trace[re(index-4)] );
      uint32_t addr3 = trace_address( trace[re(index-3)] );
      uint32_t addr2 = trace_address( trace[re(index-2)] );
      uint32_t addr1 = trace_address( trace[re(index-1)] );

      uint16_t addr  = trace_address( trace[re(index)] );
      uint8_t  data  = trace_data( trace[re(index)] );

      uint16_t vector, target;

      if( cpu == CPU_65CE02 )
      {
         /* check for IRQ or NMI on 65CE02 */
             /* first check writing to stack, which can be 16-bit */
         if( trace_is_write( trace[re(index-5)] ) &&
             trace_is_write( trace[re(index-4)] ) &&
             trace_is_write( trace[re(index-3)] ) &&
             ((addr5-1) == addr4) &&
             ((addr4-1) == addr3) &&
             /* then check fetching the vector */
             ( addr2 >= 0xFFFA ) &&
             ( addr1 == (addr2 + 1) ) )
         {
            for( n = 1; n <= 5; ++n )
            {
               eval[re(index-n)] = 0;
            }
            eval[re(index)] = 9;
         }

         if( data == 0x23 )
         {
            /* JSR (IND,X) cycles are on 65CE02:
             * 0: opcode
             * 1: read vector low
             * 2: write stack high
             * 3: write stack low
             * 4: read vector high
             * 5: read target low
             * 6: read target high
             */
            int bc = 7;
            vector = trace_data( trace[re(index+1)] ) |
                    (trace_data( trace[re(index+4)] ) << 8);
            target = trace_data( trace[re(index+5)] ) |
                    (trace_data( trace[re(index+6)] ) << 8);
            if( (trace_address( trace[re(index+1)] ) == addr+1) &&
                (trace_address( trace[re(index+4)] ) == addr+2) &&
                trace_is_write( trace[re(index+2)] ) &&
                trace_is_write( trace[re(index+3)] ) &&
                (trace_address( trace[re(index+5)] ) == vector) &&
                (trace_address( trace[re(index+bc)] ) == target)
               )
            {
               eval[re(index)] = 9;
               for( n = 1; n < bc; ++n )
               {
                  eval[re(index+n)] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
      }

      if( cpu == CPU_65816 )
      {
         vector = trace_data( trace[re(index-2)] ) |
                 (trace_data( trace[re(index-1)] ) << 8);
         if( vector >= 0xFFF0 )
         {
            /* emulation mode vector table (8-bit mode) */
                /* check stack access */
            if( ((addr5 & 0xFF00) == 0x0100) &&
                ((addr4 & 0xFF00) == 0x0100) &&
                ((addr3 & 0xFF00) == 0x0100) &&
                (vector == trace_address(addr)) )
            {
               /* a false positive could be a JMP ($FFFx) stored in stack page */
               for( n = 1; n <= 6; ++n )
               {
                  eval[re(index-n)] = 0;
               }
               eval[re(index)] = 9; // executing first instruction
            }
         }
         else if( vector >= 0xFFE0 )
         {
            /* native mode vector table (16-bit mode) */
            uint32_t addr6 = trace_address( trace[re(index-6)] );
                /* check if four byte get written to the stack */
            if( trace_is_write( trace[re(index-6)] ) &&
                trace_is_write( trace[re(index-5)] ) &&
                trace_is_write( trace[re(index-4)] ) &&
                trace_is_write( trace[re(index-3)] ) &&
                (addr6 == (addr5+1)) &&
                (addr5 == (addr4+1)) &&
                (addr4 == (addr3+1)) &&
                (vector == trace_address(addr)) )
            {
               for( n = 1; n <= 6; ++n )
               {
                  eval[re(index-n)] = 0;
               }
            }
         }

         if( data == 0xFC )
         {
            /* JSR (IND,X) cycles are on 65816:
             * 0: opcode
             * 1: read vector low
             * 2: write stack high
             * 3: write stack low
             * 4: read vector high
             * 5: dummy read on same address again
             * 6: read target low
             * 7: read target high
             */
            int bc = 8;
            vector = trace_data( trace[re(index+1)] ) |
                    (trace_data( trace[re(index+4)] ) << 8);
            target = trace_data( trace[re(index+5)] ) |
                    (trace_data( trace[re(index+6)] ) << 8);
            if( (trace_address( trace[re(index+1)] ) == addr+1) &&
                (trace_address( trace[re(index+4)] ) == addr+2) &&
                trace_is_write( trace[re(index+2)] ) &&
                trace_is_write( trace[re(index+3)] ) &&
                (trace_address( trace[re(index+3)] ) == trace_address( trace[re(index+4)] )) &&
                (trace_address( trace[re(index+7)] ) == vector) &&
                (trace_address( trace[re(index+bc)] ) == target)
               )
            {
               eval[re(index)] = 9;
               for( n = 1; n < bc; ++n )
               {
                  eval[re(index+n)] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
      }

      /* check for reset on all
       * and on 6502, 65C02, 65SC02 check for IRQ/NMI */
          /* check stack access at page $01, both read or write */
      if( ((addr5 & 0xFF00) == 0x0100) &&
          ((addr4 & 0xFF00) == 0x0100) &&
          ((addr3 & 0xFF00) == 0x0100) &&
          /* then check fetching the vector */
          ( addr2           >= 0xFFFA) &&
          ( addr1            = addr2+1 ) )
      {
         /* a false positive could be a JMP ($FFFx) stored in stack page */
         for( n = 1; n <= 5; ++n )
         {
            eval[re(index-n)] = 0;
         }
         eval[re(index  )] = 9; // executing first instruction
      }

      if( eval[re(index)] > 0 )
      {
         if( data == 0x20 )
         {
            /* JSR ABS cycles are:
             * 0: opcode
             * 1: read low
             * 2: adjust stack (not on 65CE02)
             * 3: write stack high
             * 4: write stack low
             * 5: read high
             */
            const int bc = disass_basecycles(data); /* can't be hardcoded due to 65CE02 */
            target = trace_data( trace[re(index+1)] ) |
                    (trace_data( trace[re(index+bc-1)] ) << 8);
            if( (trace_address( trace[re(index+   1)] ) == addr+1) &&
                (trace_address( trace[re(index+bc-1)] ) == addr+2) &&
                trace_is_write( trace[re(index+bc-2)] ) &&
                trace_is_write( trace[re(index+bc-3)] ) &&
                (trace_address( trace[re(index+bc  )] ) == target)
               )
            {
               eval[re(index)] = 9;
               for( n = 1; n < bc; ++n )
               {
                  eval[re(index+n)] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
         else if( data == 0x40 )
         {
            const int bc = disass_basecycles(data); /* can't be hardcoded due to 65CE02 */
            /* RTI cycles are:
             * 0: opcode
             * 1: dummy read
             * 2: stack+0 dummy (not on 65CE02)
             * 3: stack+1 processor status
             * 4: stack+2 target low
             * 5: stack+3 target high
             */
            target = trace_data( trace[re(index+bc-2)] ) |
                    (trace_data( trace[re(index+bc-1)] ) << 8);
            if( (trace_address( trace[re(index+   1)] ) == addr+1) &&
                (trace_address( trace[re(index+bc-4)] ) == (trace_address( trace[re(index+bc-3)] ) - 1) ) &&
                (trace_address( trace[re(index+bc-3)] ) == (trace_address( trace[re(index+bc-2)] ) - 1) ) &&
                (trace_address( trace[re(index+bc-2)] ) == (trace_address( trace[re(index+bc-1)] ) - 1) ) &&
                (trace_address( trace[re(index+bc  )] ) == target) )
            {
               eval[re(index)] = 9;
               for( n = 1; n < bc; ++n )
               {
                  eval[re(index)+n] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
         else if( data == 0x4c )
         {
            const int bc = 3;
            /* JMP ABS cycles are:
             * 0: opcode
             * 1: read low addr
             * 2: read high addr
             */
            target  = trace_data( trace[re(index+1)] ) |
                     (trace_data( trace[re(index+2)] ) << 8);
            if( ( trace_address( trace[re(index+1)] ) == addr+1 ) &&
                ( trace_address( trace[re(index+2)] ) == addr+2 ) &&
                ( trace_address( trace[re(index+bc)] ) == target )
               )
            {
               eval[re(index)] = 9;
               for( n = 1; n < bc; ++n )
               {
                  eval[re(index)+n] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
         else if( data == 0x60 )
         {
            const int bc = disass_basecycles(data); /* can't be hardcoded due to 65CE02 */
            /* RTS cycles are:
             * 0: opcode
             * 1: dummy read
             * 2: stack+0 dummy (not on 65CE02)
             * 3: stack+1 target low
             * 4: stack+2 target high
             * 5: dummy read to inc PC (not on 65CE02)
             */
            if( bc == 6 )
            {
               target = trace_data( trace[re(index+3)] ) |
                       (trace_data( trace[re(index+4)] ) << 8);
               if( (trace_address( trace[re(index+1)] ) == addr+1) &&
                   (trace_address( trace[re(index+2)] ) == (trace_address( trace[re(index+3)] ) - 1) ) &&
                   (trace_address( trace[re(index+3)] ) == (trace_address( trace[re(index+4)] ) - 1) ) &&
                   (trace_address( trace[re(index+5)] ) == target ) &&
                   (trace_address( trace[re(index+bc)] ) == target+1 )
                  )
               {
                  eval[re(index)] = 9;
                  for( n = 1; n < bc; ++n )
                  {
                     eval[re(index)+n] = 0;
                  }
                  eval[re(index+bc)] = 9;
               }
            }
            else if( bc == 4 )
            {
               /* RTS cycles are on 65CE02:
                * 0: opcode
                * 1: dummy read
                * 2: stack+0 target high
                * 3: stack+1 target low
                */
               target = trace_data( trace[re(index+2)] ) |
                       (trace_data( trace[re(index+3)] ) << 8);
               if( (trace_address( trace[re(index+1)] ) == addr+1) &&
                   (trace_address( trace[re(index+2)] ) == (trace_address( trace[re(index+3)] ) - 1) ) &&
                   (trace_address( trace[re(index+bc)] ) == target+1 ) )
               {
                  eval[re(index)] = 9;
                  for( n = 1; n < bc; ++n )
                  {
                     eval[re(index)+n] = 0;
                  }
                  eval[re(index+bc)] = 9;
               }
            }
         }
         else if( data == 0x6c )
         {
            int bc = 6;
            /* JMP (ABS) cycles are:
             * 0: opcode
             * 1: read low addr
             * 2: read high addr
             * 3: dummy read (only on some variants)
             * 4: target low
             * 5: target high
             */
            vector = trace_data( trace[re(index+1)] ) |
                    (trace_data( trace[re(index+2)] ) << 8);
            target = trace_data( trace[re(index+bc-2)] ) |
                    (trace_data( trace[re(index+bc-1)] ) << 8);
            if( target != trace_address( trace[re(index+bc)] ) )
            {
               /* with dummy read did not succeed, try without */
               --bc;
               target = trace_data( trace[re(index+bc-2)] ) |
                       (trace_data( trace[re(index+bc-1)] ) << 8);
            }
            if( ( trace_address( trace[re(index+   1)] ) == addr+1 ) &&
                ( trace_address( trace[re(index+   2)] ) == addr+2 ) &&
                ( trace_address( trace[re(index+bc-2)] ) == vector ) &&
                ( trace_address( trace[re(index+bc-1)] ) == vector+1 ) &&
                ( trace_address( trace[re(index+bc  )] ) == target )
               )
            {
               eval[re(index)] = 9;
               for( n = 1; n < bc; ++n )
               {
                  eval[re(index)+n] = 0;
               }
               eval[re(index+bc)] = 9;
            }
         }
         else if( data == 0x7c )
         {
            const int bc = disass_basecycles(data); /* can't be hardcoded due to 65CE02 */
            /* JMP (ABS,X) cycles are:
             * 0: opcode
             * 1: read low base addr
             * 2: read high base addr
             * 3: dummy read (not on 65CE02)
             * 4: target low
             * 5: target high
             */
            vector = trace_data( trace[re(index+1)] ) |
                    (trace_data( trace[re(index+2)] ) << 8);
            target = trace_data( trace[re(index+bc-2)] ) |
                    (trace_data( trace[re(index+bc-1)] ) << 8);
            if( ( trace_address( trace[re(index+   1)] ) == addr+1 ) &&
                ( trace_address( trace[re(index+   2)] ) == addr+2 ) &&
                ( trace_address( trace[re(index+bc-2)] ) < (vector + 0x100) ) &&
                ( trace_address( trace[re(index+bc-2)] ) == (trace_address( trace[re(index+bc-1)] ) - 1) ) &&
                ( trace_address( trace[re(index+bc  )] ) == target )
               )
            {
               eval[re(index)] = 9;
               for( n = 1; n < bc; ++n )
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
            if( cpu != CPU_65CE02 )
            {
               // every CPU except for 65CE02 does a read after opcode fetch
               if( trace_address(trace[re(index+1)]) == (addr+1) )
               {
                  eval[re(index+1)] = 0;
               }
            }
         }
      }

      /* memory access */
      if( addr3+1 == addr ) /* LDA $1234,X */
      {
         --eval[re(index-2)]; // dummy read
         --eval[re(index-1)]; // read/write memory
         ++eval[re(index)];
      }
      else if( addr2+1 == addr ) /* LDA $1234 */
      {
         --eval[re(index-1)]; // read/write memory
         ++eval[re(index)];
      }

      /* follow sure tracks */
      if( eval[re(index)] >= 9 )
      {
         int ni = getcycles( trace, entries, index );
         if( ni > 0 )
         {
            for( n = 1; n < ni; ++n )
            {
               eval[re(index+n)] = 0;
            }
            if( re(index+ni) != start )
            {
               eval[re(index+ni)] = 9;
            }
         }
      }

      if( trace_is_write( trace[re(index)] ) )
      {
         /* after a write it's more likely an opcode fetch */
         if( eval[re(index+1)] > 0 )
         {
            ++eval[re(index+1)];
         }

         /* a read before a write at the same address can't be an opcode */
         if( trace_address( trace[re(index-1)] ) == trace_address( trace[re(index)] ) )
         {
            eval[re(index-1)] = 0;
         }

         /* writes can never be opcodes */
         eval[re(index)] = 0;
      }

      if( eval[re(index)] > 3 )
      {
         int ni = getcycles( trace, entries, index );
         if( ni > 0 )
         {
            for( n = 1; n < ni; ++n )
            {
               --eval[re(index+n)];
            }
            if( re(index+ni) != start )
            {
               ++eval[re(index+ni)];
            }
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


disass_historian_t disass_historian_init( cputype_t cpu,
   uint32_t *trace, uint32_t entries, uint32_t start )
{
   disass_historian_t d = (disass_historian_t)malloc( sizeof( struct disass_historian_s ) );

   // TODO: find something better than assert (or somewhere else to check?)
   assert( (entries & (entries-1)) == 0 );

   d->cpu     = cpu;
   d->entries = entries;
   d->text    = (char*)malloc( entries * DISASS_MAX_STR_LENGTH );

   memset( d->text, 0, entries * DISASS_MAX_STR_LENGTH );
   disass_cpu( cpu );
   disass_historian_run( d, trace, start );

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
