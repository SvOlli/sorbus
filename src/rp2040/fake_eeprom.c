/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <hardware/flash.h>

#include "fake_eeprom.h"

#define FLASH_CHUNK_SIZE FLASH_SECTOR_SIZE

static const uint8_t *flash_target_base = (const uint8_t *)XIP_BASE;

uint32_t flash_size_detect()
{
   uint32_t size;
   /* sizes:    1MB               16MB */
   for( size = (1 << 20); size < (1 << 25); size <<= 1 )
   {
      if( !memcmp( flash_target_base, flash_target_base + size, FLASH_PAGE_SIZE ) )
      {
         break;
      }
   }

   return size;
}

static uint32_t last_flash_block_start()
{
   static uint32_t last_block = 0;
   if( !last_block )
   {
      last_block = flash_size_detect() - FLASH_CHUNK_SIZE;
   }
   return last_block;
}

void settings_write( void* memory, uint32_t size )
{
   assert( size <= FLASH_CHUNK_SIZE );
   flash_range_erase(last_flash_block_start(), FLASH_CHUNK_SIZE);
   flash_range_program(last_flash_block_start(), memory, FLASH_CHUNK_SIZE);
}

void settings_read( void* memory, uint32_t size )
{
   assert( size <= FLASH_CHUNK_SIZE );
   memcpy( memory, flash_target_base + last_flash_block_start(), size );
}

