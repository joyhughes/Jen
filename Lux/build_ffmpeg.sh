#!/bin/bash

# Exit on error
set -e

# Define paths
WORK_DIR="$PWD/build_deps"
FFMPEG_SRC="$PWD/external/FFmpeg"
X264_SRC="$PWD/external/x264"
INSTALL_DIR="$PWD/build_deps/install"

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
mkdir -p "$INSTALL_DIR"

# Build x264 first
echo "Building x264..."
cd "$X264_SRC"
make clean
./configure --prefix="$INSTALL_DIR" \
    --enable-static \
    --enable-pic \
    --host=$(uname -m)-apple-darwin
make -j$(sysctl -n hw.ncpu)
make install

# Build FFmpeg with x264 support
echo "Building FFmpeg..."
cd "$FFMPEG_SRC"
make clean
./configure \
    --prefix="$INSTALL_DIR" \
    --enable-gpl \
    --enable-libx264 \
    --enable-static \
    --disable-shared \
    --extra-cflags="-I$INSTALL_DIR/include" \
    --extra-ldflags="-L$INSTALL_DIR/lib" \
    --enable-pic \
    --arch=$(uname -m) \
    --target-os=darwin \
    --host=$(uname -m)-apple-darwin

make -j$(sysctl -n hw.ncpu)
make install

# Create a pkg-config file for FFmpeg
mkdir -p "$INSTALL_DIR/lib/pkgconfig"
cat > "$INSTALL_DIR/lib/pkgconfig/ffmpeg.pc" << EOF
prefix=$INSTALL_DIR
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: FFmpeg
Description: FFmpeg libraries
Version: 1.0
Libs: -L\${libdir} -lavcodec -lavformat -lavutil -lswscale -lswresample
Cflags: -I\${includedir}
EOF

echo "Build complete! Libraries installed in $INSTALL_DIR"
echo "Add these to your build system:"
echo "CPPFLAGS: -I$INSTALL_DIR/include"
echo "LDFLAGS: -L$INSTALL_DIR/lib"