
add_library(c_flod
   3rdparty/c-flod/player/flodplay_simple.c
   3rdparty/c-flod/backends/wavewriter.c 
   3rdparty/c-flod/flashlib/ByteArray.c 
   3rdparty/c-flod/neoart/flod/core/Amiga.c 
   3rdparty/c-flod/neoart/flod/core/AmigaChannel.c
   3rdparty/c-flod/neoart/flod/core/AmigaFilter.c
   3rdparty/c-flod/neoart/flod/core/AmigaPlayer.c 
   3rdparty/c-flod/neoart/flod/core/AmigaRow.c
   3rdparty/c-flod/neoart/flod/core/AmigaSample.c
   3rdparty/c-flod/neoart/flod/core/AmigaStep.c
   3rdparty/c-flod/neoart/flod/core/CoreMixer.c 
   3rdparty/c-flod/neoart/flod/core/CorePlayer.c
   3rdparty/c-flod/neoart/flod/core/Sample.c
   3rdparty/c-flod/neoart/flod/trackers/PTPlayer.c
   3rdparty/c-flod/neoart/flod/trackers/PTRow.c
   3rdparty/c-flod/neoart/flod/trackers/PTSample.c
   3rdparty/c-flod/neoart/flod/trackers/PTVoice.c
   3rdparty/c-flod/neoart/flod/trackers/STPlayer.c
   3rdparty/c-flod/neoart/flod/trackers/STVoice.c
   3rdparty/c-flod/neoart/flod/futurecomposer/FCPlayer.c
   3rdparty/c-flod/neoart/flod/futurecomposer/FCVoice.c
)
add_definitions(-DFLOD_NO_SOUNDBLASTER)

add_executable(sound_mod
   cream_of_the_earth.h
   sound-mod/sound_core.c
   sound-mod/i2s/i2s.c   
)
bin2h(cream_of_the_earth.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/CreamOfTheEarth.mod mod_data)

#target_include_directories(sound_mod PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include/")
#target_include_directories(sound_mod PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include/")

pico_generate_pio_header(sound_mod ${CMAKE_CURRENT_LIST_DIR}/sound-mod/i2s/i2s.pio)

target_link_libraries(sound_mod
   pico_stdlib
   pico_multicore
   pico_rand
   hardware_dma
   hardware_pio
   hardware_clocks
   c_flod
)
setup_target(sound_mod c-flod)


bin2h(${CMAKE_CURRENT_BINARY_DIR}/reSID_LUT_bin.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-sid/LUT.bin reSID_LUTs)

add_executable(sound_sid
    ${CMAKE_CURRENT_BINARY_DIR}/reSID_LUT_bin.h
    sound-sid/SKpico.c
    3rdparty/reSID16/envelope.cc
    3rdparty/reSID16/extfilt.cc
    3rdparty/reSID16/pot.cc
    3rdparty/reSID16/filter.cc
    3rdparty/reSID16/sid.cc
    3rdparty/reSID16/voice.cc
    3rdparty/reSID16/wave.cc
    sound-sid/reSIDWrapper.cc
)


target_include_directories(sound_sid PRIVATE
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

# are those required? need to test without...
add_compile_definitions(sound_sid PICO_NO_FPGA_CHECK=1)
add_compile_definitions(sound_sid PICO_BARE_METAL=1)
add_compile_definitions(sound_sid PICO_CXX_ENABLE_EXCEPTIONS=0)
target_compile_definitions(sound_sid PUBLIC  PICO PICO_STACK_SIZE=0x100)
target_compile_definitions(sound_sid PRIVATE PICO_MALLOC_PANIC=0)
target_compile_definitions(sound_sid PRIVATE PICO_USE_MALLOC_MUTEX=0)
target_compile_definitions(sound_sid PRIVATE PICO_DEBUG_MALLOC=0)
target_compile_options(sound_sid PRIVATE -save-temps -fverbose-asm)

target_link_libraries(sound_sid
   pico_stdlib
   pico_audio_i2s
   pico_multicore
   hardware_adc
   hardware_dma
   hardware_flash
   hardware_interp
   hardware_pwm
)

# create map/bin/hex/uf2 file in addition to ELF.
setup_target(sound_sid "sound-sid")
# make sure code runs from RAM
pico_set_binary_type(sound_sid copy_to_ram)

add_library(pico_fatfs
   3rdparty/fatfs/fatfs/ff.c
   3rdparty/fatfs/fatfs/ffsystem.c
   3rdparty/fatfs/fatfs/ffunicode.c
   3rdparty/fatfs/tf_card.c
)

target_include_directories(pico_fatfs PUBLIC
   ${CMAKE_CURRENT_LIST_DIR}/3rdparty/fatfs
   ${CMAKE_CURRENT_LIST_DIR}/3rdparty/fatfs/fatfs
   ${CMAKE_CURRENT_LIST_DIR}/3rdparty/fatfs/fatfs/conf
)

target_link_libraries(pico_fatfs PUBLIC
   pico_stdlib
   hardware_clocks
   hardware_spi
)

add_executable(sound_sd
   sound-sd/main.c
)

target_include_directories(sound_sd PUBLIC
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

target_link_libraries(sound_sd
   pico_fatfs
)

setup_target(sound_sd "sound-sd")

