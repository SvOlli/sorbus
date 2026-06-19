
# Monitor Command Prompt: core for watching the 65C02 work

# payload
src2h(payload_mcp.h payload_mcp payload_mcp)

# main program
add_executable(mcp
   cpudetect.h
   payload_mcp.h
   common/bus_rp2040_purple.c
   common/cpu_detect.c
   common/generic_helper.c
   common/getaline.c
   common/heaptrack.c
   disassemble/da_base.c
   disassemble/da_cyclecount.c
   disassemble/da_generated.c
   disassemble/da_trace.c
   mcp/main.c
   )
target_link_libraries(mcp
   pico_stdlib
   pico_multicore
   hardware_flash
   )
setup_target(mcp "mcp")
