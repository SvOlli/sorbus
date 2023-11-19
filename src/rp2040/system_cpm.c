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

#include "rp2040_purple.h"
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/binary_info.h>

#include "native/common.h"
#include "bus.h"
#include "flash/devflash.h"

bi_decl(bi_program_name("Sorbus Computer Native Core"))
bi_decl(bi_program_description("implement an own home computer flavor"))
bi_decl(bi_program_url("https://xayax.net/sorbus/"))


#include "cpm_rom.h"
#include "bdos_rom.h"

// Zum Flashes des Diskimages einkommentieren . 
#define FLASH_CPM_FS
#ifdef FLASH_CPM_FS
#include "cpmfs_rom.h"
#endif

#define CPM_START 0x0801
#define BDOS_START 0xc000
extern uint8_t memory[0x10000];
queue_t queue_uart_read;
queue_t queue_uart_write;

uint8_t testbuffer1[512];
uint8_t testbuffer2[512];

#ifdef FLASH_CPM_FS
int flashcpmfs(void){
   int err=0;

   for(int i =0;i<sizeof(cpmfs_rom)/512;i++){
      memcpy(testbuffer1,&cpmfs_rom[i*512],512);
      err+= transfer_cb(i,testbuffer1,false);
   }
   
   err+= transfer_cb(0,testbuffer2,true);
   err+= transfer_cb(1,testbuffer2,true);

   return err;

}
#endif
int main()
{
   // setup UART
   stdio_init_all();
   uart_set_translate_crlf( uart0, true );

   // give some time to connect to console
   sleep_ms(2000);

   // for toying with overclocking
   set_sys_clock_khz ( 133000, false );

   // setup between UART core and bus core
   queue_init( &queue_uart_read,  sizeof(int), 128 );
   queue_init( &queue_uart_write, sizeof(int), 128 );

   // setup the bus and run the bus core
   bus_init();
   system_init();
   flash_dev_init();
 #ifdef FLASH_CPM_FS
   flashcpmfs();
 #endif
   // copy cpm-startup
   memcpy( &memory[CPM_START], &cpm_rom[2], sizeof(cpm_rom)-2 );  // has startaddress in front
   memcpy( &memory[BDOS_START], &bdos_rom[0], sizeof(bdos_rom) );

   system_reboot();
   multicore_launch_core1( bus_run );

   // run interactive console -> should never return
   console_run();
   

   return 0;
}
