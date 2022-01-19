cmake_minimum_required(VERSION 3.22)

project(patchwerk)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -D_POSIX_C_SOURCE=199309L")

include_directories(libpatchwerk/include)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_subdirectory(libpatchwerk)
add_subdirectory(main)

enable_testing()
add_subdirectory(tests)
