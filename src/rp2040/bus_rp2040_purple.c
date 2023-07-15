/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "bus.h"

const bus_config_t bus_config = {
   .mask_address  = 0x0000FFFF,  // input:  16 contiguous bits
   .mask_data     = 0x00FF0000,  // both:   8 contiguous bits
   .mask_rw       = 0x01000000,  // input:  1 bit
   .mask_clock    = 0x02000000,  // output: 1 bit
   .mask_rdy      = 0x04000000,  // output: 1 bit (could be also input in the future)
   .mask_irq      = 0x08000000,  // output: 1 bit (could be also input in the future)
   .mask_nmi      = 0x10000000,  // output: 1 bit (could be also input in the future)
   .mask_reset    = 0x20000000,  // output: 1 bit (could be also input in the future)
   .mask_input    = 0x01FFFFFF,  // convenience
   .mask_output   = 0x3EFF0000,  // convenience
   .shift_data    = 16,
   .shift_address = 0
};


void bus_init()
{
   uint pin;

   /* set everything to open collector -> enable pull ups */
   for( pin = 0; pin < 32; ++pin )
   {
      uint32_t bit = (1 << pin);
      if( bit & bus_config.mask_input )
      {
         gpio_init(pin);
         gpio_set_dir(pin, GPIO_IN);
      }
      if( bit & bus_config.mask_output )
      {
         gpio_init(pin);
         gpio_set_dir(pin, GPIO_OUT);
         gpio_put(pin, 1);
      }
      if( bit & (bus_config.mask_input | bus_config.mask_output) )
      {
         gpio_pull_up( pin );
      }
   }

   gpio_set_dir_out_masked( bus_config.mask_output );
   gpio_set_dir_in_masked( bus_config.mask_input );

   /* cpu running, no interrupt */
   gpio_set_mask( bus_config.mask_rdy | bus_config.mask_irq | bus_config.mask_nmi );

   /* cpu in reset state */
   gpio_clr_mask( bus_config.mask_reset );
}
