#ifndef STANDALONE_PLAYER
#error "This only works with STANDALONE_PLAYER enabled"
#else

#include "rp2040_purple.h"
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/binary_info.h>

#include <hardware/gpio.h>

#include "standalone.h"
#include "sound_gpio_config.h"
#include "sound_core.h"

volatile  int play_button=0;
volatile  int next_button=0;


void init_buttons()
{
    gpio_init(SND_BUTTON_PLAY);
    gpio_init(SND_BUTTON_NEXT);
    gpio_set_pulls( SND_BUTTON_PLAY, true, false );
    gpio_set_pulls( SND_BUTTON_NEXT, true, false );

}

int debounce (int button, int state){


}


void read_buttons()
{
    int debounce_play=0;
    int debounce_next=0;
    int tmp_button;
    multicore_lockout_victim_init();
    while(1){
        sleep_ms(10);
        tmp_button=gpio_get(SND_BUTTON_NEXT);
        if (!tmp_button){ // pressed ?
            sleep_ms(10);
            tmp_button=gpio_get(SND_BUTTON_NEXT); 
            if (!tmp_button){ // still pressed ?
                play_button = 1;
            }
        }else{
            if (play_button){   // released ?
                sleep_ms(10);
                tmp_button=gpio_get(SND_BUTTON_NEXT); 
                if (tmp_button){ // still released ?
                    play_button = 0;
                    player_state=PLAYER_STATE_NEXT;
                }

            }
        }
        tmp_button=gpio_get(SND_BUTTON_PLAY);
        if (!tmp_button){ // pressed ?
            sleep_ms(10);
            tmp_button=gpio_get(SND_BUTTON_PLAY); 
            if (!tmp_button){ // still pressed ?
                if (player_state==PLAYER_STATE_PLAYING){
                   player_state=PLAYER_STATE_STOP; 
                } 
            }
        }else{
            sleep_ms(10);
            tmp_button=gpio_get(SND_BUTTON_PLAY); 
            if (tmp_button){ // still released ?
                if (player_state==PLAYER_STATE_STOPPED){
                   player_state=PLAYER_STATE_PLAY; 
                } 
  
            }
        }
    };

}

#endif

