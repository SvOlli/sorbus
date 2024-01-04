
project(sorbus_65c02)

include(65c02-toolchain.cmake)

if( CPM65_PATH )
   message(STATUS "CPM65 detected at '${CPM65_PATH}', adding build command")
   add_custom_target(cpm ALL
      COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cpm/build.sh" "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm/0" "${CPM65_PATH}"
      )
endif()

function(runmake out_file in_dir)
   message(STATUS "running Makefile in ${in_dir}")
   file(GLOB SRC_FILES "${in_dir}/*")
   add_custom_target(${out_file}
      COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/${in_dir}" "OUTDIR=${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/${out_file}"
      DEPENDS ${SRC_FILES} native_kernel)
endfunction()

function(src2hex hex_file bin_file startaddr)
   add_custom_target(${hex_file}
      COMMAND $<TARGET_FILE:bin2hex> $<TARGET_FILE:${bin_file}> "${startaddr}" "${hex_file}"
      DEPENDS ${bin_file} ${BIN2HEX})
endfunction()

function(target_type target system)
   if( "${system}" STREQUAL "apple1" )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/rawram.ld")
   elseif( "${system}" STREQUAL "mcp" )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/rawram.ld")
   elseif( "${system}" STREQUAL "native_kernel" )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/native_kernel.ld")
   elseif( "${system}" STREQUAL "native" )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/rawram.ld")
   else()
      message(FATAL_ERROR "unknown target_type: ${system}")
   endif()
endfunction()

function(target_type_addr target system startaddr)
   target_type(${target} ${system})
   if( startaddr )
      target_link_options(${target} PUBLIC --start-addr ${startaddr})
   endif()
endfunction()

add_executable(cpudetect_mcp
   cpudetect_mcp.s
   )
target_type_addr(cpudetect_mcp mcp 0x0000)

add_executable(cpudetect_apple1
   cpudetect_apple1.s
   )
target_type_addr(cpudetect_apple1 apple1 0x0280)

add_executable(cpustate
   cpustate.s
   )
target_type_addr(cpustate mcp 0x0000)

add_executable(wozmon
   wozmon.s
   )
target_type_addr(wozmon apple1 0xFF00)

add_executable(payload_mcp
   payload_mcp.s
   )
target_type_addr(payload_mcp mcp 0x0400)

add_executable(a1hello
   a1hello.s
   )
target_compile_options(a1hello
   PUBLIC -t none --cpu 65c02
   )
target_type_addr(a1hello apple1 0x0800)
src2hex(a1hello.hex a1hello 0x0800)

add_executable(native_kernel
   native_kernel.s
   native_cpmfs.s
   native_tim.s
   native_woz.s
   native_bios.s
   )
target_type(native_kernel native_kernel)

add_executable(native_cpm_bootblock
   linkcpm.s
   ../bin/cpm/sorbus.bin
   ../bin/cpm/bdos.sys
   )
target_type(native_cpm_bootblock native_kernel)

add_executable(native_basic
   native_osi_basic.s
   native_bios.s
   native_bios_stub.s
   )
target_type(native_basic native_kernel)

# This doesn't work, as the glorious CMake de-duplicates the
# "--large-alignment" option, which needs to be passed twice.
# Since it only disables warnings, so be it.
#target_link_options(native_cpm_bootblock PUBLIC
#   --asm-args --large-alignment
#   --ld-args --large-alignment
#   )

add_executable(native_writeboot
   native_writeboot.s
   )
target_type_addr(native_writeboot native 0x1000)

add_executable(native_memcheck
   memcheck.s
   )
target_type_addr(native_memcheck native 0x0100)

