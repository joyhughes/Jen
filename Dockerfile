FROM emscripten/emsdk

RUN apt update -y && apt install -y clang-13 vim tmux

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-13 100 \
    && update-alternatives --install /usr/bin/cpp ccp /usr/bin/clang++-13 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-13 100

