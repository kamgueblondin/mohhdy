#!/usr/bin/env bash
# Lance (ou imprime) QEMU x86_64 pour le runtime agent (ASSIST-052).
# Distinct du prototype : jamais build/mohhdy.bin, jamais qemu-system-i386.
#
# Defaut : --dry-run (aucune image, aucun reseau payant).
# Boot optionnel si l'operateur fournit une cloud image Linux :
#   CLOUD_IMAGE=/chemin/disk.qcow2 SEED_ISO=/chemin/seed.iso \
#     agent/scripts/qemu-agent.sh --boot
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
AGENT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PACK="${AGENT_DIR}/packaging/cloud-init"

DRY_RUN=1
DO_BOOT=0
MEM="${MOHHDY_AGENT_VM_MEM:-1024}"
CPUS="${MOHHDY_AGENT_VM_CPUS:-2}"
SSH_FWD="${MOHHDY_AGENT_VM_SSH:-2222}"
HTTP_FWD="${MOHHDY_AGENT_VM_HTTP:-8080}"
CLOUD_IMAGE="${CLOUD_IMAGE:-}"
SEED_ISO="${SEED_ISO:-}"

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

while [ $# -gt 0 ]; do
  case "$1" in
    --dry-run) DRY_RUN=1 ; DO_BOOT=0 ;;
    --boot) DRY_RUN=0 ; DO_BOOT=1 ;;
    --image)
      CLOUD_IMAGE="${2:-}"
      shift
      ;;
    --seed)
      SEED_ISO="${2:-}"
      shift
      ;;
    -h|--help)
      sed -n '1,12p' "$0"
      exit 0
      ;;
    *)
      fail "option inconnue: $1"
      ;;
  esac
  shift
done

build_cmd() {
  local image="${1:-DISK.qcow2}"
  local seed="${2:-}"
  echo "qemu-system-x86_64 \\"
  echo "  -machine q35,accel=kvm:tcg \\"
  echo "  -m ${MEM} -smp ${CPUS} \\"
  echo "  -drive if=virtio,file=${image},format=qcow2 \\"
  if [ -n "${seed}" ]; then
    echo "  -drive if=virtio,file=${seed},format=raw,readonly=on \\"
  fi
  echo "  -netdev user,id=net0,hostfwd=tcp::${HTTP_FWD}-:8080,hostfwd=tcp::${SSH_FWD}-:22 \\"
  echo "  -device virtio-net-pci,netdev=net0 \\"
  echo "  -nographic"
}

echo "ASSIST-052 QEMU agent (x86_64 Linux). Pas mohhdy.bin."
echo "cloud-init : ${PACK}/user-data + ${PACK}/meta-data"
echo "Compose sur VM : cd agent && docker compose up --build"
echo

if [ "${DRY_RUN}" -eq 1 ]; then
  echo "Dry-run (aucune image n'est lue) :"
  build_cmd "${CLOUD_IMAGE:-/var/lib/libvirt/images/mohhdy-agent.qcow2}" "${SEED_ISO:-cidata.iso}"
  echo "Seed NoCloud : genisoimage -output cidata.iso -volid cidata -joliet -rock ${PACK}/user-data ${PACK}/meta-data"
  exit 0
fi

[ "${DO_BOOT}" -eq 1 ] || fail "ni --dry-run ni --boot"
[ -n "${CLOUD_IMAGE}" ] && [ -f "${CLOUD_IMAGE}" ] \
  || fail "CLOUD_IMAGE manquante (fournir une cloud image Linux, pas mohhdy.bin)"
case "${CLOUD_IMAGE}" in
  *mohhdy.bin*|*mohhdy.iso*)
    fail "refuser le binaire / ISO du prototype AOS"
    ;;
esac
command -v qemu-system-x86_64 >/dev/null 2>&1 || fail "qemu-system-x86_64 introuvable"

cmd=(qemu-system-x86_64
  -machine q35,accel=kvm:tcg
  -m "${MEM}" -smp "${CPUS}"
  -drive "if=virtio,file=${CLOUD_IMAGE},format=qcow2"
  -netdev "user,id=net0,hostfwd=tcp::${HTTP_FWD}-:8080,hostfwd=tcp::${SSH_FWD}-:22"
  -device virtio-net-pci,netdev=net0
  -nographic
)
if [ -n "${SEED_ISO}" ]; then
  [ -f "${SEED_ISO}" ] || fail "SEED_ISO introuvable: ${SEED_ISO}"
  cmd+=(-drive "if=virtio,file=${SEED_ISO},format=raw,readonly=on")
fi
echo "Boot : ${cmd[*]}"
exec "${cmd[@]}"
