#!/bin/bash
set -e

# Define base directories
ROOT_DIR=$(pwd)
X264_DIR="${ROOT_DIR}/external/x264"
FFMPEG_DIR="${ROOT_DIR}/external/FFmpeg"
INSTALL_DIR="${ROOT_DIR}/external/build"

# Create directories
mkdir -p ${INSTALL_DIR}

echo "Building x264 with Emscripten..."
if [ ! -d "${X264_DIR}" ]; then
  mkdir -p $(dirname ${X264_DIR})
  git clone https://code.videolan.org/videolan/x264.git ${X264_DIR}
fi

cd ${X264_DIR}

# Configure with Emscripten and explicitly disable problematic features
emconfigure ./configure \
  --prefix="${INSTALL_DIR}" \
  --enable-static \
  --disable-cli \
  --disable-asm \
  --disable-thread \
  --disable-lavf \
  --disable-swscale \
  --disable-opencl \
  --disable-interlaced \
  --disable-avs \
  --disable-ffms \
  --disable-gpac \
  --disable-lsmash \
  --extra-cflags="-O3 -DEMSCRIPTEN"

# Build and install
emmake make clean
emmake make -j4
emmake make install

echo "Building FFmpeg with Emscripten..."
if [ ! -d "${FFMPEG_DIR}" ]; then
  mkdir -p $(dirname ${FFMPEG_DIR})
  git clone https://git.ffmpeg.org/ffmpeg.git ${FFMPEG_DIR}
  cd ${FFMPEG_DIR}
  git checkout release/7.1
else
  cd ${FFMPEG_DIR}
fi
export EM_PKG_CONFIG_PATH="${INSTALL_DIR}/lib/pkgconfig"
# Configure FFmpeg with minimal features
emconfigure ./configure \
  --prefix="${INSTALL_DIR}" \
  --cc="emcc" \
  --cxx="em++" \
  --ar="emar" \
  --ranlib="emranlib" \
  --enable-cross-compile \
  --target-os=none \
  --arch=x86_32 \
  --disable-asm \
  --disable-inline-asm \
  --disable-stripping \
  --disable-programs \
  --disable-doc \
  --disable-all \
  --disable-everything \
  --disable-network \
  --disable-autodetect \
  --disable-iconv \
  --enable-small \
  --enable-avcodec \
  --enable-avformat \
  --enable-swscale \
  --enable-swresample \
  --enable-avutil \
  --enable-gpl \
  --enable-encoder=libx264,libvpx \
  --enable-decoder=h264,vp8 \
  --enable-muxer=mp4,webm \
  --enable-demuxer=mov,matroska,webm \
  --enable-protocol=file \
  --enable-libx264 \
  --disable-debug \
  --extra-cxxflags="-std=c++11"

# Build and install
emmake make clean
emmake make -j4
emmake make install

echo "Build completed! Libraries installed in ${INSTALL_DIR}"