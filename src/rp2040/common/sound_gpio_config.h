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



#ifndef SOUND_PCB_V1_0   // The PCB V1.1 is default
// Busdecoding in Sidplay
#define D0           ( 0)   // ok
#define PHI          ( 8)   // ok
#define RW           ( 9)   // ok
#define CS1          (10)   // ok CS1 
#define nCS0         (11)   // ok nCS0
#define A0           (12)   // ok
#define A5           (17)
#define RESET        (24)   // o.k  
#define SND_BUTTON_NEXT (29)
#define SND_BUTTON_PLAY (24)

#else

// Busdecoding in Sidplay
#define D0           ( 0)   // ok
#define PHI          ( 9)   // ok
#define RW           (10)   // ok
#define CS1          (12)   // o.k. CS1 
#define nCS0         (11)   // HMMM is nCS0
#define A0           (14)   // ok
#define A5           (19)
#define RESET        (25)   // o.k  
// For standalone player  
#define SND_BUTTON_PLAY (29)
#define SND_BUTTON_NEXT (24)

#endif

#define SD_SCK           (22)
#define SD_CS            (21)
#define SD_MISO          (20)
#define SD_MOSI          (23)

void init_gpio (void);


#endif

