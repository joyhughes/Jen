FROM emscripten/emsdk:4.0.9

RUN apt update -y && apt install -y clang-12 vim tmux sudo git curl wget file

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-12 100 \
    && update-alternatives --install /usr/bin/cpp ccp /usr/bin/clang++-12 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-12 100

RUN cd /tmp && wget https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n4.4.6.tar.gz && cd /opt && tar xvfz /tmp/n4.4.6.tar.gz && rm /tmp/n4.4.6.tar.gz && chmod -R a+rwX /opt

RUN cd /opt/FF* && . /emsdk/emsdk_env.sh && emconfigure ./configure \
  --disable-x86asm \
  --ar=emar \
  --cc=emcc \
  --cxx=em++ \
  --objcc=emcc \
  --dep-cc=emcc \
  --disable-inline-asm \
  --disable-doc \
  --disable-stripping && \
emmake make
