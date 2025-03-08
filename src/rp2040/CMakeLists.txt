cmake_minimum_required(VERSION 3.13)

project(sorbus_rp2040)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

#string(REPLACE "-O3" "-O6" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
#string(REPLACE "-O3" "-O6" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wunused-variable")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
set(FIRMWARE_BASE "${CMAKE_CURRENT_BINARY_DIR}/jam_firmware")

function(bin2h h_file bin_file label)
   add_custom_command(OUTPUT ${h_file}
      COMMAND $<TARGET_FILE:bin2h> "${bin_file}" "${h_file}" "${label}"
      DEPENDS ${bin_file} ${BIN2H}
   )
endfunction()

function(src2h h_file bin_file label)
   add_custom_command(OUTPUT ${h_file}
      COMMAND $<TARGET_FILE:bin2h> $<TARGET_FILE:${bin_file}> "${h_file}" "${label}"
      DEPENDS ${bin_file} ${BIN2H}
   )
endfunction()

function(bin2uf2 uf2_file start_addr bin_file)
   add_custom_target(${uf2_file} ALL
      $<TARGET_FILE:bin2uf2> "${uf2_file}.uf2" "${start_addr}" "${bin_file}"
      DEPENDS ${bin_file} ${BIN2UF2}
   )
endfunction()

function(dharawrite out_file in_file)
   add_custom_target(${out_file} ALL
      $<TARGET_FILE:dharatool> dharawrite "../65c02/${in_file}" "${out_file}"
      DEPENDS ${in_file} ${dharatool}
   )
endfunction()

function(genfirmware firmware jam_rom cpm_rom cpm_dhara)
   add_custom_command(OUTPUT ${firmware}
      COMMAND $<TARGET_FILE:binjoin> "${firmware}" $<TARGET_FILE:${jam_rom}> "${cpm_rom}" $<TARGET_FILE:${cpm_dhara}>
      DEPENDS ${jam_rom} ${cpm_rom} ${cpmfs} ${BINJOIN}
   )
endfunction()

function(uf2join output)
   set(inputs)
   foreach(input ${ARGN})
      set(inputs ${inputs} ${input}.uf2)
   endforeach()
   add_custom_command(OUTPUT ${output}
      COMMAND $<TARGET_FILE:binjoin> ${output}.uf2 ${inputs}
      DEPENDS ${BINJOIN} ${ARGN}
   )
endfunction()

function(setup_target targetname incdir)
   pico_enable_stdio_usb(${targetname} 1)
   pico_enable_stdio_uart(${targetname} 0)
   pico_add_extra_outputs(${targetname})
   target_include_directories(${targetname} PRIVATE
      ${incdir}
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/common
      ${CMAKE_CURRENT_BINARY_DIR}
   )
endfunction()

bin2h(krusader.h ${CMAKE_HOME_DIRECTORY}/bin/krusader_65C02.bin krusader_e000)
src2h(cpudetect_apple1.h cpudetect_apple1 cpudetect_0280)
src2h(cpudetect.h cpudetect cpudetect)
src2h(payload_mcp.h payload_mcp payload_mcp)
src2h(a1hello.h a1hello a1hello_0800)
src2h(cpudetect_65.h cpudetect_65 cpudetect_code)
src2h(jam_rom.h jam_rom jam_rom)

# getaline_test: test core to test getaline input functionality
add_executable(getaline_test
   common/getaline_test.c
   common/getaline.c
   )
target_link_libraries(getaline_test
   pico_stdlib
   pico_multicore
   )
setup_target(getaline_test "mcp")

# Monitor Command Prompt: core for watching the 65C02 work
add_executable(mcp
   cpudetect.h
   payload_mcp.h
   common/bus_rp2040_purple.c
   common/cpu_detect.c
   common/disassemble.c
   common/disassemble_historian.c
   common/generic_helper.c
   common/getaline.c
   mcp/main.c
   )
target_link_libraries(mcp
   pico_stdlib
   pico_multicore
   )
setup_target(mcp "mcp")

# Apple 1: core for mimicking the Apple 1 Computer
add_executable(apple1
   krusader.h
   cpudetect_apple1.h
   a1hello.h
   common/bus_rp2040_purple.c
   apple1/main.c
   )
target_link_libraries(apple1
   pico_stdlib
   pico_multicore
   )
setup_target(apple1 "apple1")

# Native: core for implementing a custom Sorbus system
add_executable(jam_alpha
   cpudetect.h
   common/bus_rp2040_purple.c
   common/cpu_detect.c
   common/disassemble.c
   common/disassemble_historian.c
   common/generic_helper.c
   dhara/error.c
   dhara/journal.c
   dhara/map.c
   jam/main.c
   jam/console.c
   jam/bus.c
   jam/dhara_flash.c
   jam/event_queue.c
   )
target_link_libraries(jam_alpha
   pico_stdlib
   pico_multicore
   pico_rand
   )
setup_target(jam_alpha "jam")
#pico_set_binary_type(jam_alpha copy_to_ram)
# firmware for JAM core
add_custom_target(jam_firmware ALL
   DEPENDS jam_alpha_picotool jam_rom
)

# building firmware .uf2 file containing all ROMs
add_custom_target(jam_rom ALL
   $<TARGET_FILE:bin2uf2> "jam_rom.uf2"
      0x103FA000
      $<TARGET_FILE:jam_kernel>
      $<TARGET_FILE:jam_tools>
      $<TARGET_FILE:jam_basic>
   DEPENDS jam_kernel jam_tools jam_basic ${BIN2UF2}
)

bin2uf2(jam_cpmfs 0x10400000 jam_cpmfs.dhara)
uf2join(jam_alpha_picotool
   jam_rom
   jam_cpmfs
   jam_alpha
)
dharawrite(jam_cpmfs.dhara jam_cpmfs.img)

add_executable(fb32x32ws2812)
pico_generate_pio_header(fb32x32ws2812
   ${CMAKE_CURRENT_LIST_DIR}/fb32x32/ws2812.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
pico_generate_pio_header(fb32x32ws2812
   ${CMAKE_CURRENT_LIST_DIR}/fb32x32/bus.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
target_sources(fb32x32ws2812 PRIVATE
   fb32x32/main.c
   fb32x32/bus.c
   fb32x32/control.c
   fb32x32/hardware_ws2812.c
   fb32x32/translation_matrix_1.c
)
target_link_libraries(fb32x32ws2812 PRIVATE
   pico_stdlib
   pico_multicore
   hardware_pio
)
pico_set_binary_type(fb32x32ws2812 copy_to_ram)
setup_target(fb32x32ws2812 "fb32x32")

add_executable(fb32x32usbuart)
# depends on bus.pio
add_dependencies(fb32x32usbuart fb32x32ws2812)
target_sources(fb32x32usbuart PRIVATE
   fb32x32/main.c
   fb32x32/bus.c
   fb32x32/control.c
   fb32x32/hardware_uart.c
)
target_link_libraries(fb32x32usbuart PRIVATE
   pico_stdlib
   pico_multicore
   hardware_pio
)
pico_set_binary_type(fb32x32usbuart copy_to_ram)
setup_target(fb32x32usbuart "fb32x32")

# Build binaries for SorbusSound 
add_subdirectory(soundcore)