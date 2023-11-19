#ifndef _DEVFLASH_H_
#define _DEVFLASH_H_

#define FLASH_OFFSET 1*1024*1204  // Start after 1Meg , this will give 1 Meg for the filesystem

#include "dhara/error.h"

void flash_dev_init(void);
dhara_error_t transfer_cb(uint32_t blk_op_lba,uint8_t * blk_op_addr,bool blk_op_is_read);
int trim_cb(uint32_t blk_op_lba);
uint8_t get_disk_offset_count(void);
uint8_t get_disk_lba_no(int shifter);
void  set_disk_lba_no(uint8_t data,int shifter);
void trigger_disk_access(uint8_t data);
void write_disk_data(uint8_t data);
uint8_t read_disk_data(void);


#endif