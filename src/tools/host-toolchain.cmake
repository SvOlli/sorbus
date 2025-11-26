# this needs improvements

set(CMAKE_RANLIB "ranlib")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-g -Og -Wall -pedantic")
set(CMAKE_C_FLAGS_RELEASE "-g -Og")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-g -Og -Wall -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-g -Og")

# this is a workaround to make sure it works on Windows
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
