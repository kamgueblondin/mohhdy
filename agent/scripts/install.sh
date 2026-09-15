#!/usr/bin/env bash
# Installation native du runtime agent (ASSIST-051).
# Linux primaire. Meme contrat HTTP que Docker ASSIST-050.
# Aucun secret. Hors make ci / hors QEMU i386.
#
# Usage :
#   agent/scripts/install.sh --check
#   agent/scripts/install.sh --prefix /opt/mohhdy-agent
#   PREFIX=/tmp/mohhdy-agent agent/scripts/install.sh --prefix "$PREFIX" --start --port 18080
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
AGENT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

PREFIX=""
DO_CHECK=0
DO_START=0
DO_SYSTEMD=0
HOST="127.0.0.1"
PORT="8080"
KEEP_PREFIX=0

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

usage() {
  cat <<'EOF'
install.sh : copie le runtime Python agent et optionnellement le demarre.

  --check          fumee : prefixe temporaire, copie, /health, arret
  --prefix DIR     destination (defaut pour --check : mktemp)
  --start          lance python3 server.py et attend /health
  --host ADDR      bind (defaut 127.0.0.1 ; 0.0.0.0 en service)
  --port N         port (defaut 8080)
  --systemd        copie le unit d'exemple dans PREFIX/packaging
  -h, --help       cette aide

Variables : ADMIN_TOKEN, MOHHDY_AGENT_CONFIG, MOHHDY_AGENT_SITE_ID,
MOHHDY_AGENT_MODE, MOHHDY_AGENT_RUNTIME, MOHHDY_AGENT_BROWSER_ENGINE,
MOHHDY_AGENT_DATA, MOHHDY_AGENT_KB (identiques a Docker).
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --check) DO_CHECK=1 ;;
    --prefix)
      PREFIX="${2:-}"
      shift
      ;;
    --start) DO_START=1 ;;
    --host)
      HOST="${2:-}"
      shift
      ;;
    --port)
      PORT="${2:-}"
      shift
      ;;
    --systemd) DO_SYSTEMD=1 ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "option inconnue: $1"
      ;;
  esac
  shift
done

command -v python3 >/dev/null 2>&1 || fail "python3 introuvable"
python3 -c 'import sys; raise SystemExit(0 if sys.version_info >= (3, 9) else 1)' \
  || fail "Python 3.9+ requis (3.12 recommande, comme l'image Docker)"

[ -f "${AGENT_DIR}/server.py" ] || fail "server.py introuvable dans ${AGENT_DIR}"
[ -f "${AGENT_DIR}/tools.py" ] || fail "tools.py introuvable"
[ -f "${AGENT_DIR}/browser_fs.py" ] || fail "browser_fs.py introuvable"
[ -f "${AGENT_DIR}/browser_engine.py" ] || fail "browser_engine.py introuvable"
[ -d "${AGENT_DIR}/static" ] || fail "static/ introuvable"
[ -f "${AGENT_DIR}/config.example.json" ] || fail "config.example.json introuvable"

if [ "${DO_CHECK}" -eq 1 ]; then
  if [ -z "${PREFIX}" ]; then
    PREFIX="$(mktemp -d /tmp/mohhdy-agent-install.XXXXXX)"
  else
    mkdir -p "${PREFIX}"
    KEEP_PREFIX=1
  fi
  DO_START=1
  HOST="${HOST:-127.0.0.1}"
  if [ "${PORT}" = "8080" ]; then
    PORT="$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()')"
  fi
elif [ -z "${PREFIX}" ]; then
  PREFIX="/opt/mohhdy-agent"
fi

mkdir -p "${PREFIX}"
PREFIX="$(cd "${PREFIX}" && pwd)"

copy_runtime() {
  local dest="$1"
  local py
  mkdir -p "${dest}/static"
  for py in "${AGENT_DIR}"/*.py; do
    cp -f "${py}" "${dest}/"
  done
  cp -f "${AGENT_DIR}/config.example.json" "${dest}/config.example.json"
  cp -a "${AGENT_DIR}/static/." "${dest}/static/"
  mkdir -p "${dest}/packaging"
  if [ -f "${AGENT_DIR}/packaging/mohhdy-agent.service" ]; then
    cp -f "${AGENT_DIR}/packaging/mohhdy-agent.service" "${dest}/packaging/"
  fi
  if [ -f "${AGENT_DIR}/packaging/mohhdy-agent.env.example" ]; then
    cp -f "${AGENT_DIR}/packaging/mohhdy-agent.env.example" "${dest}/packaging/"
  fi
  [ -f "${dest}/browser_engine.py" ] || fail "copie incomplete: browser_engine.py"
}

copy_runtime "${PREFIX}"
echo "Runtime copie vers ${PREFIX}"

if [ "${DO_SYSTEMD}" -eq 1 ]; then
  echo "Unit systemd d'exemple : ${PREFIX}/packaging/mohhdy-agent.service"
  echo "A adapter (User=, WorkingDirectory=) puis copier vers /etc/systemd/system/"
  echo "ADMIN_TOKEN uniquement dans /etc/mohhdy-agent.env (hors depot)."
fi

wait_health() {
  local base="$1"
  python3 - "$base" <<'PY'
import sys, time, urllib.request
base = sys.argv[1]
deadline = time.time() + 8
last = None
while time.time() < deadline:
    try:
        with urllib.request.urlopen(base + "/health", timeout=0.4) as resp:
            body = resp.read().decode("utf-8")
        if '"status":"ok"' not in body.replace(" ", "") and '"status": "ok"' not in body:
            raise RuntimeError("health sans status ok")
        if "mohhdy-agent" not in body:
            raise RuntimeError("health sans service")
        print(body)
        raise SystemExit(0)
    except Exception as exc:
        last = exc
        time.sleep(0.05)
print("serveur injoignable:", last, file=sys.stderr)
raise SystemExit(1)
PY
}

if [ "${DO_START}" -eq 1 ]; then
  export MOHHDY_AGENT_HOST="${HOST}"
  export MOHHDY_AGENT_PORT="${PORT}"
  python3 "${PREFIX}/server.py" &
  pid=$!
  cleanup() {
    kill "${pid}" 2>/dev/null || true
    wait "${pid}" 2>/dev/null || true
    if [ "${DO_CHECK}" -eq 1 ] && [ "${KEEP_PREFIX}" -eq 0 ]; then
      rm -rf "${PREFIX}"
    fi
  }
  trap cleanup EXIT
  wait_health "http://${HOST}:${PORT}"
  echo "OK native install : http://${HOST}:${PORT}/health (pid ${pid})"
  if [ "${DO_CHECK}" -eq 1 ]; then
    echo "ASSIST-051 --check : sante HTTP verifiee, arret du processus."
  else
    trap - EXIT
    wait "${pid}"
  fi
fi
