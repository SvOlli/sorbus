/**
 * Copyright (c) 2023-2024 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cpu_detect.h"
#include "bus.h"
#include <string.h>
#include <stdio.h>

#include "cpudetect.h"
#include "da_trace.h"

#define CYCLES_TOTAL (256)
#define SHOW_RAW_DUMP_IN_DEBUG 0

static uint32_t *last_trace = 0;

uint32_t *cpu_detect_trace()
{
   return last_trace;
}

cputype_t cpu_detect( bool debug )
{
   uint32_t  trace[CYCLES_TOTAL] = { 0 };
   uint32_t  state;
   uint32_t  address;
   uint32_t  cycles_left_reset = 8;
   uint32_t  cycles_run = 0;
   bool      reset_done = false;
   cputype_t cputype;
   uint8_t   memory[0x20] = { 0 };
   char      text[128] = { 0 };
   char      *b;
   int       bsize;
   int       used;

   memcpy( &memory[0], &cpudetect[0], sizeof(memory) );
   memset( &trace[0], 0, sizeof(trace) );

   // set lines to required state
   gpio_set_mask( bus_config.mask_rdy | bus_config.mask_irq | bus_config.mask_nmi );
   for( cycles_run = 0;
        (0x00 == memory[sizeof(memory)-1]) && (cycles_run < (CYCLES_TOTAL-1));
        ++cycles_run )
   {
      if( cycles_left_reset )
      {
         --cycles_left_reset;
         gpio_clr_mask( bus_config.mask_reset );
      }
      else
      {
         gpio_set_mask( bus_config.mask_reset );
      }

      // done: wait for things to settle and set clock to high
      sleep_us( 10 );
      gpio_set_mask( bus_config.mask_clock );
      // another delay before reading the bus, without it worked _most_ of the times
      sleep_us( 2 );

      // bus should be still valid from clock low
      state = gpio_get_all();

      // setup bus direction so I/O can settle
      if( state & bus_config.mask_rw )
      {
         // read from memory and write to bus
         gpio_set_dir_out_masked( bus_config.mask_data );
      }
      else
      {
         // read from bus and write to memory write
         gpio_set_dir_in_masked( bus_config.mask_data );
      }

      address = ((state & bus_config.mask_address) >> bus_config.shift_address);
      if( address == 0xFFFD )
      {
         // once the reset vector was read, the reset is complete and
         // write to memory is allowed
         reset_done = true;
      }
      address &= sizeof(memory)-1; // we only use 32 bytes of memory

      if( state & bus_config.mask_rw )
      {
         // read from memory and write to bus
         gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
      }
      else
      {
         // read from bus and write to memory write
         if( reset_done )
         {
            // reset cycle might write to $01ff which breaks detection
            memory[address] = (gpio_get_all() >> bus_config.shift_data); // truncate is intended
         }
      }

      // done: wait for things to settle and set clock to low
      sleep_us( 7 );
      trace[cycles_run] = gpio_get_all();
      gpio_clr_mask( bus_config.mask_clock );
   }
   // make sure we've got an end marker
   trace[++cycles_run] = 0x00000000;

   cputype = (memory[sizeof(memory)-1] < CPU_UNDEF) ? memory[sizeof(memory)-1] : CPU_ERROR;
   if( debug )
   {
      int lineno = 0;
#if SHOW_RAW_DUMP_IN_DEBUG
      printf( "TRACE_START %s\n", da_cputype_name( cputype ) );
      for( int i = 0; i < CYCLES_TOTAL; ++i )
      {
         printf( "%08x\n", i < cycles_run ? trace[i] : 0 );
      }
      printf( "TRACE_END\n" );
#endif
      da_trace_t d = da_trace_init( cputype ? cputype : CPU_6502,
                                    &trace[0], CYCLES_TOTAL, 0 );
      da_cc_start( d, 0 );
      print_hexdump_buffer( 0, &cpudetect[0], sizeof(cpudetect), false );
      for( int i = 0; i < cycles_run; ++i )
      {
         if( !d->fullinfo[i].raw )
         {
            break;
         }
         b     = &text[0];
         bsize = sizeof(text)-1;
         used  = 0;

         used += snprintf( b+used, bsize-used, "%3d:", lineno );
         used += da_snf_fullinfo( b+used, bsize-used, cputype,
                                  "a w d R: y",
                                  d->fullinfo[i], DA_FLAG_NONE );
         puts( text );
      }
      da_trace_done( d );
      print_hexdump_buffer( 0, &memory[0], sizeof(memory), false );
   }

   // make sure heap memory used is minimum
   last_trace = (uint32_t*)ht_realloc( last_trace, sizeof(uint32_t) * cycles_run );
   // copy trace from stack to heap
   memcpy( last_trace, &trace[0], sizeof(uint32_t) * cycles_run );

   // run complete, evaluate detected code
   return cputype;
}
