#!/usr/bin/env bash
# Assembles the title folder dist/PPSA64007 from the build: eboot, home screen files, the
# system libc module, the CA bundle the lobby's HTTPS needs, and an empty data/ for the ROM.
#   tools/package.sh             title folder, for upload.sh (no ROM: the player adds theirs)
#   tools/package.sh --release   the same folder zipped, with its sha256
set -euo pipefail
release=false
[[ ${1:-} == --release ]] && release=true
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build="$root/build-ps5/pd"
radv="$root/radv/mihawk-vulkan-review"
id=PPSA64007
app="$root/dist/$id"

rm -rf -- "$app"
mkdir -p "$app/sce_sys" "$app/sce_module" "$app/data" "$app/texture-packs" "$app/added-content"
cp "$build/pd.x86_64" "$app/eboot.bin"
cp "$root"/sce_sys/{param.json,icon0.png,pic0.dds,pic1.dds} "$app/sce_sys/"
cp "$radv/runtime/libc.prx" "$app/sce_module/libc.prx"
cp "$root/assets/cacert.pem" "$app/cacert.pem"
# SPIR-V the Vulkan renderer would otherwise compile on first sight (tools/gen-shader-cache.sh)
[[ -f $root/assets/vulkan-shaders.bin ]] && cp "$root/assets/vulkan-shaders.bin" "$app/vulkan-shaders.bin"
cat > "$app/data/PUT YOUR ROM IN THE TITLE FOLDER.txt" <<'TXT'
Copy your Perfect Dark (USA) (Rev 1) ROM (.z64, .n64 or .v64, any file name) into the title
folder or this folder. The game puts it in place as pd.ntsc-final.z64 the first time it starts.
A GoldenEye 007 (USA) ROM in the title folder is moved to added-content/ and turned into
GoldenEye's levels and missions.
TXT
cp "$root/README.md" "$app/README.md" 2>/dev/null || true
if $release; then
    version=$(python3 -c 'import json, sys; print(json.load(open(sys.argv[1]))["contentVersion"])' "$app/sce_sys/param.json")
    zip="$root/dist/PerfectDark-PS5-$version.zip"
    rm -f "$zip"
    (cd "$root/dist" && zip -qr -9 "$zip" "$id")
    (cd "$root/dist" && sha256sum "$(basename "$zip")" > "$(basename "$zip").sha256")
    echo "package.sh: $zip ($(du -h "$zip" | cut -f1))"
else
    echo "package.sh: $app ($(du -sh "$app" | cut -f1))"
fi
