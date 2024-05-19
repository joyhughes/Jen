MAKEFLAGS += --no-builtin-rules

CFILES := $(wildcard src/*.cpp)
FILES := $(patsubst src/%.cpp,web_build/%.o,$(CFILES))

all: lux_react/src/lux.js

# Include dependency files
-include $(FILES:.o=.d)

lux_react/src/lux.js: web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_lut.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/warp_field.o web_build/UI.o nebula_files/test_widgets.json
#	em++ web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_lut.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/UI.o web_build/warp_field.o -o lux_react/src/lux.js --embed-file nebula_files -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE -s SINGLE_FILE=1 -s SAFE_HEAP=1 -s ENVIRONMENT=web -s NO_DISABLE_EXCEPTION_CATCHING -lembind
	em++ web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_lut.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/UI.o web_build/warp_field.o -o lux_react/src/lux.js --embed-file nebula_files -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE -s SINGLE_FILE=1 -s ENVIRONMENT=web -s NO_DISABLE_EXCEPTION_CATCHING -lembind

web_build/effect.o: src/effect.cpp
	em++ -O3 -MMD -MP -std=c++20 src/effect.cpp -c -o web_build/effect.o

web_build/fimage.o: src/fimage.cpp
	em++ -O3 -MMD -MP -std=c++20 src/fimage.cpp -c -o web_build/fimage.o

web_build/frgb.o: src/frgb.cpp
	em++ -O3 -MMD -MP -std=c++20 src/frgb.cpp -c -o web_build/frgb.o

web_build/gamma_lut.o: src/gamma_lut.cpp
	em++ -O3 -MMD -MP -std=c++20 src/gamma_lut.cpp -c -o web_build/gamma_lut.o
	
web_build/image.o: src/image.cpp
	em++ -O3 -MMD -MP -std=c++20 src/image.cpp -c -o web_build/image.o

web_build/life.o: src/life.cpp
	em++ -O3 -MMD -MP -std=c++20 src/life.cpp -c -o web_build/life.o

web_build/next_element.o: src/next_element.cpp
	em++ -O3 -MMD -MP -std=c++20 src/next_element.cpp -c -o web_build/next_element.o
	
web_build/offset_field.o: src/offset_field.cpp
	em++ -O3 -MMD -MP -std=c++20 src/offset_field.cpp -c -o web_build/offset_field.o

web_build/uimage.o: src/uimage.cpp
	em++ -O3 -MMD -MP -std=c++20 src/uimage.cpp -c -o web_build/uimage.o

web_build/ucolor.o: src/ucolor.cpp
	em++ -O3 -MMD -MP -std=c++20 src/ucolor.cpp -c -o web_build/ucolor.o

web_build/vect2.o: src/vect2.cpp
	em++ -O3 -MMD -MP -std=c++20 src/vect2.cpp -c -o web_build/vect2.o

web_build/vector_field.o: src/vector_field.cpp
	em++ -O3 -MMD -MP -std=c++20 src/vector_field.cpp -c -o web_build/vector_field.o

web_build/warp_field.o: src/warp_field.cpp
	em++ -O3 -MMD -MP -std=c++20 src/warp_field.cpp -c -o web_build/warp_field.o

web_build/%.o: src/%.cpp
	em++ -MMD -MP -std=c++20 $< -c -o $@
