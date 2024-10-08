cmake_minimum_required(VERSION 3.13)

project(sorbus_rp2040)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

#string(REPLACE "-O3" "-O6" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
#string(REPLACE "-O3" "-O6" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wunused-variable")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
set(FIRMWARE_BASE "${CMAKE_CURRENT_BINARY_DIR}/native_firmware")

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

function(genfirmware firmware native_rom cpm_rom cpm_dhara)
   add_custom_command(OUTPUT ${firmware}
      COMMAND $<TARGET_FILE:binjoin> "${firmware}" $<TARGET_FILE:${native_rom}> "${cpm_rom}" $<TARGET_FILE:${cpm_dhara}>
      DEPENDS ${native_rom} ${cpm_rom} ${cpmfs} ${BINJOIN}
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

function(setup_target targetname)
   pico_enable_stdio_usb(${targetname} 1)
   pico_enable_stdio_uart(${targetname} 0)
   pico_add_extra_outputs(${targetname})
   target_include_directories(${targetname} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

bin2h(krusader.h ${CMAKE_HOME_DIRECTORY}/bin/krusader_65C02.bin krusader_e000)
src2h(cpudetect_apple1.h cpudetect_apple1 cpudetect_0280)
src2h(cpudetect.h cpudetect cpudetect)
src2h(cpustate.h cpustate cpustate)
src2h(payload_mcp.h payload_mcp payload_mcp)
src2h(a1hello.h a1hello a1hello_0800)
src2h(cpudetect_65.h cpudetect_65 cpudetect_code)
src2h(native_rom.h native_rom native_rom)

# getaline_test: test core to test getaline input functionality
add_executable(getaline_test
   getaline_test.c
   getaline.c
   )
target_link_libraries(getaline_test
   pico_stdlib
   pico_multicore
   )
setup_target(getaline_test)

# Monitor Command Prompt: core for watching the 65C02 work
add_executable(mcp
   bus_rp2040_purple.c
   cpudetect.h
   cpu_detect.c
   cpustate.h
   disassemble.c
   disassemble_historian.c
   generic_helper.c
   payload_mcp.h
   system_mcp.c
   getaline.c
   )
target_link_libraries(mcp
   pico_stdlib
   pico_multicore
   )
setup_target(mcp)

# Apple 1: core for mimicking the Apple 1 Computer
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

# Native: core for implementing a custom Sorbus system
add_executable(native_alpha
   bus_rp2040_purple.c
   cpu_detect.c
   disassemble.c
   disassemble_historian.c
   generic_helper.c
   system_native.c
   dhara/error.c
   dhara/journal.c
   dhara/map.c
   native/core0.c
   native/core1.c
   native/dhara_flash.c
   native/event_queue.c
   )
target_link_libraries(native_alpha
   pico_stdlib
   pico_multicore
   pico_rand
   )
setup_target(native_alpha)
# firmware for Native core
add_custom_target(native_firmware ALL
   DEPENDS native_alpha_picotool native_rom
)

# building firmware .uf2 file containing all ROMs
add_custom_target(native_rom ALL
   $<TARGET_FILE:bin2uf2> "native_rom.uf2"
      0x103FA000
      $<TARGET_FILE:native_kernel>
      $<TARGET_FILE:native_tools>
      $<TARGET_FILE:native_basic>
   DEPENDS native_kernel native_tools native_basic ${BIN2UF2}
)

bin2uf2(native_cpmfs 0x10400000 native_cpmfs.dhara)
uf2join(native_alpha_picotool
   native_rom
   native_cpmfs
   native_alpha
)
dharawrite(native_cpmfs.dhara native_cpmfs.img)
