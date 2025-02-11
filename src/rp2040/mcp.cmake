
# Monitor Command Prompt: core for watching the 65C02 work

# payload
src2h(payload_mcp.h payload_mcp payload_mcp)

# main program
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
