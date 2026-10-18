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

#include <string.h>

#define RESET_TO_METAMENU 0


#if RESET_TO_METAMENU
static int64_t callback_reset_metamenu( alarm_id_t id, void *user_data )
{
   bus_stop_cause = META_WATCHDOG;

   // no re-firing
   return 0;
}
#endif


static int64_t callback_reset_done( alarm_id_t id, void *user_data )
{
   bool *already_running = (bool*)user_data;

#if RESET_TO_METAMENU
   // trigger meta-menu after running for a few clock cycles
   add_alarm_in_us( 100, callback_reset_metamenu, &already_running, true );
#endif

   // undo pulling reset line
   gpio_set_mask( bus_config.mask_reset );

   // clear flag
   *already_running = false;

   // no re-firing
   return 0;
}


void system_reset( bool init )
{
   // TODO: decide if breaking of init=true in a separate function
   //       is a good idea
   static bool already_running = false;

   // don't continue, if already running
   if( already_running )
   {
      return;
   }
   already_running = true;

   // pull reset line low
   gpio_clr_mask( bus_config.mask_reset );

   // doing main reset stuff
   memset( &ram[0xDF00], 0, 0x80 ); // clear out internal I/O
   clocks_reset();
   misc_reset();
   timers_reset();
   uart_reset();
   if( init )
   {
      // clear out RAM
      memset( &ram[0x0000], 0, sizeof( ram ) );
      // copy ROM from flash
      memcpy( &rom[0x0000], (const void*)FLASH_KERNEL_START, sizeof( rom ) );

      already_running = false;
   }
   else
   {
      // create alarm for release
      add_alarm_in_ms( 1, callback_reset_done, &already_running, true );
   }
}

