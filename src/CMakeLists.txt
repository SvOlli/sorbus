cmake_minimum_required(VERSION 3.13)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
   message(FATAL_ERROR "In-source build detected!")
endif()

option(PICO_PLATFORM "Platform to compile for (rp2040,rp2350,rp2350-riscv)" "rp2040")
option(CPM65_PATH "directory of CP/M 65 source code")

# initialize the SDK based on PICO_SDK_PATH and BOARD config
# note: this must happen before project()
set(CMAKE_WARN_DEPRECATED FALSE) # mbedtls will not build otherwise
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/rp2040")
set(PICO_BOARD "rp2040_purple")
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
project(sorbus)

# self-hosted tools to e.g. convert binary to text data
add_subdirectory(tools)

# code run on 65(c)02
add_subdirectory(65c02)

# the rp2040 code for the chipsets
add_subdirectory(rp2040)

