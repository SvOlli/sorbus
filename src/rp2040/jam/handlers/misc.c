/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"
#include "cpu_detect.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include <pico/rand.h>
#include <pico/stdlib.h>


static alarm_id_t scratch_mem_alarm = 0;


const static uint8_t cpufeatures[] =
{
   0x00, // CPU_ERROR
   0x01, // CPU_6502:   NMOS
   0x06, // CPU_65C02:  CMOS | bit set/reset/branch
   0x12, // CPU_65816:  CMOS | 16 bit
   0x0e, // CPU_65CE02: CMOS | bit set/reset/branch | z-register
   0x21, // CPU_6502RA: NMOS without ROR
   0x02, // CPU_65SC02: CMOS
   0x00  // CPU_UNDEF
};


//
// internal helper functions
//


static inline void random_next()
{
   ram[MEM_ADDR_RANDOM] = rand() & 0xFF;
}


static int64_t scratch_mem_action( __unused alarm_id_t id,
                                   void *user_data )
{
   // bits are defined like this:
   // 0: use page $0000
   // 1: use page $0100
   // 2: use page $0200
   // 3: use page $0300
   // 4+5: $00 nothing, $10 save, $20 restore, $30 swap
   // 6: unused
   // 7: read-only: $80 copy still in progress, $00 done

   // not accessable RAM is used as scratch memory:
   // $D000-$D3FF 1k of scratch RAM, can be copied from/to $0000-$03FF
   uint32_t flags = (uint32_t)user_data;
   int i;

   scratch_mem_alarm = 0;

   for( i = 0; i < 4; ++i )
   {
      if( flags & (1 << i) )
      {
         switch( flags & 0x30 )
         {
            case 0x10:
               memcpy( &ram[0xD000+i*0x100], &ram[i*0x100], 0x100 );
               break;
            case 0x20:
               memcpy( &ram[i*0x100], &ram[0xD000+i*0x100], 0x100 );
               break;
            case 0x30:
               {
                  uint8_t tmpbuf[0x100];
                  memcpy( &tmpbuf[0], &ram[i*0x100], 0x100 );
                  memcpy( &ram[i*0x100], &ram[0xD000+i*0x100], 0x100 );
                  memcpy( &ram[0xD000+i*0x100], &tmpbuf[0], 0x100 );
               }
               break;
         }
      }
   }
   ram[MEM_ADDR_XRAMSW] = 0;

   return 0;
}


//
// API function
//


void misc_reset()
{
   // default bank is always first ROM bank
   romptr = bankptrs[1];
   ram[MEM_ADDR_BANK] = 1;
   ram[MEM_ADDR_CPUID] = cpufeatures[cputype];

   // (re)initialize random number generator
   srand( get_rand_32() );
   // make sure that random data is in RAM
   random_next();
}


void io_irq_misc_r( uint32_t in )
{
   switch( in & 0x07 )
   {
      case 2: // random
         random_next();
         break;
      default:
         break;
   }
}


void io_irq_misc_w( uint32_t in )
{
   switch( in & 0x07 )
   {
      case 0:
         if( ram[MEM_ADDR_BANK] >= NUMBER_OF_BANKS )
         {
            // sanitize bank number
            ram[MEM_ADDR_BANK] = 1;
         }
         break;
      case 1: // trap
         bus_stop_cause = META_TRAP;
         break;
      case 3: // swap pages
         scratch_mem_alarm = add_alarm_in_us( 2, scratch_mem_action,
                                              (void*)(uint32_t)ram[MEM_ADDR_XRAMSW], true );
         break;
      case 4: // cpu feature
         // just overwrite so it can't be modified for a longer time
         ram[MEM_ADDR_CPUID] = cpufeatures[cputype];
         break;
      default:
         break;
   }
}


void info_heap( char *buffer, size_t size )
{
   extern char __StackLimit, __bss_end__;
   struct mallinfo m = mallinfo();
   uint32_t total_heap = &__StackLimit  - &__bss_end__;
   uint32_t free_heap = total_heap - m.uordblks;

   snprintf( buffer, size,
             "heap total: %6lu\n"
             "      free: %6lu\n"
             "   minimum: %6lu"
             , total_heap, free_heap, ht_freemin() );
}


int info_sysvectors( char *buffer, size_t size )
{
   return snprintf( buffer, size,
                    "UVBRK:%04X UVNBI:%04X\n"
                    "UVNMI:%04X UVIRQ:%04X\n"
                    "B0NMI:%04X B0IRQ:%04X"
                    , *((uint16_t*)&ram[MEM_ADDR_UVBRK])
                    , *((uint16_t*)&ram[MEM_ADDR_UVNBI])
                    , *((uint16_t*)&ram[MEM_ADDR_UVNMI])
                    , *((uint16_t*)&ram[MEM_ADDR_UVIRQ])
                    , *((uint16_t*)&ram[0xFFFA])
                    , *((uint16_t*)&ram[0xFFFE]) );
}
