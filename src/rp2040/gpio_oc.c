/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * small collection of functions that will use the GPIOs with open collector
 * systems:
 * - logic 0: pulling the GPIO to GND by writing a 0
 * - logic 1: let the bus drive the 1 by switching to input
 *
 * init functions do not need to be maximum performant, so no inline
 */

#include "gpio_oc.h"


void gpio_oc_init( uint pin )
{
   /* open collector needs to have GPIOs pull up by definition */
   gpio_init( pin );
   gpio_pull_up( pin );
}


void gpio_oc_init_by_mask( uint32_t mask )
{
   uint pin;
   for( pin = 0; pin < 32; ++pin )
   {
      if( (1ul << pin) & mask )
      {
         gpio_oc_init( pin );
      }
   }
}
