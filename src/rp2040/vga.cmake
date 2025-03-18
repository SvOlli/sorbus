
add_library(vga_bus
   common/vga_bus.c
)
pico_generate_pio_header(vga_bus
   ${CMAKE_CURRENT_LIST_DIR}/common/vga_bus.pio
   OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(vga_bus PUBLIC
   pico_stdlib
   pico_multicore
   hardware_pio
)
target_include_directories(vga_bus PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${PICO_SDK_PATH}/lib/tinyusb/src/common
)
# TODO: figure out a way to remove "${PICO_SDK_PATH}/lib/tinyusb/src/common"

add_executable(vga_iotest
   vga-iotest/main.c
   common/vga_bus.c
   common/vga_gpio_config.c
)
target_link_libraries(vga_iotest PRIVATE
   vga_bus
)
# vga_bus.pio is required
add_dependencies(vga_iotest vga-term)
setup_target(vga_iotest "vga-iotest")


add_executable(vga-term
   vga-term/main.c
   vga-term/picoterm_core.c
   vga-term/picoterm_conio_config.c
   vga-term/picoterm_conio.c
   vga-term/fonts/mono8_cp437.c
   vga-term/fonts/mono8_nupetscii.c
   vga-term/fonts/olivetti_thin_cp437.c
   vga-term/fonts/olivetti_thin_nupetscii.c
   vga-term/picoterm_logo.c
   vga-term/picoterm_screen.c
   vga-term/picoterm_config.c
   vga-term/picoterm_stddef.c
   vga-term/picoterm_stdio.c
   vga-term/picoterm_cursor.c
   vga-term/picoterm_dec.c
   vga-term/keybd.c
   common/vga_gpio_config.c
   common/uart_tx.c
)

pico_generate_pio_header(vga-term ${CMAKE_CURRENT_SOURCE_DIR}/common/uart_tx.pio)
target_compile_definitions(vga-term PRIVATE
   PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=4
   PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA=true
   COLUMNS=80
   ROWS=34
   VISIBLEROWS=30
   LOCALISE_US=true
   PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
   CMAKE_PROJECT_VERSION="1.6.0.32"
)
target_include_directories(vga-term PRIVATE ${CMAKE_CURRENT_LIST_DIR}/vga-term/common )
target_link_libraries(vga-term 
   pico_scanvideo_dpi
   hardware_gpio
   hardware_i2c
   hardware_adc
   hardware_uart
   hardware_irq
   hardware_flash
   tinyusb_device
   tinyusb_board
   tinyusb_host
   vga_bus
)
setup_target(vga-term "vga-term")
pico_enable_stdio_usb(vga-term 0)  # Redirect printf to USB-Serial. Conflict with TinyUSB!
pico_enable_stdio_uart(vga-term 0) # Redirect printf to uart0. But no room to place UART0 on GPIO!
