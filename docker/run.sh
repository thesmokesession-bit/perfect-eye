#!/usr/bin/env bash
# Runs a command in the build container with the project mounted at the same path.
# The host's proxy and CA bundle are passed through when the host has them.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
extra=()
for v in HTTPS_PROXY https_proxy HTTP_PROXY http_proxy NO_PROXY no_proxy; do
    [[ -n ${!v:-} ]] && extra+=(-e "$v=${!v}")
done
[[ -n ${PD_CA_BUNDLE:-} && -f $PD_CA_BUNDLE ]] && extra+=(-v "$PD_CA_BUNDLE:/etc/ssl/certs/ca-certificates.crt:ro"
    -e SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt -e GIT_SSL_CAINFO=/etc/ssl/certs/ca-certificates.crt)
exec env DOCKER_CONFIG="$root/docker/.dockercfg" docker run --rm --network host -u "$(id -u):$(id -g)" "${extra[@]}" \
    -v "$root:$root" ${PACBREW:+-v "$PACBREW:$PACBREW:ro"} -w "${WORKDIR:-$root}" -e HOME="$root/.home" -e CCACHE_DIR="$root/.home/ccache" \
    pd-ps5-build bash -lc "$*"
