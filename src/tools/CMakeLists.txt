
project(sorbus_tools)

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

# tool to convert test disassembler
add_executable(historian
   historian.c
   )

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
