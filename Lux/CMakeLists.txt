cmake_minimum_required(VERSION 3.16)

project(joylux)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message(STATUS "Build type not specified: Use Debug by default")
endif(NOT CMAKE_BUILD_TYPE)

include_directories(src)
set(CMAKE_CXX_STANDARD 20)

set(COMMON_LIB_SRCS
src/frgb.hpp
src/gamma_lut.cpp
src/gamma_lut.hpp
)

add_library(common ${COMMON_LIB_SRCS})

set(FRGB_MAIN_SRCS src/frgb.cpp)
add_executable(frgb ${FRGB_MAIN_SRCS})

target_link_libraries(frgb common)
