#!/usr/bin/env bash
# Fumee optionnelle contre une origine deja lancee (conteneur ou python3).
# Usage : BASE_URL=http://127.0.0.1:8080 ./scripts/smoke.sh
# Si l'origine a ADMIN_TOKEN, exporter la meme valeur.
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

echo "ASSIST-010/011/040 smoke contre ${BASE_URL}"

health="$(fetch /health)"
echo "${health}" | grep -q 'mohhdy-agent' || fail "health service"
echo "${health}" | grep -q '"status":"ok"' || echo "${health}" | grep -q '"status": "ok"' || fail "health status"

admin="$(fetch /admin)"
echo "${admin}" | grep -q 'mohhdy-sessions' || fail "admin sessions"
echo "${admin}" | grep -q 'ADMIN_TOKEN' || fail "admin token mention"

embed="$(fetch /embed.js)"
echo "${embed}" | grep -q 'mohhdy-launcher' || fail "embed launcher"
echo "${embed}" | grep -q '/api/sessions' || fail "embed talks to sessions"
if echo "${embed}" | grep -qi 'api_key'; then
  fail "embed ne doit pas contenir api_key"
fi
if echo "${embed}" | grep -q 'BEGIN PRIVATE KEY'; then
  fail "embed ne doit pas contenir de cle"
fi

demo="$(fetch /demo)"
echo "${demo}" | grep -q '/embed.js' || fail "demo charge embed.js"

create_a="$(curl -fsS -X POST -H 'Content-Type: application/json' \
  -d '{"site_id":"smoke_a"}' "${BASE_URL}/api/sessions")"
create_b="$(curl -fsS -X POST -H 'Content-Type: application/json' \
  -d '{"site_id":"smoke_b"}' "${BASE_URL}/api/sessions")"
sid_a="$(printf '%s' "${create_a}" | python3 -c 'import json,sys; print(json.load(sys.stdin)["session_id"])')"
sid_b="$(printf '%s' "${create_b}" | python3 -c 'import json,sys; print(json.load(sys.stdin)["session_id"])')"
[ -n "${sid_a}" ] && [ -n "${sid_b}" ] || fail "session_id manquant"
[ "${sid_a}" != "${sid_b}" ] || fail "session_id identiques"

curl -fsS -X POST -H 'Content-Type: application/json' \
  -d '{"content":"secret-alpha"}' \
  "${BASE_URL}/api/sessions/${sid_a}/messages" >/dev/null
curl -fsS -X POST -H 'Content-Type: application/json' \
  -d '{"content":"secret-beta"}' \
  "${BASE_URL}/api/sessions/${sid_b}/messages" >/dev/null

got_a="$(fetch "/api/sessions/${sid_a}")"
got_b="$(fetch "/api/sessions/${sid_b}")"
echo "${got_a}" | grep -q 'secret-alpha' || fail "message A absent"
if echo "${got_a}" | grep -q 'secret-beta'; then
  fail "fuite B vers A"
fi
echo "${got_b}" | grep -q 'secret-beta' || fail "message B absent"
if echo "${got_b}" | grep -q 'secret-alpha'; then
  fail "fuite A vers B"
fi

admin_hdr=()
if [ -n "${ADMIN_TOKEN:-}" ]; then
  admin_hdr=(-H "Authorization: Bearer ${ADMIN_TOKEN}")
fi
listing="$(curl -fsS "${admin_hdr[@]}" "${BASE_URL}/api/admin/sessions")"
echo "${listing}" | grep -q "${sid_a}" || fail "admin liste A"
echo "${listing}" | grep -q "${sid_b}" || fail "admin liste B"

echo "OK health admin embed.js demo sessions isolation"
