
add_library(sound_bus
   common/sound_bus.c
)
pico_generate_pio_header(sound_bus
   ${CMAKE_CURRENT_LIST_DIR}/common/sound_bus.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(sound_bus PUBLIC
   pico_stdlib
   pico_multicore
   hardware_pio
)
target_include_directories(sound_bus PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}
)


#target_include_directories(i2s PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include/")
#target_include_directories(i2s PRIVATE "${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include/")

add_executable(sound_iotest
   sound-iotest/main.c
   common/sound_bus.c
   common/sound_gpio_config.c
)
target_link_libraries(sound_iotest PRIVATE
   sound_bus
)
add_dependencies(sound_iotest sound_bus)
setup_target(sound_iotest "sound-iotest")

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
# fixing those warnings will be a job for later
target_compile_options(c_flod PRIVATE "--no-warnings")

bin2h(cream_of_the_earth.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/CreamOfTheEarth.mod mod_data)
bin2h(rsi_rise_up.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/MOD.rsi-rise_up mod_data)
bin2h(phantasmagoria.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/phantasmagoria.mod mod_data)
bin2h(bloodmoney_intro.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/BloodMoneyIntro.mod mod_data)
bin2h(test_mod.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/bloodmoney.mod mod_data)
bin2h(trsi_cracktro.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-mod/Mod.TrsiCrack10.Mod mod_data)
add_executable(sound_mod
   cream_of_the_earth.h
   rsi_rise_up.h
   phantasmagoria.h
   bloodmoney_intro.h
   trsi_cracktro.h
   test_mod.h
   sound-mod/sound_core.c 
   sound-mod/sound_console.c   
   sound-mod/sd_access.c  
   common/sound_gpio_config 
   3rdparty/i2s/i2s.c
   3rdparty/xmodem/xmodem.c
)
pico_generate_pio_header(sound_mod ${CMAKE_CURRENT_LIST_DIR}/3rdparty/i2s/i2s.pio)
set_property(SOURCE sound-mod/sound_core.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/rp2040/cream_of_the_earth.h)
set_property(SOURCE sound-mod/sound_core.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/rp2040/rsi_rise_up.h)
set_property(SOURCE sound-mod/sound_core.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/rp2040/phantasmagoria.h)
set_property(SOURCE sound-mod/sound_core.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/rp2040/bloodmoney_intro.h)
set_property(SOURCE sound-mod/sound_core.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/rp2040/trsi_cracktro.h)
set_property(SOURCE sound-mod/sound_core.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/rp2040/test_mod.h)



target_include_directories(sound_mod PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/i2s
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/xmodem
)


target_link_libraries(sound_mod
   pico_stdlib
   pico_multicore
   pico_rand
   hardware_dma
   hardware_pio
   hardware_clocks
   c_flod
   pico_fatfs
)
setup_target(sound_mod c-flod)

#################  Standalone player ######################



add_executable(sound_mod_player 
   cream_of_the_earth.h
   sound-mod/sound_core.c  
   sound-mod/standalone.c  
   sound-mod/sound_console.c  
   common/sound_gpio_config 
   3rdparty/i2s/i2s.c
   3rdparty/xmodem/xmodem.c
)
pico_generate_pio_header(sound_mod_player ${CMAKE_CURRENT_LIST_DIR}/3rdparty/i2s/i2s.pio)

target_compile_definitions(sound_mod_player PUBLIC -DSTANDALONE_PLAYER)


target_include_directories(sound_mod_player PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/i2s
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/xmodem
)


target_link_libraries(sound_mod_player
   pico_stdlib
   pico_multicore
   pico_rand
   hardware_dma
   hardware_pio
   hardware_clocks
   c_flod
)
setup_target(sound_mod_player c-flod)


###################### sid player #######################

bin2h(${CMAKE_CURRENT_BINARY_DIR}/reSID_LUT_bin.h ${CMAKE_CURRENT_SOURCE_DIR}/sound-sid/LUT.bin reSID_LUTs)

add_executable(sound_sid
    ${CMAKE_CURRENT_BINARY_DIR}/reSID_LUT_bin.h
    sound-sid/SKpico.c
    common/sound_gpio_config 
    3rdparty/reSID16/envelope.cc
    3rdparty/reSID16/extfilt.cc
    3rdparty/reSID16/pot.cc
    3rdparty/reSID16/filter.cc
    3rdparty/reSID16/sid.cc
    3rdparty/reSID16/voice.cc
    3rdparty/reSID16/wave.cc
    sound-sid/reSIDWrapper.cc
    3rdparty/i2s/i2s.c
)
pico_generate_pio_header(sound_sid ${CMAKE_CURRENT_LIST_DIR}/3rdparty/i2s/i2s.pio)
target_include_directories(sound_sid PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/i2s
)
# fixing those warnings will be a job for later
target_compile_options(sound_sid PRIVATE "--no-warnings")


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
   pico_multicore
   pico_fatfs
)

setup_target(sound_sd "sound-sd")

