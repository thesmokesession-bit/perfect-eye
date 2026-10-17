#!/usr/bin/env bash
# Downloads the PacBrew ports (SDL2, FreeType, libpng and the rest the build links) at a pinned
# release into .deps/pacbrew and prints the sysroot build.sh uses (PACBREW). Does nothing if the
# release is already extracted.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
version=v0.40.2
hash=a85f65de418a8e6a898c6c3e3c870d50fff7618a200e4dd59ea9692af6ecec4d
url="https://github.com/ps5-payload-dev/pacbrew-repo/releases/download/$version/ps5-payload-dev.tar.gz"
cache="$root/.deps/pacbrew"
release="$cache/$version"
archive="$cache/ps5-payload-dev-$version.tar.gz"
sysroot="$release/sysroot/user/homebrew"

if [[ ! -f $release/.complete ]]; then
    mkdir -p "$cache"
    if [[ ! -f $archive ]] || ! printf '%s  %s\n' "$hash" "$archive" | sha256sum --check --strict >/dev/null 2>&1; then
        echo "setup-pacbrew.sh: downloading PacBrew $version (about 346 MB)" >&2
        curl -sfL "$url" -o "$archive.download"
        printf '%s  %s\n' "$hash" "$archive.download" | sha256sum --check --strict >&2
        mv "$archive.download" "$archive"
    fi
    rm -rf "$release"
    mkdir -p "$release/sysroot"
    tar -xzf "$archive" -C "$release/sysroot" --strip-components=3 opt/ps5-payload-sdk/target/user/homebrew
    [[ -d $sysroot/include && -d $sysroot/lib ]] || { echo "setup-pacbrew.sh: unexpected archive layout" >&2; exit 2; }
    touch "$release/.complete"
fi
echo "$sysroot"
