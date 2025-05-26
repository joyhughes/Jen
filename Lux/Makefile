# Jen Project Makefile - Complete Build System
# FFmpeg and x264 are mandatory dependencies for H.264/MP4 video recording

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

# FFmpeg is mandatory - always use these flags
FFMPEG_CFLAGS := -I$(EXTERN_BUILD_DIR)/include
FFMPEG_LIBS := -L$(EXTERN_BUILD_DIR)/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lx264

# Compilation flags
BASE_FLAGS := -O3 -MMD -MP -std=c++20 -msimd128 $(FFMPEG_CFLAGS)
SIMD_FLAGS := -MMD -MP -std=c++20 -msimd128 $(FFMPEG_CFLAGS)

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
.PHONY: all clean deep-clean run-script no-deps

all: run-script

# Run the all-in-one build script
run-script:
	@if [ ! -f $(SCRIPT_PATH) ]; then \
		echo "Creating build script..."; \
		cp $(ROOT_DIR)/all_in_one_script.sh $(SCRIPT_PATH); \
		chmod +x $(SCRIPT_PATH); \
	fi
	@echo "Running build script..."
	@if [ "$(SKIP_DEPENDENCIES)" = "true" ]; then \
		echo "Building without FFmpeg dependencies (SKIP_DEPENDENCIES=true)"; \
		SKIP_DEPENDENCIES=true $(SCRIPT_PATH); \
	else \
		$(SCRIPT_PATH); \
	fi

# Clean targets
clean:
	rm -f $(WEB_BUILD_DIR)/*.o $(WEB_BUILD_DIR)/*.d $(OUTPUT)
	@echo "✓ Cleaned build files"

deep-clean: clean
	rm -rf $(EXTERN_BUILD_DIR)/lib/*.a $(EXTERN_LIB_DIR)/*.a
	@echo "Removed library files"

# Build without FFmpeg dependencies (for distribution)
no-deps:
	@echo "Building without FFmpeg dependencies..."
	@SKIP_DEPENDENCIES=true $(MAKE) run-script