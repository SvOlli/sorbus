
#include "disassemble.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


// to handle the ringbuffer input
#define ring(x,s) ((x) % (s))


static void disass_fulltrace_filldata( disass_fulltrace_t d, const uint32_t *trace,
                                       uint32_t start )
{
   fullinfo_t     *fullinfo   = d->fullinfo;
   int            index       = start, offset, i, n;
   uint16_t       expectedaddress;
   bool           done;
   const uint32_t entries     = d->entries;

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
         uint32_t addr = trace_address( trace[ring(index+n,entries)] );
         if( addr == expectedaddress )
         {
            switch( ++offset )
            {
               case 1:
                  fullinfo[i].data1 = trace_data( trace[ring(index+n,entries)] );
                  break;
               case 2:
                  fullinfo[i].data2 = trace_data( trace[ring(index+n,entries)] );
                  break;
               case 3:
                  fullinfo[i].data3 = trace_data( trace[ring(index+n,entries)] );
                  // found all parameters
                  done = true;
                  break;
               default:
                  break;
            }
            fullinfo[i].dataused = offset > 3 ? 3 : offset;
            ++expectedaddress;
         }

         if( ++n > BOUNDSBUFFER ) // longest 65xx instuction takes 8 clock cycles
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


disass_fulltrace_t disass_fulltrace_init( cputype_t cpu,
   uint32_t *trace, uint32_t entries, uint32_t start )
{
   disass_fulltrace_t d = (disass_fulltrace_t)malloc( sizeof( struct disass_fulltrace_s ) );

   // TODO: find something better than assert (or somewhere else to check?)
   assert( (entries & (entries-1)) == 0 );

   d->cpu      = cpu;
   d->entries  = entries;
   d->fullinfo = (fullinfo_t*)calloc( entries+2*BOUNDSBUFFER, sizeof(fullinfo_t) );

   disass_set_cpu( cpu );
   disass_fulltrace_filldata( d, trace, start );
   disass_historian_assumptions( d );

   return d;
}


void disass_fulltrace_done( disass_fulltrace_t d )
{
   memset( d->fullinfo, 0, d->entries * sizeof(uint64_t) );
   (void)mf_checkheap();
   free( d->fullinfo );
   free( d );
}


const char *disass_fulltrace_entry( disass_fulltrace_t d, uint32_t entry )
{
   static char buffer[80];
   *buffer = 0;
   
   if( entry < d->entries )
   {
      fullinfo_t fullinfo = d->fullinfo[entry+BOUNDSBUFFER];

#if 1
      snprintf( &buffer[0], sizeof(buffer)-1,
                "%5d:%s:%d:%s",
                entry,
                decode_trace( (uint32_t)fullinfo.raw, false, 0 ),
                fullinfo.eval,
                fullinfo.eval < 3 ? "" : disass_trace( fullinfo ) );
#else
      snprintf( &buffer[0], sizeof(buffer)-1,
                "%5ld:%s:%016llx:%d:%s",
                entry,
                decode_trace( (uint32_t)fullinfo.raw, false, 0 ),
                fullinfo.raw,
                fullinfo.eval,
                fullinfo.eval < 3 ? "" : disass_trace( fullinfo ) );
#endif
   }
   return &buffer[0];
}
