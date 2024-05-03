cmake_minimum_required(VERSION 3.16)

project(joylux)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message(STATUS "Build type not specified: Use Debug by default")
endif(NOT CMAKE_BUILD_TYPE)

include_directories(src)
set(CMAKE_CXX_STANDARD 20)

set(COMMON_LIB_SRCS
src/any_effect.hpp
src/any_effect.cpp
src/any_function.hpp
src/any_function.cpp
src/any_image.hpp
src/any_rule.hpp
src/any_rule.cpp
src/buffer_pair.hpp
src/buffer_pair.cpp
src/effect.hpp
src/effect.cpp
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
src/joy_concepts.hpp
src/joy_rand.hpp
src/json.hpp
src/life_hacks.hpp
src/life.hpp
src/life.cpp
src/linalg.h
src/mask_mode.hpp
src/next_element.hpp
src/next_element.cpp
src/offset_field.hpp
src/offset_field.cpp
src/scene.hpp
src/scene.cpp
src/scene_io.hpp
src/scene_io.cpp
src/ucolor.hpp
src/ucolor.cpp
src/UI.hpp
src/UI.cpp
src/uimage.hpp
src/uimage.cpp
src/vect2.cpp 
src/vect2.hpp 
src/vector_field.hpp
src/vector_field.cpp
src/warp_field.hpp
src/warp_field.cpp
src/warp.hpp
)

add_library(common ${COMMON_LIB_SRCS})

set(LUX_MAIN_SRCS src/lux.cpp)
add_executable(lux ${LUX_MAIN_SRCS})

set(SPLOOT_MAIN_SRCS src/sploot.cpp)
add_executable(sploot ${SPLOOT_MAIN_SRCS})

set(IMAGE_TEST_MAIN_SRCS src/image_test.cpp)
add_executable(image_test ${IMAGE_TEST_MAIN_SRCS})

set(CIRCLE_MAIN_SRCS src/circle.cpp)
add_executable(circle ${CIRCLE_MAIN_SRCS})

target_link_libraries(lux common)
target_link_libraries(sploot common)
target_link_libraries(image_test common)
target_link_libraries(circle common)

