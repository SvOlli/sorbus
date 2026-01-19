cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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
   pico_enable_stdio_usb(${targetname} 0)
   pico_enable_stdio_uart(${targetname} 1)
   pico_add_extra_outputs(${targetname})
   target_include_directories(${targetname} PRIVATE
      ${incdir}
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/common
      ${CMAKE_CURRENT_SOURCE_DIR}/mcurses
      ${CMAKE_CURRENT_BINARY_DIR}
   )
   target_link_options(${targetname} PRIVATE -Xlinker --print-memory-usage)
endfunction()

# payload for jam and mcp
src2h(cpudetect.h cpudetect cpudetect)

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

include(apple1.cmake)
include(mcp.cmake)
include(jam.cmake)
include(fb32x32.cmake)
include(sound.cmake)
include(vga.cmake)


find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
get_target_property(SOURCES_JAM jam_alpha SOURCES)
get_target_property(SOURCES_FB32X32 fb32x32_ws2812 SOURCES)
if (CLANG_TIDY_EXECUTABLE)
   add_custom_target(
      sca
      COMMAND ${CMAKE_SOURCE_DIR}/tools/clang-tidy-wrapper.sh
               CC=${CMAKE_C_COMPILER}
               PICO_SDK_PATH=${PICO_SDK_PATH}
               ${CLANG_TIDY_EXECUTABLE}
               -p=${CMAKE_BINARY_DIR} # Path to compile_commands.json
               #--extra-arg-before=-Wno-error
               ${SOURCES_JAM} ${SOURCES_FB32X32}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Running clang-tidy static analysis..."
      VERBATIM
   )
   message(STATUS "Static analysis target 'sca' added.")
else()
   message(WARNING "clang-tidy not found. Cannot add static analysis target.")
endif()
