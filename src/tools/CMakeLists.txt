
project(sorbus_tools)

include(host-toolchain.cmake)

add_executable(bin2h
   bin2h.c
   )

add_executable(bin2hex
   bin2hex.c
   )

add_executable(mkftl
   mkftl.c
   ../rp2040/dhara/error.c
   ../rp2040/dhara/journal.c
   ../rp2040/dhara/map.c
   )

