cmake_minimum_required(VERSION 3.13)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
   message(FATAL_ERROR "In-source build detected!")
endif()

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

# Adding dhara-libray. It only has a Makefile :(

# collect up the source files
file(GLOB DHARA_SRC_FILES_MAIN "dhara/dhara/*.c")
file(GLOB DHARA_SRC_FILES_ECC "dhara/ecc/*.c")
list(APPEND DHARA_SRC_FILES ${DHARA_SRC_FILES_MAIN} ${DHARA_SRC_FILES_ECC})
# creating static-lib
add_library(dhara ${DHARA_SRC_FILES})
