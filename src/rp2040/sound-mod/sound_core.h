#ifndef _SOUNDCORE_H_
#define _SOUNDCORE_H_

enum player_states_e{
    PLAYER_STATE_STOP=0,
    PLAYER_STATE_STOPPED,
    PLAYER_STATE_PLAY,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_RESTART,
    PLAYER_STATE_NEXT
};


#define MAX_MOD_NO 10

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
                                        [16..36] mod-name / 0 terminated
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
    __int8_t mod_name[21];
    __uint8_t terminate;

}dir_entry;


extern volatile int player_state; // How ugly ;)
extern volatile int play_mod;

bool is_mod_data_valid(int mod_no);
uint32_t get_mod_flash_offset(int mod_no);
dir_entry * get_mod_dir_entry(int mod_no);
uint32_t get_mod_size(int mod_no);
uint32_t get_mod_flash_offset(int mod_no);
bool validate_mod_data(int mod_no,uint32_t size,uint16_t crc,bool use_crc);
void erase_flash(uint32_t offset, size_t size);
void program_flash(uint32_t offset, const uint8_t* data, size_t size);




#endif