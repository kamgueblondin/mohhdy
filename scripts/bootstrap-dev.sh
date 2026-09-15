#!/usr/bin/env bash
# Installe les paquets nécessaires pour compiler MOHHDY et lancer QEMU (même
# ensemble que .github/workflows/ci.yml). Idempotent. À lancer depuis n'importe
# quel répertoire.
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
    SUDO=sudo
else
    SUDO=
fi

export DEBIAN_FRONTEND=noninteractive

echo "[bootstrap-dev] apt-get update..."
${SUDO} apt-get update -qq

echo "[bootstrap-dev] paquets de compilation + QEMU i386..."
${SUDO} apt-get install -y --no-install-recommends \
    build-essential \
    gcc-multilib \
    libc6-dev-i386 \
    nasm \
    qemu-system-x86 \
    python3

echo "[bootstrap-dev] OK. Ensuite : make all && make test-all"
echo "[bootstrap-dev] Optionnel ISO : sudo apt-get install -y grub-pc-bin xorriso"
echo "[bootstrap-dev] Optionnel GPT-2 : voir README.md (models/ depuis la release gpt2-124m-assets)"
