
project(sorbus_65c02)

include(65c02-toolchain.cmake)

if( CPM65_PATH )
   message(STATUS "CPM65 detected at '${CPM65_PATH}', adding build command")
   add_custom_target(cpm ALL
      COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cpm/build.sh" "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm/0" "${CPM65_PATH}"
      )
endif()

function(runmake target in_dir)
   message(STATUS "running Makefile in ${in_dir}")
   file(GLOB SRC_FILES "${in_dir}/*")
   add_custom_target(${target} ALL
      COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/${in_dir}" "OUTDIR=${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin"
      DEPENDS ${SRC_FILES} native_kernel)
endfunction()

function(gencpmfs target)
   set(extra_files)
   foreach(file ${CPMFS_EXTRA_TARGETS})
      set(extra_files ${extra_files} $<TARGET_FILE:${file}>)
   endforeach()
   file(GLOB CPM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm/*/*")
   add_custom_target(${target} ALL
      COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../tools/mkcpmfsimage.sh"
         "${target}"
         $<TARGET_FILE:native_cpm_bootblock>
         "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm"
         ${extra_files}
      DEPENDS ${native_cpm_bootblock} ${native_browser} ${CPM_FILES} ${CPMFS_EXTRA_TARGETS}
   )
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
   elseif( "${system}" STREQUAL "sx4" )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/native_sx4.ld")
      set(CPMFS_EXTRA_TARGETS ${target} ${CPMFS_EXTRA_TARGETS} PARENT_SCOPE)
      set_target_properties(${target} PROPERTIES PREFIX "10.")
      set_target_properties(${target} PROPERTIES SUFFIX ".sx4")
   else()
      message(FATAL_ERROR "unknown target_type: ${system}")
   endif()
endfunction()

function(target_type_sx4 target)
   target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/native_sx4.ld")
   set(SX4_FILES ${SX4_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin")
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
   native_rom/kernel.s
   native_rom/cpmfs.s
   native_rom/tim.s
   native_rom/woz.s
   native_rom/vt100.s
   native_rom/functions.s
   native_rom/bios.s
   )
target_type(native_kernel native_kernel)

add_executable(native_basic
   native_rom/osi_basic.s
   native_rom/bios.s
   native_rom/bios_stub.s
   )
target_type(native_basic native_kernel)

add_executable(native_tools
   native_rom/browser.s
   native_rom/bios.s
   native_rom/bios_stub.s
   )
target_type(native_tools native_kernel)

add_executable(native_cpm_bootblock
   linkcpm.s
   ../bin/cpm/CPM
   ../bin/cpm/BDOS
   )
# This doesn't work, as the glorious CMake de-duplicates the
# "--large-alignment" option, which needs to be passed twice.
# Since it only disables warnings, so be it.
#target_link_options(native_cpm_bootblock PUBLIC
#   --asm-args --large-alignment
#   --ld-args --large-alignment
#   )
target_type(native_cpm_bootblock native_kernel)

add_executable(native_writeboot
   native_writeboot.s
   )
target_type_addr(native_writeboot native 0xC000)

add_executable(memcheck
   memcheck.s
   )
target_type(memcheck sx4)

add_executable(1sthello
   native_hello.s
   )
target_type(1sthello sx4)

add_executable(sx4speed
   native_sx4speed.s
   )
target_type(sx4speed sx4)

gencpmfs(native_cpmfs.img)
