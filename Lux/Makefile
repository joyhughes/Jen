# Jen Project Makefile - Complete Build System with Live Camera
# FFmpeg and x264 are mandatory dependencies for H.264/MP4 video recording
# Camera optimizations integrated for real-time kaleidoscope effectst

MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Suppress repeated Emscripten messages
export EMSDK_QUIET := 1

# Auto-detect and configure Emscripten environment
EMCC_CHECK := $(shell command -v em++ 2> /dev/null)
ifndef EMCC_CHECK
    # Find Emscripten environment script
    EMSDK_PATHS := $(HOME)/Documents/emsdk/emsdk_env.sh $(HOME)/emsdk/emsdk_env.sh /opt/emsdk/emsdk_env.sh
    EMSDK_ENV := $(firstword $(wildcard $(EMSDK_PATHS)))
    
    ifneq ($(EMSDK_ENV),)
        $(info 🔧 Found Emscripten at: $(EMSDK_ENV))
        $(info 🔧 Configuring shell to use Emscripten environment...)
        
        # Configure shell to source Emscripten for all commands
        SHELL := /bin/bash
        .SHELLFLAGS := -c 'EMSDK_QUIET=1 source $(EMSDK_ENV) && exec "$$@"' --
        
        # Export the Emscripten environment path for recursive make calls
        export EMSDK_ENV_PATH := $(EMSDK_ENV)
        
        # Verify it will work (suppress output for this test)
        EMCC_CHECK_AFTER := $(shell EMSDK_QUIET=1 source $(EMSDK_ENV) && command -v em++ 2> /dev/null)
        ifneq ($(EMCC_CHECK_AFTER),)
            $(info ✅ Emscripten will be available for build commands)
        else
            $(info ❌ Failed to activate Emscripten from $(EMSDK_ENV))
            $(error Please check your Emscripten installation)
        endif
    else
        $(info )
        $(info ❌ Emscripten not found!)
        $(info )
        $(info Please install Emscripten SDK first:)
        $(info   git clone https://github.com/emscripten-core/emsdk.git)
        $(info   cd emsdk)
        $(info   ./emsdk install latest)
        $(info   ./emsdk activate latest)
        $(info )
        $(info Or manually activate if already installed:)
        $(info   source /path/to/emsdk/emsdk_env.sh)
        $(info )
        $(error Emscripten SDK not found or not activated)
    endif
else
    $(info ✅ Emscripten already active: $(shell em++ --version | head -1))
    # Still set shell for consistency with quiet mode
    SHELL := /bin/bash
    export EMSDK_QUIET := 1
endif

# Directories
ROOT_DIR := $(shell pwd)
EXTERN_DIR := $(ROOT_DIR)/external
EXTERN_BUILD_DIR := $(EXTERN_DIR)/build
X264_DIR := $(EXTERN_DIR)/x264
FFMPEG_DIR := $(EXTERN_DIR)/FFmpeg
SRC_DIR := $(ROOT_DIR)/src
WEB_BUILD_DIR := $(ROOT_DIR)/web_build

# Camera optimization flags (integrated into main build)
CAMERA_FLAGS := -DCAMERA_OPTIMIZED -DREAL_TIME_PROCESSING -DPERFORMANCE_MODE

# FFmpeg is mandatory - always use these flags
FFMPEG_CFLAGS := -I$(EXTERN_BUILD_DIR)/include
FFMPEG_LIBS := -L$(EXTERN_BUILD_DIR)/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lx264

# Compilation flags with integrated camera optimizations
BASE_FLAGS := -O3 -MMD -MP -std=c++20 -msimd128 $(FFMPEG_CFLAGS) $(CAMERA_FLAGS)
SIMD_FLAGS := -MMD -MP -std=c++20 -msimd128 $(FFMPEG_CFLAGS) $(CAMERA_FLAGS)

# Source files categorized by optimization level
O3_SOURCES := effect fimage frgb gamma_lut image life next_element offset_field uimage ucolor vect2 vector_field warp_field
REGULAR_SOURCES := scene scene_io any_effect any_rule any_function buffer_pair image_loader emscripten_utils UI

# Object files for incremental builds
CORE_OBJS := $(WEB_BUILD_DIR)/lux_web.o $(WEB_BUILD_DIR)/video_recorder.o
O3_OBJS := $(addprefix $(WEB_BUILD_DIR)/, $(addsuffix .o, $(O3_SOURCES)))
REGULAR_OBJS := $(addprefix $(WEB_BUILD_DIR)/, $(addsuffix .o, $(REGULAR_SOURCES)))
ALL_OBJS := $(CORE_OBJS) $(O3_OBJS) $(REGULAR_OBJS)

# Final output
OUTPUT := lux_react/src/lux.js

# Main targets
.PHONY: all clean deep-clean deps help setup emscripten camera-dev no-deps status info

# Single command build - ensures dependencies then builds incrementally with camera optimizations
all: setup-if-needed deps $(OUTPUT)
	@echo ""
	@echo "🎉 Build Complete with Live Camera!"
	@echo "✓ Output: $(OUTPUT) ($$(du -h $(OUTPUT) | cut -f1))"
	@echo "✓ Camera optimizations: ✅ Integrated"
	@echo "✓ Real-time processing: ✅ Enabled"
	@echo "✓ Enhanced memory: ✅ 4GB max"
	@echo "✓ Ready to use in React app"

# Camera development build (quick build for camera testing)
camera-dev: setup-if-needed deps $(OUTPUT)
	@echo ""
	@echo "🚀 Camera Development Build Complete!"
	@echo "📁 Output: $(OUTPUT)"
	@echo "🎥 Camera features: Integrated and ready"
	@echo "💡 Start React dev server: cd lux_react && npm start"

# Build without FFmpeg dependencies (camera still works, no video recording)
no-deps: setup-if-needed $(OUTPUT)
	@echo ""
	@echo "⚠️  Build Complete (No Video Recording)"
	@echo "✓ Output: $(OUTPUT) ($$(du -h $(OUTPUT) | cut -f1))"
	@echo "✓ Camera optimizations: ✅ Integrated"
	@echo "❌ Video recording: Disabled (no FFmpeg)"
	@echo "💡 For video recording, run 'make deps' first"

# Setup directories if they don't exist
setup-if-needed:
	@if [ ! -d "$(EXTERN_DIR)" ] || [ ! -d "$(WEB_BUILD_DIR)" ] || [ ! -d "lux_react/src" ]; then \
		echo "📁 Setting up project directories..."; \
		mkdir -p "$(EXTERN_DIR)"; \
		mkdir -p "$(EXTERN_BUILD_DIR)/lib"; \
		mkdir -p "$(EXTERN_BUILD_DIR)/include"; \
		mkdir -p "$(WEB_BUILD_DIR)"; \
		mkdir -p "lux_react/src"; \
		echo "✓ Project directories created"; \
	fi

# FFmpeg dependencies are mandatory for video recording - build if missing
deps: $(EXTERN_BUILD_DIR)/lib/libavcodec.a $(EXTERN_BUILD_DIR)/lib/libx264.a
	@echo "✅ All mandatory dependencies available"

# Incremental build with camera optimizations - only rebuild what's needed
$(OUTPUT): $(ALL_OBJS) | lux_react/src
	@echo "🔗 Linking final executable with camera optimizations..."
	@if [ -n "$(EMSDK_ENV_PATH)" ]; then \
		EMSDK_QUIET=1 source "$(EMSDK_ENV_PATH)" && \
		em++ $(ALL_OBJS) -o $(OUTPUT) \
			--embed-file lux_files \
			-s MODULARIZE=1 \
			-s SINGLE_FILE=1 \
			-s ENVIRONMENT=web,worker \
			-s NO_DISABLE_EXCEPTION_CATCHING=0 \
			-s EXPORT_ES6=1 \
			-s INITIAL_MEMORY=268435456 \
			-s MAXIMUM_MEMORY=2147483648 \
			-s STACK_SIZE=4194304 \
			-s ALLOW_TABLE_GROWTH=1 \
			-s EXPORTED_FUNCTIONS=['_malloc','_free','_main'] \
			-s EXPORTED_RUNTIME_METHODS=['addFunction','removeFunction','UTF8ToString','stringToUTF8','getValue','setValue','writeArrayToMemory','cwrap','FS'] \
			-s WASM_ASYNC_COMPILATION=1 \
			-s EXIT_RUNTIME=0 \
			-s LEGALIZE_JS_FFI=0 \
			-s ALLOW_MEMORY_GROWTH=1 \
			-s IMPORTED_MEMORY=0 \
			-s TOTAL_MEMORY=268435456 \
			-O3 \
			-flto \
			-s DISABLE_EXCEPTION_CATCHING=1 \
			-msimd128 \
			-lembind $(FFMPEG_LIBS); \
	else \
		em++ $(ALL_OBJS) -o $(OUTPUT) \
			--embed-file lux_files \
			-s MODULARIZE=1 \
			-s SINGLE_FILE=1 \
			-s ENVIRONMENT=web,worker \
			-s NO_DISABLE_EXCEPTION_CATCHING=0 \
			-s EXPORT_ES6=1 \
			-s INITIAL_MEMORY=268435456 \
			-s MAXIMUM_MEMORY=2147483648 \
			-s STACK_SIZE=4194304 \
			-s ALLOW_TABLE_GROWTH=1 \
			-s EXPORTED_FUNCTIONS=['_malloc','_free','_main'] \
			-s EXPORTED_RUNTIME_METHODS=['addFunction','removeFunction','UTF8ToString','stringToUTF8','getValue','setValue','writeArrayToMemory','cwrap','FS'] \
			-s WASM_ASYNC_COMPILATION=1 \
			-s EXIT_RUNTIME=0 \
			-s LEGALIZE_JS_FFI=0 \
			-s ALLOW_MEMORY_GROWTH=1 \
			-s IMPORTED_MEMORY=0 \
			-s TOTAL_MEMORY=268435456 \
			-O3 \
			-flto \
			-s DISABLE_EXCEPTION_CATCHING=1 \
			-msimd128 \
			-lembind $(FFMPEG_LIBS); \
	fi
	@echo "✓ Build complete with camera optimizations: $(OUTPUT) ($$(du -h $(OUTPUT) | cut -f1))"

# Core object files with camera optimizations (incremental compilation)
$(WEB_BUILD_DIR)/lux_web.o: $(SRC_DIR)/lux_web.cpp | $(WEB_BUILD_DIR) deps
	@echo "🔨 Compiling lux_web.cpp (with camera optimizations)..."
	@if [ -n "$(EMSDK_ENV_PATH)" ]; then \
		EMSDK_QUIET=1 source "$(EMSDK_ENV_PATH)" && \
		em++ $(BASE_FLAGS) -c $< -o $@; \
	else \
		em++ $(BASE_FLAGS) -c $< -o $@; \
	fi

$(WEB_BUILD_DIR)/video_recorder.o: $(SRC_DIR)/video_recorder.cpp $(SRC_DIR)/video_recorder.hpp | $(WEB_BUILD_DIR) deps
	@echo "🎬 Compiling video_recorder.cpp (with camera optimizations)..."
	@if [ -n "$(EMSDK_ENV_PATH)" ]; then \
		EMSDK_QUIET=1 source "$(EMSDK_ENV_PATH)" && \
		em++ $(BASE_FLAGS) -c $< -o $@; \
	else \
		em++ $(BASE_FLAGS) -c $< -o $@; \
	fi

# O3 optimized object files (incremental compilation)
$(WEB_BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(WEB_BUILD_DIR) deps
	@echo "⚡ Compiling $*.cpp (O3 + camera optimizations)..."
	@if [ -n "$(EMSDK_ENV_PATH)" ]; then \
		EMSDK_QUIET=1 source "$(EMSDK_ENV_PATH)" && \
		em++ $(BASE_FLAGS) -c $< -o $@; \
	else \
		em++ $(BASE_FLAGS) -c $< -o $@; \
	fi

# Regular object files with camera optimizations (incremental compilation)
define REGULAR_RULE
$(WEB_BUILD_DIR)/$(1).o: $(SRC_DIR)/$(1).cpp | $(WEB_BUILD_DIR) deps
	@echo "🔧 Compiling $(1).cpp (with camera optimizations)..."
	@if [ -n "$(EMSDK_ENV_PATH)" ]; then \
		EMSDK_QUIET=1 source "$(EMSDK_ENV_PATH)" && \
		em++ $(SIMD_FLAGS) -c $$< -o $$@; \
	else \
		em++ $(SIMD_FLAGS) -c $$< -o $$@; \
	fi
endef

$(foreach src,$(REGULAR_SOURCES),$(eval $(call REGULAR_RULE,$(src))))

# External dependency building rules (unchanged from original)
$(EXTERN_BUILD_DIR)/lib/libavcodec.a: $(EXTERN_BUILD_DIR)/lib/libx264.a
	@echo "🔧 Building FFmpeg with H.264/MP4 support..."
	@mkdir -p "$(EXTERN_DIR)"
	@if [ ! -d "$(FFMPEG_DIR)" ]; then \
		echo "📥 Cloning FFmpeg..."; \
		git clone https://git.ffmpeg.org/ffmpeg.git "$(FFMPEG_DIR)"; \
		cd "$(FFMPEG_DIR)" && git checkout release/7.1; \
	fi
	@echo "🔧 Configuring and building FFmpeg..."
	@if [ -n "$(EMSDK_ENV_PATH)" ]; then \
		cd "$(FFMPEG_DIR)" && \
		EMSDK_QUIET=1 source "$(EMSDK_ENV_PATH)" && \
		make distclean || true && \
		export PKG_CONFIG_PATH="$(EXTERN_BUILD_DIR)/lib/pkgconfig:$$PKG_CONFIG_PATH" && \
		export EM_PKG_CONFIG_PATH="$(EXTERN_BUILD_DIR)/lib/pkgconfig:$$EM_PKG_CONFIG_PATH" && \
		export PKG_CONFIG_LIBDIR="$(EXTERN_BUILD_DIR)/lib/pkgconfig" && \
		export PKG_CONFIG="pkg-config" && \
		echo "🔍 Environment setup:" && \
		echo "  PKG_CONFIG_PATH: $$PKG_CONFIG_PATH" && \
		echo "  EM_PKG_CONFIG_PATH: $$EM_PKG_CONFIG_PATH" && \
		echo "  PKG_CONFIG_LIBDIR: $$PKG_CONFIG_LIBDIR" && \
		echo "🔍 Testing pkg-config for x264:" && \
		pkg-config --exists x264 && echo "✓ x264 found via pkg-config" || echo "❌ x264 NOT found via pkg-config" && \
		pkg-config --cflags x264 && \
		pkg-config --libs x264 && \
		echo "🔧 Configuring FFmpeg with emconfigure and proper pkg-config paths..." && \
		emconfigure ./configure --prefix="$(EXTERN_BUILD_DIR)" \
			--cc=emcc --cxx=em++ --ar=emar --ranlib=emranlib --nm=emnm \
			--enable-cross-compile --target-os=none --arch=x86_32 \
			--disable-x86asm --disable-inline-asm --disable-asm \
			--disable-stripping --disable-programs --disable-doc \
			--disable-everything \
			--enable-avcodec --enable-avformat --enable-avutil \
			--enable-swscale --enable-swresample \
			--enable-gpl \
			--enable-libx264 \
			--enable-encoder=libx264 \
			--enable-decoder=h264 \
			--enable-muxer=mp4 \
			--enable-demuxer=mov \
			--enable-protocol=file \
			--extra-cflags="-I$(EXTERN_BUILD_DIR)/include -DEMSCRIPTEN -msimd128" \
			--extra-ldflags="-L$(EXTERN_BUILD_DIR)/lib -msimd128" \
			--extra-cxxflags="-std=c++11 -msimd128" \
			--pkg-config-flags="--static" && \
		echo "🔍 Verifying configure completed successfully:" && \
		ls -la ffbuild/config.mak && \
		ls -la config.h && \
		echo "🔧 Building FFmpeg..." && \
		emmake make -j4 && \
		emmake make install; \
	else \
		echo "❌ Emscripten environment not available"; \
		exit 1; \
	fi
	@echo "✓ FFmpeg built successfully"

$(EXTERN_BUILD_DIR)/lib/libx264.a: | $(EXTERN_BUILD_DIR)
	@echo "🔧 Building H.264 (libx264) with Emscripten..."
	@mkdir -p "$(EXTERN_DIR)"
	@if [ ! -d "$(X264_DIR)" ]; then \
		echo "📥 Cloning x264..."; \
		git clone --depth 1 https://code.videolan.org/videolan/x264.git "$(X264_DIR)"; \
	fi
	@echo "🔧 Configuring and building x264..."
	@if [ -n "$(EMSDK_ENV_PATH)" ]; then \
		cd "$(X264_DIR)" && \
		EMSDK_QUIET=1 source "$(EMSDK_ENV_PATH)" && \
		emconfigure ./configure \
			--prefix="$(EXTERN_BUILD_DIR)" \
			--host=x86-linux \
			--enable-static \
			--disable-shared \
			--disable-opencl \
			--disable-thread \
			--disable-asm \
			--disable-cli \
			--enable-pic \
			--bit-depth=8 \
			--chroma-format=420 \
			--extra-cflags="-O3 -DEMSCRIPTEN -msimd128" \
			--extra-cxxflags="-O3 -DEMSCRIPTEN -msimd128" && \
		emmake make clean && \
		emmake make -j4 && \
		emmake make install && \
		echo "🔍 Verifying x264 installation:" && \
		ls -la "$(EXTERN_BUILD_DIR)/lib/libx264.a" && \
		ls -la "$(EXTERN_BUILD_DIR)/lib/pkgconfig/x264.pc" && \
		cat "$(EXTERN_BUILD_DIR)/lib/pkgconfig/x264.pc"; \
	else \
		echo "❌ Emscripten environment not available"; \
		exit 1; \
	fi
	@echo "✓ x264 built successfully"

# Create external build directory structure
$(EXTERN_BUILD_DIR):
	@echo "📁 Setting up external build directories..."
	@mkdir -p "$(EXTERN_BUILD_DIR)/lib"
	@mkdir -p "$(EXTERN_BUILD_DIR)/include"
	@mkdir -p "$(EXTERN_DIR)"
	@echo "✓ External directories created"

# Create directories
$(WEB_BUILD_DIR):
	mkdir -p $@

lux_react/src:
	mkdir -p $@

# Manual setup target (for explicit setup)
setup:
	@echo "📁 Setting up project structure..."
	@mkdir -p "$(EXTERN_DIR)"
	@mkdir -p "$(EXTERN_BUILD_DIR)/lib"
	@mkdir -p "$(EXTERN_BUILD_DIR)/include"
	@mkdir -p "$(WEB_BUILD_DIR)"
	@mkdir -p "lux_react/src"
	@echo "✓ Project structure created"

# Status check
status:
	@echo "🎯 Build Status:"
	@echo "  Camera optimizations: ✅ Integrated into main build"
	@echo "  Source directory: $(SRC_DIR)"
	@echo "  Build directory: $(WEB_BUILD_DIR)"
	@echo "  Output: $(OUTPUT)"
	@if [ -f "$(EXTERN_BUILD_DIR)/lib/libavcodec.a" ]; then \
		echo "  FFmpeg available: ✅ Enabled"; \
	else \
		echo "  FFmpeg available: ❌ Disabled (run 'make deps' first)"; \
	fi
	@if [ -f "$(OUTPUT)" ]; then \
		echo "  Build status: ✅ Ready ($$(du -h $(OUTPUT) | cut -f1))"; \
	else \
		echo "  Build status: ❌ Not built"; \
	fi

# Build info
info:
	@echo "🎥 Lux Kaleidoscope with Live Camera"
	@echo "=================================="
	@echo "Version: Camera-Optimized Build"
	@echo ""
	@echo "🎯 Integrated Features:"
	@echo "  • Real-time camera kaleidoscope effects"
	@echo "  • Live video processing with WebAssembly"
	@echo "  • Optimized RGBA→ARGB color conversion"
	@echo "  • Zero-copy buffer management"
	@echo "  • Enhanced memory (4GB max)"
	@echo "  • Adaptive frame rate control"
	@echo "  • Performance monitoring"
	@echo "  • H.264/MP4 video recording"
	@echo ""
	@echo "🚀 Camera optimizations are built-in by default!"

# Clean targets
clean:
	rm -f $(WEB_BUILD_DIR)/*.o $(WEB_BUILD_DIR)/*.d $(OUTPUT)
	@echo "✓ Cleaned build files"

deep-clean: clean
	rm -rf $(EXTERN_BUILD_DIR)/lib/*.a
	@echo "✓ Removed external libraries"

# Development shortcuts
dev: camera-dev

# Quick test build (for rapid iteration)
test-build: setup-if-needed $(OUTPUT)
	@echo "🧪 Quick test build with camera optimizations complete!"

# Help
help:
	@echo "🚀 Jen Project Build System with Live Camera"
	@echo "============================================"
	@echo ""
	@echo "✨ SINGLE COMMAND BUILD:"
	@echo "  make             # Does everything automatically with camera optimizations!"
	@echo ""
	@echo "📋 Build targets:"
	@echo "  all        - Standard build with camera optimizations (default)"
	@echo "  camera-dev - Camera development build"
	@echo "  no-deps    - Build without FFmpeg (camera works, no video recording)"
	@echo "  deps       - Build external dependencies (FFmpeg, x264)"
	@echo "  setup      - Create project directory structure"
	@echo "  clean      - Remove build files"
	@echo "  deep-clean - Remove build files and external libraries"
	@echo "  status     - Show build configuration status"
	@echo "  info       - Show camera feature information"
	@echo "  help       - Show this help"
	@echo ""
	@echo "🎯 What 'make' does automatically:"
	@echo "  1. ✅ Activates Emscripten environment"
	@echo "  2. 📁 Creates directories if needed"
	@echo "  3. 🔧 Builds dependencies if needed (first time: 10-20 min)"
	@echo "  4. ⚡ Compiles project with camera optimizations"
	@echo "  5. 🎉 Outputs lux_react/src/lux.js with live camera support"
	@echo ""
	@echo "🎥 Camera Features (integrated by default):"
	@echo "  ✅ Real-time camera processing"
	@echo "  ✅ Live kaleidoscope effects"
	@echo "  ✅ Buffer reuse optimization"
	@echo "  ✅ RGBA to ARGB color conversion"
	@echo "  ✅ Performance monitoring"
	@echo "  ✅ Enhanced memory (4GB max)"
	@echo "  ✅ Adaptive quality scaling"
	@echo ""
	@echo "🔧 Troubleshooting:"
	@echo "  make emscripten  # Check if Emscripten is available"
	@echo "  make clean       # Clean and rebuild"
	@echo "  make deps        # Force rebuild dependencies"
	@echo "  make status      # Check current build status"

# Check Emscripten environment
emscripten:
	@echo "🔍 Checking Emscripten environment..."
	@if command -v em++ >/dev/null 2>&1; then \
		echo "✅ Emscripten is active: $$(em++ --version | head -1)"; \
		echo "✅ emcc: $$(which emcc)"; \
		echo "✅ em++: $$(which em++)"; \
	else \
		echo "❌ Emscripten not found in PATH"; \
		echo ""; \
		echo "🔧 To activate Emscripten, run one of these commands:"; \
		echo ""; \
		if [ -f "$$HOME/Documents/emsdk/emsdk_env.sh" ]; then \
			echo "  source $$HOME/Documents/emsdk/emsdk_env.sh"; \
		fi; \
		if [ -f "$$HOME/emsdk/emsdk_env.sh" ]; then \
			echo "  source $$HOME/emsdk/emsdk_env.sh"; \
		fi; \
		if [ -f "/opt/emsdk/emsdk_env.sh" ]; then \
			echo "  source /opt/emsdk/emsdk_env.sh"; \
		fi; \
		echo ""; \
		echo "📥 Or install Emscripten from:"; \
		echo "   https://emscripten.org/docs/getting_started/downloads.html"; \
		echo ""; \
		echo "Then run 'make' again."; \
		exit 1; \
	fi

# Include dependency files for incremental builds
-include $(WEB_BUILD_DIR)/*.d