#include <stdint.h>
#include <stddef.h>
//#include <kernel.h>
#include "dhara/nand.h"
#include <pico/stdlib.h>
#include <string.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include "devflash.h"


int dhara_nand_erase(const struct dhara_nand *n, dhara_block_t b,
                     dhara_error_t *err)
{
    int f = save_and_disable_interrupts();
    flash_range_erase(FLASH_OFFSET + (b*4096), 4096);
    restore_interrupts(f);
	if (err)
		*err = DHARA_E_NONE;
	return 0;
}

int dhara_nand_prog(const struct dhara_nand *n, dhara_page_t p,
                    const uint8_t *data,
                    dhara_error_t *err)
{
    int f = save_and_disable_interrupts();
    flash_range_program(FLASH_OFFSET + (p*512), data, 512);
    restore_interrupts(f);
	if (err)
		*err = DHARA_E_NONE;
	return 0;
}

int dhara_nand_read(const struct dhara_nand *n, dhara_page_t p,
					size_t offset, size_t length,
                    uint8_t *data,
                    dhara_error_t *err)
{
    memcpy(data,
        (uint8_t*)XIP_NOCACHE_NOALLOC_BASE + FLASH_OFFSET + (p*512) + offset,
        length);
	if (err)
		*err = DHARA_E_NONE;
	return 0;
}

/* vim: sw=4 ts=4 et: */

