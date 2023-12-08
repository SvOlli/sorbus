/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cpu_detect.h"
#include "bus.h"
#include <string.h>

#ifndef DEBUG_CPU_DETECT
#define DEBUG_CPU_DETECT (0)
#endif

#if DEBUG_CPU_DETECT
#include <stdio.h>
#endif

static uint32_t state;
static uint32_t address;

#include "cpudetect_mcp.h"

static uint8_t cpu_detect_result = 0xFF;


uint8_t cpu_detect_raw()
{
   return cpu_detect_result;
}


cputype_t cpu_detect()
{
   uint32_t cycles_left_reset = 8;
   uint32_t cycles_left_run = 100000;
   uint8_t memory[0x10];
   memcpy( &memory[0], &cpudetect_mcp[0], sizeof(memory) );

   // set lines to required state
   gpio_set_mask( bus_config.mask_rdy | bus_config.mask_irq | bus_config.mask_nmi );
#if DEBUG_CPU_DETECT
   printf( "cpu_detect start\n" );
#endif
   while( (0xFF == memory[0xF]) && (--cycles_left_run > 0) )
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
      sleep_us( 1 );

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
      address &= 0xF; // we only use 16 bytes of memory

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
      sleep_us( 9 );
#if DEBUG_CPU_DETECT
      printf( "%08x\n", gpio_get_all() );
#endif
      gpio_clr_mask( bus_config.mask_clock );
   }

#if DEBUG_CPU_DETECT
   printf( "cpu_detect done\n" );
#endif

   cpu_detect_result = memory[0xF];

   return get_cpu_type();   

}

cputype_t get_cpu_type(){

   // run complete, evaluate detected code
   switch( cpu_detect_result )
   {
      case 0x00:
         return CPU_6502;
      case 0x01:
         return CPU_65816;
      case 0xEA:
         return CPU_65C02;
      default:
         return CPU_UNDEF;
   }
}


const char *cputype_name( cputype_t cputype )
{
   switch( cputype )
   {
      case CPU_6502:
         return "6502";
         break;
      case CPU_65C02:
         return "65C02";
         break;
      case CPU_65816:
         return "65816";
         break;
      default:
         break;
   }
   return "unknown";
}
