#ifndef _DEVFLASH_H_
#define _DEVFLASH_H_

#define FLASH_OFFSET 1*1024*1204  // Start after 1Meg , this will give 1 Meg for the filesystem

#include "dhara/error.h"

void flash_dev_init(void);
dhara_error_t transfer_cb(uint32_t blk_op_lba,uint8_t * blk_op_addr,bool blk_op_is_read);
int trim_cb(uint32_t blk_op_lba);
uint8_t get_disk_lba_no(int shifter);
void  set_disk_lba_no(uint8_t data,int shifter);
void trigger_disk_access_read(uint8_t data,uint8_t *memory);
void trigger_disk_access_write(uint8_t data,uint8_t *memory);
void set_disk_dma_addr(uint8_t data,bool is_low_byte);
uint8_t get_disk_dma_addr(bool is_low_byte);



#endif