
# JAM: just another machine
# core for implementing a custom Sorbus system

# firmware for JAM core
add_custom_target(jam_firmware ALL
   DEPENDS jam_alpha_picotool jam_rom
)

# building firmware .uf2 file containing all ROMs
add_custom_target(jam_rom ALL
   $<TARGET_FILE:bin2uf2> "jam_rom.uf2"
      0x103FA000
      $<TARGET_FILE:jam_kernel>
      $<TARGET_FILE:jam_tools>
      $<TARGET_FILE:jam_basic>
   DEPENDS jam_kernel jam_tools jam_basic ${BIN2UF2}
)

# create wear leveling image from raw binary image
# image is created in src/65c02
dharawrite(jam_cpmfs.dhara jam_cpmfs.img)

# convert wear leveling image to UF2
bin2uf2(jam_cpmfs 0x10400000 jam_cpmfs.dhara)

# the full monty
uf2join(jam_alpha_picotool
   jam_rom
   jam_cpmfs
   jam_alpha
)

# main program
add_executable(jam_alpha
   common/bus_rp2040_purple.c
   common/cpu_detect.c
   common/disassemble.c
   common/disassemble_historian.c
   common/generic_helper.c
   common/mf_checkheap.c
   common/putcharset.c
   common/xmodem.c
   mcurses/mcurses.c
   mcurses/mc_disass.c
   mcurses/mc_hexedit.c
   mcurses/mc_historian.c
   mcurses/mc_phyio_rp2040.c
   mcurses/mc_extra.c
   mcurses/mc_view.c
   mcurses/mc_xmodem.c
   3rdparty/dhara/error.c
   3rdparty/dhara/journal.c
   3rdparty/dhara/map.c
   jam/main.c
   jam/console.c
   jam/bus.c
   jam/dhara_flash.c
   jam/event_queue.c
   )
set_property(SOURCE common/cpu_detect.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cpudetect.h)

target_link_libraries(jam_alpha
   pico_stdlib
   pico_multicore
   pico_rand
   )
setup_target(jam_alpha "jam")
#pico_set_binary_type(jam_alpha copy_to_ram)
