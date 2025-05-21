#!/bin/bash
# Complete Build Script for Jen Project
# This script builds external dependencies and compiles the project with exact Makefile flags

# Exit on first error
set -e

# Define directories
ROOT_DIR=$(pwd)
EXTERN_DIR="${ROOT_DIR}/external"
EXTERN_BUILD_DIR="${ROOT_DIR}/external/build"
VPX_DIR="${EXTERN_DIR}/vpx"
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

# Build and install FFmpeg and VP8 dependencies
build_dependencies() {
  show_message "Building External Dependencies (FFmpeg and VP8)"
  
  # Check if libraries already exist
  echo "Checking for existing FFmpeg libraries..."
  if [ -f "${EXTERN_BUILD_DIR}/lib/libavcodec.a" ] && [ -f "${EXTERN_BUILD_DIR}/lib/libvpx.a" ]; then
    echo "✓ FFmpeg and VP8 libraries already built"
    return 0
  fi
  
  # Build VP8 (libvpx)
  echo "Building VP8 (libvpx) with Emscripten..."
  if [ ! -d "${VPX_DIR}" ]; then
    mkdir -p "$(dirname "${VPX_DIR}")"
    git clone https://chromium.googlesource.com/webm/libvpx "${VPX_DIR}"
  fi
  
  cd "${VPX_DIR}"
  emconfigure ./configure \
    --prefix="${EXTERN_BUILD_DIR}" \
    --target=generic-gnu \
    --disable-examples \
    --disable-docs \
    --disable-tools \
    --disable-unit-tests \
    --disable-vp9 \
    --enable-vp8 \
    --enable-static \
    --disable-shared \
    --extra-cflags="-O3 -DEMSCRIPTEN"
  
  emmake make clean
  emmake make -j4
  emmake make install
  
  # Build FFmpeg
  echo "Building FFmpeg with VP8/WebM support..."
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
    --enable-libvpx \
    --enable-encoder=libvpx_vp8 \
    --enable-decoder=vp8 \
    --enable-muxer=webm \
    --enable-demuxer=webm \
    --enable-protocol=file \
    --extra-cflags="-I${EXTERN_BUILD_DIR}/include -DEMSCRIPTEN -s WASM=1" \
    --extra-ldflags="-L${EXTERN_BUILD_DIR}/lib" \
    --extra-cxxflags="-std=c++11"
  
  emmake make -j4
  emmake make install
  
  # Verify the libraries were built correctly
  echo "Verifying libraries in ${EXTERN_BUILD_DIR}/lib:"
  ls -la "${EXTERN_BUILD_DIR}/lib/"
  
  # Go back to the root directory
  cd "${ROOT_DIR}"
  
  echo "✓ Dependencies built successfully"
}

# Build the project with exactly the same flags as the Makefile
build_project() {
  show_message "Building Project with Exact Makefile Flags"
  
  # FFmpeg configuration - exactly as in Makefile
  FFMPEG_CFLAGS="-I${EXTERN_BUILD_DIR}/include -DUSE_FFMPEG=1"
  FFMPEG_LIBS="-L${EXTERN_BUILD_DIR}/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lvpx"
  
  # Files that need FFmpeg flags - exactly as in Makefile
  echo "Compiling lux_web.cpp with FFmpeg flags..."
  em++ -O3 -MMD -MP -std=c++20 ${FFMPEG_CFLAGS} src/lux_web.cpp -c -o web_build/lux_web.o
  
  echo "Compiling video_recorder.cpp with FFmpeg flags..."
  em++ -O3 -MMD -MP -std=c++20 ${FFMPEG_CFLAGS} src/video_recorder.cpp -c -o web_build/video_recorder.o
  
  # Individual file compilation rules - exactly as in Makefile
  echo "Compiling effect.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/effect.cpp -c -o web_build/effect.o
  
  echo "Compiling fimage.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/fimage.cpp -c -o web_build/fimage.o
  
  echo "Compiling frgb.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/frgb.cpp -c -o web_build/frgb.o
  
  echo "Compiling gamma_lut.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/gamma_lut.cpp -c -o web_build/gamma_lut.o
  
  echo "Compiling image.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/image.cpp -c -o web_build/image.o
  
  echo "Compiling life.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/life.cpp -c -o web_build/life.o
  
  echo "Compiling next_element.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/next_element.cpp -c -o web_build/next_element.o
  
  echo "Compiling offset_field.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/offset_field.cpp -c -o web_build/offset_field.o
  
  echo "Compiling uimage.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/uimage.cpp -c -o web_build/uimage.o
  
  echo "Compiling ucolor.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/ucolor.cpp -c -o web_build/ucolor.o
  
  echo "Compiling vect2.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/vect2.cpp -c -o web_build/vect2.o
  
  echo "Compiling vector_field.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/vector_field.cpp -c -o web_build/vector_field.o
  
  echo "Compiling warp_field.cpp..."
  em++ -O3 -MMD -MP -std=c++20 src/warp_field.cpp -c -o web_build/warp_field.o
  
  # Main target - linking with EXACTLY the same flags as the Makefile
  echo "Linking target with exact flags from Makefile..."
  em++ web_build/lux_web.o web_build/life.o web_build/any_effect.o web_build/any_function.o web_build/any_rule.o web_build/buffer_pair.o web_build/effect.o web_build/image.o web_build/uimage.o web_build/ucolor.o web_build/fimage.o web_build/frgb.o web_build/vector_field.o web_build/vect2.o web_build/offset_field.o web_build/gamma_lut.o web_build/image_loader.o web_build/scene.o web_build/scene_io.o web_build/next_element.o web_build/UI.o web_build/warp_field.o web_build/emscripten_utils.o web_build/video_recorder.o -o lux_react/src/lux.js --embed-file lux_files -s MODULARIZE -s SINGLE_FILE=1 -s ENVIRONMENT=web,worker -s NO_DISABLE_EXCEPTION_CATCHING -s EXPORT_ES6=1 -s ASSERTIONS=1 -s INITIAL_MEMORY=671088640 -s MAXIMUM_MEMORY=2147483648 -s STACK_SIZE=10485760 -s TOTAL_STACK=20971520 -s ALLOW_TABLE_GROWTH=1 -s RESERVED_FUNCTION_POINTERS=200 -s EXPORTED_FUNCTIONS=['_malloc','_free','_main'] -s EXPORTED_RUNTIME_METHODS=['FS','addFunction','removeFunction','UTF8ToString','stringToUTF8','getValue','setValue','writeArrayToMemory','cwrap'] -s MODULARIZE=1 -s SINGLE_FILE=1 -s NO_DISABLE_EXCEPTION_CATCHING=1 -s ASSERTIONS=2 -s WASM_ASYNC_COMPILATION=1 -s SUPPORT_LONGJMP=1 -s EXIT_RUNTIME=0 -s LEGALIZE_JS_FFI=0 -s ALLOW_MEMORY_GROWTH=1 -lembind ${FFMPEG_LIBS}
  
  show_message "Build Complete"
  echo "Output file created at: lux_react/src/lux.js"
}

# Main execution flow
main() {
  show_message "Starting Jen Project Build"
  setup_dirs
  build_dependencies  
  build_project       
  show_message "All Tasks Completed Successfully!"
}

# Run the main function
main