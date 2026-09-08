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


// identification
const uint8_t sorbus_id[] = { 'S', 'B', 'C', '2', '3', 1, 3, 0 };
const uint8_t *sorbus_id_p = &sorbus_id[0];
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

static inline void sorbus_id_next()
{
   // data has been fetched, get next
   ram[MEM_ADDR_CPUID] = *sorbus_id_p;

   // now move pointer
   if( *sorbus_id_p )
   {
      // no $00 copied, so there's still data
      ++sorbus_id_p;
   }
   else
   {
      // a $00 got copied to memory, now reset
      sorbus_id_p = sorbus_id;
   }
}


static inline void random_next()
{
   ram[MEM_ADDR_RANDOM] = rand() & 0xFF;
}


static inline void handle_scratch_mem( uint8_t flags )
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
   uint8_t tmpbuf[0x100];
   int i;

   ram[MEM_ADDR_XRAMSW] |= XRAMSW_STATUS_BIT;
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
               memcpy( &tmpbuf[0], &ram[i*0x100], 0x100 );
               memcpy( &ram[i*0x100], &ram[0xD000+i*0x100], 0x100 );
               memcpy( &ram[0xD000+i*0x100], &tmpbuf[0], 0x100 );
               break;
         }
      }
   }
   ram[MEM_ADDR_XRAMSW] &= ~XRAMSW_STATUS_BIT;
}


static inline void bank_select( uint8_t bank )
{
   // rom banks are counted starting with 1, as 0 is RAM
   if( bank > (sizeof(rom) / 0x2000) )
   {
      // fallback bank is always kernel
      bank = 1;
   }
   if( bank == 0 )
   {
      romvec = &ram[0xE000];
   }
   else
   {
      // right now with just a few banks, all banks are copied from flash
      // at startup
      // if there are too many banks, the data could be copied from flash
      // to a single 8K RAM buffer during each bankswitch
      romvec = &rom[(bank - 1) * 0x2000];
   }

   // make sure that RAM reflects config
   ram[MEM_ADDR_BANK] = bank;
}


//
// API function
//


void misc_reset()
{
   // default bank is always first ROM bank
   bank_select( 1 );

   // (re)initialize random number generator
   srand( get_rand_32() );
   // make sure that random data is in RAM
   random_next();

   // write first byte to buffer
   ram[MEM_ADDR_CPUID] = sorbus_id[0];
   // set pointer to next;
   sorbus_id_p = &sorbus_id[1];
}


void io_post_misc( bool rw, uint8_t data, uint16_t address )
{
   switch( address & 0x7 )
   {
      case 0: // bank
         if( !rw )
         {
            bank_select( ram[address] );
         }
         break;
      case 1: // trap/id
         if( rw )
         {
            // read
            sorbus_id_next();
         }
         else
         {
            // write
            ram[MEM_ADDR_CPUID] = *sorbus_id_p; // restore desired value
            bus_stop_cause = BUSMSG_META_TRAP;
         }
         break;
      case 2: // random
         random_next();
         break;
      case 3: // swap pages
         if( !rw )
         {
            // signal that it's done
            handle_scratch_mem( ram[MEM_ADDR_XRAMSW] );
         }
         break;
      case 4: // cpu feature
         // just overwrite so it can't be modified
         ram[MEM_ADDR_CPUID] = cpufeatures[cputype];
         break;
      default:
         // just as RAM
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
