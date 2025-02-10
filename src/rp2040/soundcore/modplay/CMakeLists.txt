cmake_minimum_required(VERSION 3.13)

project(sound_core)
# Native: core for sound output on the plugin-soundcard
add_library(c_flod
         c-flod/player/flodplay_simple.c
         c-flod/backends/wavewriter.c 
         c-flod/flashlib/ByteArray.c 
         c-flod/neoart/flod/core/Amiga.c 
         c-flod/neoart/flod/core/AmigaChannel.c
         c-flod/neoart/flod/core/AmigaFilter.c
         c-flod/neoart/flod/core/AmigaPlayer.c 
         c-flod/neoart/flod/core/AmigaRow.c
         c-flod/neoart/flod/core/AmigaSample.c
         c-flod/neoart/flod/core/AmigaStep.c
         c-flod/neoart/flod/core/CoreMixer.c 
         c-flod/neoart/flod/core/CorePlayer.c
         c-flod/neoart/flod/core/Sample.c
         c-flod/neoart/flod/trackers/PTPlayer.c
         c-flod/neoart/flod/trackers/PTRow.c
         c-flod/neoart/flod/trackers/PTSample.c
         c-flod/neoart/flod/trackers/PTVoice.c
         c-flod/neoart/flod/trackers/STPlayer.c
         c-flod/neoart/flod/trackers/STVoice.c
         c-flod/neoart/flod/futurecomposer/FCPlayer.c
         c-flod/neoart/flod/futurecomposer/FCVoice.c
)
add_definitions(-DFLOD_NO_SOUNDBLASTER)

add_executable(sound_core
   sound_core.c
   i2s/i2s.c   
   )
target_include_directories(sound_core PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include/")
target_include_directories(sound_core PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include/")
#./pico-sdk/src/rp2_common/hardware_pio/include/hardware/pio.h 

pico_generate_pio_header(sound_core ${CMAKE_CURRENT_LIST_DIR}/i2s/i2s.pio)

target_link_libraries(sound_core
   pico_stdlib
   pico_multicore
   pico_rand
   hardware_dma
   hardware_pio
   hardware_clocks
   c_flod
   )
setup_target(sound_core c-flod i2s)
