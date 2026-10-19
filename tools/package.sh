#!/usr/bin/env bash
# Assembles the title folders from the build: dist/PPSA64007 (Perfect Dark) and
# dist/PPSA64008 (the GoldenEye tile: the same game, which opens GoldenEye at once).
# Each has the eboot, its home screen files, the system libc module, the CA bundle the
# lobby's HTTPS needs, the shader cache and an empty data/ for the player's ROMs.
#   tools/package.sh             title folders and the two .command tools, for upload.sh (no ROM: the player adds theirs)
#   tools/package.sh --release   the same folders zipped, with the zip's sha256
set -euo pipefail
release=false
[[ ${1:-} == --release ]] && release=true
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build="$root/build-ps5/pd"
radv="$root/radv/mihawk-vulkan-review"

title() {
    local id=$1 sys=$2 note=$3
    local app="$root/dist/$id"
    rm -rf -- "$app"
    mkdir -p "$app/sce_sys" "$app/sce_module" "$app/data" "$app/texture-packs" "$app/added-content"
    cp "$build/pd.x86_64" "$app/eboot.bin"
    cp "$sys"/{param.json,icon0.png,pic0.dds,pic1.dds} "$app/sce_sys/"
    cp "$radv/runtime/libc.prx" "$app/sce_module/libc.prx"
    cp "$root/assets/cacert.pem" "$app/cacert.pem"
    # Get Mods and Maps' list until the release's newer one answers (port/src/modmode.c)
    cp "$root/assets/mod-catalogue.txt" "$app/mod-catalogue.txt"
    # SPIR-V the Vulkan renderer would otherwise compile on first sight (tools/gen-shader-cache.sh)
    [[ -f $root/assets/vulkan-shaders.bin ]] && cp "$root/assets/vulkan-shaders.bin" "$app/vulkan-shaders.bin"
    printf '%s\n' "$note" > "$app/data/PUT YOUR ROM IN THE TITLE FOLDER.txt"
    cp "$root/README.md" "$app/README.md"
    echo "package.sh: $app ($(du -sh "$app" | cut -f1))"
}

title PPSA64007 "$root/sce_sys" "Copy your Perfect Dark (USA) (Rev 1) ROM (.z64, .n64 or .v64, any file name) into the title
folder or this folder. The game puts it in place as pd.ntsc-final.z64 the first time it starts.
A GoldenEye 007 (USA) ROM in the title folder is moved to added-content/ and turned into
GoldenEye's levels and missions."
title PPSA64008 "$root/sce_sys_ge" "The GoldenEye tile runs inside Perfect Dark, so it needs both ROMs: copy your Perfect Dark
(USA) (Rev 1) ROM and your GoldenEye 007 (USA) ROM (.z64, .n64 or .v64, any file names) into the
title folder. The first start puts them in place and converts GoldenEye, then opens it."

# beside the title folders: the Mac's (or any Unix's) wireless install/update and save sync
cp "$root/tools/ps5-push.command" "$root/tools/ps5-sync-saves.command" "$root/dist/"

if $release; then
    # contentVersion 01.001.000 -> 1.1.0
    version=$(python3 -c 'import json, sys; print(".".join(str(int(p)) for p in json.load(open(sys.argv[1]))["contentVersion"].split(".")))' "$root/sce_sys/param.json")
    zip="$root/dist/PerfectEye-$version.zip"
    rm -f "$zip"
    (cd "$root/dist" && zip -qr -9 "$zip" PPSA64007 PPSA64008 ps5-push.command ps5-sync-saves.command)
    (cd "$root/dist" && sha256sum "$(basename "$zip")" > "$(basename "$zip").sha256")
    echo "package.sh: $zip ($(du -h "$zip" | cut -f1))"
fi
