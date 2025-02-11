
# Apple 1: core for mimicking the Apple 1 Computer

# payload
bin2h(krusader.h ${CMAKE_HOME_DIRECTORY}/bin/krusader_65C02.bin krusader_e000)
src2h(cpudetect_apple1.h cpudetect_apple1 cpudetect_0280)
src2h(a1hello.h a1hello a1hello_0800)

# main program
add_executable(apple1
   krusader.h
   cpudetect_apple1.h
   a1hello.h
   common/bus_rp2040_purple.c
   apple1/main.c
   )
target_link_libraries(apple1
   pico_stdlib
   pico_multicore
   )
setup_target(apple1 "apple1")
