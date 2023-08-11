/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * unfinished so far, idea how to dump CPU internal state
 */

#include "reg_dump.h"
#include "bus.h"

#include "cpustate.h"

// memcpy
#include <string.h>

typedef struct {
   uint8_t A;
   uint8_t X;
   uint8_t Y;
   uint8_t SP;
   uint8_t P;
} cpu65c02_regs_t;

typedef enum {
   CPU65C02_N = 1 << 7,
   CPU65C02_V = 1 << 6,
   CPU65C02__ = 1 << 5,
   CPU65C02_B = 1 << 4,
   CPU65C02_D = 1 << 3,
   CPU65C02_I = 1 << 2,
   CPU65C02_C = 1 << 1,
   CPU65C02_Z = 1 << 0
} cpu65c02_flags_t;

cpu65c02_regs_t reg_dump()
{
   cpu65c02_regs_t retval;
   uint32_t cycles_left_reset = 8;
   uint8_t memory[0x20];
   uint8_t stack[0x10];
   int consecutive_stack_reads = 0;

   memcpy( &memory[0], &cpustate[0], sizeof(memory) );

   while(consecutive_stack_reads < 4)
   {
      // done: set clock to high
      gpio_set_mask( bus_config.mask_clock );

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

      if( (address & 0xFF00) = 0x100 )
      {
         // handle stack

         // we don't need a full stack 16 bytes wrapping is way more then enough
         address &= 0xF;

         if( state & bus_config.mask_rw )
         {
            // read from memory and write to bus
            gpio_put_masked( bus_config.mask_data, ((uint32_t)stack[address]) << bus_config.shift_data );
            ++consecutive_stack_reads;
         }
         else
         {
            // read from bus and write to memory write
            stack[address] = (gpio_get_all() >> bus_config.shift_data); // truncate is intended
            consecutive_stack_reads = 0;
         }
      }
      else
      {
         // handle non-stack address space

         address &= 0x1F; // we only use 32 bytes of memory
         consecutive_stack_reads = 0;

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
      }

      // done: set clock to low
      gpio_clr_mask( bus_config.mask_clock );
   }

   retval.A  = memory[0x00];
   retval.X  = memory[0x01];
   retval.Y  = memory[0x02];
   retval.SP = memory[0x03];
   retval.P  = memory[0x04];

   return retval;
}

