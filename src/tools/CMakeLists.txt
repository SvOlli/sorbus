
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
bin2h(jam_forth.h  ${CMAKE_BINARY_DIR}/65c02/jam_forth.bin  jam_forth  sorbus_rte_memory.c)
add_library(sorbus_rte STATIC
   mc_phyio_linux.c
   sorbus_rte_memory.c
   sorbus_rte_trace.c
   ${CMAKE_SOURCE_DIR}/rp2040/common/heaptrack.c
   ${CMAKE_SOURCE_DIR}/rp2040/common/putcharset.c
   ${CMAKE_SOURCE_DIR}/rp2040/disassemble/da_base.c
   ${CMAKE_SOURCE_DIR}/rp2040/disassemble/da_cyclecount.c
   ${CMAKE_SOURCE_DIR}/rp2040/disassemble/da_generated.c
   ${CMAKE_SOURCE_DIR}/rp2040/disassemble/da_memory.c
   ${CMAKE_SOURCE_DIR}/rp2040/disassemble/da_trace.c
   ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mcurses.c
   ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_extra.c
   ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_trace.c
   ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_view.c
   )
target_include_directories(sorbus_rte PUBLIC
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_SOURCE_DIR}/rp2040/common
   ${CMAKE_SOURCE_DIR}/rp2040/disassemble
   ${CMAKE_SOURCE_DIR}/rp2040/mcurses
   )

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
   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/dhara/error.c
   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/dhara/journal.c
   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/dhara/map.c
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
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/disassemble.c)
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/fulltrace.c)
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/historian.c)
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_historian.c)

# tool to test part of disassembler
add_executable(fulltrace
   fulltrace.c
)
target_link_libraries(fulltrace PRIVATE sorbus_rte)
set_property(SOURCE fulltrace APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/beancounter.c)
set_property(SOURCE fulltrace APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/disassemble.c)
set_property(SOURCE fulltrace APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/fulltrace.c)
set_property(SOURCE fulltrace APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/historian.c)

# tool to test rework of disassembler
add_executable(newtrace
   newtrace.c
)
target_link_libraries(newtrace PRIVATE sorbus_rte)

# tool to test part of disassembler
add_executable(mc_trace_test
   mc_trace_test.c
   ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_trace.c
)
target_link_libraries(mc_trace_test PRIVATE sorbus_rte)

# tool to test memory disassembler
#add_executable(disass
#   disass.c
#)
#target_link_libraries(disass PRIVATE sorbus_rte)
#target_include_directories(disass PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
#set_property(SOURCE disass.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/disassemble/disassemble.c)
#set_property(SOURCE disass.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_disass.c)
#set_property(SOURCE disass.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/mcurses/mc_view.c)

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
#   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/peiselmon/6502.c
#   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/peiselmon/editscreen.c
#   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/peiselmon/output.c
#   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/peiselmon/parse.c
#   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/peiselmon/processor.c
#   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/peiselmon/target_memory.c
#   ${CMAKE_SOURCE_DIR}/rp2040/3rdparty/peiselmon/window.c
#   peiselmon.c
#   )
#target_link_libraries(peiselmon PRIVATE sorbus_rte)
