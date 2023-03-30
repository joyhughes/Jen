MAKEFLAGS += --no-builtin-rules

CFILES := $(wildcard src/*.cpp)
FILES := $(patsubst src/%.cpp,web_build/%.o,$(CFILES))

all: web_build/pixel_sort.js web_build/lux.js

#web_build/pixel_sort.js: web_build/life_web.o web_build/life.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/gamma_LUT.o web_build/image_loader.o
#	em++ -O2 web_build/life.o web_build/life_web.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/gamma_LUT.o web_build/image_loader.o -o web_build/pixel_sort.js --preload-file samples/crab_nebula.jpg  -lembind

web_build/lux.js: web_build/lux_web.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_LUT.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/warp_field.o
	em++ -O2 web_build/lux_web.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o  web_build/offset_field.o web_build/gamma_LUT.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o  web_build/warp_field.o -o web_build/lux.js --preload-file curly_files -s ALLOW_MEMORY_GROWTH=1 -lembind

web_build/any_effect.o: src/any_effect.cpp
	em++ -O2 src/any_effect.cpp -c -o web_build/any_effect.o  -std=c++20

web_build/any_function.o: src/any_function.cpp
	em++ -O2 src/any_function.cpp -c -o web_build/any_function.o  -std=c++20

web_build/any_rule.o: src/any_rule.cpp
	em++ -O2 src/any_rule.cpp -c -o web_build/any_rule.o  -std=c++20

web_build/effect.o : src/effect.cpp
	em++ -O2 src/effect.cpp -c -o web_build/effect.o  -std=c++20

web_build/fimage.o: src/fimage.cpp src/fimage.hpp src/image_loader.hpp src/image.hpp src/ucolor.hpp src/frgb.hpp src/gamma_LUT.hpp src/linalg.h
	em++ -O2 src/fimage.cpp -c -o web_build/fimage.o  -std=c++20

web_build/frgb.o: src/frgb.cpp src/frgb.hpp src/linalg.h
	em++ -O2 src/frgb.cpp -c -o web_build/frgb.o  -std=c++20

web_build/gamma_LUT.o: src/gamma_LUT.cpp
	em++ -O2 src/gamma_LUT.cpp -c -o web_build/gamma_LUT.o  -std=c++20

web_build/image.o: src/image.cpp
	em++ -O2 src/image.cpp -c -o web_build/image.o  -std=c++20

web_build/image_loader.o: src/image_loader.cpp
	em++ -O2 src/image_loader.cpp -c -o web_build/image_loader.o  -std=c++20

web_build/life.o: src/life.cpp
	em++ -O2 src/life.cpp -c -o web_build/life.o  -std=c++20

web_build/life_web.o: src/life_web.cpp
	em++ -O2 src/life_web.cpp -c -o web_build/life_web.o  -std=c++20

web_build/lux_web.o: src/lux_web.cpp
	em++ -O2 src/lux_web.cpp -c -o web_build/lux_web.o  -std=c++20

web_build/next_element.o: src/next_element.cpp
	em++ -O2 src/next_element.cpp -c -o web_build/next_element.o  -std=c++20

web_build/offset_field.o: src/offset_field.cpp
	em++ -O2 src/offset_field.cpp -c -o web_build/offset_field.o  -std=c++20

web_build/scene.o: src/scene.cpp
	em++ -O2 src/scene.cpp -c -o web_build/scene.o  -std=c++20

web_build/scene_io.o: src/scene_io.cpp
	em++ -O2 src/scene_io.cpp -c -o web_build/scene_io.o  -std=c++20

web_build/ucolor.o: src/ucolor.cpp
	em++ -O2 src/ucolor.cpp -c -o web_build/ucolor.o  -std=c++20

web_build/uimage.o: src/uimage.cpp
	em++ -O2 src/uimage.cpp -c -o web_build/uimage.o  -std=c++20

web_build/vector_field.o: src/vector_field.cpp
	em++ -O2 src/vector_field.cpp -c -o web_build/vector_field.o  -std=c++20

web_build/vect2.o: src/vect2.cpp
	em++ -O2 src/vect2.cpp -c -o web_build/vect2.o  -std=c++20

web_build/warp_field.o: src/warp_field.cpp
	em++ -O2 src/warp_field.cpp -c -o web_build/warp_field.o  -std=c++20

# web_build/%.o : src/%.cpp
#	em++ -c $< -o $@
