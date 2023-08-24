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
 * functions are written to by portable while not waisting any cycles,
 * nevertheless they might be slightly faster when copy/paste the static
 * inline implementation into the code instead of calling
 * 
 * will be verified if faster performance is required
 */

#ifndef GPIO_OC_H
#define GPIO_OC_H GPIO_OC_H

#include <hardware/gpio.h>

void gpio_oc_init( uint pin );
void gpio_oc_init_by_mask( uint32_t mask );

static inline void gpio_oc_set( uint pin, bool value )
{
   uint32_t mask = 1ul << pin;
   if( value )
   {
      /* set high by setting to input */ 
      gpio_set_dir_in_masked( mask );
      gpio_set_mask( mask );
   }
   else
   {
      /* set low by writing 0 */
      gpio_clr_mask( mask );
      gpio_set_dir_out_masked( mask );
   }
}


static inline void gpio_oc_set_by_mask( uint32_t mask, uint32_t value )
{
   /* bits to be set? */
   uint32_t ones  = value & mask;
   uint32_t zeros = ~value & mask;
   if( ones )
   {
      /* set high by setting to input */ 
      gpio_set_dir_in_masked( ones );
      gpio_set_mask( ones );
   }
   
   /* bits to be cleared? */
   if( zeros )
   {
      /* set low by writing 0 */
      gpio_clr_mask( zeros );
      gpio_set_dir_out_masked( zeros );
   }
}

#endif
