/***************************************************************************/
/* Holds definitions for the SorbusSound board                             */
/***************************************************************************/
#ifndef SOUND_GPIO_CONFIG_H
#define SOUND_GPIO_CONFIG_H SOUND_GPIO_CONFIG_H

// Common for ModPlay and Sidplay
#define SND_DIN      ( 0)   // Unsued
#define SND_FLT      (25)
#define SND_CLKBASE  (26)   // occupies two pins for blck and lr-clock
#define SND_DOUT     (28)
#define SND_DEMP     (29)
#define SND_SCK      ( 0)   // unused


// Busdecoding in Sidplay
#define D0           ( 0)   // ok
#define PHI          ( 8)   // ok
#define RW           ( 9)   // ok
#define CS1          (10)   // ok CS1
#define nCS0         (11)   // ok nCS0
#define A0           (12)   // ok
#define A5           (17)
#define RESET        (24)   // o.k

#define SD_MISO      (20)
#define SD_CS        (21)
#define SD_SCK       (22)
#define SD_MOSI      (23)

// used by standalone modplayer only
#define SND_BUTTON_PLAY (24)
#define SND_BUTTON_NEXT (29)

void init_gpio (void);

#endif

