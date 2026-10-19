/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"
#include "bus.h"
#include "cpu_detect.h"
#include "da_base.h"

#include <pico/multicore.h>
//#include <hardware/clocks.h>

// number of states should be power of 2
#define BUSLOG_SIZE (1024)
#if (BUSLOG_SIZE & (BUSLOG_SIZE-1))
#error BUSLOG_SIZE is not a power of 2
#endif
uint32_t buslog_states[BUSLOG_SIZE] = { 0 };
uint32_t cycle_counter = 0;      // this is also hard bound to buslog_states index

uint8_t  ram[0x10000] = { 0 };   // 64k of RAM and I/O
uint8_t  rom[FLASH_DRIVE_START_TXT-FLASH_KERNEL_START_TXT]; // buffer for roms
const uint8_t *romptr;           // pointer into current ROM/RAM bank at $E000
uint16_t address = 0;
uint32_t bus_stop_cause = 0;
//uint32_t busstate;
uint32_t multicore_fifo_overflow_counter = 0;

uint8_t *bankptrs[0x100] = { 0 };


void setup_bankptrs()
{
   bankptrs[0] = &ram[0];
   for( uint i = 1; i < 0x100; ++i )
   {
      bankptrs[i] = i <= NUMBER_OF_BANKS ? &rom[(i-1) << 13] : &rom[0];
      bankptrs[i] -= 0xE000;
   }
}


void debug_raw_backtrace()
{
   printf( "\nTRACE_START %s\n", da_cputype_name( cputype ) );
   for( int i = cycle_counter; i < (cycle_counter + BUSLOG_SIZE); ++i )
   {
      printf( "%08lx\n", buslog_states[i & (BUSLOG_SIZE-1)] );
   }
   printf( "TRACE_END\n" );
}


void debug_get_backtrace( uint32_t **trace, uint32_t *entries, uint32_t *start )
{
   *trace   = &buslog_states[0];
   *entries = BUSLOG_SIZE;
   *start   = cycle_counter & (BUSLOG_SIZE - 1);
}


void info_bus_vars( char *b, size_t bsize )
{
   int   used = 0;
   int i;
   used += snprintf( b+used, bsize-used, "ram    @ 0x%p\n", &ram[0] );
   used += snprintf( b+used, bsize-used, "rom    @ 0x%p\n", &rom[0] );
   used += snprintf( b+used, bsize-used, "romptr @ 0x%p\n", romptr );
   for( i = 0; i < 7; ++i )
   {
      used += snprintf( b+used, bsize-used, "ptr[%d] @ 0x%p\n", i, bankptrs[i] );
   }
}


void bus_run()
{
   for(;;)
   {
      // clock starts with low
      // set at bottom of loop

      // valid here are:
      // - (65816 only) BA0-7, not supported
      // - RWB, MLB, VPB, A0-A15, VDA, VPA: tADS =  40ns @ 8MHz (~320ns @ 1MHz ~ 32%)
      // - RWB, MLB, VPB, A0-A15, VDA, VPA: tADS = 150ns @ 2MHz (~300ns @ 1MHz ~ 30%)

      // log last states
      if( gpio_get_all() & BUS_CONFIG_mask_rdy )
      {
         //if( (address & 0xFE00) == 0xDE00 )
         if( (address >> 8) == 0xDF )
         //if( (address >> 9) == 0x6F ) // this is faster
         {
            // this is a bit tricky as two things are done here:
            // on write (bit 24=0): only trigger at DF7F or lower
            // on read  (bit 24=1): only trigger at DF0F or lower
            if( (buslog_states[cycle_counter & (BUSLOG_SIZE-1)] & 0x0100007f)
                < 0x0100000f )
            {
               if( multicore_fifo_wready() )
               {
                  multicore_fifo_push_blocking_inline( buslog_states[cycle_counter & (BUSLOG_SIZE-1)] & BUS_CONFIG_mask_input );
               }
               else
               {
                  ++multicore_fifo_overflow_counter;
#if 1
                  internal_error( __FILE__, __LINE__, " fifo overflow", buslog_states[cycle_counter & (BUSLOG_SIZE-1)] );
#endif
               }
               if( address == 0xDF00 )
               {
                  romptr = bankptrs[ram[address]];
               }
            }
         }

         // state is not valid on the data when RP2040 is writing
         //buslog_states[cycle_counter & (BUSLOG_SIZE-1)] = gpio_get_all();
         cycle_counter++;
      }

      // set default: don't write to data bus -> input
      gpio_set_dir_in_masked( BUS_CONFIG_mask_data );

      // done: set clock to high
      gpio_set_mask( BUS_CONFIG_mask_clock );

      address = ((gpio_get_all() & BUS_CONFIG_mask_address) >> BUS_CONFIG_shift_address);

      /* memory map as handled here
       * x = handled here
       * $0000-$0003   external I/O
       * $0004-$CFFF x RAM
       * $D000-$D7FF   external I/O (guaranteed)
       * $D800-$DDFF   external I/O (planned)
       * $DE00-$DEFF   reserved (now: RAM)
       * $DF00-$DF7F x internal I/O (or just RAM)
       * $DF80-$DFFF x RAM
       * $E000-$FFFF x ROM/RAM
       */

      // setup data (will change with ASM implementation)
      //if( (address <= 0x0003) || ((address & 0xF800) == 0xD000) )
      if( (address < 0x0004) || ((address >> 11) == 0x1A) )
      {
         // external i/o: keep hands off the bus and just log
         buslog_states[cycle_counter & (BUSLOG_SIZE-1)] = gpio_get_all();
      }
      else
      {
         // address is set as global variable
         if( gpio_get_all() & BUS_CONFIG_mask_rw )
         {
            register uint32_t data;
            // CPU is reading -> RP2040 is writing
            gpio_set_dir_out_masked( BUS_CONFIG_mask_data );

            if( address < ROM_START )
            {
               data = ram[address];
            }
            else
            {
               data = romptr[address];
            }
            gpio_put_masked( BUS_CONFIG_mask_data, data << BUS_CONFIG_shift_data );
            buslog_states[cycle_counter & (BUSLOG_SIZE-1)] =
               (gpio_get_all() & ~BUS_CONFIG_mask_data) | (data << BUS_CONFIG_shift_data);
         }
         else
         {
            // CPU is writing -> RP2040 is reading

            // fetch again, as write data is available later on the bus than address
            // data. also, gpio_get_all() is faster than accessing a variable
            ram[address] = gpio_get_all() >> BUS_CONFIG_shift_data;
            buslog_states[cycle_counter & (BUSLOG_SIZE-1)] = gpio_get_all();
         }
      }

      // done: set clock to low
      gpio_clr_mask( BUS_CONFIG_mask_clock );
   }
}
