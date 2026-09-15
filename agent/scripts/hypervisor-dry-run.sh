#!/usr/bin/env bash
# Fumee / dry-run hyperviseur ASSIST-052.
# Valide les recettes QEMU/KVM cloud-init et compose-sur-VM.
# Ne telecharge aucune image. Ne boote pas mohhdy.bin. Hors make ci.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
AGENT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${AGENT_DIR}/.." && pwd)"
PACK="${AGENT_DIR}/packaging"

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

[ -f "${PACK}/cloud-init/user-data" ] || fail "user-data manquant"
[ -f "${PACK}/cloud-init/meta-data" ] || fail "meta-data manquant"
[ -f "${PACK}/qemu-agent.cmd" ] || fail "qemu-agent.cmd manquant"
[ -f "${PACK}/mohhdy-agent.service" ] || fail "unit systemd manquant"
[ -f "${AGENT_DIR}/docker-compose.yml" ] || fail "docker-compose.yml manquant"
[ -f "${AGENT_DIR}/scripts/qemu-agent.sh" ] || fail "qemu-agent.sh manquant"

head -n 1 "${PACK}/cloud-init/user-data" | grep -q '^#cloud-config' \
  || fail "user-data doit commencer par #cloud-config"

grep -q 'mohhdy-agent' "${PACK}/cloud-init/meta-data" \
  || fail "meta-data sans hostname d'instance"

grep -q 'hostfwd=tcp::8080-:8080' "${PACK}/qemu-agent.cmd" \
  || fail "qemu-agent.cmd doit publier 8080"
grep -q 'qemu-system-x86_64' "${PACK}/qemu-agent.cmd" \
  || fail "ASSIST-052 utilise qemu-system-x86_64, pas i386"

# Ne pas confondre avec l'ISO GRUB du prototype : les recettes
# peuvent l'interdire, mais ne doivent pas booter ces artefacts.
if grep -E '\-(kernel|drive|cdrom)[= ].*mohhdy\.(bin|iso)|qemu-system-i386 ' \
  "${PACK}/qemu-agent.cmd" "${AGENT_DIR}/scripts/qemu-agent.sh" \
  "${PACK}/cloud-init/user-data"; then
  fail "recette hyperviseur agent ne doit pas booter mohhdy.bin / ISO AOS / i386"
fi
grep -q 'mohhdy.bin' "${PACK}/qemu-agent.cmd" \
  || fail "qemu-agent.cmd doit distinguer le prototype (mohhdy.bin)"

secret_hits="$(grep -RniE 'sk-proj|BEGIN PRIVATE KEY|sk_live|sk_test|password=.+|api_key=.+' \
  "${PACK}" "${AGENT_DIR}/scripts/qemu-agent.sh" "${AGENT_DIR}/scripts/hypervisor-dry-run.sh" \
  || true)"
if [ -n "${secret_hits}" ]; then
  echo "${secret_hits}" >&2
  fail "secret detecte dans le packaging hyperviseur"
fi

# Compose-sur-VM : meme fichier que Docker local.
grep -q 'mohhdy-agent:' "${AGENT_DIR}/docker-compose.yml" \
  || fail "compose sans service mohhdy-agent"

if command -v qemu-system-x86_64 >/dev/null 2>&1; then
  qemu-system-x86_64 -version | head -n 1
else
  echo "NOTE: qemu-system-x86_64 absent ; dry-run fichiers seulement."
fi

echo "Commande QEMU documentee :"
grep -E '^# qemu-system-x86_64' -A 20 "${PACK}/qemu-agent.cmd" | sed 's/^# //'
echo
"${AGENT_DIR}/scripts/qemu-agent.sh" --dry-run

echo "OK ASSIST-052 dry-run (pas de boot, pas d'image cloud, pas de mohhdy.bin)"
echo "Repo : ${REPO_ROOT}"
