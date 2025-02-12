/***************************************************************************/
/* Holds definitions for the SorbusSound board                             */
/***************************************************************************/
#ifndef SOUND_GPIO_CONFIG_H
#define SOUND_GPIO_CONFIG_H SOUND_GPIO_CONFIG_H

// Common for ModPlay and Sidplay
#define SND_DIN      ( 0)   // Unsued 
#define SND_DEMP     (29)
#define SND_FLT      (25)
#define SND_CLKBASE  (26)   // occupies two pin 8 +9 for blck and lr-clock
#define SND_DOUT     (28)   
#define SND_SCK      ( 0)   // unused

// Busdecoding in Sidplay
#define D0           ( 0)   // ok
#define PHI          ( 8)   // ok
#define RW           ( 9)   // ok
#define CS1          (10)   // o.k. CS1 
#define nCS2         (11)   // HMMM is nCS2
#define A0           (12)   // ok
#define A5           (17)
//#define LED_BUILTIN  (24)   //  J4 Pin4
#define RESET        (24)   // o.k  , check auf LED_BUILTIN
//#define AUDIO_PIN    (29)   // auf j4 pin 5

#endif
