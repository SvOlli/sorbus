
project(sorbus_65c02)

include(65c02-toolchain.cmake)

if( CPM65_PATH )
   message(STATUS "CP/M-65 detected at '${CPM65_PATH}', adding build command")
   add_custom_target(cpm ALL
      COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cpm/build.sh" "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm/0" "${CPM65_PATH}"
      )
endif()

function(runmake target in_dir)
   message(STATUS "running Makefile in ${in_dir}")
   file(GLOB SRC_FILES "${in_dir}/*")
   add_custom_target(${target} ALL
      COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/${in_dir}" "OUTDIR=${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin"
      DEPENDS ${SRC_FILES} jam_kernel)
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
         $<TARGET_FILE:jam_cpm_bootblock>:$<TARGET_FILE:jam_nmos6502_bootblock>
         "${CMAKE_CURRENT_SOURCE_DIR}/../bin/cpm"
         ${extra_files}
      DEPENDS ${jam_cpm_bootblock} ${jam_nmos6502_bootblock} ${jam_browser} ${CPM_FILES} ${CPMFS_EXTRA_TARGETS}
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
   elseif( "${system}" STREQUAL "jam_kernel" )
      target_include_directories(${target} PRIVATE
         "${CMAKE_CURRENT_SOURCE_DIR}/jam"
      )
      target_compile_options(${target} PRIVATE
         # this structure prevents de-duplication in case more than one include is needed
         "SHELL:--asm-include-dir ${CMAKE_CURRENT_SOURCE_DIR}/jam"
      )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/jam_kernel.ld")
   elseif( "${system}" STREQUAL "sx4" )
      target_include_directories(${target} PRIVATE
         "${CMAKE_CURRENT_SOURCE_DIR}/jam"
      )
      target_compile_options(${target} PRIVATE
         # this structure prevents de-duplication in case more than one include is needed
         "SHELL:--asm-include-dir ${CMAKE_CURRENT_SOURCE_DIR}/jam"
      )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/jam_sx4.ld")
      set(CPMFS_EXTRA_TARGETS ${target} ${CPMFS_EXTRA_TARGETS} PARENT_SCOPE)
      if( "${ARGV2}" STREQUAL "" )
         set_target_properties(${target} PROPERTIES PREFIX "10.")
      else()
         set_target_properties(${target} PROPERTIES PREFIX "${ARGV2}")
      endif()
      set_target_properties(${target} PROPERTIES SUFFIX ".sx4")
   else()
      message(FATAL_ERROR "unknown target_type: ${system}")
   endif()
endfunction()

macro(quick_sx4 target)
   add_executable(${target}
      jam/sx4/${target}.s
      )
   target_type(${target} sx4)
endmacro()

macro(quick_6502asm target)
   add_executable(${target}
      jam/6502asm/${target}.s
      )
   target_type(${target} sx4 "9.")
endmacro()

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

###########
# Apple I #
###########

add_executable(cpudetect_apple1
   apple1/cpudetect.s
   )
target_type_addr(cpudetect_apple1 apple1 0x0280)

add_executable(wozmon
   apple1/wozmon.s
   )
target_type_addr(wozmon apple1 0xFF00)

add_executable(a1hello
   apple1/a1hello.s
   )
target_compile_options(a1hello
   PUBLIC -t none --cpu 65c02
   )
target_type_addr(a1hello apple1 0x0800)
src2hex(a1hello.hex a1hello 0x0800)

#######
# MCP #
#######

add_executable(payload_mcp
   mcp/payload.s
   )
target_type_addr(payload_mcp mcp 0x0400)

##############
# JAM kernel #
##############

add_executable(jam_kernel
   jam/rom/kernel.s
   jam/rom/cpmfs.s
   jam/rom/tim.s
   jam/rom/woz.s
   jam/rom/vt100.s
   jam/rom/lineinput.s

   jam/mon/main.s
   jam/mon/asciihex.s
   jam/mon/assembler.s
   jam/mon/disassembler.s
   jam/mon/hexdump.s
   jam/mon/interndrive.s
   jam/mon/memory.s
   jam/mon/registers.s
   jam/mon/storage.s
   jam/mon/table65sc02.s

   jam/rom/bios.s
   )
target_compile_options(jam_kernel
   PUBLIC --cpu 65sc02
)
target_type(jam_kernel jam_kernel)

add_executable(jam_tools
   jam/rom/tools.s
   jam/rom/browser.s
   jam/rom/gensine.s
   jam/rom/bios.s
   jam/rom/bios_stub.s
   )
target_compile_options(jam_tools
   PUBLIC --cpu 65sc02
)
target_type(jam_tools jam_kernel)

add_executable(jam_basic
   jam/rom/osi_basic.s
   jam/rom/bios.s
   jam/rom/bios_stub.s
   )
target_compile_options(jam_basic
   PUBLIC --cpu 65sc02
)
target_type(jam_basic jam_kernel)

##################
# JAM bootblocks #
##################

add_executable(jam_cpm_bootblock
   cpm/linkcpm.s
   ../bin/cpm/CPM
   ../bin/cpm/BDOS
   jam/rom/bios.s
   jam/rom/bios_stub.s
   )
target_link_options(jam_cpm_bootblock PUBLIC
    # this structure prevents de-duplication
    "SHELL:--asm-args --large-alignment"
    "SHELL:--ld-args --large-alignment"
   )
target_type(jam_cpm_bootblock jam_kernel)

add_executable(jam_nmos6502_bootblock
   jam/nmos6502/kernel.s
   jam/rom/lineinput.s
   jam/rom/vt100.s

   jam/mon/main.s
   jam/mon/asciihex.s
   jam/mon/assembler.s
   jam/mon/disassembler.s
   jam/mon/hexdump.s
   jam/mon/interndrive.s
   jam/mon/papertape.s
   jam/mon/memory.s
   jam/mon/registers.s
   jam/mon/table6502.s

   jam/nmos6502/bios.s
   )
target_compile_options(jam_nmos6502_bootblock
   PUBLIC --cpu 6502
)
target_type(jam_nmos6502_bootblock jam_kernel)

#######################
# JAM SX4 executables #
#######################

add_executable(1sthello
   jam/sx4/a1hello.s
   )
target_type(1sthello sx4)

add_executable(sx4speed
   jam/sx4/speed.s
   )
target_type(sx4speed sx4)

quick_sx4(asmprint)
quick_sx4(bcdcheck)
quick_sx4(brk-test)
quick_sx4(clock)
quick_sx4(cpmfsck)
quick_sx4(int-test)
quick_sx4(inst-ass)
quick_sx4(keycode)
quick_sx4(memcheck)
quick_sx4(opcodes)
quick_sx4(wozmon2c)
quick_sx4(wrtboot)

quick_6502asm(amiga)
quick_6502asm(depth)
quick_6502asm(intro1)
quick_6502asm(palette)
quick_6502asm(random)
quick_6502asm(santa)

add_executable(8bit_dp
   jam/sx4/8bit_dance_party.s
)
target_type(8bit_dp sx4 "1.")

add_executable(animatrn
   jam/sx4/animatron.s
)
target_type(animatrn sx4 "1.")


gencpmfs(jam_cpmfs.img)

