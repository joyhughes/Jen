cmake_minimum_required(VERSION 3.16)

project(joylux)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message(STATUS "Build type not specified: Use Debug by default")
endif(NOT CMAKE_BUILD_TYPE)

include_directories(src)
set(CMAKE_CXX_STANDARD 20)

set(COMMON_LIB_SRCS
src/fimage.hpp
src/fimage.cpp
src/frgb.cpp
src/frgb.hpp
src/gamma_lut.cpp
src/gamma_lut.hpp
src/image_loader.hpp
src/image_loader.cpp
src/image.hpp
src/image.cpp
src/joy_rand.hpp
src/linalg.h
src/next_element.hpp
src/scene.hpp
src/scene.cpp
src/ucolor.hpp
src/ucolor.cpp
src/uimage.hpp
src/uimage.cpp
src/vect2.cpp 
src/vect2.hpp 
src/vector_field.hpp
src/vector_field.cpp
)

add_library(common ${COMMON_LIB_SRCS})

set(LUX_MAIN_SRCS src/lux.cpp)
add_executable(lux ${LUX_MAIN_SRCS})

target_link_libraries(lux common)
