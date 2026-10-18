#!/bin/bash
# Perfect Dark PS5: sends the title folders beside this file (PPSA64007, PPSA64008) to the PS5 over
# the network, through the FTP server the jailbreak runs. Double-click it on a Mac, or run
#   ./ps5-push.command [folder holding the title folders]
# The PS5's address (ip:port, as the FTP payload shows it) is asked once and kept in .ps5-host
# beside this file; PS5_HOST overrides it. The game's own files (eboot.bin, sce_sys, sce_module,
# cacert.pem, the shader cache) go every time; everything else (ROMs, texture packs) only when the
# PS5 does not have it yet, so the first push installs and every push after it is an update.
# The console's saves, pd.ini and pd.log are never touched.
set -eu
here=$(cd -- "$(dirname -- "$0")" && pwd)
src=${1:-$here}
dir=${PS5_DIR:-/data/homebrew}
hostfile="$here/.ps5-host"
host=${PS5_HOST:-$(cat "$hostfile" 2>/dev/null || true)}
if [ -z "$host" ]; then
    printf "PS5 address and FTP port (for example 192.168.1.20:1337): "
    read -r host
    [ -n "$host" ] || { echo "no address given"; exit 1; }
    echo "$host" > "$hostfile"
fi
cd "$src"
ls -d PPSA6400[78] >/dev/null 2>&1 || { echo "no PPSA64007 or PPSA64008 folder in $src"; exit 1; }

tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
url() { local p=${1// /%20}; p=${p//#/%23}; echo "ftp://$host$dir/${p//\?/%3F}"; }
always='^[^/]*/(eboot\.bin|cacert\.pem|vulkan-shaders\.bin|mod-catalogue\.txt|README\.md|sce_sys/.*|sce_module/.*)$'
never='(^|/)(eeprom\.bin|pd\.ini|pd\.log|[^/]*-times\.txt|\.[^/]*)$'

# The console's FTP server can refuse to overwrite a file (550), so each file goes up under a
# temporary name, the old one is deleted (if there) and the new one renamed.
put() {
    curl -sS --ftp-create-dirs -T "$1" "$(url "$1").new" &&
    curl -sS "ftp://$host/" -Q "*DELE $dir/$1" -Q "RNFR $dir/$1.new" -Q "RNTO $dir/$1" -o /dev/null
}

curl -sS --connect-timeout 5 -l "ftp://$host/" -o /dev/null ||
    { echo "cannot reach the PS5's FTP server at $host (wrong address in $hostfile?)"; exit 1; }

sent=0 kept=0
find PPSA6400[78] -type d | sort > "$tmp/dirs"
while IFS= read -r d; do
    curl -s -l "$(url "$d")/" 2>/dev/null | tr -d '\r' > "$tmp/remote" || true
    find "$d" -maxdepth 1 -type f | sort > "$tmp/files"
    while IFS= read -r f; do
        if echo "$f" | grep -Eq "$never"; then continue; fi
        if echo "$f" | grep -Eq "$always" || ! grep -qxF "$(basename "$f")" "$tmp/remote"; then
            echo "  $f"
            put "$f" || { echo "failed: $f"; exit 1; }
            sent=$((sent + 1))
        else
            kept=$((kept + 1))
        fi
    done < "$tmp/files"
done < "$tmp/dirs"
echo "Sent $sent file(s) to the PS5 at $host ($kept already there). Start Perfect Dark from the home screen."
