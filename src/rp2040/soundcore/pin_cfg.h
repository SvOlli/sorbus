/****************************************************************************************/
/* Holds definitions for the SorbusSound board */
/****************************************************************************************/
#ifndef _PIN_CFG_H_
#define _PIN_CFG_H_

// Common for ModPlay and Sidplay
#define SND_SCK 0       // Unused
#define SND_DOUT 28   
#define SND_DIN 0       // Unsued 
#define SND_CLKBASE 26     // occupies two pin 8 +9 for blck and lr-clock
#define SND_FLT 22
#define SND_DEMP 13

// Busdecoding in Sidplay
#define D0			0  // ok
#define A0			14 // ok
#define A5			19
#define nCS2		11   // HMMM is nCS2
#define OE_DATA		8    // not needed , auf j4 Pin 1
#define RW			10  // ok
#define PHI			9   // ok
#define AUDIO_PIN	29  // auf j4 pin 5
#define CS1			12  // o.k. CS1 
#define RESET		25  // o.k  , check auf LED_BUILTIN
#define LED_BUILTIN 24  //  J4 Pin4

#endif