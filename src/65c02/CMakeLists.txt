
project(sorbus_65c02)

include(65c02-toolchain.cmake)

function(src2hex hex_file bin_file startaddr)
   add_custom_target(${hex_file}
      COMMAND $<TARGET_FILE:bin2hex> $<TARGET_FILE:${bin_file}> "${startaddr}" "${hex_file}"
      DEPENDS ${bin_file} ${BIN2HEX})
endfunction()

add_executable(cpudetect_mcp
   cpudetect_mcp.s
   )
target_link_options(cpudetect_mcp
   PUBLIC --start-addr 0x0000
   )

add_executable(cpudetect_apple1
   cpudetect_apple1.s
   )
target_link_options(cpudetect_apple1
   PUBLIC --start-addr 0x0280
   )

add_executable(cpustate
   cpustate.s
   )
target_link_options(cpustate
   PUBLIC --start-addr 0x0000
   )

add_executable(wozmon
   wozmon.s
   )
target_link_options(wozmon
   PUBLIC --start-addr 0xFF00
   )

add_executable(payload_mcp
   payload_mcp.s
   )
src2hex(payload_mcp.hex payload_mcp 0x400)

add_executable(a1hello
   a1hello.s
   )
target_compile_options(a1hello
   PUBLIC -t none --cpu 65c02
   )
target_link_options(a1hello
   PUBLIC --start-addr 0x800
   )
src2hex(a1hello.hex a1hello 0x800)

