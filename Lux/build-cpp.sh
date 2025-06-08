#!/bin/bash

# Incremental C++ Build Script for Jen
# Only rebuilds changed source files

set -e

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔨 Incremental C++ Build for Jen${NC}"

# Source Emscripten environment
source /opt/emsdk/emsdk_env.sh > /dev/null 2>&1

# Directories
SRC_DIR="/app/src"
BUILD_DIR="/app/web_build"
OUTPUT="/app/lux_react/src/lux.js"

# Ensure build directory exists
mkdir -p "$BUILD_DIR"

# Ensure STB image library is available
STB_DIR="/app/stb_image"
if [[ ! -d "$STB_DIR" || ! -f "$STB_DIR/stb_image.h" ]]; then
    echo -e "${YELLOW}📥 Downloading STB image library...${NC}"
    mkdir -p "$STB_DIR"
    wget -q -O "$STB_DIR/stb_image.h" https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    wget -q -O "$STB_DIR/stb_image_write.h" https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    echo -e "${GREEN}✓ STB image library downloaded${NC}"
fi

# Build flags
FFMPEG_CFLAGS="-I/app/external/build/include"
STB_CFLAGS="-I/app"
CAMERA_FLAGS="-DCAMERA_OPTIMIZED -DREAL_TIME_PROCESSING -DPERFORMANCE_MODE"
BASE_FLAGS="-O3 -MMD -MP -std=c++20 -msimd128 ${FFMPEG_CFLAGS} ${STB_CFLAGS} ${CAMERA_FLAGS}"
SIMD_FLAGS="-MMD -MP -std=c++20 -msimd128 ${FFMPEG_CFLAGS} ${STB_CFLAGS} ${CAMERA_FLAGS}"

# Optimized source files (O3)
O3_SOURCES="effect fimage frgb gamma_lut image life next_element offset_field uimage ucolor vect2 vector_field warp_field"

# Regular source files
REGULAR_SOURCES="scene scene_io any_effect any_rule any_function buffer_pair image_loader emscripten_utils UI"

# Core files
CORE_SOURCES="lux_web video_recorder"

# FFmpeg linking flags
FFMPEG_LIBS="-L/app/external/build/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lx264"

# Function to check if source is newer than object
needs_rebuild() {
    local src_file="$1"
    local obj_file="$2"
    
    if [[ ! -f "$obj_file" ]]; then
        return 0  # Object doesn't exist, need to build
    fi
    
    if [[ "$src_file" -nt "$obj_file" ]]; then
        return 0  # Source is newer
    fi
    
    # Check dependencies (if .d file exists)
    local dep_file="${obj_file%.o}.d"
    if [[ -f "$dep_file" ]]; then
        while IFS= read -r line; do
            if [[ "$line" =~ .*\.h.*:$ ]]; then
                local header="${line%:}"
                if [[ -f "$header" && "$header" -nt "$obj_file" ]]; then
                    return 0  # Header is newer
                fi
            fi
        done < "$dep_file"
    fi
    
    return 1  # No rebuild needed
}

# Compile function
compile_file() {
    local src_name="$1"
    local flags="$2"
    local src_file="$SRC_DIR/${src_name}.cpp"
    local obj_file="$BUILD_DIR/${src_name}.o"
    
    if needs_rebuild "$src_file" "$obj_file"; then
        echo -e "${YELLOW}🔨 Compiling ${src_name}.cpp${NC}"
        em++ $flags -c "$src_file" -o "$obj_file"
        return 0
    else
        echo -e "${GREEN}✓ ${src_name}.cpp up to date${NC}"
        return 1
    fi
}

# Track if any files were rebuilt
REBUILT=false

# Compile core files
echo -e "${BLUE}Building core files...${NC}"
for src in $CORE_SOURCES; do
    if compile_file "$src" "$BASE_FLAGS"; then
        REBUILT=true
    fi
done

# Compile O3 optimized files
echo -e "${BLUE}Building optimized files...${NC}"
for src in $O3_SOURCES; do
    if compile_file "$src" "$BASE_FLAGS"; then
        REBUILT=true
    fi
done

# Compile regular files
echo -e "${BLUE}Building regular files...${NC}"
for src in $REGULAR_SOURCES; do
    if compile_file "$src" "$SIMD_FLAGS"; then
        REBUILT=true
    fi
done

# Check if final linking is needed
NEED_LINK=false
if [[ "$REBUILT" == true ]]; then
    NEED_LINK=true
elif [[ ! -f "$OUTPUT" ]]; then
    NEED_LINK=true
    echo -e "${YELLOW}📎 Output file missing, need to link${NC}"
else
    # Check if any object file is newer than final output
    for obj in "$BUILD_DIR"/*.o; do
        if [[ "$obj" -nt "$OUTPUT" ]]; then
            NEED_LINK=true
            break
        fi
    done
fi

# Link if needed
if [[ "$NEED_LINK" == true ]]; then
    echo -e "${BLUE}🔗 Linking final WebAssembly...${NC}"
    
    # Remove any test files that might interfere
    rm -f "$BUILD_DIR"/test_*.o
    
    # Link all object files
    em++ "$BUILD_DIR"/*.o -o "$OUTPUT" \
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
        -s 'EXPORTED_FUNCTIONS=["_malloc","_free","_main"]' \
        -s 'EXPORTED_RUNTIME_METHODS=["addFunction","removeFunction","UTF8ToString","stringToUTF8","getValue","setValue","writeArrayToMemory","cwrap","FS"]' \
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
        -lembind $FFMPEG_LIBS
    
    echo -e "${GREEN}✅ Build complete: $(du -h "$OUTPUT" | cut -f1)${NC}"
else
    echo -e "${GREEN}✅ Everything up to date!${NC}"
fi

echo -e "${BLUE}🎉 Incremental build finished!${NC}" 