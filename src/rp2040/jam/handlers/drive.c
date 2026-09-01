/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"
#include "event_queue.h"
#include "dhara_flash.h"
#include "3rdparty/dhara/error.h"


uint16_t dhara_flash_size = 0;
#define DHARA_SYNC_DELAY (100000)


void event_flash_sync( __unused uint32_t value )
{
   // event handler for flushing dhara journal
   // this event is max 1 time in queue

   dhara_flash_sync();
}



void io_post_intdrive( bool rw, uint8_t data, uint16_t address )
{
   uint16_t *lba = (uint16_t*)&ram[MEM_ADDR_ID_LBA];
   uint16_t *mem = (uint16_t*)&ram[MEM_ADDR_ID_MEM];
   int retval = 0;

   if( rw )
   {
      // nothing needs to be done after read
      return;
   }

   // signalize that work was started
   ram[address] = 0x00;
   // sanity checks
   if( !dhara_flash_size )
   {
      // size = 0: no dhara image found
      ram[address] |= 0xF0;
   }
   // filter out bad ranges, removing second line would write protect ROM
   if( (*mem < 0x0004) ||                                    // zeropage I/O
       ((*mem > (0xD000-SECTOR_SIZE)) && (*mem < 0xDF80)) || // $Dxxx I/O
       (*mem > (0x10000-SECTOR_SIZE)) )                      // would go out of bounds
   {
      // DMA would run into I/O which is not possible, only RAM works
      ram[address] |= 0xF1;
   }
   if( *lba >= 0x9000 ) // could also be if( *lba >= dhara_flash_size )
   {
      // only 36864 sectors are available
      // sectors up to 32767 are used with CP/M-fs
      // sectors 32768 to 36863 are only accessable via raw sector read write
      ram[address] |= 0xF2;
   }
   if( ram[address] )
   {
      // error, do not continue
      return;
   }

   switch( address & 7 )
   {
      case 4: // read
         retval = dhara_flash_read( *lba, &ram[*mem] );
         if( queue_event_contains( BUSMSG_EVENT_FLASH_SYNC ) )
         {
            // delay any waiting sync events
            queue_event_cancel( BUSMSG_EVENT_FLASH_SYNC );
            queue_event_add( DHARA_SYNC_DELAY, BUSMSG_EVENT_FLASH_SYNC );
         }
         break;
      case 5: // write
         retval = dhara_flash_write( *lba, &ram[*mem] );
         // drop any waiting sync events, since a new one is created
         queue_event_cancel( BUSMSG_EVENT_FLASH_SYNC );
         // after ~.1 seconds without additions writes run sync
         queue_event_add( DHARA_SYNC_DELAY, BUSMSG_EVENT_FLASH_SYNC );
         break;
      case 7: // trim
         retval = dhara_flash_trim( *lba );
         // drop any waiting sync events, since a new one is created
         queue_event_cancel( BUSMSG_EVENT_FLASH_SYNC );
         // after ~.1 seconds without additions writes run sync
         queue_event_add( DHARA_SYNC_DELAY, BUSMSG_EVENT_FLASH_SYNC );
         break;
      default:
         // unused addresses are treated like RAM, already handled
         break;
   }

   if( retval )
   {
      // report error, do not continue
      ram[address] = 0xF4;
      return;
   }

   // increment pointers
   (*lba)++;
   (*mem) += SECTOR_SIZE;
   // signalize that operation was completed successfully
   ram[address] = 0x80;
}


int info_internaldrive( char *buffer, size_t size )
{
   dhara_flash_info_t dhara_info;
   uint16_t *lba = (uint16_t*)&ram[MEM_ADDR_ID_LBA];
   uint16_t *mem = (uint16_t*)&ram[MEM_ADDR_ID_MEM];

   dhara_flash_info( &dhara_info );
   uint64_t hw_size  = dhara_info.erase_cells * dhara_info.erase_size;
   uint64_t lba_size = dhara_info.sectors * dhara_info.sector_size;

   return snprintf( buffer, size,
      /*01*/ "hw sector size:  %08lx (%lu)\n"
      /*02*/ "hw num sectors:  %08lx (%lu)\n"
      /*03*/ "hw size:         %6.2fMB\n"
      /*04*/ "page size:       %08lx (%lu)\n"
      /*05*/ "pages:           %08lx (%lu)\n"
      /*06*/ "lba sector size: %08lx (%lu)\n"
      /*07*/ "lba num sectors: %08lx (%lu)\n"
      /*08*/ "lba size:        %6.2fMB\n"
      /*09*/ "gc ratio         %08lx (%lu)\n"
      /*10*/ "read status:     %lu\n"
      /*11*/ "read error:      %s\n"
      /*12*/ "ID_LBA ($%04x):  %04x (sector used for next transfer)\n"
      /*13*/ "ID_MEM ($%04x):  %04x (memory used for next transfer)\n"
      /*01*/ , dhara_info.erase_size, dhara_info.erase_size
      /*02*/ , dhara_info.erase_cells, dhara_info.erase_cells
      /*03*/ , (float)hw_size / (0x100000)   // convert to megabytes
      /*04*/ , dhara_info.page_size, dhara_info.page_size
      /*05*/ , dhara_info.pages, dhara_info.pages
      /*06*/ , dhara_info.sector_size, dhara_info.sector_size
      /*07*/ , dhara_info.sectors, dhara_info.sectors
      /*08*/ , (float)lba_size / (0x100000)  // convert to megabytes
      /*09*/ , dhara_info.gc_ratio, dhara_info.gc_ratio
      /*10*/ , dhara_info.read_status
      /*11*/ , dhara_strerror( dhara_info.read_errcode )
      /*12*/ , MEM_ADDR_ID_LBA, *lba
      /*13*/ , MEM_ADDR_ID_MEM, *mem
   );
}
