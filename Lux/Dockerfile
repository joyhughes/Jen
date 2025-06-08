# Jen Application - Unified Development Environment
# Complete build environment with Emscripten, FFmpeg, and x264 for H.264/MP4 video recording
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV EMSDK_QUIET=1

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    git \
    wget \
    curl \
    python3 \
    python3-pip \
    pkg-config \
    yasm \
    nasm \
    # Additional build tools
    autoconf \
    automake \
    libtool \
    make \
    patch \
    # Required for Node.js installation
    ca-certificates \
    gnupg \
    lsb-release \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Install modern Node.js (v18 LTS) from NodeSource
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && \
    apt-get install -y nodejs && \
    rm -rf /var/lib/apt/lists/* && \
    # Verify installation
    node --version && \
    npm --version

# Set working directory
WORKDIR /app

# Install Emscripten SDK
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk && \
    cd /opt/emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

# Set up Emscripten environment
ENV PATH="/opt/emsdk:/opt/emsdk/upstream/emscripten:${PATH}"
ENV EMSDK=/opt/emsdk
ENV EM_CONFIG=/opt/emsdk/.emscripten
ENV EMSDK_NODE=/opt/emsdk/node/18.20.3_64bit/bin/node

# Copy project files
COPY . /app/

# Create required directories and download STB image library
RUN mkdir -p /app/external/build/lib && \
    mkdir -p /app/external/build/include && \
    mkdir -p /app/web_build && \
    mkdir -p /app/lux_react/src && \
    mkdir -p /app/stb_image && \
    cd /app/stb_image && \
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h && \
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h

# Build x264 with Emscripten
RUN cd /app/external && \
    rm -rf x264 && \
    rm -rf ffmpeg && \
    git clone --depth 1 https://code.videolan.org/videolan/x264.git && \
    cd x264 && \
    bash -c "source /opt/emsdk/emsdk_env.sh && \
    emconfigure ./configure \
        --prefix='/app/external/build' \
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
        --extra-cflags='-O3 -DEMSCRIPTEN -msimd128' \
        --extra-cxxflags='-O3 -DEMSCRIPTEN -msimd128' && \
    emmake make clean && \
    emmake make -j\$(nproc) && \
    emmake make install"

# Build FFmpeg with x264 support using Emscripten
RUN cd /app/external && \
    git clone https://git.ffmpeg.org/ffmpeg.git && \
    cd ffmpeg && \
    git checkout release/7.1 && \
    bash -c "source /opt/emsdk/emsdk_env.sh && \
    export PKG_CONFIG_PATH='/app/external/build/lib/pkgconfig:\${PKG_CONFIG_PATH}' && \
    export EM_PKG_CONFIG_PATH='/app/external/build/lib/pkgconfig:\${EM_PKG_CONFIG_PATH}' && \
    export PKG_CONFIG_LIBDIR='/app/external/build/lib/pkgconfig' && \
    export PKG_CONFIG='pkg-config' && \
    emconfigure ./configure --prefix='/app/external/build' \
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
        --extra-cflags='-I/app/external/build/include -DEMSCRIPTEN -msimd128' \
        --extra-ldflags='-L/app/external/build/lib -msimd128' \
        --extra-cxxflags='-std=c++11 -msimd128' \
        --pkg-config-flags='--static' && \
    emmake make -j\$(nproc) && \
    emmake make install"

# Set up environment variables for build
ENV FFMPEG_CFLAGS="-I/app/external/build/include"
ENV FFMPEG_LIBS="-L/app/external/build/lib -lavcodec -lavformat -lavutil -lswscale -lswresample -lx264"
ENV CAMERA_FLAGS="-DCAMERA_OPTIMIZED -DREAL_TIME_PROCESSING -DPERFORMANCE_MODE"

# Build C++ source files to object files
RUN bash -c "source /opt/emsdk/emsdk_env.sh && \
    echo 'Starting C++ compilation with camera optimizations...' && \
    # Core files with camera optimizations
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/lux_web.cpp -o /app/web_build/lux_web.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/video_recorder.cpp -o /app/web_build/video_recorder.o && \
    # O3 optimized files
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/effect.cpp -o /app/web_build/effect.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/fimage.cpp -o /app/web_build/fimage.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/frgb.cpp -o /app/web_build/frgb.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/gamma_lut.cpp -o /app/web_build/gamma_lut.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/image.cpp -o /app/web_build/image.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/life.cpp -o /app/web_build/life.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/next_element.cpp -o /app/web_build/next_element.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/offset_field.cpp -o /app/web_build/offset_field.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/uimage.cpp -o /app/web_build/uimage.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/ucolor.cpp -o /app/web_build/ucolor.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/vect2.cpp -o /app/web_build/vect2.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/vector_field.cpp -o /app/web_build/vector_field.o && \
    em++ -O3 -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/warp_field.cpp -o /app/web_build/warp_field.o && \
    # Regular files with camera optimizations
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/scene.cpp -o /app/web_build/scene.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/scene_io.cpp -o /app/web_build/scene_io.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/any_effect.cpp -o /app/web_build/any_effect.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/any_rule.cpp -o /app/web_build/any_rule.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/any_function.cpp -o /app/web_build/any_function.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/buffer_pair.cpp -o /app/web_build/buffer_pair.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/image_loader.cpp -o /app/web_build/image_loader.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/emscripten_utils.cpp -o /app/web_build/emscripten_utils.o && \
    em++ -MMD -MP -std=c++20 -msimd128 \${FFMPEG_CFLAGS} \${CAMERA_FLAGS} -c /app/src/UI.cpp -o /app/web_build/UI.o && \
    echo 'All source files compiled successfully!' && \
    ls -la /app/web_build/*.o"

# Final linking step to create lux.js with camera optimizations
RUN bash -c "source /opt/emsdk/emsdk_env.sh && \
    echo 'Linking WebAssembly with video recording support...' && \
    # Clean up any test files first
    rm -f /app/web_build/test_*.o && \
    # List object files for verification
    ls -la /app/web_build/*.o && \
    echo 'Proceeding with final linking...' && \
    em++ /app/web_build/*.o -o /app/lux_react/src/lux.js \
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
        -s 'EXPORTED_FUNCTIONS=[\"_malloc\",\"_free\",\"_main\"]' \
        -s 'EXPORTED_RUNTIME_METHODS=[\"addFunction\",\"removeFunction\",\"UTF8ToString\",\"stringToUTF8\",\"getValue\",\"setValue\",\"writeArrayToMemory\",\"cwrap\",\"FS\"]' \
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
        -lembind \${FFMPEG_LIBS} && \
    echo 'WebAssembly linking completed successfully!' && \
    ls -la /app/lux_react/src/lux.js && \
    echo 'Final lux.js size:' && \
    du -h /app/lux_react/src/lux.js"

# Install React dependencies if package.json exists
WORKDIR /app/lux_react
RUN if [ -f package.json ]; then \
        echo "Installing React dependencies with Node.js $(node --version)..." && \
        # Clear any existing node_modules and package-lock.json
        rm -rf node_modules package-lock.json && \
        # Install with audit fix to address vulnerabilities
        npm install && \
        npm audit fix || echo "Audit fix completed with warnings" && \
        echo "React dependencies installed successfully"; \
    fi

# Verify the build
RUN echo "🎉 Build verification:" && \
    ls -la /app/lux_react/src/lux.js && \
    echo "✅ FFmpeg libraries:" && \
    ls -la /app/external/build/lib/libav*.a && \
    ls -la /app/external/build/lib/libx264.a && \
    echo "✅ Build complete with video recording support!"

# Set up runtime environment
WORKDIR /app
ENV NODE_ENV=development

# Expose ports for React development server
EXPOSE 3000 5173

# Default command
CMD ["bash"]
