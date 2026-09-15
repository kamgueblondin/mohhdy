#!/usr/bin/env bash
# Demarre l'instance Mohhdy copiee par install.sh (premier plan).
# Meme variables d'environnement que Docker ASSIST-050.
set -euo pipefail

PREFIX="${1:-}"
if [ -z "${PREFIX}" ]; then
  if [ -f ./server.py ]; then
    PREFIX="."
  else
    PREFIX="/opt/mohhdy-agent"
  fi
fi

[ -f "${PREFIX}/server.py" ] || {
  echo "FAIL: ${PREFIX}/server.py introuvable" >&2
  exit 1
}

export MOHHDY_AGENT_HOST="${MOHHDY_AGENT_HOST:-0.0.0.0}"
export MOHHDY_AGENT_PORT="${MOHHDY_AGENT_PORT:-8080}"
cd "${PREFIX}"
exec python3 server.py
