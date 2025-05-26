#!/bin/bash
# Complete Build Script for Jen Project - H.264/MP4 Version
# This script builds external dependencies and compiles the project with exact Makefile flags
# Now with incremental compilation support

# Exit on first error
set -e

# Activate Emscripten environment if not already active
if ! command -v em++ &> /dev/null; then
    echo "Activating Emscripten environment..."
    if [ -f "$HOME/Documents/emsdk/emsdk_env.sh" ]; then
        source "$HOME/Documents/emsdk/emsdk_env.sh"
    elif [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
        source "$HOME/emsdk/emsdk_env.sh"
    elif [ -f "/opt/emsdk/emsdk_env.sh" ]; then
        source "/opt/emsdk/emsdk_env.sh"
    else
        echo "❌ Error: Could not find emsdk_env.sh. Please ensure Emscripten is installed and activated."
        echo "You can activate it manually with: source /path/to/emsdk/emsdk_env.sh"
        exit 1
    fi
    
    # Verify Emscripten is now available
    if ! command -v em++ &> /dev/null; then
        echo "❌ Error: em++ still not found after attempting to activate Emscripten."
        echo "Please install and activate Emscripten SDK manually."
        echo "Visit: https://emscripten.org/docs/getting_started/downloads.html"
        exit 1
    fi
fi

echo "✓ Emscripten available: $(em++ --version | head -1)"

# Define directories
ROOT_DIR=$(pwd)
EXTERN_DIR="${ROOT_DIR}/external"
EXTERN_BUILD_DIR="${ROOT_DIR}/external/build"
X264_DIR="${EXTERN_DIR}/x264"
FFMPEG_DIR="${EXTERN_DIR}/FFmpeg"
SRC_DIR="${ROOT_DIR}/src"
WEB_BUILD_DIR="${ROOT_DIR}/web_build"

# Function to display progress message
show_message() {
  echo -e "\n===== $1 ====="
}

# Set up necessary directories
setup_dirs() {
  mkdir -p "${WEB_BUILD_DIR}"
  mkdir -p "${EXTERN_BUILD_DIR}/lib"
  mkdir -p "${EXTERN_BUILD_DIR}/include"
  mkdir -p "lux_react/src"
  echo "✓ Directories created"
}

# Function to check if source file is newer than object file
needs_compilation() {
  local src_file="$1"
  local obj_file="$2"
  
  # If object file doesn't exist, need to compile
  if [ ! -f "$obj_file" ]; then
    return 0  # true - needs compilation
  fi
  
  # If source file is newer than object file, need to compile
  if [ "$src_file" -nt "$obj_file" ]; then
    return 0  # true - needs compilation
  fi
  
  # Check if any header dependencies are newer (simple check for common headers)
  local src_dir=$(dirname "$src_file")
  local base_name=$(basename "$src_file" .cpp)
  local header_file="${src_dir}/${base_name}.hpp"
  
  if [ -f "$header_file" ] && [ "$header_file" -nt "$obj_file" ]; then
    return 0  # true - needs compilation
  fi
  
  return 1  # false - no compilation needed
}

# Incremental compilation function
compile_if_needed() {
  local src_file="$1"
  local obj_file="$2"
  local compile_flags="$3"
  
  if needs_compilation "$src_file" "$obj_file"; then
    echo "Compiling $(basename "$src_file")..."
    em++ $compile_flags "$src_file" -c -o "$obj_file"
    return 0
  else
    echo "Skipping $(basename "$src_file") (up to date)"
    return 1
  fi
}

# Build and install FFmpeg and H.264 dependencies
build_dependencies() {
  show_message "Building External Dependencies (FFmpeg and H.264)"
  
  # Check if libraries already exist
  echo "Checking for existing FFmpeg libraries..."
  if [ -f "${EXTERN_BUILD_DIR}/lib/libavcodec.a" ] && [ -f "${EXTERN_BUILD_DIR}/lib/libx264.a" ]; then
    echo "✓ FFmpeg and H.264 libraries already built"
    return 0
  fi
  
  # Build H.264 (libx264)
  echo "Building H.264 (libx264) with Emscripten..."
  if [ ! -d "${X264_DIR}" ]; then
    mkdir -p "$(dirname "${X264_DIR}")"
    git clone --depth 1 https://code.videolan.org/videolan/x264.git "${X264_DIR}"
  fi
  
  cd "${X264_DIR}"
  emconfigure ./configure \
    --prefix="${EXTERN_BUILD_DIR}" \
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
    --extra-cflags="-O3 -DEMSCRIPTEN -msimd128 -matomics -mbulk-memory -pthread" \
    --extra-cxxflags="-O3 -DEMSCRIPTEN -msimd128 -matomics -mbulk-memory -pthread"
  
  emmake make clean
  emmake make -j4
  emmake make install
  
  # Build FFmpeg
  echo "Building FFmpeg with H.264/MP4 support..."
  if [ ! -d "${FFMPEG_DIR}" ]; then
    mkdir -p "$(dirname "${FFMPEG_DIR}")"
    git clone https://git.ffmpeg.org/ffmpeg.git "${FFMPEG_DIR}"
    cd "${FFMPEG_DIR}"
    git checkout release/7.1
  else
    cd "${FFMPEG_DIR}"
  fi
  
  make distclean || true
  export PKG_CONFIG_PATH="${EXTERN_BUILD_DIR}/lib/pkgconfig:${PKG_CONFIG_PATH}"
  export EM_PKG_CONFIG_PATH="${EXTERN_BUILD_DIR}/lib/pkgconfig:${EM_PKG_CONFIG_PATH}"
  
  emconfigure ./configure --prefix="${EXTERN_BUILD_DIR}" \
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
    --extra-cflags="-I${EXTERN_BUILD_DIR}/include -DEMSCRIPTEN -s WASM=1 -msimd128 -matomics -mbulk-memory -pthread" \
    --extra-ldflags="-L${EXTERN_BUILD_DIR}/lib -msimd128 -matomics -mbulk-memory -pthread" \
    --extra-cxxflags="-std=c++11 -msimd128 -matomics -mbulk-memory -pthread"
  
  emmake make -j4
  emmake make install
  
  # Verify the libraries were built correctly
  echo "Verifying libraries in ${EXTERN_BUILD_DIR}/lib:"
  ls -la "${EXTERN_BUILD_DIR}/lib/"
  
  # Go back to the root directory
  cd "${ROOT_DIR}"
  
  echo "✓ Dependencies built successfully"
}

# Build the project with incremental compilation
build_project() {
  show_message "Building Project with Incremental Compilation..."
  
  # Verify FFmpeg headers are available
  if [ ! -d "${EXTERN_BUILD_DIR}/include/libavcodec" ]; then
    echo "❌ FFmpeg headers not found in ${EXTERN_BUILD_DIR}/include"
    echo "Please run the dependency build first"
    exit 1
  fi
  
  # FFmpeg configuration flags
  FFMPEG_CFLAGS="-I${EXTERN_BUILD_DIR}/include -DUSE_FFMPEG=1 -msimd128"
  FFMPEG_LIBS="-L${EXTERN_BUILD_DIR}/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lx264"
  
  # Base compilation flags
  BASE_FLAGS="-O3 -MMD -MP -std=c++20 -msimd128"
  SIMD_FLAGS="-MMD -MP -std=c++20 -msimd128"
  
  # Track compilation status
  local files_compiled=0
  local files_skipped=0
  
  # Files that need FFmpeg flags
  echo "Checking FFmpeg-dependent files..."
  if compile_if_needed "src/lux_web.cpp" "web_build/lux_web.o" "$BASE_FLAGS $FFMPEG_CFLAGS"; then
    ((files_compiled++))
  else
    ((files_skipped++))
  fi
  
  if compile_if_needed "src/video_recorder.cpp" "web_build/video_recorder.o" "$BASE_FLAGS $FFMPEG_CFLAGS"; then
    ((files_compiled++))
  else
    ((files_skipped++))
  fi
  
  # Individual file compilation rules - exactly as in Makefile
  echo "Checking other source files..."
  
  # Files with O3 optimization
  local o3_files=(
    "effect" "fimage" "frgb" "gamma_lut" "image" "life" 
    "next_element" "offset_field" "uimage" "ucolor" 
    "vect2" "vector_field" "warp_field"
  )
  
  for file in "${o3_files[@]}"; do
    if compile_if_needed "src/${file}.cpp" "web_build/${file}.o" "$BASE_FLAGS"; then
      ((files_compiled++))
    else
      ((files_skipped++))
    fi
  done
  
  # Files without O3 optimization
  local regular_files=(
    "scene" "scene_io" "any_effect" "any_rule" "any_function" 
    "buffer_pair" "image_loader" "emscripten_utils" "UI"
  )
  
  for file in "${regular_files[@]}"; do
    if compile_if_needed "src/${file}.cpp" "web_build/${file}.o" "$SIMD_FLAGS"; then
      ((files_compiled++))
    else
      ((files_skipped++))
    fi
  done
  
  # Check for any remaining .cpp files that might have been missed
  echo "Checking for any additional source files..."
  for src_file in src/*.cpp; do
    if [ -f "$src_file" ]; then
      base_name=$(basename "$src_file" .cpp)
      obj_file="web_build/${base_name}.o"
      
      # Skip files we don't want to compile
      if [[ "$base_name" == "life_web" || "$base_name" == "warp" || "$base_name" == "lux" || "$base_name" == "unit_tests" || "$base_name" == "image_test" || "$base_name" == "sploot" || "$base_name" == "circle" || "$base_name" == "borg" || "$base_name" == "hello_react" ]]; then
        continue
      fi
      
      # Skip files we already handled (FFmpeg-dependent files)
      if [[ "$base_name" == "lux_web" || "$base_name" == "video_recorder" ]]; then
        continue
      fi
      
      # Skip files we already handled (O3 optimized files)
      if [[ "$base_name" == "effect" || "$base_name" == "fimage" || "$base_name" == "frgb" || "$base_name" == "gamma_lut" || "$base_name" == "image" || "$base_name" == "life" || "$base_name" == "next_element" || "$base_name" == "offset_field" || "$base_name" == "uimage" || "$base_name" == "ucolor" || "$base_name" == "vect2" || "$base_name" == "vector_field" || "$base_name" == "warp_field" ]]; then
        continue
      fi
      
      # Skip files we already handled (regular files)
      if [[ "$base_name" == "scene" || "$base_name" == "scene_io" || "$base_name" == "any_effect" || "$base_name" == "any_rule" || "$base_name" == "any_function" || "$base_name" == "buffer_pair" || "$base_name" == "image_loader" || "$base_name" == "emscripten_utils" || "$base_name" == "UI" ]]; then
        continue
      fi
      
      echo "Found additional file: $base_name"
      if compile_if_needed "$src_file" "$obj_file" "$SIMD_FLAGS"; then
        ((files_compiled++))
      else
        ((files_skipped++))
      fi
    fi
  done
  
  echo "Compilation summary: $files_compiled files compiled, $files_skipped files skipped"
  
  # Collect all required object files for linking
  local required_objects=(
    "web_build/lux_web.o"
    "web_build/life.o"
    "web_build/any_effect.o"
    "web_build/any_function.o"
    "web_build/any_rule.o"
    "web_build/buffer_pair.o"
    "web_build/effect.o"
    "web_build/image.o"
    "web_build/uimage.o"
    "web_build/ucolor.o"
    "web_build/fimage.o"
    "web_build/frgb.o"
    "web_build/vector_field.o"
    "web_build/vect2.o"
    "web_build/offset_field.o"
    "web_build/gamma_lut.o"
    "web_build/image_loader.o"
    "web_build/scene.o"
    "web_build/scene_io.o"
    "web_build/next_element.o"
    "web_build/UI.o"
    "web_build/warp_field.o"
    "web_build/emscripten_utils.o"
    "web_build/video_recorder.o"
  )
  
  # Check that all required object files exist
  echo "Verifying required object files..."
  local missing_objects=()
  for obj in "${required_objects[@]}"; do
    if [ ! -f "$obj" ]; then
      missing_objects+=("$obj")
    fi
  done
  
  if [ ${#missing_objects[@]} -gt 0 ]; then
    echo "❌ Missing required object files:"
    printf '%s\n' "${missing_objects[@]}"
    exit 1
  fi
  
  # Check if linking is needed (if any object file is newer than the final output)
  local need_linking=false
  if [ ! -f "lux_react/src/lux.js" ]; then
    need_linking=true
    echo "Output file doesn't exist - linking needed"
  else
    for obj in "${required_objects[@]}"; do
      if [ "$obj" -nt "lux_react/src/lux.js" ]; then
        need_linking=true
        echo "Object file $obj is newer than output - linking needed"
        break
      fi
    done
  fi
  
  if [ "$need_linking" = true ]; then
    echo "Linking final executable..."
    em++ "${required_objects[@]}" -o lux_react/src/lux.js \
      --embed-file lux_files \
      -s MODULARIZE=1 \
      -s SINGLE_FILE=1 \
      -s ENVIRONMENT=web,worker \
      -s NO_DISABLE_EXCEPTION_CATCHING=0 \
      -s EXPORT_ES6=1 \
      -s INITIAL_MEMORY=671088640 \
      -s MAXIMUM_MEMORY=2147483648 \
      -s ALLOW_TABLE_GROWTH=1 \
      -s EXPORTED_FUNCTIONS=['_malloc','_free','_main'] \
      -s EXPORTED_RUNTIME_METHODS=['addFunction','removeFunction','UTF8ToString','stringToUTF8','getValue','setValue','writeArrayToMemory','cwrap'] \
      -s WASM_ASYNC_COMPILATION=1 \
      -s EXIT_RUNTIME=0 \
      -s LEGALIZE_JS_FFI=0 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s IMPORTED_MEMORY=1 \
      -O3 \
      -flto \
      -s DISABLE_EXCEPTION_CATCHING=1 \
      -msimd128 \
      -lembind ${FFMPEG_LIBS}
    
    echo "✓ Linking completed"
  else
    echo "✓ Output file is up to date - no linking needed"
  fi
  
  show_message "Build Complete"
  if [ -f "lux_react/src/lux.js" ]; then
    local file_size=$(du -h lux_react/src/lux.js | cut -f1)
    echo "✓ Output file: lux_react/src/lux.js (${file_size})"
  else
    echo "❌ Output file not found"
    exit 1
  fi
}

# Main execution flow
main() {
  show_message "Starting Jen Project Build (H.264/MP4 Incremental)"
  setup_dirs
  build_dependencies  
  build_project
  show_message "All Tasks Completed Successfully!"
}

# Run the main function
main