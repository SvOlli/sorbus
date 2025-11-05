#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pico/stdlib.h>
#include <hardware/sync.h>
#include <pico/multicore.h>

#include <hardware/flash.h>

#include "rp2040_purple.h"

#include "sound_console.h"
#include "common/sound_gpio_config.h"
#include "sound_core.h"
#include "xmodem.h"

int menu_state=0;
int mod_slot=0;
int filter_mode=0;

uint32_t  mod_flashp;
uint8_t local_buf[FLASH_SECTOR_SIZE];
int mod_flash_rest=0;
uint8_t empty_buf[0x80]={0,};


/* For xmodem receive, we need to provide the HW specific functions*/
int _inbyte(int msec){


   int c = getchar_timeout_us( msec*1000 );
   if( c == PICO_ERROR_TIMEOUT )
   {
      return -1;
   }
   return c;

}
void _outbyte(unsigned char c){

    putchar(c);
}

int received_chunk(unsigned char * buf, int size){

    /* Write Flash here*/
    if (size<FLASH_SECTOR_SIZE){
        memcpy(local_buf+mod_flash_rest,buf,size);
        mod_flash_rest+=size;
        if (mod_flash_rest>=FLASH_SECTOR_SIZE){
          erase_flash( mod_flashp, FLASH_SECTOR_SIZE);
          program_flash( mod_flashp, local_buf, FLASH_SECTOR_SIZE );
          mod_flashp+=FLASH_SECTOR_SIZE;
          mod_flash_rest=0;
        }
    }else{
        erase_flash( mod_flashp, size);
        program_flash( mod_flashp, buf, size );
        mod_flashp+=size;

    }

}


void print_menu(){

    printf("-----------------------------------------\n");
    printf("      Sorbus Sound V1.0 Debug menu \n");
    printf ("-----------------------------------------\n");
    printf("\n");
    printf(" d : print content of mod flash slots \n");
    printf(" 0-9 : start mod file in slot 0 - 9   \n");
    printf(" r : restart current song\n");
    printf(" s : stop current song\n");
    printf(" n : play next song\n");
    printf(" u : upload mod file to slot \n");
    printf(" f : toggle filtermodes \n");
    printf("\n");
    printf("\n");


}

void print_upload_menu(){
    printf ("Uploading song to slot ? (0-9)\n");
    printf ("Press other key to cancel\n");

}


void print_mod_dir(){

    printf ("----------------------------------------------\n");
    printf ("SlotNo\t| Size \t\t| Name\n");
    for (int i=0;i<MAX_MOD_NO;i++){
        printf("    %d\t| ",i);
        if (is_mod_data_valid(i)){
                dir_entry * mod_dir_entry=get_mod_dir_entry(i);
                printf("%06d\t| %s",mod_dir_entry->size,mod_dir_entry->mod_name);
        }
        printf("\n");
    }
    printf ("---------------------------------------------\n\n");


}
int getkey( char *key )
{
   int c;
   c = getchar_timeout_us( 1000 );
   if( c == PICO_ERROR_TIMEOUT )
   {
      return 0;
   }
   *key = tolower(c);
#if DEBUG_GETALINE
   printf( "\ngetkey: %02x\n", c );
#endif
   return 1;
}

void sound_console(void){

    char read_char;

    if (getkey(&read_char)){
        /* Xmodem-upload */
        if (menu_state==1){
           switch (read_char){
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    mod_slot=read_char-'0';
                    printf("Uploading to slot %d\n",mod_slot);
                    mod_flashp= get_mod_flash_offset(mod_slot);
                    printf("Start XModem Transfer now");
                    int32_t ret=xmodemReceive();
                    if (ret<0){
                        printf("\nFailure in reception %d\n",ret);
                    }else{
                        if (mod_flash_rest){
                            mod_flash_rest=FLASH_SECTOR_SIZE-0x80;
                            received_chunk(empty_buf,0x80);
                        }
                        validate_mod_data(mod_slot,ret,0,false);
                        sleep_ms(500);
                        printf("\nReception successful. received  %d bytes\n",ret);
                        menu_state=0;
                    }
                    print_menu();
                    return;
                break;

                case 0x0a:  // ignore
                case 0x0d:
                break;

                default:
                    printf("Upload canceled , got '%c' 0x%02x\n",read_char,read_char);
                    print_menu();
                    menu_state=0;
                break;
            }

        }
        /* Main-menu */
        if (menu_state==0){
            switch (read_char){
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    int mod_no=read_char-'0';
                    if (is_mod_data_valid(mod_no)){
                        play_mod=mod_no;
                        player_state= PLAYER_STATE_PLAY;
                    }else{
                        printf("No valid mod in slot %d\n\n",mod_no);
                    }
                    print_menu();
                break;
                case 's':
                    printf("Stopping song \n");
                    player_state= PLAYER_STATE_STOP;
                    print_menu();
                break;
                case 'n':
                    printf("Playing next song \n");
                    player_state= PLAYER_STATE_NEXT;
                    print_menu();
                break;
                case 'r':
                    printf("Restarting song \n");
                    player_state= PLAYER_STATE_RESTART;
                    print_menu();
                break;
                case 'd':
                    print_mod_dir();
                    print_menu();
                    break;
                case 'u':
                   // player_state= PLAYER_STATE_STOP;
                    menu_state=1;
                    print_upload_menu();
                    break;
                case 'f':
                    filter_mode++;
                    filter_mode&=0x03;
                    gpio_put(SND_DEMP,filter_mode&0x01);  // no De-Emphasis
                    gpio_put(SND_FLT,(filter_mode>>1)&0x01);  // no De-Emphasis
                    printf ("Demphesisfilter is %d\n",filter_mode&0x01);
                    printf ("FIR filter is %d\n",(filter_mode>>1)&0x01);
                break;

                case 0x0a:  // ignore
                case 0x0d:
                    break;

                default:
                    printf ("Hmmm ?? \n");
                    print_menu();

                break;
            }
        }

    }
}