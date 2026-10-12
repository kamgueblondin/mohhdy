#!/usr/bin/env bash
# Install the CI toolchain. A warm .apt-cache (actions/cache in ci.yml)
# installs with dpkg and does not call apt-get update. A slow mirror made
# `apt-get update` take 12 minutes on main run 38249068719 and the production
# job then hit timeout-minutes: 20 during make qemu-persistence.
# Epoch is part of this file: bump it to invalidate a bad cache key.
# epoch=1
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CACHE="${MOHHDY_APT_CACHE:-$ROOT/.apt-cache}"
export DEBIAN_FRONTEND=noninteractive

PKGS=(
    build-essential
    gcc-multilib
    libc6-dev-i386
    nasm
    python3-cryptography
    qemu-system-x86
)

tools_ok() {
    command -v nasm >/dev/null 2>&1 || return 1
    command -v qemu-system-i386 >/dev/null 2>&1 || return 1
    python3 -c 'import cryptography' >/dev/null 2>&1 || return 1
    echo 'int main(void){return 0;}' | gcc -m32 -x c - -o /tmp/mohhdy-m32-check >/dev/null 2>&1 || return 1
    rm -f /tmp/mohhdy-m32-check
}

start=$(date +%s)
if tools_ok; then
    echo "[ci-deps] toolchain already present ($(( $(date +%s) - start ))s)"
    exit 0
fi

shopt -s nullglob
debs=("$CACHE"/*.deb)
if [ "${#debs[@]}" -gt 0 ]; then
    echo "[ci-deps] dpkg -i ${#debs[@]} cached packages (no apt-get update)"
    if sudo dpkg -i "${debs[@]}" && tools_ok; then
        echo "[ci-deps] cache ok ($(( $(date +%s) - start ))s)"
        exit 0
    fi
    echo "[ci-deps] cache incomplete, falling back to apt-get"
fi

sudo tee /etc/apt/apt.conf.d/99mohhdy-ci >/dev/null <<'EOF'
Acquire::Retries "3";
Acquire::http::Timeout "30";
Acquire::https::Timeout "30";
Acquire::ftp::Timeout "30";
EOF

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    -o APT::Keep-Downloaded-Packages=true \
    "${PKGS[@]}"
mkdir -p "$CACHE"
archives=(/var/cache/apt/archives/*.deb)
if [ "${#archives[@]}" -gt 0 ]; then
    cp -f "${archives[@]}" "$CACHE"/
    echo "[ci-deps] saved ${#archives[@]} debs to $CACHE"
else
    echo "[ci-deps] apt kept no debs; next run will use apt-get again"
fi
tools_ok
echo "[ci-deps] apt-get ok ($(( $(date +%s) - start ))s)"
