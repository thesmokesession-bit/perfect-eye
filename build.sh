#!/usr/bin/env bash
# Builds Perfect Dark (Dab's Mod) for the PS5 in the build container.
#   ./build.sh radv        RADV, the payload SDK fork and the title converter (once)
#   ./build.sh shaderc     a static shaderc for the console (once)
#   ./build.sh configure   CMake configure (build-ps5/pd)
#   ./build.sh [build]     compile and link eboot.bin
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
: "${PACBREW:=$("$root/tools/setup-pacbrew.sh")}"
export PACBREW
radv="$root/radv/mihawk-vulkan-review"
sdk="$radv/.deps/native/ps5-payload-sdk"
env="export PS5_PAYLOAD_SDK=$sdk RADV_ROOT=$radv PACBREW=$PACBREW SHADERC_PS5=$root/deps/shaderc-ps5 RADV_INCLUDE_ROOT=$radv/.deps/native/radv-release;"
hash=$(git -C "$root" rev-parse --short=9 HEAD)
case ${1:-build} in
radv)
    WORKDIR="$radv" "$root/docker/run.sh" "
        bash tools/setup-native-dependencies.sh && bash tools/build-radv.sh release &&
        n=tooling/native && z=.deps/native/zlib/root/usr && mkdir -p build/host &&
        clang++-18 -std=c++20 -O2 -I \$z/include \$n/native_app_builder.cpp \$n/self_container.cpp \
            \$n/elf_object.cpp \$n/sce_module_writer.cpp \$z/lib/libz.a -o build/host/ps5-native-tool &&
        bash tools/rebuild-libc.sh" ;;
shaderc)
    "$root/docker/run.sh" "$env tools/build-shaderc.sh" ;;
configure)
    "$root/docker/run.sh" "$env
        cmake -S src-upstream -B build-ps5/pd -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
            -DCMAKE_TOOLCHAIN_FILE=$root/cmake/ps5-toolchain.cmake -DUPDATE_CHANNEL=dev \
            -DPS5_UPDATE_REPO=${PS5_UPDATE_REPO:-} -DPS5_VERSION_HASH=$hash" ;;
build)
    # the build's commit is this repo's: src-upstream stays at the Dab's Mod commit the patch is
    # made against, so its own would be the same for every Perfect Eye build and Check for
    # Updates would never see a new one
    "$root/docker/run.sh" "$env cmake build-ps5/pd -DPS5_VERSION_HASH=$hash > /dev/null &&
        cmake --build build-ps5/pd -- -k 0" ;;
*)
    echo "usage: $0 [radv|shaderc|configure|build]" >&2; exit 2 ;;
esac
