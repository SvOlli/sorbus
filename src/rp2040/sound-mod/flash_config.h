
#ifndef __NATIVE_FLASH_CONFIG_H__
#define __NATIVE_FLASH_CONFIG_H__ __NATIVE_FLASH_CONFIG_H__

// full flash size found in rp2040_purple.h
#include "rp2040_purple.h"

// addresses as text, will be later reused with proper guards
#define FLASH_KERNEL_START_TXT  0x103FA000
#define FLASH_DRIVE_START_TXT   0x10400000

#define FLASH_PAGE_SIZE    4096


#endif
