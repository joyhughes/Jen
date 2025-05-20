MAKEFLAGS += --no-builtin-rules

# Directories
ROOT_DIR := $(shell pwd)
EXTERN_BUILD_DIR := $(ROOT_DIR)/external/build
VPX_DIR := $(ROOT_DIR)/external/vpx
FFMPEG_DIR := $(ROOT_DIR)/external/FFmpeg
SRC_DIR := $(ROOT_DIR)/src
WEB_BUILD_DIR := $(ROOT_DIR)/web_build

# Source files
CFILES := $(wildcard $(SRC_DIR)/*.cpp)
FILES := $(patsubst $(SRC_DIR)/%.cpp,$(WEB_BUILD_DIR)/%.o,$(CFILES))

# FFmpeg configuration
FFMPEG_CFLAGS = -I$(EXTERN_BUILD_DIR)/include -DUSE_FFMPEG=1
FFMPEG_LIBS = -L$(EXTERN_BUILD_DIR)/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lvpx

# Check if libraries are already built
CODEC_BUILT := $(shell test -f "$(EXTERN_BUILD_DIR)/lib/libavcodec.a" && test -f "$(EXTERN_BUILD_DIR)/lib/libvpx.a" && echo "yes")

# Main targets
.PHONY: all clean codec-setup check-dirs

all: check-dirs codec-setup lux_react/src/lux.js

# Create necessary directories
check-dirs:
	@mkdir -p $(WEB_BUILD_DIR)
	@mkdir -p $(EXTERN_BUILD_DIR)
	@mkdir -p lux_react/src

# Include dependency files
-include $(FILES:.o=.d)

# Build codecs if not already built
codec-setup:
ifeq ($(CODEC_BUILT),)
	@echo "Building external dependencies..."
	@bash -c 'set -e; \
	ROOT_DIR="$(ROOT_DIR)"; \
	VPX_DIR="$(VPX_DIR)"; \
	FFMPEG_DIR="$(FFMPEG_DIR)"; \
	INSTALL_DIR="$(EXTERN_BUILD_DIR)"; \
	echo "Building VP8 (libvpx) with Emscripten..."; \
	if [ ! -d "$${VPX_DIR}" ]; then \
		mkdir -p $$(dirname $${VPX_DIR}); \
		git clone https://chromium.googlesource.com/webm/libvpx $${VPX_DIR}; \
	fi; \
	cd $${VPX_DIR}; \
	emconfigure ./configure \
		--prefix="$${INSTALL_DIR}" \
		--target=generic-gnu \
		--disable-examples \
		--disable-docs \
		--disable-tools \
		--disable-unit-tests \
		--disable-vp9 \
		--enable-vp8 \
		--enable-static \
		--disable-shared \
		--extra-cflags="-O3 -DEMSCRIPTEN"; \
	emmake make clean; \
	emmake make -j4; \
	emmake make install; \
	echo "Building FFmpeg with VP8/WebM support..."; \
	if [ ! -d "$${FFMPEG_DIR}" ]; then \
		mkdir -p $$(dirname $${FFMPEG_DIR}); \
		git clone https://git.ffmpeg.org/ffmpeg.git $${FFMPEG_DIR}; \
		cd $${FFMPEG_DIR}; \
		git checkout release/7.1; \
	else \
		cd $${FFMPEG_DIR}; \
	fi; \
	make distclean || true; \
	export PKG_CONFIG_PATH="$${INSTALL_DIR}/lib/pkgconfig:$${PKG_CONFIG_PATH}"; \
	export EM_PKG_CONFIG_PATH="$${INSTALL_DIR}/lib/pkgconfig:$${EM_PKG_CONFIG_PATH}"; \
	emconfigure ./configure --prefix=${INSTALL_DIR} \
		--cc=emcc --cxx=em++ --ar=emar --ranlib=emranlib --nm=emnm \
		--enable-cross-compile --target-os=none --arch=x86_32 \
		--disable-x86asm --disable-inline-asm --disable-asm \
		--disable-stripping --disable-programs --disable-doc \
		--disable-everything \
		--enable-avcodec --enable-avformat --enable-avutil \
		--enable-swscale --enable-swresample \
		--enable-gpl \
		--enable-libvpx \
		--enable-encoder=libvpx_vp8 \
		--enable-decoder=vp8 \
		--enable-muxer=webm \
		--enable-demuxer=webm \
		--enable-protocol=file \
		--extra-cflags="-I$${INSTALL_DIR}/include -DEMSCRIPTEN -s WASM=1" \
		--extra-ldflags="-L$${INSTALL_DIR}/lib" \
		--extra-cxxflags="-std=c++11"; \
	emmake make -j4; \
	emmake make install; \
	echo "Verifying VP8 encoder support..."; \
	find "$${INSTALL_DIR}/lib" -name "*.a" | xargs -n1 nm -g 2>/dev/null | grep -i "vpx\|vp8" || echo "Warning: VP8 symbols not found"; \
	echo "Build completed! Libraries installed in $${INSTALL_DIR}"'
else
	@echo "External dependencies already built, skipping."
endif

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

# Deep clean (removes external dependencies too)
deep-clean: clean
	rm -rf $(EXTERN_BUILD_DIR)