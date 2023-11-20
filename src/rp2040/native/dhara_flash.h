#ifndef __DHARA_FLASH_H__
#define __DHARA_FLASH_H__ __DHARA_FLASH_H__

#include <stdint.h>

typedef struct {
   uint32_t sector_size;
   uint32_t page_size;
   uint32_t erase_size;
   uint32_t erase_cells;
   uint32_t pages;
   uint32_t sectors;
   uint32_t gc_ratio;
   uint32_t read_status;
   uint32_t read_errcode;
} dhara_flash_info_t;

// ratio for garbage collection (look at mkftl.c)
#define GC_RATIO     (2)
// CP/M (and also API) block size: 128
#define SECTOR_SIZE  (0x80)
// dhara block size: 512 (256 and 2048 make worse usage ratio)
#define PAGE_SIZE    (0x200)
// flash block size: 4096
#define BLOCK_SIZE   (0x1000)
// mass storage offset: 4M, leaving 12M raw flash for storage
#define FLASH_OFFSET (0x400000)
// full flash size found in rp2040_purple.h
#include "../rp2040_purple.h"

int dhara_flash_init();
int dhara_flash_read( uint16_t lba, uint8_t *data );
int dhara_flash_write( uint16_t lba, const uint8_t *data );
void dhara_flash_sync();
void dhara_flash_info( uint16_t lba, uint8_t *data, dhara_flash_info_t *info );

#endif
