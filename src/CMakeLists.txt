cmake_minimum_required(VERSION 3.13)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
   message(FATAL_ERROR "In-source build detected!")
endif()

# Asuming , we have a purple board with 16 MB Flash
add_definitions ( -DPICO_FLASH_SIZE_BYTES=16777216 )

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
project(sorbus)

# self-hosted tools to e.g. convert binary to text data
add_subdirectory(tools)

# code run on 65(c)02
add_subdirectory(65c02)

# the rp2040 code for the chipsets
add_subdirectory(rp2040)

# add library for FlashFileSystem
add_subdirectory(littlefs-lib)

# add library for xmodem protocol
add_subdirectory(xmodem)
