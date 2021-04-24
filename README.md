# CLion + Emscripten

Install Emscripten by following commands:

    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ./emsdk install latest
    ./emsdk activate latest

Copy and paste environment variables printed by the last command
to environment variables section in newly created CMake configuration
in CLion. Also add CMake launch arguments:

    -DCMAKE_TOOLCHAIN_FILE=emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake

If you have any problems try to restart CLion after everything is done.
