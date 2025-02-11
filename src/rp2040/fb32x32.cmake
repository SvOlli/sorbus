
# FB32X32: 32x32 pixel framebuffer similar to 6502asm.com

# WS2812 variant
add_executable(fb32x32ws2812)
pico_generate_pio_header(fb32x32ws2812
   ${CMAKE_CURRENT_LIST_DIR}/fb32x32/ws2812.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
pico_generate_pio_header(fb32x32ws2812
   ${CMAKE_CURRENT_LIST_DIR}/fb32x32/bus.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
target_sources(fb32x32ws2812 PRIVATE
   fb32x32/main.c
   fb32x32/bus.c
   fb32x32/control.c
   fb32x32/hardware_ws2812.c
   fb32x32/translation_matrix_1.c
)
target_link_libraries(fb32x32ws2812 PRIVATE
   pico_stdlib
   pico_multicore
   hardware_pio
)
# just for fun run out of ram
#pico_set_binary_type(fb32x32ws2812 copy_to_ram)
setup_target(fb32x32ws2812 "fb32x32")

# variant for emulation via UART
add_executable(fb32x32usbuart)
# depends on bus.pio
add_dependencies(fb32x32usbuart fb32x32ws2812)
target_sources(fb32x32usbuart PRIVATE
   fb32x32/main.c
   fb32x32/bus.c
   fb32x32/control.c
   fb32x32/hardware_uart.c
)
target_link_libraries(fb32x32usbuart PRIVATE
   pico_stdlib
   pico_multicore
   hardware_pio
)
# just for fun run out of ram
#pico_set_binary_type(fb32x32usbuart copy_to_ram)
setup_target(fb32x32usbuart "fb32x32")
