
project(sorbus_tools)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GD REQUIRED gdlib)

include(host-toolchain.cmake)

# tool to convert binary data to header file for including in source code
add_executable(bin2h
   bin2h.c
   )

# tool to convert binary data to hex files for WozMon
add_executable(bin2hex
   bin2hex.c
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
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/disassemble.c)
set_property(SOURCE historian APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/disassemble_historian.c)

# tool to test curses implementation
add_executable(mcurses
   mcurses.c
   )
set_property(SOURCE mcurses APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/mcurses.c)
set_property(SOURCE mcurses APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/mcurses_hexedit.c)
set_property(SOURCE mcurses APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/rp2040/common/mcurses_sorbus.c)

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
