
#include "cpu_detect.h"
#include "bus.h"
#include <string.h>

static uint32_t state;
static uint32_t address;

#include "cpudetect_mcp.h"

cputype_t cpu_detect()
{
   uint32_t cycles_left_reset = 8;
   uint8_t memory[0x10];
   memcpy( &memory[0], &cpudetect_mcp[0], sizeof(memory) );

   while(0xFF == memory[0xF])
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
      sleep_us(1);
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
      sleep_us(1);
      gpio_clr_mask( bus_config.mask_clock );
   }

   // run complete, evaluate detected code
   switch( memory[0xF] )
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

