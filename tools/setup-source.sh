#!/usr/bin/env bash
# Sets up the sources the PS5 build needs, next to this repository's files:
#   src-upstream/  Dab's Mod at the pinned commit with this port's patches/dabsmod.patch applied
#                  (left uncommitted, so the build keeps upstream's commit hash: online play
#                  needs every player on the same build)
#   radv/          Mihawk's PS5_Vulkan, PS5_Mesa and PS5_PayloadSDK at the pinned commits
#   deps/shaderc   shaderc and its third-party sources at pinned commits
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
upstream_url=https://github.com/DabDavis/perfect-dark-dabs-mod
# DABS_COMMIT picks another Dab's Mod commit (the release job passes the one its dev build names)
upstream_commit=${DABS_COMMIT:-3ee8ed0cd549b64350848655c43ae1568e449c6d}   # dabs-mod-dev, patch notes 66
src="$root/src-upstream"
if [[ ! -d $src ]]; then
    git clone -q "$upstream_url" "$src"
    git -C "$src" checkout -q "$upstream_commit"
    git -C "$src" apply --3way "$root/patches/dabsmod.patch" ||
        { echo "setup-source.sh: patches/dabsmod.patch does not apply to $upstream_commit" >&2; exit 1; }
fi

mkdir -p "$root/radv"
for pin in "PS5_Vulkan mihawk-vulkan-review 5b5e4fc2d80fdb67a4f61ba9e6a8a424026bb8a5" \
           "PS5_Mesa mihawk-mesa-review 7b59ef27c1b09b9671bc4153c41940c3155c3af2" \
           "PS5_PayloadSDK mihawk-sdk-review b83202be73e930050e00de0f1b4d9ec46c0391bf"; do
    read -r repo dir commit <<< "$pin"
    [[ -d $root/radv/$dir ]] || git clone -q "https://github.com/mihawk-99/$repo" "$root/radv/$dir"
    git -C "$root/radv/$dir" checkout -q "$commit"
done
ln -sfn mihawk-sdk-review "$root/radv/PS5_PayloadSDK"
ln -sfn mihawk-mesa-review "$root/radv/PS5_Mesa"

sc="$root/deps/shaderc"
if [[ ! -d $sc ]]; then
    git clone -q --depth 1 -b v2025.4 https://github.com/google/shaderc.git "$sc"
    for pin in "glslang KhronosGroup/glslang d213562e35573012b6348b2d584457c3704ac09b" \
               "spirv-tools KhronosGroup/SPIRV-Tools 19042c8921f35f7bec56b9e5c96c5f5691588ca8" \
               "spirv-headers KhronosGroup/SPIRV-Headers 01e0577914a75a2569c846778c2f93aa8e6feddd"; do
        read -r dir repo commit <<< "$pin"
        git init -q "$sc/third_party/$dir"
        git -C "$sc/third_party/$dir" fetch -q --depth 1 "https://github.com/$repo" "$commit"
        git -C "$sc/third_party/$dir" checkout -q FETCH_HEAD
    done
fi
echo "setup-source.sh: sources ready; next: ./build.sh radv, ./build.sh shaderc, ./build.sh configure, ./build.sh"
