#!/usr/bin/env bash
# Sends dist/ (tools/package.sh) to a console over FTP: tools/ps5-push.command, which asks for the
# console's address once (PS5_HOST overrides it) and sends the game's files plus whatever the
# console is missing. PS5_DIR: /data/homebrew by default.
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
exec "$root/tools/ps5-push.command" "$root/dist"
