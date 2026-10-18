#!/usr/bin/env bash
# Fills vulkan-shaders.bin (SPIR-V under a hash of its GLSL, the same on every driver) by
# booting every mission and arena of a Linux build headless on Mesa's lavapipe.
#   tools/gen-shader-cache.sh <dir with pd.x86_64, data/ and pd.ini>
set -uo pipefail
dir=$1
cd "$dir"
stages="90 92 9 22 25 27 28 29 30 33 34 36 38 39 42 44 45 47 48 49 51 52 53 55 56 79 23 31 32 37 41 50 57 59 60 61 65 66 67 68 69 71"
for s in $stages; do
    SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy timeout 600 ./pd.x86_64 --portable --vulkan \
        --boot-stage "$s" --exit-frame 1200 > "gen-$s.log" 2>&1
    echo "stage $s exit $? $(grep -o 'Vulkan: [0-9]* shaders in the cache' "gen-$s.log")"
done
