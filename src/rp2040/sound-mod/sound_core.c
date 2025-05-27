/**
 * Copyright (c) 2023 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program implements Apple Computer I emulation
 * for the Sorbus Computer
 */

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "rp2040_purple.h"
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/binary_info.h>
#include "i2s.h"
#include "hardware/dma.h" 

#include <hardware/clocks.h>
#include <hardware/flash.h>
#include "flash_config.h"
#include "common/sound_gpio_config.h"

//#include "cream_of_the_earth.h"
//#include "rsi_rise_up.h"
//#include "phantasmagoria.h"
#include "bloodmoney_intro.h"
//#include "test_mod.h"
//#include "trsi_cracktro.h"

#define SET_CLOCK_125MHZ set_sys_clock_pll( 1500000000, 6, 2 );
 // for ModPlayer we need a multiple of 44,1kHz *32 
#define SET_CLOCK_144MHZ    set_sys_clock_khz( 144000, true );
#define SET_CLOCK_FAST   set_sys_clock_pll( 1500000000, 5, 1 );

#define DELAY_Nx3p2_CYCLES( c )								\
    asm volatile( "mov  r0, %[_c]\n\t"						\
				  "1: sub  r0, r0, #1\n\t"					\
				  "bne   1b"  : : [_c] "r" (c) : "r0", "cc", "memory" );



// TODO : replace with well known table based function
uint16_t crc16(uint8_t *p, uint8_t l)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (l--)
    {
        x = crc >> 8 ^ *p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }
    return crc;
}


/*****************************************************************************************************/
/*             flash System   :
                                    1. page directory
                                        256 Bytes per entry 
                                        [0]  Valid Yes = 0x55 /No !=0x55 
                                        [1] Byte ununsed
                                        [2..3] Crc / checksum  
                                        [4..7] Size
                                        [8..11] Offset
                                        [12..15] unused
                                        [16..254] mod-name / 0 terminated 
                                        [255] 0 
                                        200 bytes
                                    2. page - 257. page Flashdata MOD1  ( 1Mb )
                                    258. page - 513. page Flashdata MOD2  ( 1Mb )
                                    ....
                                    up to 8 Mods


*/
/*****************************************************************************************************/

typedef struct dir_entry_t
{
    __uint8_t valid;
    __uint8_t unused1;
    __uint16_t crc;
    __uint32_t size;
    __uint32_t unused2;
    __int8_t mod_name[238];
    __uint8_t terminate;

}dir_entry;


/* For each mod , 1 Mb space ( minus 1 page ) is reserved in Flash 
   the last page contains the direnty. This is written, when all mod-data has been flashed 
   The reserved space has to be a multiple of FLASH_PAGE_SIZE
*/

#define MAX_MOD_NO 10


uint32_t mod_flash_offset [MAX_MOD_NO]={0x100000*1,0x100000*2,0x100000*3,0x100000*4,0x100000*5,
                                        0x100000*6,0x100000*7,0x100000*8,0x100000*9,0x100000*10 };

#define CFG_CONFIG_SIZE 4096        // one sector

const uint8_t __in_flash( "section_config" ) __attribute__( ( aligned( FLASH_PAGE_SIZE ) ) ) flashCFG[ CFG_CONFIG_SIZE ];

#define FLASH_CONFIG_OFFSET ((uint32_t)flashCFG - XIP_BASE)
const uint8_t *pConfigXIP = (const uint8_t *)flashCFG;
bool configValid=false;

#define CFG_CRC1 30
#define CFG_CRC2 31
#define CFG_SIZE 32

#define CFG_DIR_OFFSET 256

unsigned char config [CFG_CONFIG_SIZE];

void clearDirEntries(void){

    memset (config,0,CFG_CONFIG_SIZE-CFG_DIR_OFFSET);

}

uint8_t * getNextDirEntry(uint8_t * currentEntry){

    uint8_t * nextEntry = currentEntry+sizeof(dir_entry);

    do {
        if (nextEntry[0]==0x55){ // Check if valid
            return nextEntry;
        }

    }while (nextEntry < (pConfigXIP+CFG_CONFIG_SIZE-sizeof(dir_entry)));

    return NULL; // no valid entry found

}

void readConfiguration()
{
	//memcpy( prgDirectory, prgDirectory_Flash, 16 * 24 );

	memcpy( config, pConfigXIP, CFG_SIZE );	

	//DELAY_READ_BUS = busTimings[ 0 ];
	//DELAY_PHI2     = busTimings[ 1 ];

	//SET_CLOCK_FAST
    SET_CLOCK_144MHZ

	uint16_t c = crc16( config, CFG_CRC1 );

	if ( ( c & 255 ) != config[ CFG_CRC1 ] || ( c >> 8 ) != config[ CFG_CRC2 ] )
	{
		// load default values
		extern void setDefaultConfiguration();
		setDefaultConfiguration();
        clearDirEntries();
	}
    configValid=true;
}


void writeConfiguration()
{
	SET_CLOCK_125MHZ
	//sleep_ms( 2 );
	DELAY_Nx3p2_CYCLES( 85000 );
	flash_range_erase( FLASH_CONFIG_OFFSET, FLASH_PAGE_SIZE );

	uint16_t c = crc16( config, CFG_CRC1 );
	config[ CFG_CRC1 ] = c & 255;
	config[ CFG_CRC2 ] = c >> 8;

	flash_range_program( FLASH_CONFIG_OFFSET, config, FLASH_PAGE_SIZE );

	SET_CLOCK_125MHZ
	DELAY_Nx3p2_CYCLES( 85000 );
	readConfiguration();
}

uint32_t get_mod_flash_offset(int mod_no){
    
    assert (mod_no<=MAX_MOD_NO);
    return mod_flash_offset[mod_no];

}

void invalidate_mod_data(int mod_no){

    assert (mod_no<=MAX_MOD_NO);
    uint32_t flash_off = get_mod_flash_offset(mod_no) + 0x100000 -FLASH_PAGE_SIZE;
    flash_range_erase( flash_off, FLASH_PAGE_SIZE );

}

dir_entry * get_mod_dir_entry(int mod_no){

    assert (mod_no<=MAX_MOD_NO);

    uint32_t flash_off = get_mod_flash_offset(mod_no) + 0x100000 -FLASH_PAGE_SIZE;
    return (dir_entry *) (flash_off + XIP_BASE) ;

}

bool is_mod_data_valid(int mod_no){

    assert (mod_no<=MAX_MOD_NO);

    dir_entry * mod_dir_entry=get_mod_dir_entry(mod_no);

    if (mod_dir_entry->valid==0x28){
        return true;
    }
    return false;
}

bool validate_mod_data(int mod_no,uint32_t size,uint16_t crc){

    assert (mod_no<=MAX_MOD_NO);
    invalidate_mod_data(mod_no);
    dir_entry local_dir;
    dir_entry * mod_dir_entry=get_mod_dir_entry(mod_no);

    local_dir.valid=0x28;
    local_dir.size=size;
    local_dir.crc=crc16((uint8_t*)(get_mod_flash_offset(mod_no)+XIP_BASE),size);
    if (local_dir.crc != crc){
        // Flashing failed
        return false;
    }
    flash_range_program((uint32_t)mod_dir_entry-XIP_BASE,(uint8_t*)&local_dir,sizeof(local_dir));

    return true;
}


uint16_t write_mod_data(uint32_t * mod_data, uint32_t size , int mod_no){

    assert (mod_no<=MAX_MOD_NO);
    assert (size<=0x100000);

    uint32_t flash_p = mod_flash_offset[mod_no];
    uint8_t local_page[FLASH_PAGE_SIZE];
    
    // We can erase the whole area    
    flash_range_erase( flash_p, size);
    // TODO: make last sector copy only until the reminder of size  -FLASH_PAGE_SIZE 
    // but we cannot flash from flash to flash, so copy it to RAM first
    for(uint32_t i=0;i<size;i+=FLASH_PAGE_SIZE){
        uint8_t * mod_src=((uint8_t*)mod_data)+i;
        memcpy(local_page,mod_src,FLASH_PAGE_SIZE);
	    flash_range_program( flash_p +i, local_page, FLASH_PAGE_SIZE );
    }
    return (crc16((uint8_t*)mod_data,size));

}

const i2s_config i2s_config_pcm5102 = {44100, 256, 32, SND_SCK, SND_DOUT, SND_DIN, SND_CLKBASE, false};

static __attribute__((aligned(8))) pio_i2s i2s;

#define DEBUG_I2S_CLK

bi_decl(bi_program_name("Sorbus Computer SoundSystem"))
bi_decl(bi_program_description("Using Re-sid or ModPlayer"))
bi_decl(bi_program_url("https://xayax.net/sorbus/"))



extern int play_chunk(int32_t* output_buffer,size_t buffer_size);

static void process_audio(const int32_t* input, int32_t* output, size_t num_frames) {
    play_chunk(output,num_frames);
}
volatile int player_state=1;

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
extern int main_player(const uint8_t *mod_data,size_t mod_data_size);
extern void main_player_init(void);
extern void main_player_close(void);

#ifdef DEBUG_I2S_CLK
extern void calc_clocks(const i2s_config* config, pio_i2s_clocks* clocks);
extern bool validate_sck_bck_sync(pio_i2s_clocks* clocks);
#endif
int main()
{
    pio_i2s_clocks clocks;

   // setup UART
   stdio_init_all();
   //console_set_crlf( true );

#if 1
   // give some time to connect to console
   sleep_ms( 2000 );
#endif

   // Check for valid mod in slot 0 
   if (!is_mod_data_valid(0)){
    // Nope, we got to flash it first
    uint16_t crc= write_mod_data((uint32_t*)mod_data,sizeof(mod_data),0);
    validate_mod_data(0,sizeof(mod_data),crc);
   }  

   // for ModPlayer we need a multiple of 44,1kHz *32
   set_sys_clock_khz( 144000, true );

   // setup the bus and run the bus core
  // multicore_launch_core1( bus_run );


   printf ("Sorbus Sound System initialising I2S\n");
   init_gpio();
#ifdef DEBUG_I2S_CLK
   calc_clocks(&i2s_config_pcm5102, &clocks);
   validate_sck_bck_sync(&clocks);
#endif

    
   main_player((uint8_t*)(mod_flash_offset[0]+XIP_BASE),sizeof(mod_data));
  // main_player((uint8_t*)(mod_data),sizeof(mod_data));
   i2s_program_start_output(pio0, &i2s_config_pcm5102, dma_i2s_in_handler, &i2s);

   while(1){
        if (player_state==0){
            // disable the dma-handler
            dma_channel_set_irq0_enabled(DMA_IRQ_0, false);
            main_player_close();
            main_player((uint8_t*)(mod_flash_offset[0]+XIP_BASE),sizeof(mod_data));
            player_state=1;
            dma_channel_set_irq0_enabled(DMA_IRQ_0, true);
        }

   };

   // free-up everything
   main_player_close();

   return 0;
}
