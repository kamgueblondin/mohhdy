#!/usr/bin/env bash
# Fumee optionnelle contre une origine deja lancee (conteneur ou python3).
# Usage : BASE_URL=http://127.0.0.1:8080 ./scripts/smoke.sh
set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

fetch() {
  local path="$1"
  curl -fsS "${BASE_URL}${path}"
}

echo "ASSIST-050 smoke contre ${BASE_URL}"

health="$(fetch /health)"
echo "${health}" | grep -q 'mohhdy-agent' || fail "health service"
echo "${health}" | grep -q '"status":"ok"' || echo "${health}" | grep -q '"status": "ok"' || fail "health status"

admin="$(fetch /admin)"
echo "${admin}" | grep -q 'mohhdy-sessions' || fail "admin sessions"
echo "${admin}" | grep -q 'Aucune session' || fail "admin placeholder"

embed="$(fetch /embed.js)"
echo "${embed}" | grep -q 'mohhdy-launcher' || fail "embed launcher"
if echo "${embed}" | grep -qi 'api_key'; then
  fail "embed ne doit pas contenir api_key"
fi
if echo "${embed}" | grep -q 'BEGIN PRIVATE KEY'; then
  fail "embed ne doit pas contenir de cle"
fi

demo="$(fetch /demo)"
echo "${demo}" | grep -q '/embed.js' || fail "demo charge embed.js"

echo "OK health admin embed.js demo"
