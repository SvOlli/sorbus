
project(sorbus_tools)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GD REQUIRED gdlib)

include(host-toolchain.cmake)

function(bin2h h_file bin_file label c_file)
# NOTE: label is used twice here:
# 1) label in output.h file
# 2) name of the dependency label in 65c02 CMakeFiles.txt
   add_custom_command(OUTPUT ${h_file}
      COMMAND $<TARGET_FILE:bin2h> "${bin_file}" "${CMAKE_CURRENT_BINARY_DIR}/${h_file}" "${label}"
      DEPENDS ${bin_file} ${BIN2H} ${label}
   )
   set_property(SOURCE ${c_file} APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${h_file})
endfunction()

bin2h(jam_kernel.h ${CMAKE_BINARY_DIR}/65c02/jam_kernel.bin jam_kernel sorbus_rte_memory.c)
bin2h(jam_tools.h  ${CMAKE_BINARY_DIR}/65c02/jam_tools.bin  jam_tools  sorbus_rte_memory.c)
bin2h(jam_basic.h  ${CMAKE_BINARY_DIR}/65c02/jam_basic.bin  jam_basic  sorbus_rte_memory.c)
add_library(sorbus_rte STATIC
   mc_phyio_linux.c
   sorbus_rte_memory.c
   ../rp2040/common/putcharset.c
   ../rp2040/mcurses/mcurses.c
   ../rp2040/mcurses/mc_extra.c
   ../rp2040/mcurses/mc_view.c
   )
target_include_directories(sorbus_rte PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ../rp2040/common)
target_include_directories(sorbus_rte INTERFACE ../rp2040/mcurses)

# tool to convert binary data to header file for including in source code
add_executable(bin2h
   bin2h.c
   )

# tool to convert binary data to hex files for WozMon
add_executable(bin2hex
   bin2hex.c
   )

# tool to output UTF-8 sequences for any 16-bit glyphs
add_executable(unicode
   unicode.c
   )

# tool to convert to and from wear leveling format
add_executable(dharatool
   dharatool.c
   ../rp2040/3rdparty/dhara/error.c
   ../rp2040/3rdparty/dhara/journal.c
   ../rp2040/3rdparty/dhara/map.c
   )

# tool to convert binary data to UF2 files
add_executable(bin2uf2
   bin2uf2.cpp
   )
target_include_directories(bin2uf2 PRIVATE
   ${PICO_SDK_PATH}/src/common/boot_uf2_headers/include
   )

# tool to concatinate binary data
add_executable(binjoin
   binjoin.c
   )

# tool to test part of disassembler
add_executable(historian
   historian.c
)
target_link_libraries(historian PRIVATE sorbus_rte)
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/disassemble.c)
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/disassemble_fulltrace.c)
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/disassemble_historian.c)

# tool to test memory disassembler
add_executable(disass
   disass.c
)
target_link_libraries(disass PRIVATE sorbus_rte)
target_include_directories(disass PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_property(SOURCE disass.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/disassemble.c)
set_property(SOURCE disass.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_disass.c)
set_property(SOURCE disass.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_view.c)

# tool to test curses implementation
#add_executable(mcurses
#   mcurses.c
#   mcurses_phyio.c
#   ../rp2040/mcurses/mcurses.c
#   ../rp2040/mcurses/mcurses_hexedit.c
#   ../rp2040/mcurses/mcurses_sorbus.c
#   ../rp2040/mcurses/mcurses_view.c
#   )

# tool to convert binary data to TIM paper tape format
add_executable(timcat
   timcat.c
   )

# tool to convert binary data to TIM paper tape format
add_executable(translation_matrix
   translation_matrix.c
   )

# tool to convert binary data to WozMon input
add_executable(wozcat
   wozcat.c
   )

# tool to convert 32x32 pixel image to executable for fb32x32
add_executable(gdleddiagram
   gdleddiagram.c
   gdhelper.c
   )
target_link_libraries(gdleddiagram ${GD_LIBRARIES})
target_include_directories(gdleddiagram PUBLIC ${GD_INCLUDE_DIRS})
target_compile_options(gdleddiagram PUBLIC ${GD_CFLAGS_OTHER})

# tool to convert 32x32 pixel image to executable for fb32x32
add_executable(img2sx4
   img2sx4.c
   gdhelper.c
   )
target_link_libraries(img2sx4 ${GD_LIBRARIES})
target_include_directories(img2sx4 PUBLIC ${GD_INCLUDE_DIRS})
target_compile_options(img2sx4 PUBLIC ${GD_CFLAGS_OTHER})

#add_executable(peiselmon
#   ../rp2040/3rdparty/peiselmon/6502.c
#   ../rp2040/3rdparty/peiselmon/editscreen.c
#   ../rp2040/3rdparty/peiselmon/output.c
#   ../rp2040/3rdparty/peiselmon/parse.c
#   ../rp2040/3rdparty/peiselmon/processor.c
#   ../rp2040/3rdparty/peiselmon/target_memory.c
#   ../rp2040/3rdparty/peiselmon/window.c
#   peiselmon.c
#   )
#target_link_libraries(peiselmon PRIVATE sorbus_rte)
