CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT(cachealg)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} 
					algorithms/
					extra/
					include/
					../third-party/
					)

SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_CXX_FLAGS "-std=c++11 -O3 -Wall")


SET( COMPONENTS
	extra
)

FOREACH(COMPONENT ${COMPONENTS})
   ADD_SUBDIRECTORY(${COMPONENT})
ENDFOREACH(COMPONENT)

ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
TARGET_LINK_LIBRARIES (${PROJECT_NAME} ${COMPONENTS})
