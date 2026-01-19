#!/bin/bash
set -e
set -o pipefail
set -u

error_trap() {
    local exit_code=$?
    echo -e "\n${RED}BUILD FAILED! Exit code: $exit_code, Line: $1${NC}"
    exit $exit_code
}

trap 'error_trap $LINENO "$BASH_COMMAND"' ERR

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

FORCE_REBUILD=false
if [[ "${1:-}" == "--force" || "${1:-}" == "-f" ]]; then
    FORCE_REBUILD=true
    echo -e "${RED}FORCE REBUILD${NC}"
else
    echo -e "${BLUE}Incremental C++ Build${NC}"
fi

if ! source /opt/emsdk/emsdk_env.sh > /dev/null; then
    echo -e "${RED}ERROR: Failed to source Emscripten environment${NC}"
    exit 1
fi

SRC_DIR="/app/src"
BUILD_DIR="/app/web_build"
OUTPUT="/app/lux_react/src/lux.js"

mkdir -p "$BUILD_DIR"

STB_DIR="/app/stb_image"
if [[ ! -d "$STB_DIR" || ! -f "$STB_DIR/stb_image.h" ]]; then
    mkdir -p "$STB_DIR" || exit 1
    wget -q -O "$STB_DIR/stb_image.h" https://raw.githubusercontent.com/nothings/stb/master/stb_image.h || exit 1
    wget -q -O "$STB_DIR/stb_image_write.h" https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h || exit 1
fi

FFMPEG_CFLAGS="-I/app/external/build/include"
STB_CFLAGS="-I/app"
CAMERA_FLAGS="-DCAMERA_OPTIMIZED -DREAL_TIME_PROCESSING -DPERFORMANCE_MODE"
BASE_FLAGS="-O3 -MMD -MP -std=c++20 -msimd128 ${FFMPEG_CFLAGS} ${STB_CFLAGS} ${CAMERA_FLAGS}"
SIMD_FLAGS="-MMD -MP -std=c++20 -msimd128 ${FFMPEG_CFLAGS} ${STB_CFLAGS} ${CAMERA_FLAGS}"

O3_SOURCES="effect fimage frgb gamma_lut image life next_element offset_field uimage ucolor vect2 vector_field warp_field"
REGULAR_SOURCES="scene scene_io any_effect any_rule any_function buffer_pair image_loader emscripten_utils UI"
CORE_SOURCES="lux_web video_recorder"
FFMPEG_LIBS="-L/app/external/build/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lx264"

check_json_changes() {
    local json_timestamp_file="$BUILD_DIR/.json_timestamp"
    local current_json_hash=""

    if [[ -d "lux_files" ]]; then
        current_json_hash=$(find lux_files -name "*.json" -type f -exec sha256sum {} \; 2>/dev/null | sort | sha256sum | cut -d' ' -f1)
    fi

    if [[ -f "$json_timestamp_file" ]]; then
        local stored_hash=$(cat "$json_timestamp_file" 2>/dev/null || echo "")
        if [[ "$current_json_hash" != "$stored_hash" ]]; then
            echo "$current_json_hash" > "$json_timestamp_file"
            return 0
        fi
    else
        echo "$current_json_hash" > "$json_timestamp_file"
        return 0
    fi

    return 1
}

needs_rebuild() {
    local src_file="$1"
    local obj_file="$2"

    if [[ "$FORCE_REBUILD" == true ]]; then
        return 0
    fi

    if [[ ! -f "$obj_file" ]]; then
        return 0
    fi

    if [[ "$src_file" -nt "$obj_file" ]]; then
        return 0
    fi

    local dep_file="${obj_file%.o}.d"
    if [[ -f "$dep_file" ]]; then
        while IFS= read -r line; do
            if [[ "$line" =~ .*\.h.*:$ ]]; then
                local header="${line%:}"
                if [[ -f "$header" && "$header" -nt "$obj_file" ]]; then
                    return 0
                fi
            fi
        done < "$dep_file"
    fi

    return 1
}

compile_file() {
    local src_name="$1"
    local flags="$2"
    local src_file="$SRC_DIR/${src_name}.cpp"
    local obj_file="$BUILD_DIR/${src_name}.o"

    if [[ ! -f "$src_file" ]]; then
        echo -e "${RED}ERROR: Source not found: $src_file${NC}"
        exit 1
    fi

    if needs_rebuild "$src_file" "$obj_file"; then
        echo -e "${YELLOW}Compiling ${src_name}.cpp${NC}"
        em++ $flags -c "$src_file" -o "$obj_file" || exit 1
        return 0
    fi
    return 1
}

JSON_CHANGED=false
if check_json_changes; then
    JSON_CHANGED=true
fi

REBUILT=false
CPP_FILES_CHANGED=false

if [[ "$FORCE_REBUILD" == true ]]; then
    rm -f "$BUILD_DIR"/*.o "$BUILD_DIR"/*.d
    REBUILT=true
    CPP_FILES_CHANGED=true
    JSON_CHANGED=true
fi

for src in $CORE_SOURCES; do
    if compile_file "$src" "$BASE_FLAGS"; then
        REBUILT=true
        CPP_FILES_CHANGED=true
    fi
done

for src in $O3_SOURCES; do
    if compile_file "$src" "$BASE_FLAGS"; then
        REBUILT=true
        CPP_FILES_CHANGED=true
    fi
done

for src in $REGULAR_SOURCES; do
    if compile_file "$src" "$SIMD_FLAGS"; then
        REBUILT=true
        CPP_FILES_CHANGED=true
    fi
done

NEED_LINK=false
if [[ "$FORCE_REBUILD" == true ]]; then
    NEED_LINK=true
elif [[ "$REBUILT" == true ]]; then
    NEED_LINK=true
elif [[ "$JSON_CHANGED" == true ]]; then
    NEED_LINK=true
elif [[ ! -f "$OUTPUT" ]]; then
    NEED_LINK=true
else
    for obj in "$BUILD_DIR"/*.o; do
        if [[ "$obj" -nt "$OUTPUT" ]]; then
            NEED_LINK=true
            break
        fi
    done

    if [[ -d "lux_files" ]]; then
        for json in lux_files/*.json; do
            if [[ -f "$json" && "$json" -nt "$OUTPUT" ]]; then
                NEED_LINK=true
                break
            fi
        done
    fi
fi

if [[ "$NEED_LINK" == true ]]; then
    echo -e "${BLUE}Linking...${NC}"

    if ! ls "$BUILD_DIR"/*.o 1> /dev/null 2>&1; then
        echo -e "${RED}ERROR: No object files found${NC}"
        exit 1
    fi

    rm -f "$BUILD_DIR"/test_*.o

    if [[ ! -d "lux_files" ]]; then
        echo -e "${RED}ERROR: lux_files directory not found${NC}"
        exit 1
    fi

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
        -lembind $FFMPEG_LIBS || exit 1

    if [[ ! -f "$OUTPUT" ]]; then
        echo -e "${RED}ERROR: Output file was not created${NC}"
        exit 1
    fi

    echo -e "${GREEN}Build complete: $(du -h "$OUTPUT" | cut -f1)${NC}"
else
    echo -e "${GREEN}Up to date${NC}"
fi

trap - ERR