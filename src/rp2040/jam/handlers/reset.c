/**
 * Copyright (c) 2023-2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements a JAM (Just Another Machine) custom platform
 * for the Sorbus Computer
 */

#include "jam.h"
#include "bus.h"
#include "event_queue.h"

#include <string.h>


void event_reset( uint32_t value )
{
   switch( value )
   {
      case BUSMSG_RESET_START:
         // reset all subsystems
         clocks_reset();
         misc_reset();
         timers_reset();
         uart_reset();
         break;
      case BUSMSG_RESET_CLEAR:
         gpio_set_mask( bus_config.mask_reset );
         break;
      default:
         printf( __FILE__ "(%d): internal error unexpected reset value: %08x\n",
                 __LINE__, value );
   }
}


void system_init()
{
   // cpu in reset state
   gpio_clr_mask( bus_config.mask_reset );
   // this will trigger event_reset indirectly

   // clear out RAM
   memset( &ram[0x0000], 0, sizeof( ram ) );
   // copy ROM from flash
   memcpy( &rom[0x0000], (const void*)FLASH_KERNEL_START, sizeof( rom ) );
}

