MAKEFLAGS += --no-builtin-rules

# Source files
CFILES := $(wildcard src/*.cpp)
FILES := $(patsubst src/%.cpp,web_build/%.o,$(CFILES))
EXTERN_BUILD_DIR = $(shell pwd)/external/build

# FFmpeg configuration
FFMPEG_CFLAGS = -I$(EXTERN_BUILD_DIR)/include -DUSE_FFMPEG=1
FFMPEG_LIBS = -L$(EXTERN_BUILD_DIR)/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lx264 -lvpx


all: lux_react/src/lux.js

# Include dependency files
-include $(FILES:.o=.d)

# Main target - linking all object files
# Adding video_recorder.o to the end as specified
lux_react/src/lux.js: web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_lut.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/warp_field.o web_build/UI.o web_build/emscripten_utils.o web_build/video_recorder.o
	em++ web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_lut.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/UI.o web_build/warp_field.o web_build/emscripten_utils.o web_build/video_recorder.o -o lux_react/src/lux.js --embed-file lux_files -s MODULARIZE -s SINGLE_FILE=1 -s ENVIRONMENT=web,worker -s NO_DISABLE_EXCEPTION_CATCHING -s EXPORT_ES6=1 -s ASSERTIONS=1 -s INITIAL_MEMORY=671088640 -s MAXIMUM_MEMORY=2147483648 -s STACK_SIZE=10485760 -s TOTAL_STACK=20971520 -s ALLOW_TABLE_GROWTH=1 -s RESERVED_FUNCTION_POINTERS=200 -s EXPORTED_FUNCTIONS=['_malloc','_free','_main'] -s EXPORTED_RUNTIME_METHODS=['FS','addFunction','removeFunction','UTF8ToString','stringToUTF8','getValue','setValue','writeArrayToMemory','cwrap'] -s MODULARIZE=1 -s SINGLE_FILE=1 -s NO_DISABLE_EXCEPTION_CATCHING=1 -s ASSERTIONS=2 -s WASM_ASYNC_COMPILATION=1 -s SUPPORT_LONGJMP=1 -s EXIT_RUNTIME=0 -s LEGALIZE_JS_FFI=0 -s ALLOW_MEMORY_GROWTH=1  -lembind $(FFMPEG_LIBS)

# Files that need FFmpeg flags
web_build/lux_web.o: src/lux_web.cpp
	em++ -O3 -MMD -MP -std=c++20 $(FFMPEG_CFLAGS) src/lux_web.cpp -c -o web_build/lux_web.o

web_build/video_recorder.o: src/video_recorder.cpp
	em++ -O3 -MMD -MP -std=c++20 $(FFMPEG_CFLAGS) src/video_recorder.cpp -c -o web_build/video_recorder.o

# Individual file compilation rules matching your original Makefile exactly
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

# Generic rule for remaining object files
web_build/%.o: src/%.cpp
	em++ -MMD -MP -std=c++20 $< -c -o $@

# Clean target
clean:
	rm -f web_build/*.o web_build/*.d lux_react/src/lux.js