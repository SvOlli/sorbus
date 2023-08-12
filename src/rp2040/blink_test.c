/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pico/stdlib.h>
#include <pico/binary_info.h>
#include <stdio.h>
#include "fake_eeprom.h"

#ifdef PICO_DEFAULT_LED_PIN
bi_decl(bi_1pin_with_name(PICO_DEFAULT_LED_PIN, "LED"));
bi_decl(bi_program_description("blinks one time for each MB of flash storage"));
#endif

int main()
{
    stdio_init_all();

#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    uint32_t flash_size = flash_size_detect() / (1<<20);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true)
    {
        for( int i = 0; i < flash_size; ++i )
        {
           gpio_put(LED_PIN, 1);
           printf("pin %d on\n",LED_PIN);
           sleep_ms(250);
           gpio_put(LED_PIN, 0);
           printf("pin %d off\n",LED_PIN);
           sleep_ms(250);
        }
        printf("flash size: %dMB\n", flash_size);
        sleep_ms( 1000 );
    }
#endif
}
