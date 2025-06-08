  
#include <tf_card.h>
#include <ff.h>
#include "fatfs/diskio.h"
#include "common/sound_gpio_config.h"


bool init_sd_card (void)  {
  
  pico_fatfs_spi_config_t config = {
      spi0,
      CLK_SLOW_DEFAULT,
      CLK_FAST_DEFAULT,
      SD_MISO,
      SD_CS,
      SD_SCK,
      SD_MOSI,
      true  // use internal pullup
   };
   pico_fatfs_set_config(&config);

   DSTATUS status = disk_initialize(0);
   if (status == STA_NODISK){
    return false;
   }

   return true;
}

int get_mod_entries(void){

    DIR root_dir;
    FILINFO curr_file;

    FATFS fs_obj;
    FRESULT res;

    res=f_mount(&fs_obj,"/",1);

    res=f_opendir (&root_dir,".");
    if (res != FR_OK){
        return res;
    }
    do {
        res= f_readdir(&root_dir,&curr_file);
    }while (res == FR_OK);
    
}