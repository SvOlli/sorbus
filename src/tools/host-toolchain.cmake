# this needs improvements

set(CMAKE_C_COMPILER "cc")
set(CMAKE_C_FLAGS "-Wall -pedantic")

set(CMAKE_CXX_COMPILER "c++")
set(CMAKE_CXX_FLAGS "-Wall -pedantic")

# this is a workaround to make sure it works on Windows
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

