#!/usr/bin/env bash
# Lists the title's imports that no usable system module provides: names only
# libScePosixForWebKit's or libkernel_sys's stubs define are null at run time (PS5_Vulkan
# tools/radv-link.sh), and AGC's come from the RADV recipe's own stubs. Prints nothing and
# exits 0 when every import is safe.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
sdk="$root/radv/mihawk-vulkan-review/.deps/native/ps5-payload-sdk"
elf=${1:-$root/build-ps5/pd/pd.x86_64.elf}
nm="$sdk/bin/llvm-nm"
good=$(for f in "$sdk"/target/lib/*.so; do
    case $(basename "$f" .so) in libScePosixForWebKit|libkernel_sys) continue ;; esac
    "$nm" -D --defined-only "$f" | awk '{print $3}'
done | sort -u)
bad=$(comm -23 <("$nm" -D --undefined-only "$elf" | awk '{print $2}' | sort -u) <(echo "$good") | grep -v '^sceAgc' || true)
if [[ -n $bad ]]; then
    echo "check-imports.sh: imports that are null at run time:" >&2
    echo "$bad" >&2
    exit 1
fi
echo "check-imports.sh: every import comes from a usable module"
