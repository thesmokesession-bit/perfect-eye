#!/usr/bin/env bash
# Links Perfect Dark (Dab's Mod) into a PS5 title executable with the PS5_Vulkan title recipe
# (tools/build-radv-title.sh, tools/radv-link.sh): the static RADV driver, the payload SDK
# fork's platform layer and C++ runtime, the PacBrew libraries, then converts and signs it.
#
# CMake calls it as the executable link rule (cmake/ps5-toolchain.cmake):
#   link-title.sh <output> <objects and libraries as CMake passes them>
# It writes <output> (the signed eboot.bin) and <output>.elf (the unsigned ELF, with symbols).
set -euo pipefail

: "${RADV_ROOT:?set RADV_ROOT to the PS5_Vulkan checkout}"
: "${PACBREW:?set PACBREW to the PacBrew sysroot (user/homebrew)}"
out=$1
shift

root=$RADV_ROOT
sdk_root="$root/.deps/native/ps5-payload-sdk"
archive="${RADV_ARCHIVE:-$root/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a}"
native="$root/tooling/native"
tool="$root/build/host/ps5-native-tool"
work="$(dirname "$out")/ps5-link-$(basename "$out")"
mkdir -p "$work/obj" "$work/stubs"

cc() { PS5_PAYLOAD_SDK="$sdk_root" sh "$root/tooling/prospero-clang18" "$@"; }

# CMake's libraries: PacBrew -l names become archives; the system modules come from the
# SDK's stub libraries below; libm, libatomic and the C++ runtime are part of the recipe.
inputs=()
for arg in "$@"; do
    case $arg in
        -lm|-latomic|-static-libstdc++|-pthread|-ldl|-lpthread|-Wl,*) ;;
        -lSce*) ;;
        -l*)
            lib="$PACBREW/lib/lib${arg#-l}.a"
            [[ -f $lib ]] || { echo "link-title.sh: no archive for $arg" >&2; exit 2; }
            inputs+=("$lib") ;;
        -*) ;;
        *) inputs+=("$arg") ;;
    esac
done
# zlib is already in the RADV archive, which is linked whole. SDL2's iconv is PacBrew's libiconv.
inputs+=("$PACBREW/lib/libiconv.a")

# Entry point and C++ runtime glue of the native app builder.
cc -std=c++20 -O2 -fno-exceptions -fno-rtti -c "$native/app_crt.cpp" -o "$work/obj/app_crt.o"
cc -std=c++20 -O2 -fno-exceptions -fno-rtti -c "$native/app_cpp_runtime.cpp" -o "$work/obj/app_cpp_runtime.o"
# C library functions the SDK's libc lacks.
cc -std=c11 -O2 -c "$(dirname "${BASH_SOURCE[0]}")/ps5-shims.c" -o "$work/obj/ps5-shims.o"
# The title's start-up: log, crash handler, video driver, splash screen (ps5_platform.c).
cc -std=c11 -O2 -isystem "$PACBREW/include/SDL2" -isystem "$PACBREW/include" \
    -c "$(dirname "${BASH_SOURCE[0]}")/ps5_platform.c" -o "$work/obj/ps5_platform.o"
cc -std=c11 -O2 -c "$(dirname "${BASH_SOURCE[0]}")/ps5_rom.c" -o "$work/obj/ps5_rom.o"
# Relative paths resolved against an emulated working directory (ps5_paths.c).
cc -std=c11 -O2 -fvisibility=hidden -c "$(dirname "${BASH_SOURCE[0]}")/ps5_paths.c" -o "$work/obj/ps5_paths.o"
path_wraps=()
# Not getcwd, lstat or access: system modules import those names, and a wrapped name they
# import would become an export of the title, which the native converter refuses.
for name in chdir stat open fopen mkdir rmdir unlink remove rename truncate chmod opendir realpath; do
    path_wraps+=("--wrap=$name")
done

# AGC comes from system modules; these host-link stubs only name the imports.
stub() {
    local library=$1 source=$2
    cc -std=c11 -O2 -fPIC -c "$root/$source" -o "$work/obj/${library}_stub.o"
    "$sdk_root/bin/prospero-lld" --shared -soname "${library}.prx" \
        -o "$work/stubs/${library}.so" "$work/obj/${library}_stub.o"
}
stub libSceAgc vendor/ps5/sdk/stubs/agc_canary_link_stub.c
stub libSceAgcDriver vendor/ps5/sdk/stubs/agc_driver_canary_link_stub.c

# shellcheck source=/dev/null
source "$root/tools/radv-link.sh"
radv_link_recipe "$root" "$sdk_root" "$archive" || exit 2
# getaddrinfo is the title's own (ps5_platform.c, through the console's resolver): the SDK's
# ps5_getaddrinfo only fails, and online play and the lobby could not resolve a hostname.
for i in "${!radv_link_flags[@]}"; do
    case ${radv_link_flags[$i]} in
    --defsym=getaddrinfo=*) radv_link_flags[$i]=--defsym=getaddrinfo=pd_ps5_getaddrinfo ;;
    --defsym=freeaddrinfo=*) radv_link_flags[$i]=--defsym=freeaddrinfo=pd_ps5_freeaddrinfo ;;
    esac
done
"$sdk_root/bin/prospero-lld" "${radv_linker_script[@]}" --eh-frame-hdr "${radv_link_flags[@]}" "${path_wraps[@]}" \
    --version-script "$native/app-symbols.map" --exclude-libs=ALL \
    --version-script "$(dirname "${BASH_SOURCE[0]}")/title-local.map" \
    -e _start -o "$work/llvm-pie.elf" \
    "$work/obj/app_crt.o" "$work/obj/app_cpp_runtime.o" "$work/obj/ps5-shims.o" "$work/obj/ps5_paths.o" \
    "$work/obj/ps5_platform.o" "$work/obj/ps5_rom.o" \
    --start-group "${inputs[@]}" --end-group \
    "$work/stubs/libSceAgc.so" "$work/stubs/libSceAgcDriver.so" \
    "${radv_link_inputs[@]}" \
    --as-needed "$sdk_root"/target/lib/*.so
cp "$work/llvm-pie.elf" "$out.elf"
"$tool" link --in "$work/llvm-pie.elf" --out "$work/eboot.elf" \
    --stub-dir "$sdk_root/target/lib" --stub "$work/stubs/libSceAgc.so" \
    --stub "$work/stubs/libSceAgcDriver.so" --module-sdk 0x02000009 \
    --companion-sdk 0x08050001 --file-name eboot.elf
"$tool" self --sign --in "$work/eboot.elf" --out "$out" --magic 0x1D3D154F
"$tool" self --inspect --file "$out" > /dev/null
echo "link-title.sh: $out ($(stat -c %s "$out") bytes)"
