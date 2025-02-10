cmake_minimum_required(VERSION 3.13)

project(SKpico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)



## Currently needed for "audio_i2s" 
## As we switch to included i2s implementation, we will get rid of this
set (PICO_EXTRAS_FETCH_FROM_GIT_PATH "${PICO_SDK_PATH}/..")
set (PICO_EXTRAS_FETCH_FROM_GIT ON)
message("STORE PICO EXTRAS to: ${PICO_EXTRAS_FETCH_FROM_GIT_PATH}")
include(pico_extras_import.cmake)
include(${PICO_EXTRAS_PATH}/post_init.cmake)

#set (PICO_COPY_TO_RAM 1)
add_compile_definitions(SKpico PICO_NO_FPGA_CHECK=1)
add_compile_definitions(SKpico PICO_BARE_METAL=1)
add_compile_definitions(SKpico PICO_CXX_ENABLE_EXCEPTIONS=0)
add_compile_definitions(SKpico PICO_STDIO_UART=0)
add_compile_definitions(SKpico PICO_COPY_TO_RAM=1)

bin2h(reSID_LUT_bin.h ${CMAKE_CURRENT_SOURCE_DIR}/LUT.bin reSID_LUTs)

add_executable(SKpico
    reSID_LUT_bin.h
    SKpico.c
    reSID16/envelope.cc
    reSID16/extfilt.cc
    reSID16/pot.cc
    reSID16/filter.cc
    reSID16/sid.cc
    reSID16/voice.cc
    reSID16/wave.cc
    reSIDWrapper.cc
)
target_include_directories(SKpico PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(SKpico PUBLIC  PICO PICO_STACK_SIZE=0x100)
target_compile_definitions(SKpico PRIVATE PICO_MALLOC_PANIC=0)
target_compile_definitions(SKpico PRIVATE PICO_USE_MALLOC_MUTEX=0)
target_compile_definitions(SKpico PRIVATE PICO_DEBUG_MALLOC=0)
target_compile_options(SKpico PRIVATE -save-temps -fverbose-asm)

set_target_properties(SKpico PROPERTIES PICO_TARGET_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/memmap_copy_to_ram_skpico.ld)

target_link_libraries(SKpico pico_stdlib pico_multicore hardware_dma hardware_interp hardware_pwm pico_audio_i2s hardware_flash hardware_adc)
#pico_audio_spdif 

pico_set_program_name(SKpico "SKpico")
pico_set_program_version(SKpico "0.1")

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(SKpico)
