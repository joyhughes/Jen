MAKEFLAGS += --no-builtin-rules

CFILES := $(wildcard src/*.cpp)
FILES := $(patsubst src/%.cpp,web_build/%.o,$(CFILES))

all: lux_react/src/lux.js

# Include dependency files
-include $(FILES:.o=.d)

lux_react/src/lux.js: web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_LUT.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/warp_field.o web_build/UI.o nebula_files/random_copy.json
#	em++ web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_LUT.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/UI.o web_build/warp_field.o -o lux_react/src/lux.js --embed-file nebula_files -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE -s SINGLE_FILE=1 -s SAFE_HEAP=1 -s ENVIRONMENT=web -s NO_DISABLE_EXCEPTION_CATCHING -lembind
	em++ web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_LUT.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/UI.o web_build/warp_field.o -o lux_react/src/lux.js --embed-file nebula_files -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE -s SINGLE_FILE=1 -s ENVIRONMENT=web -lembind

web_build/%.o: src/%.cpp
	em++ -MMD -MP -std=c++20 $< -c -o $@