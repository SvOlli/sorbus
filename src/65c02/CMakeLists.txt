
project(sorbus_65c02)

include(65c02-toolchain.cmake)

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
   elseif( "${system}" STREQUAL "native_rom" )
      target_link_options(${target} PUBLIC "-C${CMAKE_CURRENT_SOURCE_DIR}/native_rom.ld")
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

add_executable(native_writeboot1
   native_writeboot1.s
   )
target_type_addr(native_writeboot1 native 0x1000)

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

add_executable(native_rom
   native_rom.s
   native_tim.s
   native_woz.s
   )
target_type(native_rom native_rom)
