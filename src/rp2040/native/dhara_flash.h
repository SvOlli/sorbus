#ifndef __DHARA_FLASH_H__
#define __DHARA_FLASH_H__ __DHARA_FLASH_H__

#include <stdint.h>

// ratio for garbage collection (look at mkftl.c)
#define GC_RATIO     (2)
// CP/M block size: 128
#define PAGE_SIZE    (0x80)
// flash block size: 4096
#define BLOCK_SIZE   (0x1000)
// mass storage offset: 3M
#define FLASH_OFFSET (0x300000)
// full flash size found in rp2040_purple.h
#include "../rp2040_purple.h"

int dhara_flash_init();
int dhara_flash_read( uint16_t lba, uint8_t *data );
int dhara_flash_write( uint16_t lba, uint8_t *data );

#endif
