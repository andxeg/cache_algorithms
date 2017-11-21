cmake_minimum_required (VERSION 2.6)

set (cachealg cachealg)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} 
					../third-party/
					)

set (CMAKE_BUILD_TYPE "Release")
set (CMAKE_CXX_FLAGS "-std=c++11 -O3 -Wall")

set (sources
    main.cpp)

add_executable(${cachealg} ${sources})
