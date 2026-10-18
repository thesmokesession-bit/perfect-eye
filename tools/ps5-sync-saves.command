#!/bin/bash
# Perfect Dark PS5: keeps the saves the same on the PS5 and a PC copy of the game (the Dab's Mod
# build of the same version): eeprom.bin (agent files, unlocks, the Combat Simulator's setups) and
# GoldenEye's best times. Close the game on both before running it. Double-click it on a Mac, or run
#   ./ps5-sync-saves.command [the PC copy's save folder]
# The save folder is the one pd.log calls "save dir"; without one this looks in the usual places.
# The PS5's address is asked once and kept in .ps5-host beside this file (PS5_HOST overrides it).
#
# A file changed in one place since the last sync goes everywhere. Changed in more than one place,
# the newest wins (or you pick, when this runs in a terminal). Every copy it replaces is kept in
# <save folder>/.ps5-sync/backups first.
set -eu
here=$(cd -- "$(dirname -- "$0")" && pwd)
dir=${PS5_DIR:-/data/homebrew}
titles=${PS5_TITLES:-PPSA64007 PPSA64008}
hostfile="$here/.ps5-host"
host=${PS5_HOST:-$(cat "$hostfile" 2>/dev/null || true)}
if [ -z "$host" ]; then
    printf "PS5 address and FTP port (for example 192.168.1.20:1337): "
    read -r host
    [ -n "$host" ] || { echo "no address given"; exit 1; }
    echo "$host" > "$hostfile"
fi

pc=${1:-}
if [ -z "$pc" ]; then
    for d in "$here" "$here"/* "$here"/*/* "$HOME/Library/Application Support/perfectdark" "$HOME/.local/share/perfectdark"; do
        if [ -f "$d/eeprom.bin" ] && [ ! -d "$d/sce_sys" ]; then pc=$d; break; fi
    done
fi
[ -n "$pc" ] && [ -d "$pc" ] || { echo "Which folder holds the PC copy's saves? Run: $0 <folder> (pd.log's \"save dir\")"; exit 1; }
pc=$(cd "$pc" && pwd)
state="$pc/.ps5-sync"
mkdir -p "$state/backups"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
ftp="ftp://$host$dir"
mtime() { stat -c %Y "$1" 2>/dev/null || stat -f %m "$1"; }
sha() { shasum -a 256 "$1" | cut -d' ' -f1; }
put() {
    curl -sS -T "$1" "$ftp/$2.new" &&
    curl -sS "ftp://$host/" -Q "*DELE $dir/$2" -Q "RNFR $dir/$2.new" -Q "RNTO $dir/$2" -o /dev/null
}

live=""
for t in $titles; do
    if curl -s --connect-timeout 5 -l "$ftp/$t/" 2>/dev/null | tr -d '\r' > "$tmp/$t.ls"; then
        live="$live $t"
        mkdir -p "$tmp/$t"
    fi
done
[ -n "$live" ] || { echo "no Perfect Dark folder on the PS5 at $host (wrong address in $hostfile?)"; exit 1; }
echo "PC saves: $pc"
echo "PS5: $host,$live"

stamp=$(date +%Y%m%d-%H%M%S)-$$
files=$( (ls "$pc"; cat "$tmp"/*.ls) | grep -E '^(eeprom\.bin|[A-Za-z0-9]+-times\.txt)$' | sort -u)
for f in $files; do
    # every copy there is: "label path"
    : > "$tmp/copies"
    [ -f "$pc/$f" ] && echo "PC $pc/$f" >> "$tmp/copies"
    for t in $live; do
        if grep -qxF "$f" "$tmp/$t.ls" && curl -sfR -o "$tmp/$t/$f" "$ftp/$t/$f"; then
            echo "$t $tmp/$t/$f" >> "$tmp/copies"
        fi
    done

    base=$(cat "$state/$f.sha" 2>/dev/null || true)
    # the copies changed since the last sync, one line per different content
    : > "$tmp/changed"
    while read -r label path; do
        h=$(sha "$path")
        [ "$h" = "$base" ] && continue
        grep -q " $h " "$tmp/changed" 2>/dev/null && continue
        echo "$(mtime "$path") $label $h $path" >> "$tmp/changed"
    done < "$tmp/copies"

    n=$(wc -l < "$tmp/changed" | tr -d ' ')
    if [ "$n" -eq 0 ]; then
        echo "$f: the same everywhere"
        continue
    fi
    sort -rn "$tmp/changed" -o "$tmp/changed"
    pick=1
    if [ "$n" -gt 1 ] && [ -t 0 ]; then
        echo "$f changed in more than one place since the last sync:"
        i=1
        while read -r when label h path; do
            echo "  $i) $label, saved $(date -r "$when" 2>/dev/null || date -d "@$when")"
            i=$((i + 1))
        done < "$tmp/changed"
        printf "Keep which? [1, the newest] "
        read -r pick < /dev/tty || pick=1
        case $pick in ''|*[!0-9]*) pick=1 ;; esac
        [ "$pick" -ge 1 ] && [ "$pick" -le "$n" ] || pick=1
    fi
    read -r _ wlabel wh win <<EOF
$(sed -n "${pick}p" "$tmp/changed")
EOF

    # the PC copy, then each title folder: back up whatever is replaced, then replace it
    if [ -f "$pc/$f" ] && [ "$(sha "$pc/$f")" != "$wh" ]; then
        cp -p "$pc/$f" "$state/backups/$f.PC-$stamp"
    fi
    if [ ! -f "$pc/$f" ] || [ "$(sha "$pc/$f")" != "$wh" ]; then
        cp "$win" "$pc/$f.new" && mv "$pc/$f.new" "$pc/$f"
    fi
    for t in $live; do
        if [ -f "$tmp/$t/$f" ]; then
            [ "$(sha "$tmp/$t/$f")" = "$wh" ] && continue
            cp -p "$tmp/$t/$f" "$state/backups/$f.$t-$stamp"
        fi
        put "$win" "$t/$f" || { echo "failed to send $f to $t"; exit 1; }
    done
    echo "$wh" > "$state/$f.sha"
    echo "$f: the copy from $wlabel, everywhere now"
done
echo "Saves in sync."
