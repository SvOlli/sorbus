
#ifndef __NATIVE_FLASH_CONFIG_H__
#define __NATIVE_FLASH_CONFIG_H__ __NATIVE_FLASH_CONFIG_H__

// full flash size found in rp2040_purple.h
#include "../rp2040_purple.h"

// addresses as text, will be later reused with proper guards
#define FLASH_KERNEL_START_TXT  0x103FE000
#define FLASH_DRIVE_START_TXT   0x10400000

// ratio for garbage collection (look at mkftl.c)
#define GC_RATIO                (2)
// API (and also CP/M) block size: 128
#define SECTOR_SIZE             (0x80)
// dhara block size: 512 (256 and 2048 make worse usage ratio)
#define PAGE_SIZE               (0x200)
// flash block size: 4096
#define BLOCK_SIZE              (0x1000)
// mass storage offset: 4M, leaving 12M raw flash for storage
#define FLASH_DRIVE_START       (FLASH_DRIVE_START_TXT)
#define FLASH_DRIVE_END         (XIP_BASE+PICO_FLASH_SIZE_BYTES)

#endif
