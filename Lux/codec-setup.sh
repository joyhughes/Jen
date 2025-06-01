#!/bin/bash
set -e

# Define base directories
ROOT_DIR=$(pwd)
VPX_DIR="${ROOT_DIR}/external/vpx"
FFMPEG_DIR="${ROOT_DIR}/external/FFmpeg"
INSTALL_DIR="${ROOT_DIR}/external/build"

# Create directories
mkdir -p ${INSTALL_DIR}

echo "Building VP8 (libvpx) with Emscripten..."
if [ ! -d "${VPX_DIR}" ]; then
  mkdir -p $(dirname ${VPX_DIR})
  git clone https://chromium.googlesource.com/webm/libvpx ${VPX_DIR}
fi

cd ${VPX_DIR}

# Configure libvpx for VP8 only
emconfigure ./configure \
  --prefix="${INSTALL_DIR}" \
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

# Build and install libvpx
emmake make clean
emmake make -j4
emmake make install

echo "Building FFmpeg with VP8/WebM support..."
if [ ! -d "${FFMPEG_DIR}" ]; then
  mkdir -p $(dirname ${FFMPEG_DIR})
  git clone https://git.ffmpeg.org/ffmpeg.git ${FFMPEG_DIR}
  cd ${FFMPEG_DIR}
  git checkout release/7.1
else
  cd ${FFMPEG_DIR}
fi

make distclean || true

# Set pkg-config paths correctly
export PKG_CONFIG_PATH="${INSTALL_DIR}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export EM_PKG_CONFIG_PATH="${INSTALL_DIR}/lib/pkgconfig:${EM_PKG_CONFIG_PATH}"

emconfigure ./configure --prefix=/Users/namvdo/Desktop/rnd/temp/Jen/Lux/external/build \
  --cc=emcc --cxx=em++ --ar=emar --ranlib=emranlib --nm=emnm \
  --enable-cross-compile --target-os=none --arch=x86_32 \
  --disable-x86asm --disable-inline-asm --disable-asm \
  --disable-stripping --disable-programs --disable-doc \
  --disable-everything \
  --enable-avcodec --enable-avformat --enable-avutil \
  --enable-swscale --enable-swresample \
  --enable-gpl \
  --enable-libvpx \
  --enable-libx264 \
  --enable-encoder=libvpx_vp8,libx264 \
  --enable-decoder=vp8,h264 \
  --enable-muxer=webm,mp4 \
  --enable-demuxer=webm,mov \
  --enable-protocol=file \
  --extra-cflags="-I${INSTALL_DIR}/include -DEMSCRIPTEN -s WASM=1" \
  --extra-ldflags="-L${INSTALL_DIR}/lib" \
  --extra-cxxflags="-std=c++11"

# Build and install FFmpeg
emmake make -j4
emmake make install

# Verify installation
echo "Verifying VP8 encoder support..."
find "${INSTALL_DIR}/lib" -name "*.a" | xargs -n1 nm -g 2>/dev/null | grep -i "vpx\|vp8"

echo "Build completed! Libraries installed in ${INSTALL_DIR}"