
#include "disassemble.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PREFETCH 0
#define BOUNDSBUFFER (8)

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
   fullinfo_t  *fullinfo;
};

static uint16_t getcycles( disass_historian_t d, int index )
{
   fullinfo_t *fullinfo = d->fullinfo;
   uint16_t address     = fullinfo[index].address;
   uint8_t  data        = fullinfo[index].data;

   uint16_t nextaddr    = address + disass_bytes( data );
   uint8_t  bc          = disass_basecycles( data );

   if( disass_is_jump( data ) )
   {
      switch( disass_addrmode( data )  )
      {
         int8_t offset;
         uint16_t indirect;
         case AI: /* JMP ($0000) */
         case AIX: /* JMP ($0000,X) */
            nextaddr = fullinfo[index+bc].address;
            indirect = fullinfo[index+bc-2].data | (fullinfo[index+bc-1].data << 8);
            if( indirect == nextaddr )
            {
               return bc;
            }
            /* let's check if we need one cycle more */
            nextaddr = fullinfo[index+bc+1].address;
            indirect = fullinfo[index+bc-1].data | (fullinfo[index+bc].data << 8);
            if( indirect == nextaddr )
            {
               return bc + 1;
            }
            break;
         case REL: /* 8-bit branch */
            offset = fullinfo[index+1].data;
            nextaddr = address + 2 + offset;
            if( fullinfo[index+2].address == nextaddr )
            {
               /* 65CE02 branch taken */
               return 2;
            }
            else if( fullinfo[index+3].address == nextaddr )
            {
               /* branch taken */
               return 3;
            }
            else if( fullinfo[index+4].address == nextaddr )
            {
               /* branch taken crossing page */
               return 4;
            }
            /* assume branch not taken */
            return 2;
            break;
         case ZPNR:
            offset = fullinfo[index+4].data;
            nextaddr = address + 3 + offset;
            if( fullinfo[index+4].address == nextaddr )
            {
               /* 65CE02 branch taken... or not, who cares? */
               return 4;
            }
            else if( fullinfo[index+6].address == nextaddr )
            {
               /* branch taken */
               return 6;
            }
            else if( fullinfo[index+7].address == nextaddr )
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
      /* todo add BCD check for 65(S)C02 here! */
      switch( disass_extracycles( data ) )
      {
         case 0:
            return bc;
            break;
         case 1:
         case 2:
            if( fullinfo[index+bc].address == nextaddr )
            {
               return bc;
            }
            else if( fullinfo[index+bc+2].address == nextaddr )
            {
               return bc+1;
            }
            else if( fullinfo[index+bc+2].address == nextaddr )
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


static void disass_historian_fulldata( disass_historian_t d, const uint32_t *trace, uint32_t start )
{
   fullinfo_t *fullinfo = (fullinfo_t*)(d->fullinfo);
   int index = start, offset, i, n;
   uint16_t expectedaddress;
   bool done;
   const uint32_t entries = d->entries;

   for( i = BOUNDSBUFFER; i < (entries+BOUNDSBUFFER); ++i )
   {
      // set up bits 31-0: flags, data, address
      fullinfo[i].raw   = trace[index];
      fullinfo[i].eval  = 3;

      /* find arguments */
      expectedaddress   = fullinfo[i].address+1;
      done = false;
      n = 1;
      offset = 0;
      while( !done )
      {
         uint32_t addr = trace_address( trace[re(index+n)] );
         if( addr == expectedaddress )
         {
            switch( ++offset )
            {
               case 1:
                  fullinfo[i].data1 = trace_data( trace[re(index+n)] );
                  break;
               case 2:
                  fullinfo[i].data2 = trace_data( trace[re(index+n)] );
                  break;
               case 3:
                  fullinfo[i].data3 = trace_data( trace[re(index+n)] );
                  // found all parameters
                  done = true;
                  break;
               default:
                  break;
            }
            fullinfo[i].bits30_31 = offset > 3 ? 3 : offset;
            ++expectedaddress;
         }

         if( ++n > 10 ) // longest 65xx instuction takes 8 clock cycles
         {
            // could not find all parameters, well duh!
            done = true;
         }
      }

      if( ++index >= entries )
      {
         // clean wrap around
         index = 0;
      }
   }
}


static void disass_historian_assumptions( disass_historian_t d )
{
   cputype_t cpu = d->cpu;
   fullinfo_t *fullinfo = d->fullinfo;
   int i, n;
   /* step 2: run some assumptions */

   for( i = BOUNDSBUFFER; i < (d->entries + BOUNDSBUFFER); ++i )
   {
      uint32_t addr5 = fullinfo[i-5].address;
      uint32_t addr4 = fullinfo[i-4].address;
      uint32_t addr3 = fullinfo[i-3].address;
      uint32_t addr2 = fullinfo[i-2].address;
      uint32_t addr1 = fullinfo[i-1].address;

      uint16_t addr  = fullinfo[i].address;
      uint8_t  data  = fullinfo[i].data;

      uint16_t vector, target;

      if( cpu == CPU_65CE02 )
      {
         /* check for IRQ or NMI on 65CE02 */
             /* first check writing to stack, which can be 16-bit */
         if( !fullinfo[i-5].rw && !fullinfo[i-4].rw && !fullinfo[i-3].rw &&
             ((addr5-1) == addr4) &&
             ((addr4-1) == addr3) &&
             /* then check fetching the vector */
             ( addr2 >= 0xFFFA ) &&
             ( addr1 == (addr2 + 1) ) )
         {
            for( n = 1; n <= 5; ++n )
            {
               fullinfo[i-n].eval = 0;
            }
            fullinfo[i].eval = 9;
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
            vector = fullinfo[i+1].data | (fullinfo[i+4].data << 8);
            target = fullinfo[i+5].data | (fullinfo[i+6].data << 8);
            if( (fullinfo[i+1].address == addr+1) &&
                (fullinfo[i+4].address == addr+2) &&
                !fullinfo[i+2].rw &&!fullinfo[i+2].rw &&
                (fullinfo[i+5].address == vector) &&
                (fullinfo[i+bc].address == target)
               )
            {
               fullinfo[i].eval = 9;
               for( n = 1; n < bc; ++n )
               {
                  fullinfo[i+n].eval = 0;
               }
               fullinfo[i+bc].eval = 9;
            }
         }
      }

      if( cpu == CPU_65816 )
      {
#if 0
         vector = fullinfo[i-2].data | (fullinfo[i-1].data << 8);
         if( vector >= 0xFFF0 )
         {
            /* emulation mode vector table (8-bit mode) */
                /* check stack access */
            if( ((addr5 & 0xFF00) == 0x0100) &&
                ((addr4 & 0xFF00) == 0x0100) &&
                ((addr3 & 0xFF00) == 0x0100) &&
                (vector == addr) )
            {
               /* a false positive could be a JMP ($FFFx) stored in stack page */
               for( n = 1; n <= 6; ++n )
               {
                  fullinfo[i-n].eval = 0;
               }
               fullinfo[i].eval = 9; // executing first instruction
            }
         }
         else if( vector >= 0xFFE0 )
         {
            /* native mode vector table (16-bit mode) */
            uint32_t addr6 = trace_address( trace[re(index-6)] );
                /* check if four byte get written to the stack */
            if( !fullinfo[i-6].rw && !fullinfo[i-5].rw
                !fullinfo[i-4].rw && !fullinfo[i-3].rw
                (addr6 == (addr5+1)) &&
                (addr5 == (addr4+1)) &&
                (addr4 == (addr3+1)) &&
                (vector == addr) )
            {
               for( n = 1; n <= 6; ++n )
               {
                  fullinfo[i-n].eval = 0;
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
                    (trace_data( trace[re(index+4)] << 8);
            target = trace_data( trace[re(index+5)] ) |
                    (trace_data( trace[re(index+6)] << 8);
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
#endif
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
            fullinfo[i-n].eval = 0;
         }
         fullinfo[i].eval = 9; // executing first instruction
      }

      if( fullinfo[i].eval > 0 )
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
            target = fullinfo[i+1].data | (fullinfo[i+bc-1].data << 8);
            if( (fullinfo[i+   1].address == addr+1) &&
                (fullinfo[i+bc-1].address == addr+2) &&
                !fullinfo[i+bc-2].rw && !fullinfo[i+bc-3].rw &&
                (fullinfo[i+bc  ].address == target)
               )
            {
               fullinfo[i].eval = 9;
               for( n = 1; n < bc; ++n )
               {
                  fullinfo[i+n].eval = 0;
               }
               fullinfo[i+bc].eval = 9;
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
            target = fullinfo[i+bc-2].data | (fullinfo[i+bc-1].data << 8);
            if( (fullinfo[i+   1].address == addr+1) &&
                (fullinfo[i+bc-4].address == (fullinfo[i+bc-3].address - 1)) &&
                (fullinfo[i+bc-3].address == (fullinfo[i+bc-2].address - 1)) &&
                (fullinfo[i+bc-2].address == (fullinfo[i+bc-1].address - 1)) &&
                (fullinfo[i+bc  ].address == target) )
            {
               fullinfo[i].eval = 9;
               for( n = 1; n < bc; ++n )
               {
                  fullinfo[i+n].eval = 0;
               }
               fullinfo[i+bc].eval = 9;
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
            target = fullinfo[i+1].data | (fullinfo[i+2].data << 8);
            if( ( fullinfo[i+ 1].address == addr+1 ) &&
                ( fullinfo[i+ 2].address == addr+2 ) &&
                ( fullinfo[i+bc].address == target ) )
            {
               fullinfo[i].eval = 9;
               for( n = 1; n < bc; ++n )
               {
                  fullinfo[i+n].eval = 0;
               }
               fullinfo[i+bc].eval = 9;
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
               target = fullinfo[i+3].data | (fullinfo[i+4].data << 8);
               if( (fullinfo[i+ 1].address == addr+1) &&
                   (fullinfo[i+ 2].address == (fullinfo[i+3].address - 1) ) &&
                   (fullinfo[i+ 3].address == (fullinfo[i+4].address - 1) ) &&
                   (fullinfo[i+ 5].address == target ) &&
                   (fullinfo[i+bc].address == target+1 )
                  )
               {
                  fullinfo[i].eval = 9;
                  for( n = 1; n < bc; ++n )
                  {
                     fullinfo[i+n].eval = 0;
                  }
                  fullinfo[i+bc].eval = 9;
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
               target = fullinfo[i+2].data | (fullinfo[i+3].data << 8);
               if( (fullinfo[i+ 1].address == addr+1) &&
                   (fullinfo[i+ 2].address == (fullinfo[i+3].address - 1) ) &&
                   (fullinfo[i+bc].address == target+1 ) )
               {
                  fullinfo[i].eval = 9;
                  for( n = 1; n < bc; ++n )
                  {
                     fullinfo[i+n].eval = 0;
                  }
                  fullinfo[i+bc].eval = 9;
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
            vector = fullinfo[i+1].data    | (fullinfo[i+2].data << 8);
            target = fullinfo[i+bc-2].data | (fullinfo[i+bc-1].data << 8);
            if( target != fullinfo[i+bc].address )
            {
               /* with dummy read did not succeed, retry without */
               --bc;
               target = fullinfo[i+bc-2].data | (fullinfo[i+bc-1].data << 8);
            }
            if( ( fullinfo[i+   1].address == addr+1 ) &&
                ( fullinfo[i+   2].address == addr+2 ) &&
                ( fullinfo[i+bc-2].address == vector ) &&
                ( fullinfo[i+bc-1].address == vector+1 ) &&
                ( fullinfo[i+bc  ].address == target )
               )
            {
               fullinfo[i].eval = 9;
               for( n = 1; n < bc; ++n )
               {
                  fullinfo[i+n].eval = 0;
               }
               fullinfo[i+bc].eval = 9;
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
            vector = fullinfo[i+1].data    | (fullinfo[i+2].data << 8);
            target = fullinfo[i+bc-2].data | (fullinfo[i+bc-1].data << 8);
            if( ( fullinfo[i+   1].address == addr+1 ) &&
                ( fullinfo[i+   2].address == addr+2 ) &&
                ( fullinfo[i+bc-2].address < (vector + 0x100) ) &&
                ( fullinfo[i+bc-2].address == (fullinfo[i+bc-1].address - 1) ) &&
                ( fullinfo[i+bc  ].address == target )
               )
            {
               fullinfo[i].eval = 9;
               for( n = 1; n < bc; ++n )
               {
                  fullinfo[i+n].eval = 0;
               }
               fullinfo[i+bc].eval = 9;
            }
         }

#if PREFETCH
         /* prefetch */
         if( (addr1 == addr) &&
             ((cpu == CPU_6502) || (cpu == CPU_6502RA)) )
         {
            fullinfo[i-1].eval = 0; // dummy read
            ++(fullinfo[i].eval);
            if( cpu != CPU_65CE02 )
            {
               // every CPU except for 65CE02 does a read after opcode fetch
               if( (fullinfo[i].eval > 2 ) &&
                   (fullinfo[i+1].address == (addr+1)) )
               {
                  fullinfo[i+1].eval = 0;
               }
            }
         }
#endif
      }

      /* memory access */
      if( addr3+1 == addr ) /* LDA $1234,X */
      {
         --(fullinfo[i-2].eval); // dummy read
         --(fullinfo[i-1].eval); // read/write memory
         ++(fullinfo[i  ].eval);
      }
      else if( addr2+1 == addr ) /* LDA $1234 */
      {
         --(fullinfo[i-1].eval); // read/write memory
         ++(fullinfo[i  ].eval);
      }

      /* follow sure tracks */
      if( fullinfo[i].eval >= 9 )
      {
         int ni = getcycles( d, i );
         if( ni > 0 )
         {
            for( n = 1; n < ni; ++n )
            {
               fullinfo[i+n].eval = 0;
            }
         }
      }

      if( !fullinfo[i].rw )
      {
         /* after a write it's more likely an opcode fetch */
         if( fullinfo[i+1].eval > 0 )
         {
            ++(fullinfo[i+1].eval);
         }

         /* a read before a write at the same address can't be an opcode */
         if( fullinfo[i-1].address == fullinfo[i].address )
         {
            fullinfo[i-1].eval = 0;
         }

         /* writes can never be opcodes */
         fullinfo[i].eval = 0;
      }

      if( fullinfo[i].eval > 3 )
      {
         int ni = getcycles( d, i );
         if( disass_bcdextracycles( data ) )
         {
            /* check if an extra cycle was taken */
            if( fullinfo[i+ni].address == fullinfo[i+ni+1].address )
            {
               ++ni;
            }
         }
         if( ni > 0 )
         {
            for( n = 1; n < ni; ++n )
            {
               --(fullinfo[i+n].eval);
            }
         }
      }
      
   }

   for( i = BOUNDSBUFFER; i < (d->entries + BOUNDSBUFFER); ++i )
   {
      if( fullinfo[i].eval < 0 )
      {
         fullinfo[i].eval = 0;
      }
      if( fullinfo[i].eval > 9 )
      {
         fullinfo[i].eval = 9;
      }
   }
}


disass_historian_t disass_historian_init( cputype_t cpu,
   uint32_t *trace, uint32_t entries, uint32_t start )
{
   disass_historian_t d = (disass_historian_t)malloc( sizeof( struct disass_historian_s ) );

   // TODO: find something better than assert (or somewhere else to check?)
   assert( (entries & (entries-1)) == 0 );

   d->cpu      = cpu;
   d->entries  = entries;
   d->fullinfo = (fullinfo_t*)calloc( entries+2*BOUNDSBUFFER, sizeof(fullinfo_t) );

   disass_set_cpu( cpu );
#if 1
   disass_historian_fulldata( d, trace, start );
   disass_historian_assumptions( d );
#else
   disass_historian_run( d, trace, start );
#endif

   return d;
}


void disass_historian_done( disass_historian_t d )
{
   memset( d->fullinfo, 0, d->entries * sizeof(uint64_t) );
   free( d->fullinfo );
   free( d );
}


const char *disass_historian_entry( disass_historian_t d, uint32_t entry )
{
   static char buffer[80];
   *buffer = 0;
   
   if( entry < d->entries )
   {
      fullinfo_t fullinfo = d->fullinfo[entry+BOUNDSBUFFER];

      snprintf( &buffer[0], sizeof(buffer)-1,
                "%5d:%s:%d:%s",
                entry,
                decode_trace( fullinfo.trace, false, 0 ),
                fullinfo.eval,
                fullinfo.eval < 3 ? "" : disass_trace( fullinfo ) );
   }
   return &buffer[0];
}
