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

http_code() {
  local method="$1"
  local path="$2"
  local data="${3:-}"
  if [ -n "${data}" ]; then
    curl -sS -o /tmp/mohhdy-smoke-body -w "%{http_code}" -X "${method}" \
      -H 'Content-Type: application/json' \
      "${admin_hdr[@]}" \
      -d "${data}" \
      "${BASE_URL}${path}"
  else
    curl -sS -o /tmp/mohhdy-smoke-body -w "%{http_code}" -X "${method}" \
      "${admin_hdr[@]}" \
      "${BASE_URL}${path}"
  fi
}

echo "ASSIST-012/030/031/041 smoke contre ${BASE_URL}"

health="$(fetch /health)"
echo "${health}" | grep -q 'mohhdy-agent' || fail "health service"
echo "${health}" | grep -q '"status":"ok"' || echo "${health}" | grep -q '"status": "ok"' || fail "health status"

admin="$(fetch /admin)"
echo "${admin}" | grep -q 'mohhdy-sessions' || fail "admin sessions"
echo "${admin}" | grep -q 'ADMIN_TOKEN' || fail "admin token mention"
echo "${admin}" | grep -q 'File humain' || fail "admin file humain"
echo "${admin}" | grep -q 'Prendre la main' || fail "admin takeover"

embed="$(fetch /embed.js)"
echo "${embed}" | grep -q 'mohhdy-launcher' || fail "embed launcher"
echo "${embed}" | grep -q '/api/sessions' || fail "embed talks to sessions"
echo "${embed}" | grep -q 'Parler a un humain' || fail "embed escalate button"
echo "${embed}" | grep -q 'human_active' || fail "embed handoff status"
if echo "${embed}" | grep -qi 'api_key'; then
  fail "embed ne doit pas contenir api_key"
fi
if echo "${embed}" | grep -q 'BEGIN PRIVATE KEY'; then
  fail "embed ne doit pas contenir de cle"
fi
if echo "${embed}" | grep -q 'admin.takeover'; then
  fail "embed ne doit pas exposer admin.takeover"
fi
if echo "${embed}" | grep -q 'acl\.'; then
  fail "embed ne doit pas exposer un prefixe acl"
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

printf '%s' "${create_a}" | grep -q 'admin.takeover' && fail "widget session A expose admin.takeover"
printf '%s' "${create_a}" | grep -q 'acl\.' && fail "widget session A expose acl"

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

code="$(http_code POST "/api/sessions/${sid_a}/tools" '{"tool":"dom.click"}')"
[ "${code}" = "403" ] || fail "outil revoque/absent doit etre 403 (got ${code})"
grep -q 'capability_denied' /tmp/mohhdy-smoke-body || fail "outil refuse sans capability_denied"

curl -fsS "${admin_hdr[@]}" -X POST -H 'Content-Type: application/json' \
  -d '{"revoke":["dom.click"]}' \
  "${BASE_URL}/api/admin/sessions/${sid_b}/capabilities" >/dev/null
code="$(http_code POST "/api/sessions/${sid_b}/tools" '{"tool":"dom.click"}')"
[ "${code}" = "403" ] || fail "revoke dom.click doit rester 403"

curl -fsS -X POST -H 'Content-Type: application/json' \
  -d '{"reason":"smoke humain"}' \
  "${BASE_URL}/api/sessions/${sid_a}/escalate" >/dev/null
got_a="$(fetch "/api/sessions/${sid_a}")"
echo "${got_a}" | grep -q 'waiting_human' || fail "session A pas waiting_human"

queue="$(curl -fsS "${admin_hdr[@]}" "${BASE_URL}/api/admin/sessions?status=waiting_human")"
echo "${queue}" | grep -q "${sid_a}" || fail "file humain sans session A"
if echo "${queue}" | grep -q "${sid_b}"; then
  # B a aussi ete force-escaladee par l'outil refuse
  true
fi

taken="$(curl -fsS "${admin_hdr[@]}" -X POST -H 'Content-Type: application/json' \
  -d '{}' "${BASE_URL}/api/admin/sessions/${sid_a}/takeover")"
echo "${taken}" | grep -q "${sid_a}" || fail "takeover session_id"
echo "${taken}" | grep -q 'human_active' || fail "takeover human_active"

human="$(curl -fsS "${admin_hdr[@]}" -X POST -H 'Content-Type: application/json' \
  -d '{"content":"reponse humaine smoke"}' \
  "${BASE_URL}/api/admin/sessions/${sid_a}/messages")"
echo "${human}" | grep -q 'reponse humaine smoke' || fail "message humain absente"

visitor="$(fetch "/api/sessions/${sid_a}")"
echo "${visitor}" | grep -q 'reponse humaine smoke' || fail "widget ne voit pas l'humain"
echo "${visitor}" | grep -q "${sid_a}" || fail "handoff doit garder le meme session_id"
echo "${visitor}" | grep -q 'human_active' || fail "visitor status human_active"

after="$(curl -fsS -X POST -H 'Content-Type: application/json' \
  -d '{"content":"merci smoke"}' \
  "${BASE_URL}/api/sessions/${sid_a}/messages")"
echo "${after}" | grep -q '"auto_reply":false' || echo "${after}" | grep -q '"auto_reply": false' || fail "auto_reply encore vrai"
if echo "${after}" | grep -q '"agent_message":{'; then
  fail "agent a repondu apres takeover"
fi

echo "OK health admin embed.js demo isolation revoke escalate handoff"
