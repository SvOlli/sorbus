
#include <pico/binary_info.h>

/* VGA GPIOs defined by <pico-extras>/src/rp2_common/pico_scanvideo_dpi/scanvideo.c */
bi_decl(bi_1pin_with_name(5,              "!CS"));
bi_decl(bi_pin_mask_with_name(0x03FC0000, "D0-D7"));
bi_decl(bi_1pin_with_name(26,             "R/!W"));
bi_decl(bi_1pin_with_name(27,             "CLK"));
bi_decl(bi_1pin_with_name(28,             "!IRQ/A0"));
bi_decl(bi_1pin_with_name(29,             "!NMI"));
