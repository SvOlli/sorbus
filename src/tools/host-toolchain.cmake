# this needs improvements

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-g -Og -Wall -pedantic -fsanitize=address,undefined")
set(CMAKE_C_FLAGS_RELEASE "-g -Og")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-g -Og -Wall -pedantic -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-g -Og")

set(CMAKE_RANLIB "ranlib")

# this is a workaround to make sure it works on Windows
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
