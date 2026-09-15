#!/usr/bin/env bash
# Fumee OS-UI contre une origine deja lancee (conteneur ou python3 osui/server.py).
# Usage : BASE_URL=http://127.0.0.1:8080 ./scripts/smoke.sh
set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
admin_hdr=()

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
  local extra=("${admin_hdr[@]}")
  if [ -n "${REQ_ORIGIN:-}" ]; then
    extra+=(-H "Origin: ${REQ_ORIGIN}")
  fi
  if [ -n "${data}" ]; then
    curl -sS -o /tmp/mohhdy-osui-smoke-body -w "%{http_code}" -X "${method}" \
      -H 'Content-Type: application/json' \
      "${extra[@]}" \
      -d "${data}" \
      "${BASE_URL}${path}"
  else
    curl -sS -o /tmp/mohhdy-osui-smoke-body -w "%{http_code}" -X "${method}" \
      "${extra[@]}" \
      "${BASE_URL}${path}"
  fi
}

echo "OS-UI-0/1/2 smoke contre ${BASE_URL}"

shell="$(fetch /)"
echo "${shell}" | grep -q 'id="os-topbar"' || fail "shell topbar"
echo "${shell}" | grep -q 'id="os-desktop"' || fail "shell desktop"
echo "${shell}" | grep -q 'id="os-chat"' || fail "chat central"
echo "${shell}" | grep -q 'data-mode="center"' || fail "chat mode center"
echo "${shell}" | grep -q 'id="os-slash-registry"' || fail "registre slash"
echo "${shell}" | grep -q 'data-slash="/help"' || fail "/help"
echo "${shell}" | grep -q 'data-slash="/browser"' || fail "/browser"
echo "${shell}" | grep -q 'data-slash="/shell"' || fail "/shell"
echo "${shell}" | grep -q 'data-drag="chat"' || fail "affordance drag chat"
echo "${shell}" | grep -q 'data-pane="browser"' || fail "pane Browser-OS"
echo "${shell}" | grep -q 'data-pane="support"' || fail "pane Support"
echo "${shell}" | grep -q 'data-pane="admin"' || fail "pane Admin"
echo "${shell}" | grep -q 'data-pane="status"' || fail "pane Status"
echo "${shell}" | grep -q 'data-pane="shell"' || fail "pane Shell OS"
echo "${shell}" | grep -q 'data-pane="fs"' || fail "pane FS"
echo "${shell}" | grep -q 'llm=stub_echo' || fail "shell llm stub"
echo "${shell}" | grep -q 'phase3_complete=false' || fail "shell phase3"
echo "${shell}" | grep -q 'us031_complete=false' || fail "shell us031"
if echo "${shell}" | grep -qi 'api_key'; then
  fail "shell ne doit pas contenir api_key"
fi

health="$(fetch /health)"
echo "${health}" | grep -q 'mohhdy-os' || fail "health service"
echo "${health}" | grep -q '"shell":"osui"' || echo "${health}" | grep -q '"shell": "osui"' || fail "health shell"
echo "${health}" | grep -q 'stub_echo' || fail "health llm"
echo "${health}" | grep -q '"phase3_complete":false' || echo "${health}" | grep -q '"phase3_complete": false' || fail "phase3"
echo "${health}" | grep -q '"us031_complete":false' || echo "${health}" | grep -q '"us031_complete": false' || fail "us031"
echo "${health}" | grep -q '"chromium_session_engine":false' || echo "${health}" | grep -q '"chromium_session_engine": false' || fail "chromium flag"
echo "${health}" | grep -q 'center_chat' || fail "interaction center_chat"
echo "${health}" | grep -q '"slash":"/help"' || echo "${health}" | grep -q '"slash": "/help"' || fail "commands /help"
echo "${health}" | grep -q '"slash":"/browser"' || echo "${health}" | grep -q '"slash": "/browser"' || fail "commands /browser"

curl -fsS "${BASE_URL}/os/os.js" -o /tmp/mohhdy-osui-smoke-js
grep -q 'setChatMode("float")' /tmp/mohhdy-osui-smoke-js || fail "js float on open"
grep -q 'mohhdy.os.chat.pos' /tmp/mohhdy-osui-smoke-js || fail "js persist position"
grep -q 'window.MohhdyOS' /tmp/mohhdy-osui-smoke-js || fail "js API MohhdyOS"

curl -fsS "${BASE_URL}/os/os.css" -o /tmp/mohhdy-osui-smoke-css
grep -q 'data-mode="float"' /tmp/mohhdy-osui-smoke-css || fail "css float chat"
grep -q -- '--os-chat-z' /tmp/mohhdy-osui-smoke-css || fail "css chat z-index"

create="$(curl -fsS -X POST -H 'Content-Type: application/json' \
  -d '{"site_id":"osui_smoke"}' "${BASE_URL}/api/sessions")"
sid="$(printf '%s' "${create}" | python3 -c 'import json,sys; print(json.load(sys.stdin)["session_id"])')"
[ -n "${sid}" ] || fail "session_id manquant"

if [ -n "${ADMIN_TOKEN:-}" ]; then
  admin_hdr=(-H "Authorization: Bearer ${ADMIN_TOKEN}")
fi
listing="$(curl -fsS "${admin_hdr[@]}" "${BASE_URL}/api/admin/sessions")"
echo "${listing}" | grep -q "${sid}" || fail "admin liste session"

REQ_ORIGIN="https://evil.example"
code="$(http_code POST "/api/sessions" '{"site_id":"osui_smoke"}')"
unset REQ_ORIGIN
[ "${code}" = "403" ] || fail "origine etrangere doit etre 403 (got ${code})"
grep -q 'origin_denied' /tmp/mohhdy-osui-smoke-body || fail "sans origin_denied"
grep -q 'request_id' /tmp/mohhdy-osui-smoke-body || fail "sans request_id"

curl -fsS "${admin_hdr[@]}" -X POST -H 'Content-Type: application/json' \
  -d '{"grant":["dom.click","mcp.invoice.create"]}' \
  "${BASE_URL}/api/admin/sessions/${sid}/capabilities" >/dev/null

origin_payload="$(printf '{"tool":"dom.click","origin":"%s","args":{"selector":"#menu-toggle"}}' "${BASE_URL}")"
code="$(http_code POST "/api/sessions/${sid}/tools" "${origin_payload}")"
[ "${code}" = "200" ] || fail "geste allowliste doit etre 200 (got ${code})"
grep -q 'dom_simulator' /tmp/mohhdy-osui-smoke-body || fail "geste sans simulateur"

inv_payload="$(printf '{"tool":"mcp.invoice.create","origin":"%s","args":{"customer":"OSUI","amount":"10.00"}}' "${BASE_URL}")"
code="$(http_code POST "/api/sessions/${sid}/tools" "${inv_payload}")"
[ "${code}" = "200" ] || fail "facture accordee doit etre 200 (got ${code})"

echo "OK osui chat-center slash-registry float-drag session origin-deny admin-list gesture invoice"
