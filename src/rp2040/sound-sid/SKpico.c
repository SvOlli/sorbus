/*
       ______/  _____/  _____/     /   _/    /             /
     _/           /     /     /   /  _/     /   ______/   /  _/             ____/     /   ______/   ____/
      ___/       /     /     /   ___/      /   /         __/                    _/   /   /         /     /
         _/    _/    _/    _/   /  _/     /  _/         /  _/             _____/    /  _/        _/    _/
  ______/   _____/  ______/   _/    _/  _/    _____/  _/    _/          _/        _/    _____/    ____/

  SKpico.c

  SIDKick pico - SID-replacement with dual-SID/SID+fm emulation using a RPi pico, reSID 0.16 and fmopl 
  Copyright (c) 2023/2024 Carsten Dachsbacher <frenetic@dachsbacher.de>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma GCC optimize( "Ofast", "omit-frame-pointer", "modulo-sched", "modulo-sched-allow-regmoves", "gcse-sm", "gcse-las", "inline-small-functions", "delete-null-pointer-checks", "expensive-optimizations" ) 

// enable output via PWM
//#define OUTPUT_VIA_PWM

//enable output via PCM5102-DAC
#define USE_DAC

// will be available later
//#define USE_SPDIF

// enable flashing LED
//#define FLASH_LED

// enable heuristics for detecting digi-playing techniques
#define SUPPORT_DIGI_DETECT

// enable support of special 8-bit DAC mode
#define SID_DAC_MODE_SUPPORT

// enable RGB LED on GPIO 23 (do not use this with original Pico)
//#define USE_RGB_LED

#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include "hardware/vreg.h"
#include "hardware/pwm.h"  
#include "hardware/flash.h"
#include "hardware/structs/bus_ctrl.h" 
#include "i2s.h"
#include "hardware/dma.h" 


#ifdef USE_SPDIF
//#include "pico/audio_spdif.h"
#endif
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/resets.h"
#include "hardware/watchdog.h"
#include "common/sound_gpio_config.h"

#ifdef FMOPL_ENABLED
#include "fmopl.h"
#endif
extern uint8_t FM_ENABLE;


const volatile uint8_t __in_flash() busTimings[ 8 ] = { 11, 15, 1, 2, 3, 4, 5, 6 };
//const volatile uint8_t __in_flash() busTimings[ 8 ] = { 5, 7, 1, 2, 3, 4, 5, 6 };
uint8_t DELAY_READ_BUS, DELAY_PHI2;
 
// support straight DAC output
#ifdef SID_DAC_MODE_SUPPORT
#define SID_DAC_OFF      0
#define SID_DAC_MONO8    1
#define SID_DAC_STEREO8  2
#define SID_DAC_MONO16   4
#define SID_DAC_STEREO16 8
uint8_t sidDACMode = SID_DAC_OFF;
#endif

volatile int32_t discrepancy = 0;

#define VERSION_STR_SIZE  36
static const __not_in_flash( "mydata" ) unsigned char VERSION_STR[ VERSION_STR_SIZE ] = {
#if defined( USE_SPDIF )
  0x53, 0x4b, 0x10, 0x09, 0x03, 0x0f, '0', '.', '2', '1', '/', 0x53, 0x50, 0x44, 0x49, 0x46, 0, 0, 0, 0,   // version string to show
#endif
#if defined( USE_DAC ) 
  0x53, 0x4b, 0x10, 0x09, 0x03, 0x0f, '0', '.', '2', '1', '/', 0x44, 0x41, 0x43, '6', '4', 0, 0, 0, 0,   // version string to show
#elif defined( OUTPUT_VIA_PWM )
  0x53, 0x4b, 0x10, 0x09, 0x03, 0x0f, '0', '.', '2', '1', '/', 0x50, 0x57, 0x4d, '6', '4', 0, 0, 0, 0,   // version string to show
#endif
  0x53, 0x4b, 0x10, 0x09, 0x03, 0x0f, 0x00, 0x00,   // signature + extension version 0
  0, 21,                                            // firmware version with stepping = 0.12
#ifdef SID_DAC_MODE_SUPPORT                         // support DAC modes? which?
  SID_DAC_MONO8 | SID_DAC_STEREO8,
#else
  0,
#endif
  0, 0, 0, 0, 0 };

extern void initReSID();
extern void resetReSID();
extern void emulateCyclesReSID( int cyclesToEmulate );
extern void emulateCyclesReSIDSingle( int cyclesToEmulate );
extern uint16_t crc16( const uint8_t *p, uint8_t l );
extern void updateConfiguration();
extern void writeReSID( uint8_t A, uint8_t D );
extern void writeReSID2( uint8_t A, uint8_t D );
extern void outputReSID( int16_t *left, int16_t *right );
extern void readRegs( uint8_t *p1, uint8_t *p2 );


#define bRESET		( 1 << RESET )
#define bCS1		( 1 << CS1 )
#define bnCS0	    ( 1 << nCS0 )
#define bPHI		( 1 << PHI )
#define bRW			( 1 << RW )
//#define bOE			( 1 << OE_DATA )

#define AUDIO_I2S_CLOCK_PIN_BASE SND_CLKBASE  //ok
#define AUDIO_I2S_DATA_PIN	SND_DOUT		 //ok

#define DAC_BITS	( ( 3 << AUDIO_I2S_CLOCK_PIN_BASE ) | ( 1 << AUDIO_I2S_DATA_PIN ) )

#define VIC_HALF_CYCLE( g )	( !( (g) & bPHI ) )
#define CPU_HALF_CYCLE( g )	(  ( (g) & bPHI ) )
#define WRITE_ACCESS( g )	( !( (g) & bRW ) )
#define READ_ACCESS( g )	(  ( (g) & bRW ) )
#define SID_ACCESS( g )		( ( (g) & bCS1 ) && (!( (g) & bnCS0 )) )
#define SID_ADDRESS( g )	(  ( (g) >> A0 ) & 0x1f )
#define SID_UPPER_ADDRESS( g )	(  ( (g) >> A0 ) & 0xe0 )
#define SID_DATA( g )	       ( (g) & 0xff )
#define SID_RESET( g )	    ( !( (g) & bRESET ) )

#define WAIT_FOR_VIC_HALF_CYCLE { do { g = *gpioInAddr; } while ( !( VIC_HALF_CYCLE( g ) ) ); }
#define WAIT_FOR_CPU_HALF_CYCLE { do { g = *gpioInAddr; } while ( !( CPU_HALF_CYCLE( g ) ) ); }

#define SET_DATA( D )   \
      { sio_hw->gpio_set = ( D ); \
        sio_hw->gpio_clr = ( (~(uint32_t)( D )) & 255 ); } 

const __not_in_flash( "mydata" ) uint32_t  sidFlags[ 6 ] = { bCS1, ( 1 << A5 ), ( 1 << nCS0 ), ( 1 << A5 ) | ( 1 << nCS0 ), ( 1 << nCS0 ), ( 1 << nCS0 ) };
extern uint32_t SID2_FLAG;
extern uint8_t  SID2_IOx_global;

// audio settings
#define AUDIO_RATE (44100)
#define AUDIO_VALS 2834
#define AUDIO_BITS 11
#define AUDIO_BIAS ( AUDIO_VALS / 2 )
#define SAMPLES_PER_BUFFER (256)

extern uint32_t C64_CLOCK;

#define SET_CLOCK_125MHZ set_sys_clock_pll( 1500000000, 6, 2 );
#define SET_CLOCK_FAST   set_sys_clock_pll( 1500000000, 5, 1 );

#define DELAY_Nx3p2_CYCLES( c )								\
    asm volatile( "mov  r0, %[_c]\n\t"						\
				  "1: sub  r0, r0, #1\n\t"					\
				  "bne   1b"  : : [_c] "r" (c) : "r0", "cc", "memory" );


extern uint8_t config[ 64 ];


#ifdef USE_DAC

uint player_state =0;

const i2s_config i2s_config_pcm5102 = {44100, 256, 32, SND_SCK, SND_DOUT, SND_DIN, SND_CLKBASE, false};

static __attribute__((aligned(8))) pio_i2s i2s;

uint16_t audioPos = 0, 
		 audioOutPos = 0;
int32_t  audioBuffer[ STEREO_BUFFER_SIZE * 2 ];
uint8_t  firstOutput = 1;

uint16_t play_chunk (int32_t * output_buffer, int buffer_size){

	memcpy(output_buffer,&audioBuffer[audioOutPos],STEREO_BUFFER_SIZE*sizeof(int32_t));

}

static void dma_i2s_in_handler(void) {
		/* We're double buffering using chained TCBs. By checking which buffer the
		 * DMA is currently reading from, we can identify which buffer it has just
		 * finished reading (the completion of which has triggered this interrupt).
		 */
		if (*(int32_t**)dma_hw->ch[i2s.dma_ch_out_ctrl].read_addr == i2s.output_buffer) {
			// It is inputting to the second buffer so we can overwrite the first
			player_state=play_chunk(i2s.output_buffer, STEREO_BUFFER_SIZE);
		} else {
			// It is currently inputting the first buffer, so we write to the second
			player_state=play_chunk(&i2s.output_buffer[STEREO_BUFFER_SIZE], STEREO_BUFFER_SIZE);
		}
		dma_hw->ints0 = 1u << i2s.dma_ch_out_data;  // clear the IRQ
	}

#endif

#ifdef USE_SPDIF
	removed for now
#endif

uint64_t c64CycleCounter = 0;

volatile int32_t newSample = 0xffff, newLEDValue;
volatile uint64_t lastSIDEmulationCycle = 0;

uint8_t outRegisters[ 34 * 2 ];
uint8_t *outRegisters_2 = &outRegisters[ 34 ];

uint8_t fmFakeOutput = 0;
uint8_t fmAutoDetectStep = 0;
uint8_t hack_OPL_Sample_Value[ 2 ];
uint8_t hack_OPL_Sample_Enabled;

#define sidAutoDetectRegs outRegisters

#define SID_MODEL_DETECT_VALUE_8580 2
#define SID_MODEL_DETECT_VALUE_6581 3
#define REG_AUTO_DETECT_STEP		32
#define REG_MODEL_DETECT_VALUE		33

uint8_t busValue = 0;
int32_t busValueTTL = 0;

uint16_t SID_CMD = 0xffff;

#define  RING_SIZE 256
uint16_t ringBuf[ RING_SIZE ];
uint32_t ringTime[ RING_SIZE ];
uint8_t  ringWrite = 0;
uint8_t  ringRead  = 0;

uint32_t SampleCount=0;
uint32_t SampleCount_play=0;
int SampleMiss=0;


// Use alarm 0
#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0


static void alarm_irq(void) ;


static void alarm_in_us(uint32_t delay_us) {
    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    // Set irq handler for alarm irq
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    // Enable the alarm irq
    irq_set_enabled(ALARM_IRQ, true);
    // Enable interrupt in block and at processor

    // Alarm is only 32 bits so if trying to delay more
    // than that need to be careful and keep track of the upper
    // bits
    uint64_t target = timer_hw->timerawl + delay_us;

    // Write the lower 32 bits of the target time to the alarm which
    // will arm it
    timer_hw->alarm[ALARM_NUM] = (uint32_t) target;
}


static void alarm_irq(void) {
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

    newSample = 0xfffe;
	SampleCount ++;
	c64CycleCounter+=C64_CLOCK/44100;

	// Retrigger
	alarm_in_us(C64_CLOCK/44100);
 
}

void init_timer (void){

	alarm_in_us(C64_CLOCK/44100);
	

}
void resetEverything() 
{
	ringRead = ringWrite = 0;
}

uint8_t stateGoingTowardsTransferMode = 0;

typedef enum {
	DD_IDLE = 0,
	DD_PREP,
	DD_CONF,
	DD_PREP2,
	DD_CONF2,
	DD_SET,
	DD_VAR1,
	DD_VAR2
} DD_STATE;

DD_STATE ddTB_state[ 3 ] = { DD_IDLE, DD_IDLE, DD_IDLE };
uint64_t ddTB_cycle[ 3 ] = { 0, 0, 0 };
uint8_t  ddTB_sample[ 3 ] = { 0, 0, 0 };

DD_STATE ddPWM_state[ 3 ] = { DD_IDLE, DD_IDLE, DD_IDLE };
uint64_t ddPWM_cycle[ 3 ] = { 0, 0, 0 };
uint8_t  ddPWM_sample[ 3 ] = { 0, 0, 0 };

#define DD_TB_TIMEOUT0	135
// def 22 and 12
#define DD_TB_TIMEOUT	22
#define DD_PWM_TIMEOUT	22

uint8_t  ddActive[ 3 ];
uint64_t ddCycle[ 3 ] = { 0, 0, 0 };
uint8_t	 sampleValue[ 3 ] = { 0 };

extern void outputDigi( uint8_t voice, int32_t value );

uint8_t SID2_IOx;

volatile uint8_t doReset = 0;

void updateEmulationParameters()
{
	SID2_IOx = SID2_IOx_global;
}

inline uint8_t median( uint8_t *x )
{
	int sum, minV, maxV;

	sum = minV = maxV = x[ 0 ];

	sum += x[ 1 ];
	if ( x[ 1 ] < minV ) minV = x[ 1 ];
	if ( x[ 1 ] > maxV ) maxV = x[ 1 ];

	sum += x[ 2 ];
	if ( x[ 2 ] < minV ) minV = x[ 2 ];
	if ( x[ 2 ] > maxV ) maxV = x[ 2 ];

	return sum - minV - maxV;
}

//#define DEBUG_I2S_CLK

void debug_clock(void){
#ifdef DEBUG_I2S_CLK
pio_i2s_clocks clocks;

   calc_clocks(&i2s_config_pcm5102, &clocks);
   validate_sck_bck_sync(&clocks);
#endif
}
void runEmulation()
{
	irq_set_mask_enabled( 0xffffffff, 0 );

	// setup PWM for LED 
#ifdef LED_BUILTIN
	gpio_init( LED_BUILTIN );
	gpio_set_dir( LED_BUILTIN, GPIO_OUT );
	gpio_set_function( LED_BUILTIN, GPIO_FUNC_PWM );

	int led_pin_slice = pwm_gpio_to_slice_num( LED_BUILTIN );
	pwm_config configLED = pwm_get_default_config();
	pwm_config_set_clkdiv( &configLED, 1 );
	pwm_config_set_wrap( &configLED, AUDIO_VALS );
	pwm_init( led_pin_slice, &configLED, true );
	gpio_set_drive_strength( LED_BUILTIN, GPIO_DRIVE_STRENGTH_2MA );
	pwm_set_gpio_level( LED_BUILTIN, 0 );
#endif

	// setup PWM and PWM mode for audio 
	#ifdef OUTPUT_VIA_PWM
	gpio_set_dir( AUDIO_PIN, GPIO_OUT );
	gpio_set_function( AUDIO_PIN, GPIO_FUNC_PWM );

	int audio_pin_slice = pwm_gpio_to_slice_num( AUDIO_PIN );
	pwm_config config = pwm_get_default_config();
	pwm_config_set_clkdiv( &config, 1 );
	pwm_config_set_wrap( &config, AUDIO_VALS );
	pwm_init( audio_pin_slice, &config, true );
	gpio_set_drive_strength( AUDIO_PIN, GPIO_DRIVE_STRENGTH_12MA );
	pwm_set_gpio_level( AUDIO_PIN, 0 );

	static const uint32_t PIN_DCDC_PSM_CTRL = 23;
	gpio_init( PIN_DCDC_PSM_CTRL );
	gpio_set_dir( PIN_DCDC_PSM_CTRL, GPIO_OUT );
	gpio_put( PIN_DCDC_PSM_CTRL, 1 );
	#endif

	initReSID();
	
#ifdef FMOPL_ENABLED

	FM_OPL *pOPL = ym3812_init( 3579545, AUDIO_RATE );
	for ( int i = 0x40; i < 0x56; i++ )
	{
		ym3812_write( pOPL, 0, i );
		ym3812_write( pOPL, 1, 63 );
	}
	fmFakeOutput = 0;
	hack_OPL_Sample_Value[ 0 ] = hack_OPL_Sample_Value[ 1 ] = 64;
	hack_OPL_Sample_Enabled = 0;
#endif 

	updateEmulationParameters();

	#ifdef USE_DAC  
	debug_clock();
	i2s_program_start_output(pio0, &i2s_config_pcm5102, dma_i2s_in_handler, &i2s);

	gpio_put(SND_DEMP,1);  //  De-Emphasis on

	#endif
	#ifdef USE_SPDIF
	ap = initSPDIF();
	#endif

	init_timer();
	#ifdef SID_DAC_MODE_SUPPORT
	int32_t DAC_L = 0, DAC_R = 0;
	#endif

	extern uint8_t SID_DIGI_DETECT;	// from config: heuristics activated?
	uint8_t  sampleTechnique = 0;
	uint16_t silence = 0;
	uint16_t ramp = 0;
	int32_t  lastS = 0;

	uint64_t lastD418Cycle = 0;

	while ( 1 )
	{
		if ( doReset )
		{
			watchdog_reboot( 0, 0, 0 );
		}

		uint64_t targetEmulationCycle = c64CycleCounter;
		while ( ringRead != ringWrite )
		{
			#ifdef SID_DAC_MODE_SUPPORT
			// this is placed here, as we don't use time stamps in DAC mode
			if ( sidDACMode && !( ringBuf[ ringRead ] & ( 1 << 15 ) ) )
			{
				register uint16_t cmd = ringBuf[ ringRead ++ ];
				uint8_t reg = ( cmd >> 8 ) & 0x1f;

				if ( sidDACMode == SID_DAC_STEREO8 )
				{
					if ( reg == 0x18 )
						DAC_L = ( (int)( cmd & 255 ) - 128 ) << 7;
					if ( reg == 0x19 )
						DAC_R = ( (int)( cmd & 255 ) - 128 ) << 7; 
				} else
				if ( sidDACMode == SID_DAC_MONO8 && reg == 0x18 )
				{
					DAC_L = DAC_R = ( (int)( cmd & 255 ) - 128 ) << 7;
				}
				continue;
			} 
			#endif


			uint64_t cmdTime = (uint64_t)ringTime[ ringRead ];

			if ( cmdTime > lastSIDEmulationCycle )
			{
				targetEmulationCycle = cmdTime;
				break;
			}
			
			register uint16_t cmd = ringBuf[ ringRead ++ ];

			if ( cmd & ( 1 << 15 ) )
			{
#ifdef FMOPL_ENABLED
				if ( FM_ENABLE )
				{
					ym3812_write( pOPL, ( ( cmd >> 8 ) >> 4 ) & 1, cmd & 255 );
				} else
#endif				
				{
					writeReSID2( ( cmd >> 8 ) & 0x1f, cmd & 255 );
				}
			} else
			{
				uint8_t reg = cmd >> 8;

				writeReSID( reg, cmd & 255 );

				// pseudo stereo
				if ( SID2_FLAG == ( 1 << 31 ) )
					writeReSID2( reg, cmd & 255 );

				#ifdef SUPPORT_DIGI_DETECT

				//
				// Digi-Playing Detection to bypass reSID
				// (the heuristics below are based on the findings by J�rgen Wothke used in WebSid (https://bitbucket.org/wothke/websid/src/master/) )
				//

				if ( SID_DIGI_DETECT )
				{
					uint8_t voice = 0;
					if ( reg >  6 && reg < 14 ) { voice = 1; reg -= 7; }
					if ( reg > 13 && reg < 21 ) { voice = 2; reg -= 14; }

					//
					// test-bit technique

					#define DD_STATE_TBC( state, cycle )	{ ddTB_state[ voice ] = state; ddTB_cycle[ voice ] = cycle; }
					#define DD_STATE_TB( state )			{ ddTB_state[ voice ] = state; }
					#define DD_NO_TIMEOUT( threshold )		( ( c64CycleCounter - ddTB_cycle[ voice ] ) < threshold )
					
					#define DD_GET_SAMPLE( s )	{	DD_STATE_TBC( DD_IDLE, 0 );					\
													ddActive[ voice ] = sampleTechnique = 2;	\
													sampleValue[ voice ] = s;					\
													ddCycle[ voice ] = c64CycleCounter; }

					if ( reg == 4 )	
					{	
						uint8_t v = cmd & 0x19;
						switch ( v ) 
						{
						case 0x11:	
							DD_STATE_TBC( DD_PREP, c64CycleCounter );
							break;
						case 0x8: case 0x9:	
							if ( ddTB_state[ voice ] == DD_PREP && DD_NO_TIMEOUT( DD_TB_TIMEOUT0 ) )
								DD_STATE_TBC( DD_SET, c64CycleCounter - 4 ) else
								DD_STATE_TB( DD_IDLE )
							break;
						case 0x1:	// GATE
							if ( ddTB_state[ voice ] == DD_SET && DD_NO_TIMEOUT( DD_TB_TIMEOUT ) )
								DD_STATE_TB( DD_VAR1 ) else
							if ( ddTB_state[ voice ] == DD_VAR2 && DD_NO_TIMEOUT( DD_TB_TIMEOUT ) )
								DD_GET_SAMPLE( ddTB_sample[ voice ] ) else
								DD_STATE_TB( DD_IDLE )
							break;
						case 0x0:	
							if ( ddTB_state[ voice ] == DD_VAR2 && DD_NO_TIMEOUT( DD_TB_TIMEOUT ) )
								DD_GET_SAMPLE( ddTB_sample[ voice ] )
							break;
						}
					} else
					if ( reg == 1 ) 
					{	
						if ( ddTB_state[ voice ] == DD_SET && DD_NO_TIMEOUT( DD_TB_TIMEOUT ) )
						{
							ddTB_sample[ voice ] = cmd & 255;
							DD_STATE_TBC( DD_VAR2, c64CycleCounter )
						} else 
						if ( ddTB_state[ voice ] == DD_VAR1 && DD_NO_TIMEOUT( DD_TB_TIMEOUT ) )
							DD_GET_SAMPLE( cmd & 255 )
					}

					//
					// pulse modulation technique
					
					#define DD_PWM_STATE_TBC( state, cycle )	{ ddPWM_state[ voice ] = state; ddPWM_cycle[ voice ] = cycle; }
					#define DD_PWM_STATE_TB( state )			{ ddPWM_state[ voice ] = state; }
					#define DD_PWM_NO_TIMEOUT( threshold )		( ( c64CycleCounter - ddPWM_cycle[ voice ] ) < threshold )

					#define DD_PWM_GET_SAMPLE( s )	{	DD_PWM_STATE_TBC( DD_IDLE, 0 );				\
														ddActive[ voice ] = sampleTechnique = 1;	\
														sampleValue[ voice ] = s;					\
														ddCycle[ voice ] = c64CycleCounter; }

					if ( reg == 4 ) 
					{
						uint8_t v = cmd & 0x49;	
						switch ( v ) 
						{
						case 0x49:	
							if ( ( ddPWM_state[ voice ] == DD_PREP ) && DD_PWM_NO_TIMEOUT( DD_PWM_TIMEOUT ) )
								DD_PWM_STATE_TBC( DD_CONF, c64CycleCounter ) else
								DD_PWM_STATE_TBC( DD_PREP2, c64CycleCounter ) 
							break;
						case 0x41:	
							if ( ( ddPWM_state[ voice ] == DD_CONF || ddPWM_state[ voice ] == DD_CONF2 ) && DD_PWM_NO_TIMEOUT( DD_PWM_TIMEOUT ) )
								DD_PWM_GET_SAMPLE( ddPWM_sample[ voice ] ) else
								DD_PWM_STATE_TB( DD_IDLE )
							break;
						}
					} else 
					if ( reg == 2 ) 
					{
						if ( ( ddPWM_state[ voice ] == DD_PREP2 ) && DD_PWM_NO_TIMEOUT( DD_PWM_TIMEOUT ) )
							DD_PWM_STATE_TBC( DD_CONF2, c64CycleCounter ) else
							DD_PWM_STATE_TBC( DD_PREP, c64CycleCounter )
						ddPWM_sample[ voice ] = cmd & 255;
					}
				}
				#endif
			}
		} // while

		uint64_t curCycleCount = targetEmulationCycle;

		#ifdef SID_DAC_MODE_SUPPORT
		if ( !sidDACMode )
		#endif
		if ( lastSIDEmulationCycle < curCycleCount )
		{
			#ifdef SUPPORT_DIGI_DETECT
			if ( SID_DIGI_DETECT )
			{
				uint16_t v;

				if ( c64CycleCounter - ddCycle[ 0 ] > 250 ) { ddActive[ 0 ] = 0; outputDigi( 0, 0 ); }
				if ( c64CycleCounter - ddCycle[ 1 ] > 250 ) { ddActive[ 1 ] = 0; outputDigi( 1, 0 ); }
				if ( c64CycleCounter - ddCycle[ 2 ] > 250 ) { ddActive[ 2 ] = 0; outputDigi( 2, 0 ); }

				if ( ddActive[ 0 ] ) { *(int16_t *)&v = ( sampleValue[ 0 ] - 128 ) << 8; v &= ~3; v |= sampleTechnique; outputDigi( 0, *(int16_t *)&v ); }
				if ( ddActive[ 1 ] ) { *(int16_t *)&v = ( sampleValue[ 1 ] - 128 ) << 8; v &= ~3; v |= sampleTechnique; outputDigi( 1, *(int16_t *)&v ); }
				if ( ddActive[ 2 ] ) { *(int16_t *)&v = ( sampleValue[ 2 ] - 128 ) << 8; v &= ~3; v |= sampleTechnique; outputDigi( 2, *(int16_t *)&v ); }

				#ifdef USE_RGB_LED
				if ( sampleTechnique == 1 ) digiD418Visualization = 2;
				#endif
			}
			#endif

			uint64_t cyclesToEmulate = curCycleCount - lastSIDEmulationCycle;
			lastSIDEmulationCycle = curCycleCount;
#ifdef FMOPL_ENABLED
			if ( FM_ENABLE )
				emulateCyclesReSIDSingle( cyclesToEmulate ); else
#endif				
				emulateCyclesReSID( cyclesToEmulate );
			readRegs( &outRegisters[ 0x1b ], &outRegisters[ 0x1b+34 ] );
		}


		if ( newSample == 0xfffe )
		{
			int16_t L, R;
			SampleCount_play ++;
			if (abs(SampleCount-SampleCount_play)>2){
				// missed a Sample
				SampleMiss++;
				SampleCount_play=SampleCount;

			}

			#ifdef SID_DAC_MODE_SUPPORT
			if ( sidDACMode )
			{
				L = DAC_L;
				R = DAC_R;
				#ifdef USE_RGB_LED
				digiD418Visualization = 2;
				#endif
			} else
			#endif
#ifdef FMOPL_ENABLED
			if ( FM_ENABLE )
			{
				OPLSAMPLE fm;
				ym3812_update_one( pOPL, &fm, 1 );

				if ( hack_OPL_Sample_Enabled )
					fm = ( (uint16_t)hack_OPL_Sample_Value[ 0 ] << 5 ) + ( (uint16_t)hack_OPL_Sample_Value[ 1 ] << 5 );

				extern void outputReSIDFM( int16_t * left, int16_t * right, int32_t fm, uint8_t fmHackEnable, uint8_t *fmDigis );
				outputReSIDFM( &L, &R, (int32_t)fm, hack_OPL_Sample_Enabled, hack_OPL_Sample_Value );
			} else
#endif			
				outputReSID( &L, &R );

			#if defined( USE_DAC ) 

			// fill buffer, skip/stretch as needed
			audioBuffer[ audioPos +1 ] = R<<16; 
			audioBuffer[ audioPos    ] = L<<16;

			audioPos +=2;
			if ( audioPos < STEREO_BUFFER_SIZE *2 ){
				if (audioPos>=STEREO_BUFFER_SIZE){
					audioOutPos=0;		// This is the start of the DMA-buffer					
				}
			}else{
				audioPos = 0;
				audioOutPos=STEREO_BUFFER_SIZE;
			}

			newSample = 0xffff;
			#else
			
			// PWM output via C64/C128 mainboard
			int32_t s_ = L + R;

			int32_t s = s_ + 65536;
			s = ( s * AUDIO_VALS ) >> 17;

			if ( abs( s - lastS ) == 0 ) {
				if ( silence < 65530 ) silence ++;
			} else
				silence = 0;
			
			lastS = s;
			
			#define RAMP_BITS 9
			#define RAMP_LENGTH ( 1 << RAMP_BITS )

			if ( silence > 16384 ) {
				if ( ramp ) ramp --;
			} else {
				if ( ramp < ( RAMP_LENGTH - 1 ) ) ramp ++;
			}
			
			if ( ramp < ( RAMP_LENGTH - 1 ) ) s = ( s * ramp ) >> RAMP_BITS;

			newSample = s;
		#endif
		}
	}
}

void readConfiguration();
void writeConfiguration();

#define CONFIG_MODE_CYCLES		25000
#define TRANSFER_MODE_CYCLES	30000


/*
uint8_t  transferStage   = 0;
uint16_t launcherAddress = ( launchCode[ 1 ] << 8 ) + launchCode[ 0 ];
uint8_t *transferData	 = (uint8_t *)&launchCode[ 2 ],
		*transferDataEnd = (uint8_t *)&launchCode[ launchSize ];
uint16_t jumpAddress     = 0xD401;
uint8_t  transferReg[ 32 ] = {
	0x78, 0x48, 0x68, 0xA9, launchCode[ 2 ], 0x48, 0x68, 0x8D, 
	launchCode[ 0 ], launchCode[ 1 ], 0x48, 0x68, 0x4C, 0x01, 0xD4 };

const uint8_t __not_in_flash( "mydata" ) jmpCode[ 3 ] = { 0x4c, 0x00, 0xd4 }; // jmp $d400
*/

static inline void handleSidRead(int Address){

	gpio_set_dir_masked( 0xff, 0xff );
	uint8_t D = outRegisters [Address];
	SET_DATA( D );

}
static inline void handleSidWrite(int Address,uint8_t Data,bool Sid2){

		uint16_t SID_CMD = ( Address << 8 ) | Data;
		if (Sid2){
			SID_CMD |= 1 << 15;  // For SID2
		}
		ringTime[ ringWrite ] = (uint64_t)c64CycleCounter;
		ringBuf[ ringWrite ++ ] = SID_CMD;


}
static inline void handleConfigRead(int Address){

	gpio_set_dir_masked( 0xff, 0xff );
	uint8_t D = config [Address];
	SET_DATA( D );

}
// defined in reSIDWrappers
#define CFG_CONFIG_WRITE 29

static inline void handleConfigWrite(int Address,uint8_t Data){

	register uint32_t g;
	volatile const uint32_t *gpioInAddr = &sio_hw->gpio_in;


	if ( Address == CFG_CONFIG_WRITE )
	{
		if ( Data >= 0xfe )
		{
			// update settings and write / do not write to flash
			updateConfiguration();
			updateEmulationParameters();
			if ( Data == 0xff ) writeConfiguration();
			WAIT_FOR_VIC_HALF_CYCLE
			WAIT_FOR_CPU_HALF_CYCLE
		} else
		if ( Data == 0xfb )
		{
			doReset = 0;
		}
	}else{
		config[Address]=Data;	
	}
}

void handleBus()
{
	irq_set_mask_enabled( 0xffffffff, 0 );

	outRegisters[ 0x19 ] = 0;
	outRegisters[ 0x1A ] = 0;
	outRegisters[ 0x1B ] = 0;
	outRegisters[ 0x1C ] = 0;
	outRegisters[ 0x22 + 0x19 ] = 0;
	outRegisters[ 0x22 + 0x1A ] = 0;
	outRegisters[ 0x22 + 0x1B ] = 0;
	outRegisters[ 0x22 + 0x1C ] = 0;

	fmFakeOutput = fmAutoDetectStep = 0;
	outRegisters[ REG_AUTO_DETECT_STEP ] = outRegisters[ REG_AUTO_DETECT_STEP + 34 ] = 0;
	outRegisters[ REG_MODEL_DETECT_VALUE ] = ( config[ /*CFG_SID1_TYPE*/0 ] == 0 ) ? SID_MODEL_DETECT_VALUE_6581 : SID_MODEL_DETECT_VALUE_8580;
	outRegisters[ REG_MODEL_DETECT_VALUE + 34 ] = ( config[ /*CFG_SID2_TYPE*/8 ] == 0 ) ? SID_MODEL_DETECT_VALUE_6581 : SID_MODEL_DETECT_VALUE_8580;

#ifdef LED_BUILTIN
	register uint32_t gpioDir = bOE | ( 1 << LED_BUILTIN ), gpioDirCur = 0;
#endif

	register uint32_t g;
	register uint32_t A;
	register uint8_t  DELAY_READ_BUS_local = DELAY_READ_BUS,
		DELAY_PHI2_local = DELAY_PHI2;
	register uint8_t  D;
	volatile const uint32_t *gpioInAddr = &sio_hw->gpio_in;
	
	register volatile uint8_t disableDataLines = 0;
	//register uint32_t curSample = 0;

	uint8_t skipMeasurements = 0;

	//uint16_t prgLength;
	//uint8_t  *transferPayload;
	uint8_t  addrLines = 99;

	int16_t  stateInConfigMode = 0;
	uint32_t stateConfigRegisterAccess = 0;

	volatile uint32_t cs1count=0;
	volatile uint32_t cs2count=0;
	volatile uint8_t volatile shadow_reg[64]={0,};

#ifdef bOE
	gpio_set_dir_all_bits( gpioDir );
	sio_hw->gpio_clr = bOE;
#endif

	SID2_IOx = SID2_IOx_global;

	WAIT_FOR_CPU_HALF_CYCLE
	WAIT_FOR_VIC_HALF_CYCLE
	WAIT_FOR_CPU_HALF_CYCLE

	/*   __     __      __        __                     __               __
		/__` | |  \    |__) |  | /__` __ |__|  /\  |\ | |  \ |    | |\ | / _`
		.__/ | |__/    |__) \__/ .__/    |  | /~~\ | \| |__/ |___ | | \| \__>
	*/
handleSIDCommunication:

	
	stateInConfigMode = 0;
	for (int i =0; i<64;i++){
		shadow_reg[i]=0;
	}

	while ( true )
	{
		//
		// wait for VIC-halfcycle
		//
        WAIT_FOR_VIC_HALF_CYCLE

		if ( disableDataLines )
		{
			gpio_set_dir_masked( 0xff, 0 );
			disableDataLines = 0;
		}

#ifdef bOE
		if ( gpioDir != gpioDirCur )
		{
			gpioDirCur = gpioDir;
		}
#endif
#ifndef USE_DAC 
		if ( newSample < 0xfffe )
		{
			#ifdef OUTPUT_VIA_PWM
			pwm_set_gpio_level( AUDIO_PIN, newSample );
			#endif
			#ifdef FLASH_LED
			pwm_set_gpio_level( LED_BUILTIN, newLEDValue );
			#endif

			newSample = 0xffff;
		}
#endif
		if ( busValueTTL < 0 )
		{
			busValue = 0;
		} else
			busValueTTL --;

		static uint16_t resetCnt = 0;
		if ( SID_RESET( g ) )
			resetCnt ++; else
			resetCnt = 0;

		if ( resetCnt > 100 && doReset == 0 )
		{
			doReset = 1;
		}

		register uint8_t CPUWritesDelay = DELAY_READ_BUS_local;

		//
		// wait for CPU-halfcycle
		//
		WAIT_FOR_CPU_HALF_CYCLE

		DELAY_Nx3p2_CYCLES( DELAY_PHI2_local )

		g = *gpioInAddr;

		uint8_t *reg;
		/* TODO :
				 Handle FM_ENABLE
		         Handle SID_DETECT
		*/

		if (SID_ACCESS(g)){
			A = SID_ADDRESS( g );
			D =  SID_DATA(g);
			switch ( SID_UPPER_ADDRESS(g))
			{
			/* Address $d400 - $d41f , access to SID1*/	
			case 0x00:
				if ( READ_ACCESS( g ) ){
					handleSidRead(A);
					disableDataLines = 1;
				}else{
					handleSidWrite(A,D,false);
	  				shadow_reg[A]=D;

				}

				break;
			/* Address $d420 - $d43f , access to SID2*/	
			case 0x20:
				if ( READ_ACCESS( g ) ){
					handleSidRead(A+34);
					disableDataLines = 1;
				}else{
					handleSidWrite(A,D,true);
	  				shadow_reg[A+32]=D;
				}

				/* code */
				break;
			/* Address $d4e0 - $d4ff , config-register
			   $d4ff for selection of player */	
			case 0xe0:
				if (A==0x1f){
					//Switch Sid / Mod / whateverplayer here 
				}else{
					if ( READ_ACCESS( g ) ){
						handleConfigRead(A);
						disableDataLines = 1;
					}else{
						handleConfigWrite(A,D);				
					}
			}
				break;
			/* Address $d440 - $d4df , mod-player*/			
			default:
				break;
			}
		}			
	}

}

const uint8_t __in_flash( "section_config" ) __attribute__( ( aligned( FLASH_SECTOR_SIZE ) ) ) flashCFG[ 4096 ];

#define FLASH_CONFIG_OFFSET ((uint32_t)flashCFG - XIP_BASE)
const uint8_t *pConfigXIP = (const uint8_t *)flashCFG;

#define CFG_CRC1 30
#define CFG_CRC2 31
#define CFG_SIZE 32
 
void readConfiguration()
{

	memcpy( config, pConfigXIP, CFG_SIZE );	

	DELAY_READ_BUS = busTimings[ 0 ];
	DELAY_PHI2     = busTimings[ 1 ];

	SET_CLOCK_FAST

	uint16_t c = crc16( config, CFG_CRC1 );

	if ( ( c & 255 ) != config[ CFG_CRC1 ] || ( c >> 8 ) != config[ CFG_CRC2 ] )
	{
		// load default values
		extern void setDefaultConfiguration();
		setDefaultConfiguration();
	}
}

void writeConfiguration()
{
	SET_CLOCK_125MHZ
	//sleep_ms( 2 );
	DELAY_Nx3p2_CYCLES( 85000 );
	flash_range_erase( FLASH_CONFIG_OFFSET, FLASH_SECTOR_SIZE );

	uint16_t c = crc16( config, CFG_CRC1 );
	config[ CFG_CRC1 ] = c & 255;
	config[ CFG_CRC2 ] = c >> 8;

	flash_range_program( FLASH_CONFIG_OFFSET, config, FLASH_PAGE_SIZE );

	SET_CLOCK_125MHZ
	DELAY_Nx3p2_CYCLES( 85000 );
	readConfiguration();
}

extern void setDefaultConfiguration();

int main()
{
	vreg_set_voltage( VREG_VOLTAGE_1_30 );
	readConfiguration();
	setDefaultConfiguration();
	SET_CLOCK_FAST
	init_gpio();
#ifdef LED_BUILTIN
	gpio_set_dir_all_bits( bOE | ( 1 << LED_BUILTIN ) | ( 1 << 23 ) );
#endif	
	// start bus handling and emulation
	multicore_launch_core1( handleBus );
	bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_PROC1_BITS;

	runEmulation();
	return 0;
}
