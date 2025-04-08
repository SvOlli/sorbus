
#include <pico/binary_info.h>
#include <hardware/gpio.h>
#include "sound_gpio_config.h"

bi_decl(bi_pin_mask_with_name(0x000000FF, "D0-D7"));
bi_decl(bi_1pin_with_name(PHI,            "CLK"));
bi_decl(bi_1pin_with_name(RW,             "R/!W"));
bi_decl(bi_1pin_with_name(CS1,            "CS1"));
bi_decl(bi_1pin_with_name(nCS0,           "!CS0"));
bi_decl(bi_pin_mask_with_name(0x000FF000, "A0-A7"));
bi_decl(bi_pin_range_with_func(20, 23,    GPIO_FUNC_SPI));
bi_decl(bi_1pin_with_name(RESET,          "!RESET"));
bi_decl(bi_1pin_with_name(SND_FLT,        "FLT"));
bi_decl(bi_1pin_with_name(SND_CLKBASE+0,  "BCK"));
bi_decl(bi_1pin_with_name(SND_CLKBASE+1,  "LRCK"));
bi_decl(bi_1pin_with_name(SND_DOUT,       "DIN"));
bi_decl(bi_1pin_with_name(SND_DEMP,       "DEMP"));



void init_gpio()
{
    for ( int i = 0; i < 26; i++ )
    {
        gpio_init( i );
    }
    gpio_init( 28 );
    gpio_set_pulls( nCS0, false, true );
    gpio_set_pulls( CS1, true, false );
    gpio_set_pulls( RESET, true, false );

    gpio_init(SND_SCK);
    gpio_set_dir(SND_SCK, GPIO_OUT);
    gpio_init(SND_DOUT);
    gpio_set_dir(SND_DOUT, GPIO_OUT);
    gpio_init(SND_CLKBASE);
    gpio_set_dir(SND_CLKBASE, GPIO_OUT);
    gpio_init(SND_CLKBASE+1);
    gpio_set_dir(SND_CLKBASE+1, GPIO_OUT);
    gpio_init(SND_FLT);
    gpio_put(SND_FLT,0);  // FIR Filter
    gpio_set_dir(SND_FLT, GPIO_OUT);
    gpio_init(SND_DEMP);
    gpio_set_dir(SND_DEMP, GPIO_OUT);
    gpio_put(SND_DEMP,0);  // no De-Emphasis
    //Databus
    gpio_set_dir_masked(0xff,0);
}
