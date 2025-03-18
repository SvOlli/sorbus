#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <hardware/clocks.h>

#include <tf_card.h>
#include <ff.h>

int main()
{
   pico_fatfs_spi_config_t config = {
      spi0,
      CLK_SLOW_DEFAULT,
      CLK_FAST_DEFAULT,
      20,
      21,
      22,
      23,
      true  // use internal pullup
   };
   pico_fatfs_set_config(&config);

   return 0;
}

