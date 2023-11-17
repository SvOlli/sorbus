cmake_minimum_required(VERSION 3.13)

project(sorbus_rp2040)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

#string(REPLACE "-O3" "-O6" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
#string(REPLACE "-O3" "-O6" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wunused-variable")

function(bin2h h_file bin_file label)
   add_custom_command(OUTPUT ${h_file}
      COMMAND $<TARGET_FILE:bin2h> "${bin_file}" "${h_file}" "${label}"
      DEPENDS ${bin_file} ${BIN2H})
endfunction()

function(src2h h_file bin_file label)
   add_custom_command(OUTPUT ${h_file}
      COMMAND $<TARGET_FILE:bin2h> $<TARGET_FILE:${bin_file}> "${h_file}" "${label}"
      DEPENDS ${bin_file} ${BIN2H})
endfunction()

function(setup_target targetname)
   pico_enable_stdio_usb(${targetname} 1)
   pico_enable_stdio_uart(${targetname} 0)
   pico_add_extra_outputs(${targetname})
   target_include_directories(${targetname} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

bin2h(krusader.h ${CMAKE_HOME_DIRECTORY}/bin/krusader_65C02.bin krusader_e000)
# wenn man die CPM Buildumgebung hat, können die Files gebaut werden

#bin2h(cpm_rom.h ${CMAKE_HOME_DIRECTORY}/../../cpm65/.obj/sorbus.exe cpm_rom)
#bin2h(bdos_rom.h ${CMAKE_HOME_DIRECTORY}/../../cpm65/.obj/bdos.sys bdos_rom)
#bin2h(cpmfs_rom.h ${CMAKE_HOME_DIRECTORY}/../../cpm65/.obj/generic-1m-cpmfs.img cpmfs_rom)
src2h(cpudetect_apple1.h cpudetect_apple1 cpudetect_0280)
src2h(cpudetect_mcp.h cpudetect_mcp cpudetect_mcp)
src2h(cpustate.h cpustate cpustate)
src2h(payload_mcp.h payload_mcp payload_mcp)
src2h(a1hello.h a1hello a1hello_0800)
src2h(cpudetect_65.h cpudetect_65 cpudetect_code)
src2h(native_rom.h native_rom native_rom)

add_executable(getaline_test
   getaline_test.c
   getaline.c
   )
target_link_libraries(getaline_test
   pico_stdlib
   pico_multicore
   )
setup_target(getaline_test)

add_executable(mcp
   bus_rp2040_purple.c
   cpudetect_mcp.h
   cpu_detect.c
   cpustate.h
   payload_mcp.h
   system_mcp.c
   getaline.c
   fake_eeprom.c
   )
target_link_libraries(mcp
   pico_stdlib
   pico_multicore
   )
setup_target(mcp)

add_executable(apple1
   krusader.h
   cpudetect_apple1.h
   a1hello.h
   bus_rp2040_purple.c
   system_apple1.c
   )
target_link_libraries(apple1
   pico_stdlib
   pico_multicore
   )
setup_target(apple1)

add_executable(native_test
   native_rom.h
   bus_rp2040_purple.c
   system_native.c
#   gpio_oc.c
   native/core0.c
   native/core1.c
   native/event_queue.c
   native/menu.c
   )
target_link_libraries(native_test
   pico_stdlib
   pico_multicore
   pico_rand
   )
setup_target(native_test)

add_executable(cpm_os
   cpm_rom.h
   bdos_rom.h
   native_rom.h
   cpmfs_rom.h
   bus_rp2040_purple.c
   system_cpm.c
#   gpio_oc.c
   native/core0.c
   native/core1.c
   native/event_queue.c
   native/menu.c
   flash/devflash.c
   flash/rawflash.c
   )
target_include_directories(cpm_os PUBLIC ${CMAKE_HOME_DIRECTORY}/dhara )
# wenn man die CPM Buildumgebung nicht hat, können die eingecheckten Files benutze werden
target_include_directories(cpm_os PUBLIC ${CMAKE_HOME_DIRECTORY}/bin_headers )

target_link_libraries(cpm_os
   pico_stdlib
   pico_multicore
   pico_rand
   dhara
   )
setup_target(cpm_os)


