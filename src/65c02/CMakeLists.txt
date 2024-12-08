
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
   add_custom_target(${target}_cc65_sdk ALL
      COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/../../cc65-sdk" "SORBUS_65C02_DIR=${CMAKE_CURRENT_SOURCE_DIR}" "EXPORT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm/10"
   )
   file(GLOB CPM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm/*/*")
   add_custom_target(${target} ALL
      COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../tools/mkcpmfsimage.sh"
         "${target}"
         $<TARGET_FILE:native_cpm_bootblock>:$<TARGET_FILE:native_nmos6502_bootblock>
         "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm"
         ${extra_files}
      DEPENDS ${native_cpm_bootblock} ${native_nmos6502_bootblock} ${native_browser} ${CPM_FILES} ${CPMFS_EXTRA_TARGETS}
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

add_executable(cpudetect
   cpudetect.s
   )
target_type_addr(cpudetect mcp 0x0000)

add_executable(cpudetect_apple1
   apple1/cpudetect.s
   )
target_type_addr(cpudetect_apple1 apple1 0x0280)

#add_executable(cpustate
#   cpustate.s
#   )
#target_type_addr(cpustate mcp 0x0000)

add_executable(wozmon
   apple1/wozmon.s
   )
target_type_addr(wozmon apple1 0xFF00)

add_executable(payload_mcp
   payload_mcp.s
   )
target_type_addr(payload_mcp mcp 0x0400)

add_executable(a1hello
   apple1/a1hello.s
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
   native_rom/lineinput.s
   native_rom/bios.s

   mon/main.s
   mon/asciihex.s
   mon/assembler.s
   mon/disassembler.s
   mon/hexdump.s
   mon/interndrive.s
   mon/registers.s
   mon/storage.s
   mon/table65sc02.s
   )
target_compile_options(native_kernel
   PUBLIC --cpu 65sc02
)
target_type(native_kernel native_kernel)

add_executable(native_basic
   native_rom/osi_basic.s
   native_rom/bios.s
   native_rom/bios_stub.s
   )
target_compile_options(native_basic
   PUBLIC --cpu 65sc02
)
target_type(native_basic native_kernel)

add_executable(native_tools
   native_rom/browser.s
   native_rom/gensine.s
   native_rom/bios.s
   native_rom/bios_stub.s
   )
target_compile_options(native_tools
   PUBLIC --cpu 65sc02
)
target_type(native_tools native_kernel)

add_executable(native_nmos6502_bootblock
   nmos6502/kernel.s
   native_rom/lineinput.s
   native_rom/vt100.s
   nmos6502/bios.s

   mon/main.s
   mon/asciihex.s
   mon/assembler.s
   mon/disassembler.s
   mon/hexdump.s
   mon/interndrive.s
   mon/papertape.s
   mon/registers.s
   mon/table6502.s
   )
target_compile_options(native_nmos6502_bootblock
   PUBLIC --cpu 6502
)
target_type(native_nmos6502_bootblock native_kernel)

add_executable(native_cpm_bootblock
   linkcpm.s
   ../bin/cpm/CPM
   ../bin/cpm/BDOS
   native_rom/bios.s
   native_rom/bios_stub.s
   )
# This doesn't work, as the glorious CMake de-duplicates the
# "--large-alignment" option, which needs to be passed twice.
# Since it only disables warnings, so be it.
#target_link_options(native_cpm_bootblock PUBLIC
#   --asm-args --large-alignment
#   --ld-args --large-alignment
#   )
target_type(native_cpm_bootblock native_kernel)

# SX4 executables
add_executable(1sthello
   sx4/a1hello.s
   )
target_type(1sthello sx4)

add_executable(asmprint
   sx4/asmprint.s
   )
target_type(asmprint sx4)

add_executable(bcdcheck
   sx4/bcdcheck.s
   )
target_type(bcdcheck sx4)

add_executable(brk-test
   sx4/brk-test.s
   )
target_type(brk-test sx4)

add_executable(cpmfsck
   sx4/cpmfsck.s
   )
target_type(cpmfsck sx4)

add_executable(int-test
   sx4/int-test.s
   )
target_type(int-test sx4)

add_executable(inst-ass
   sx4/inst-ass.s
   )
target_type(inst-ass sx4)

add_executable(keycode
   sx4/keycode.s
   )
target_type(keycode sx4)

add_executable(memcheck
   sx4/memcheck.s
   )
target_type(memcheck sx4)

add_executable(sx4speed
   sx4/speed.s
   )
target_type(sx4speed sx4)

add_executable(opcodes
   sx4/opcodes.s
   )
target_type(opcodes sx4)

add_executable(wozmon2c
   sx4/wozmon2c.s
   )
target_type(wozmon2c sx4)

add_executable(wrtboot
   sx4/wrtboot.s
   )
target_type(wrtboot sx4)

gencpmfs(native_cpmfs.img)
