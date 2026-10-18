#!/bin/bash
# Perfect Dark PS5: sends the title folders beside this file (PPSA64007, PPSA64008) to the PS5 over
# the network, through the FTP server the jailbreak runs. Double-click it on a Mac, or run
#   ./ps5-push.command [folder holding the title folders]
# The PS5's address (ip:port, as the FTP payload shows it) is asked once and kept in .ps5-host
# beside this file; PS5_HOST overrides it. The game's own files (eboot.bin, sce_sys, sce_module,
# cacert.pem, the shader cache) go every time; everything else (ROMs, texture packs) only when the
# PS5 does not have it at that size, so the first push installs and every push after it is an update.
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
# PS5_TITLES: which title folders (both by default, those that are here)
titles=""
for t in ${PS5_TITLES:-PPSA64007 PPSA64008}; do [ -d "$t" ] && titles="$titles $t"; done
[ -n "$titles" ] || { echo "no PPSA64007 or PPSA64008 folder in $src"; exit 1; }

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

put_abs() {
    curl -sS -T "$1" "ftp://$host$2.new" &&
    curl -sS "ftp://$host/" -Q "*DELE $2" -Q "RNFR $2.new" -Q "RNTO $2" -o /dev/null
}

curl -sS --connect-timeout 5 "ftp://$host/" -o /dev/null ||
    { echo "cannot reach the PS5's FTP server at $host (wrong address in $hostfile?)"; exit 1; }

# Files the PS5 does not have at all go up in batches over one connection (a
# texture pack is thousands of small files); a replacement goes through put()
batch=()
flush() {
    [ ${#batch[@]} -eq 0 ] || curl -sS --ftp-create-dirs "${batch[@]}" || { echo "failed sending a batch"; exit 1; }
    batch=()
}

sent=0 kept=0
find $titles -type d | sort > "$tmp/dirs"
while IFS= read -r d; do
    # the console's server lists with LIST only (no NLST): "size<TAB>name" for each file
    curl -s "$(url "$d")/" 2>/dev/null | tr -d '\r' |
        awk '$1 ~ /^-/ { s = $5; for (i = 1; i <= 8; i++) $i = ""; sub(/^ +/, ""); print s "\t" $0 }' > "$tmp/remote" || true
    find "$d" -maxdepth 1 -type f | sort > "$tmp/files"
    while IFS= read -r f; do
        if echo "$f" | grep -Eq "$never"; then continue; fi
        if echo "$f" | grep -Eq "$always" || ! grep -qxF "$(wc -c < "$f" | tr -d ' ')	$(basename "$f")" "$tmp/remote"; then
            echo "  $f"
            if cut -f2 "$tmp/remote" | grep -qxF "$(basename "$f")"; then
                put "$f" || { echo "failed: $f"; exit 1; }
            else
                batch+=(-T "$f" "$(url "$f")")
                [ ${#batch[@]} -lt 120 ] || flush
            fi
            sent=$((sent + 1))
        else
            kept=$((kept + 1))
        fi
    done < "$tmp/files"
    flush
done < "$tmp/dirs"
# The home screen draws a title's art from the copies made when the title was installed
# (/user/app/<title>/icon0.png and sce_sys, /user/appmeta/<title>): new art in sce_sys shows only
# once those are replaced too (and the PS5 restarted)
for t in $titles; do
    for f in icon0.png pic0.dds pic1.dds; do
        [ -f "$t/sce_sys/$f" ] || continue
        for d in "/user/appmeta/$t" "/user/app/$t/sce_sys"; do
            put_abs "$t/sce_sys/$f" "$d/$f" || echo "  (could not refresh $d/$f)"
        done
    done
    [ -f "$t/sce_sys/icon0.png" ] && { put_abs "$t/sce_sys/icon0.png" "/user/app/$t/icon0.png" || echo "  (could not refresh /user/app/$t/icon0.png)"; }
done
echo "Sent $sent file(s) to the PS5 at $host ($kept already there). Start Perfect Dark from the home screen."
