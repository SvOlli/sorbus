
#include <pico/binary_info.h>
#include <hardware/gpio.h>
#include "sound_gpio_config.h"

bi_decl(bi_pin_mask_with_name(0x000000FF, "D0-D7"));
bi_decl(bi_1pin_with_name(PHI,            "CLK"));
bi_decl(bi_1pin_with_name(RW,             "R/!W"));
bi_decl(bi_1pin_with_name(CS1,            "CS1"));
bi_decl(bi_1pin_with_name(nCS2,           "!CS2"));
bi_decl(bi_pin_mask_with_name(0x000FF000, "A0-A7"));
bi_decl(bi_pin_range_with_func(20, 23,    GPIO_FUNC_SPI));
bi_decl(bi_1pin_with_name(RESET,          "!RESET"));
bi_decl(bi_1pin_with_name(SND_FLT,        "FLT"));
bi_decl(bi_1pin_with_name(SND_CLKBASE+0,  "BCK"));
bi_decl(bi_1pin_with_name(SND_CLKBASE+1,  "LRCK"));
bi_decl(bi_1pin_with_name(SND_DOUT,       "DIN"));
bi_decl(bi_1pin_with_name(SND_DEMP,       "DEMP"));
