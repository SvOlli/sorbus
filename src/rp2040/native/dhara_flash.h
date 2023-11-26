#ifndef __DHARA_FLASH_H__
#define __DHARA_FLASH_H__ __DHARA_FLASH_H__

#include <stdint.h>
#include "flash_config.h"

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

// returns size of disk as number of LBAs, 0=error
uint16_t dhara_flash_init();
int dhara_flash_read( uint16_t lba, uint8_t *data );
int dhara_flash_write( uint16_t lba, const uint8_t *data );
int dhara_flash_trim( uint16_t lba );
void dhara_flash_sync();
void dhara_flash_info( uint16_t lba, uint8_t *data, dhara_flash_info_t *info );

#endif
