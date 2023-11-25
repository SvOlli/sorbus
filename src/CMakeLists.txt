cmake_minimum_required(VERSION 3.13)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
   message(FATAL_ERROR "In-source build detected!")
endif()

# initialize the SDK based on PICO_SDK_PATH and BOARD config
# note: this must happen before project()
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/rp2040")
set(PICO_BOARD "rp2040_purple")
include(pico_sdk_import.cmake)
project(sorbus)

# self-hosted tools to e.g. convert binary to text data
add_subdirectory(tools)

# code run on 65(c)02
add_subdirectory(65c02)

# the rp2040 code for the chipsets
add_subdirectory(rp2040)

