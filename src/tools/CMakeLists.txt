
project(sorbus_tools)

include(host-toolchain.cmake)


add_executable(bin2h
   bin2h.c
   )

add_executable(bin2hex
   bin2hex.c
   )

