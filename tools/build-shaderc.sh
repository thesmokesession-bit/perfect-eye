#!/usr/bin/env bash
# Builds a static shaderc (with glslang and SPIRV-Tools) for the PS5 into deps/shaderc-ps5:
# the Vulkan renderer compiles its combiner shaders at run time. Run in the build container.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
src="$root/deps/shaderc"
build="$root/deps/build-shaderc-ps5"
prefix="$root/deps/shaderc-ps5"
: "${PS5_PAYLOAD_SDK:?}"
cmake -S "$src" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$PS5_PAYLOAD_SDK/toolchain/prospero.cmake" \
    -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON -DSHADERC_SKIP_COPYRIGHT_CHECK=ON \
    -DSPIRV_SKIP_EXECUTABLES=ON -DSPIRV_SKIP_TESTS=ON -DSPIRV_WERROR=OFF \
    -DENABLE_GLSLANG_BINARIES=OFF -DGLSLANG_TESTS=OFF -DENABLE_CTEST=OFF \
    -DCMAKE_VERBOSE_MAKEFILE=OFF > "$build.configure.log"
cmake --build "$build" --target shaderc_combined
mkdir -p "$prefix/lib" "$prefix/include"
cp "$build/libshaderc/libshaderc_combined.a" "$prefix/lib/"
cp -r "$src/libshaderc/include/shaderc" "$prefix/include/"
echo "build-shaderc.sh: $prefix/lib/libshaderc_combined.a"
