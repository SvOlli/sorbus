
# FB32X32: 32x32 pixel framebuffer similar to 6502asm.com

#
add_library(fb32x32bus
   fb32x32/fb32x32_bus.c
)
pico_generate_pio_header(fb32x32bus
   ${CMAKE_CURRENT_LIST_DIR}/fb32x32/fb32x32_bus.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(fb32x32bus PUBLIC
   pico_stdlib
   pico_multicore
   hardware_pio
)
target_include_directories(fb32x32bus PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}
)

# WS2812 variant
add_executable(fb32x32_ws2812
   fb32x32/main.c
   fb32x32/control.c
   fb32x32/hardware_ws2812.c
   fb32x32/translation_matrix_1.c
)
pico_generate_pio_header(fb32x32_ws2812
   ${CMAKE_CURRENT_LIST_DIR}/fb32x32/ws2812.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(fb32x32_ws2812 PRIVATE
   fb32x32bus
)
# just for fun run out of ram
#pico_set_binary_type(fb32x32_ws2812 copy_to_ram)
setup_target(fb32x32_ws2812 "fb32x32")

# variant for emulation via UART
add_executable(fb32x32_usbuart
   fb32x32/main.c
   fb32x32/control.c
   fb32x32/hardware_uart.c
)
target_link_libraries(fb32x32_usbuart PRIVATE
   fb32x32bus
)
# just for fun run out of ram
#pico_set_binary_type(fb32x32_usbuart copy_to_ram)
setup_target(fb32x32_usbuart "fb32x32")

# variant for just testing PIO
add_executable(fb32x32_iotest
   fb32x32/iotest.c
)
target_link_libraries(fb32x32_iotest PRIVATE
   fb32x32bus
)
# just for fun run out of ram
#pico_set_binary_type(fb32x32_usbuart copy_to_ram)
setup_target(fb32x32_iotest "fb32x32")
