cmake_minimum_required (VERSION 2.6)

set (opt opt)

set (CMAKE_BUILD_TYPE "Release")
set (CMAKE_CXX_FLAGS "-std=c++11 -O3 -Wall")

set (sources
    main.cpp)

add_executable(${opt} ${sources})
