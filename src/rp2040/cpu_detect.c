/**
 * Copyright (c) 2023-2024 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cpu_detect.h"
#include "bus.h"
#include <string.h>
#include <stdio.h>

#ifndef count_of
#define count_of(a) (sizeof(a)/sizeof(a[0]))
#endif

#include "cpudetect.h"

static char* cputype_names[] =
{
   "NONE",
   "6502",
   "65C02",
   "65816",
   "65CE02",
   "65SC02"
};


/* defined in disassemble.c */
void disass_hexdump( uint8_t *memory, uint16_t address, uint32_t size );


cputype_t cpu_detect( bool debug )
{
   uint32_t state;
   uint32_t address;
   uint32_t cycles_left_reset = 16;
   uint32_t cycles_left_run = 256;
   uint8_t memory[0x20] = { 0 };
   memcpy( &memory[0], &cpudetect[0], sizeof(memory) );

   if( debug )
   {
      disass_hexdump( &memory[0], 0, sizeof(memory) );
   }

   // set lines to required state
   gpio_set_mask( bus_config.mask_rdy | bus_config.mask_irq | bus_config.mask_nmi );
   while( (0x00 == memory[sizeof(memory)-1]) && (--cycles_left_run > 0) )
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
      address &= sizeof(memory)-1; // we only use 32 bytes of memory

      if( state & bus_config.mask_rw )
      {
         // read from memory and write to bus
         gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
      }
      else
      {
         // read from bus and write to memory write
         memory[address] = (gpio_get_all() >> bus_config.shift_data); // truncate is intended
      }

      // done: wait for things to settle and set clock to low
      sleep_us( 7 );
      if( debug )
      {
         printf( "%08x\n", gpio_get_all() );
      }
      gpio_clr_mask( bus_config.mask_clock );
   }

   if( debug )
   {
      disass_hexdump( &memory[0], 0, sizeof(memory) );
   }

   // run complete, evaluate detected code
   return memory[sizeof(memory)-1] < CPU_UNDEF ? memory[sizeof(memory)-1] : CPU_ERROR;
}


const char *cputype_name( cputype_t cputype )
{
   if( cputype >= count_of(cputype_names) )
   {
      cputype = CPU_ERROR;
   }
   return cputype_names[cputype];
}
