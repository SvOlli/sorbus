cmake_minimum_required(VERSION 3.13)


add_library(xmodem-lib INTERFACE)
target_include_directories(xmodem-lib INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(xmodem-lib INTERFACE hardware_flash)
target_sources(xmodem-lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/xmodem.c
    ${CMAKE_CURRENT_LIST_DIR}/xmodem.h
    ${CMAKE_CURRENT_LIST_DIR}/crc16.h
    ${CMAKE_CURRENT_LIST_DIR}/crc16.c
)



