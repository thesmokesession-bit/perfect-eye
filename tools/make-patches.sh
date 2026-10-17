#!/usr/bin/env bash
# Writes patches/dabsmod.patch from the uncommitted changes in src-upstream/.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
mkdir -p "$root/patches"
git -C "$root/src-upstream" diff > "$root/patches/dabsmod.patch"
echo "make-patches.sh: patches/dabsmod.patch ($(grep -c '^diff --git' "$root/patches/dabsmod.patch") files)"
