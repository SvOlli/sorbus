#include <time.h>

#include "../backends/wavewriter.h"

#include "../flashlib/ByteArray.h"

#include "../neoart/flod/futurecomposer/FCPlayer.h"
#include "../neoart/flod/trackers/PTPlayer.h"
#include "../neoart/flod/trackers/STPlayer.h"

#define WAVEBUFFER_SIZE (COREMIXER_MAX_BUFFER * 2 * sizeof(float))


enum PlayerType {
	P_A_PT=0,
	P_A_FC,
	P_A_ST,
	P_MAX
};

enum HardwareType {
	HT_AMIGA,
	HT_MAX,
};

static const char player_hardware[] = {
	[P_A_FC]  = HT_AMIGA,
	[P_A_PT]  = HT_AMIGA,
	[P_A_ST]  = HT_AMIGA,
};

enum HardwareType current_hw = HT_MAX;

static const char *player_name[] = {
	[P_A_FC]  = "Future Composer",
	[P_A_PT]  = "ProTracker",
	[P_A_ST]  = "SoundTracker",
};

typedef void (*player_ctor_func) (struct CorePlayer*, struct CoreMixer*);
typedef struct CorePlayer* (*player_new_func) (struct CoreMixer*);
typedef void (*hardware_ctor_func) (struct CoreMixer*);


static const player_new_func player_new[] = {
	[P_A_FC]  = (player_new_func) FCPlayer_new,
	[P_A_PT]  = (player_new_func) PTPlayer_new,
	[P_A_ST]  = (player_new_func) STPlayer_new,
};

static const player_ctor_func player_ctors[] = {
	[P_A_FC]  = (player_ctor_func) FCPlayer_ctor,
	[P_A_PT]  = (player_ctor_func) PTPlayer_ctor,
	[P_A_ST]  = (player_ctor_func) STPlayer_ctor,
};

static const hardware_ctor_func hardware_ctors[] = {
	[HT_AMIGA] = (hardware_ctor_func) Amiga_ctor,
};

enum BackendType {
	BE_WAVE,
	BE_MAX,
};

typedef int (*backend_write_func) (struct Backend *, void*, size_t);
typedef int (*backend_close_func) (struct Backend *);
typedef int (*backend_init_func)  (struct Backend *, void*);

static const struct BackendInfo {
	const char *name;
	backend_init_func  init_func;
	backend_write_func write_func;
	backend_close_func close_func;
} backend_info[] = {
	[BE_WAVE] = {
		.name = "WaveWriter",
		.init_func  = (backend_init_func)  WaveWriter_init,
		.write_func = (backend_write_func) WaveWriter_write,
		.close_func = (backend_close_func) WaveWriter_close,
	}
};
 

static union {
	struct CorePlayer *core;
	struct FCPlayer *fc;
	struct PTPlayer *pt;
	struct STPlayer *st;
} player;

static union {
	struct CoreMixer core;
	struct Amiga amiga;
} hardware;

struct ByteArray *wave;
unsigned char * wave_buffer=NULL;


int main_player(const uint8_t *mod_data,size_t mod_data_size){

	enum BackendType backend_type = BE_WAVE;

	struct ByteArray stream;
	ByteArray_ctor(&stream);

	ByteArray_open_mem(&stream,(char *)mod_data,mod_data_size);

	unsigned i;


	player.core=CorePlayer_new(&hardware.core);

	for(i = 0; i < P_MAX; i++) {
		if(current_hw != player_hardware[i]) {
			current_hw = player_hardware[i];
			hardware_ctors[current_hw](&hardware.core);
		}
		free(player.core);
		player.core=player_new[i](&hardware.core);
		// ctor already in new
		// player_ctors[i](player.core, &hardware.core);
		if(ByteArray_get_length(&stream) > player.core->min_filesize) {
			CorePlayer_load(player.core, &stream);
			if (player.core->version) {
				printf("Playing : %s \n",player.core->title);
				printf("::: using %s player :::\n", player_name[i]);
				goto play;
			}
		}
	}

	printf("couldn't find a player for tune\n");
	return 1;

	union {
		struct Backend backend;
		struct WaveWriter ww;
	} writer;

play:

	// FIXME SOUNDCHANNEL_BUFFER_MAX is currently needed, because the CoreMixer descendants will 
	// misbehave if the stream buffer size is not COREMIXER_MAX_BUFFER * 2 * sizeof(float)
	wave=ByteArray_new();
	wave->endian = BAE_LITTLE;
	wave_buffer=malloc(WAVEBUFFER_SIZE);

	ByteArray_open_mem(wave, wave_buffer, WAVEBUFFER_SIZE);

	hardware.core.wave = wave;


	player.core->initialize(player.core);
	printf("playing subsong [%d/%d]\n", player.core->playSong + 1, player.core->lastSong + 1);

	return 0;	
}

/* free Buffers here*/
void main_player_close(void){

	
	free(wave_buffer);
	wave_buffer=NULL;
	free(wave);
	wave=NULL;
	free(player.core);

}



int play_chunk(int32_t* output_buffer,size_t buffer_size){

#define MAX_PLAYTIME (60 * 5)
	const unsigned bytespersec = 44100 * 2 * (16 / 8);
	const unsigned max_bytes = bytespersec * MAX_PLAYTIME;
	static unsigned int bytes_played = 0;


//	if(CoreMixer_get_complete(&hardware.core)) {
	if(player.core->hardware->completed) {
		return 0;  // Song ended
	}
	if (!wave->pos){
	 	hardware.core.accurate(&hardware.core);
		bytes_played =0;
	}

	if(wave->pos) {
		for (int i =0;i<buffer_size;i+=2){
			output_buffer[i]=((int32_t)((uint16_t)wave_buffer[bytes_played+(i*2)]+(uint16_t)(wave_buffer[bytes_played+(i*2)+1]<<8)))<<16;	
			output_buffer[i+1]=((int32_t)((uint16_t)wave_buffer[bytes_played+(i*2)+2]+((uint16_t)wave_buffer[bytes_played+(i*2)+3]<<8)))<<16;	
		}
		bytes_played +=buffer_size*2;
		wave->pos -= buffer_size*2;
		return 2;
	} 
	if(bytes_played >= max_bytes) {
		printf("hit timeout\n"); 
	}
	

	if(++player.core->playSong <= player.core->lastSong) {
		return 1;
	}

	return 3;
}

