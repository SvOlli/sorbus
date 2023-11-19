#include <stdio.h>
#include "dhara/map.h"
#include "dhara/nand.h"
#include <pico/stdlib.h>
#include <hardware/flash.h>
#include "devflash.h"

uint32_t current_lba;
uint32_t buffer_lba=0xffffffff;
uint32_t current_offset;

static struct dhara_map dhara;
static const struct dhara_nand nand = 
{
	.log2_page_size = 9, /* 512 bytes */
	.log2_ppb = 12 - 9, /* 4096 bytes */
	.num_blocks = (PICO_FLASH_SIZE_BYTES - FLASH_OFFSET) / 4096,
};

static uint8_t journal_buf[512];
static uint8_t tmp_buf[512];

int dhara_nand_is_bad(const struct dhara_nand* n, dhara_block_t b)
{
	return 0;
}

void dhara_nand_mark_bad(const struct dhara_nand *n, dhara_block_t b) {}

int dhara_nand_is_free(const struct dhara_nand *n, dhara_page_t p)
{
	dhara_error_t err = DHARA_E_NONE;

	dhara_nand_read(&nand, p, 0, 512, tmp_buf, &err);
	if (err != DHARA_E_NONE)
		return 0;
	for (int i=0; i<512; i++)
		if (tmp_buf[i] != 0xff)
			return 0;
	return 1;
}

int dhara_nand_copy(const struct dhara_nand *n,
                    dhara_page_t src, dhara_page_t dst,
                    dhara_error_t *err)
{
	dhara_nand_read(&nand, src, 0, 512, tmp_buf, err);
	if (*err != DHARA_E_NONE)
		return -1;

	return dhara_nand_prog(&nand, dst, tmp_buf, err);
}

dhara_error_t transfer_cb(uint32_t blk_op_lba,uint8_t * blk_op_addr,bool blk_op_is_read)
{
	dhara_error_t err = DHARA_E_NONE;
	if (blk_op_is_read)
		dhara_map_read(&dhara, blk_op_lba, blk_op_addr, &err);
	else
		dhara_map_write(&dhara, blk_op_lba, blk_op_addr, &err);
	
	return err;
}





int trim_cb(uint32_t blk_op_lba)
{
	dhara_sector_t sector = blk_op_lba;
	if (sector < (nand.num_blocks << nand.log2_ppb))
		dhara_map_trim(&dhara, sector, NULL);
	return 0;
}


uint8_t get_disk_lba_no(int shifter){

	if (shifter>2){
		return 0;
	}
	shifter=shifter*8;
	uint32_t lba_mask=0xff<<shifter;
    return (uint8_t)((current_lba&lba_mask)>>shifter);
}

uint8_t get_disk_offset_count(void){
		return (uint8_t)(current_offset&0xff);
	}

void  set_disk_lba_no(uint8_t data,int shifter){

	uint32_t lba_mask=0;
	uint32_t lba_data=0;

	switch (shifter){
		case 0:
			lba_mask=0xffff00;
			lba_data=(uint32_t)data;
		break;	
		case 1:
			lba_mask=0xff00ff;
			lba_data=((uint32_t)data)<<8;
		break;	
		case 2:
			lba_mask=0x00ffff;
 			lba_data=((uint32_t)data<<16);

		break;	
		default:
			return;

	}
    current_lba&=lba_mask;
	current_lba|=lba_data;
}

void trigger_disk_access(uint8_t data){

	if (data == 0xaa) { // Read-access
		current_offset=0;
		// check if already read
		if ((current_lba&0xfffffe) != (buffer_lba&0xfffffe)){
			// Blocksize in Flash is 512-bytes, but we read 256 as one block
			int err=DHARA_E_NONE;
			dhara_map_read(&dhara, current_lba>>1, tmp_buf, &err);
			if (err!=DHARA_E_NONE){
				printf ("Error writing flash!\n");
			}
			buffer_lba=current_lba;
		}
		if (current_lba&0x01){
			current_offset=256;
		}else{
			current_offset=0;
		}
	}
	if (data == 0x55) { // Write-access
		// Blocksize in Flash is 512-bytes, but we read 256 as one block
		// check if already read
		if ((current_lba&0xfffffe) != (buffer_lba&0xfffffe)){
			// Blocksize in Flash is 512-bytes, but we read 256 as one block
			int err=DHARA_E_NONE;
			dhara_map_read(&dhara, current_lba>>1, tmp_buf, &err);
			if (err!=DHARA_E_NONE){
				printf ("Error writing flash!\n");
			}
			buffer_lba=current_lba;
			buffer_lba=current_lba;
		}
		if (current_lba&0x01){
			current_offset=256;
		}else{
			current_offset=0;
		}

	}
	// if data is anything else, do nothing, must be a failed write
}

void write_disk_data(uint8_t data){

	if (current_offset>=sizeof(tmp_buf)){
		return;
	}
	tmp_buf[current_offset++]=data;
	// check if 256 bytes were written
	if ((current_offset&0xff)==0x00){
		// Write-back buffer
		int err=DHARA_E_NONE;
		dhara_map_write(&dhara, current_lba>>1, tmp_buf, &err);
		if (err!=DHARA_E_NONE){
			printf ("Error writing flash!\n");
		}else{
			dhara_map_sync(&dhara,&err);
		}
		current_offset=0xffff;
		buffer_lba=0xffffffff;
	}

};

uint8_t read_disk_data(void){

	if (current_offset>=sizeof(tmp_buf)){
		return 0;
	}	
	uint8_t data= tmp_buf[current_offset++];
	if (current_offset&0xff==0x00){
			current_offset=0xffff;
	}
	return (data);

}

void flash_dev_init(void)
{
	
	printf("NAND flash, ");
	dhara_map_init(&dhara, &nand, journal_buf, 10);
	dhara_error_t err = DHARA_E_NONE;
	dhara_map_resume(&dhara, &err);
	uint32_t lba = dhara_map_capacity(&dhara);
	printf("%dkB physical %dkB logical at 0x%p: \n",
        nand.num_blocks * 4, lba / 2, XIP_NOCACHE_NOALLOC_BASE + FLASH_OFFSET);
	printf ("Flashinit result : %d\n",err);
	//Some serious error occured, lets format the whole thing
	if (err>2){
		printf ("Flashinit clean map \n",err);
		dhara_map_clear(&dhara);
	}
	current_lba=0;
	current_offset=0;
}

/* vim: sw=4 ts=4 et: */

