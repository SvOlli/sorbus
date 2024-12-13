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
#include "disassemble.h"

#define CYCLES_TOTAL (256)
#if (CYCLES_TOTAL & (CYCLES_TOTAL-1))
#error CYCLES_TOTAL is not a power of 2
#endif
#define SHOW_RAW_DUMP_IN_DEBUG 0


cputype_t cpu_detect( bool debug )
{
   uint32_t  trace[CYCLES_TOTAL];
   uint32_t  state;
   uint32_t  address;
   uint32_t  cycles_left_reset = 8;
   uint32_t  cycles_run = 0;
   bool      reset_done = false;
   cputype_t cputype;
   uint8_t   memory[0x20] = { 0 };

   memcpy( &memory[0], &cpudetect[0], sizeof(memory) );
   memset( &trace[0], 0, sizeof(trace) );

   // set lines to required state
   gpio_set_mask( bus_config.mask_rdy | bus_config.mask_irq | bus_config.mask_nmi );
   for( cycles_run = 0;
        (0x00 == memory[sizeof(memory)-1]) && (cycles_run < CYCLES_TOTAL);
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

   cputype = (memory[sizeof(memory)-1] < CPU_UNDEF) ? memory[sizeof(memory)-1] : CPU_ERROR;
   if( debug )
   {
      int lineno = 0;
#if SHOW_RAW_DUMP_IN_DEBUG
      printf( "TRACE_START %s\n", cputype_name( cputype ) );
      for( int i = 0; i < CYCLES_TOTAL; ++i )
      {
         printf( "%08x\n", i < cycles_run ? trace[i] : 0 );
      }
      printf( "TRACE_END\n" );
#endif
      disass_historian_t d = disass_historian_init( cputype ? cputype : CPU_6502,
                                                    &trace[0], CYCLES_TOTAL, 0 );
      hexdump_buffer( &cpudetect[0], sizeof(cpudetect) );
      for( int i = 0; i < cycles_run; ++i )
      {
         if( !disass_historian_entry( d, i ) )
         {
            break;
         }
         if( trace[i] & bus_config.mask_reset )
         {
            ++lineno;
         }
         printf( "%3d:%s:%s\n", lineno,
                 decode_trace( trace[i], false, 0 ),
                 disass_historian_entry( d, i ) );
      }
      disass_historian_done( d );
      hexdump_buffer( &memory[0], sizeof(memory) );
   }

   // run complete, evaluate detected code
   return cputype;
}
