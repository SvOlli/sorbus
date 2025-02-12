

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

add_executable(sound-mod
   cream_of_the_earth.h
   sound-mod/sound_core.c
   sound-mod/i2s/i2s.c   
)
bin2h(cream_of_the_earth.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/CreamOfTheEarth.mod mod_data)

#target_include_directories(sound-mod PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include/")
#target_include_directories(sound-mod PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include/")
#./pico-sdk/src/rp2_common/hardware_pio/include/hardware/pio.h 

pico_generate_pio_header(sound-mod ${CMAKE_CURRENT_LIST_DIR}/sound-mod/i2s/i2s.pio)

target_link_libraries(sound-mod
   pico_stdlib
   pico_multicore
   pico_rand
   hardware_dma
   hardware_pio
   hardware_clocks
   c_flod
)
setup_target(sound-mod c-flod i2s)


bin2h(reSID_LUT_bin.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-sid/LUT.bin reSID_LUTs)

add_executable(SKpico
    reSID_LUT_bin.h
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

add_compile_definitions(SKpico PICO_NO_FPGA_CHECK=1)
add_compile_definitions(SKpico PICO_BARE_METAL=1)
add_compile_definitions(SKpico PICO_CXX_ENABLE_EXCEPTIONS=0)
add_compile_definitions(SKpico PICO_STDIO_UART=0)
#add_compile_definitions(SKpico PICO_COPY_TO_RAM=1)

target_include_directories(SKpico PRIVATE
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

target_compile_definitions(SKpico PUBLIC  PICO PICO_STACK_SIZE=0x100)
target_compile_definitions(SKpico PRIVATE PICO_MALLOC_PANIC=0)
target_compile_definitions(SKpico PRIVATE PICO_USE_MALLOC_MUTEX=0)
target_compile_definitions(SKpico PRIVATE PICO_DEBUG_MALLOC=0)
target_compile_options(SKpico PRIVATE -save-temps -fverbose-asm)

set_target_properties(SKpico PROPERTIES PICO_TARGET_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/sound-sid/memmap_copy_to_ram_skpico.ld)

target_link_libraries(SKpico pico_stdlib pico_multicore hardware_dma hardware_interp hardware_pwm pico_audio_i2s hardware_flash hardware_adc)
#pico_audio_spdif 

pico_set_program_name(SKpico "SKpico")
pico_set_program_version(SKpico "0.1")

# create map/bin/hex/uf2 file in addition to ELF.
setup_target(SKpico "sound-sid")
