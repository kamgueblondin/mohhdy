#!/usr/bin/env bash
# Fumee optionnelle contre une origine deja lancee (conteneur ou python3).
# Usage : BASE_URL=http://127.0.0.1:8080 ./scripts/smoke.sh
# Si l'origine a ADMIN_TOKEN, exporter la meme valeur.
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
  if [ -n "${REQ_REFERER:-}" ]; then
    extra+=(-H "Referer: ${REQ_REFERER}")
  fi
  if [ -n "${data}" ]; then
    curl -sS -o /tmp/mohhdy-smoke-body -w "%{http_code}" -X "${method}" \
      -H 'Content-Type: application/json' \
      "${extra[@]}" \
      -d "${data}" \
      "${BASE_URL}${path}"
  else
    curl -sS -o /tmp/mohhdy-smoke-body -w "%{http_code}" -X "${method}" \
      "${extra[@]}" \
      "${BASE_URL}${path}"
  fi
}

echo "ASSIST-020/021/022/060/061 smoke contre ${BASE_URL}"

health="$(fetch /health)"
echo "${health}" | grep -q 'mohhdy-agent' || fail "health service"
echo "${health}" | grep -q '"status":"ok"' || echo "${health}" | grep -q '"status": "ok"' || fail "health status"
echo "${health}" | grep -q 'dom_simulator' || fail "health harness"

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
echo "${embed}" | grep -q 'origin_denied' || fail "embed sans origin_denied"
echo "${embed}" | grep -q 'location.origin' || fail "embed sans origin document"
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

demo_app="$(fetch /demo-app)"
echo "${demo_app}" | grep -q 'menu-toggle' || fail "demo-app menu"
echo "${demo_app}" | grep -q 'invoice-customer' || fail "demo-app formulaire"
echo "${demo_app}" | grep -q 'simulateur DOM' || fail "demo-app doit dire simulateur"

browser="$(fetch /browser)"
echo "${browser}" | grep -q 'ASSIST-060' || fail "page /browser"
echo "${browser}" | grep -q 'simulateur DOM' || fail "browser doit dire simulateur"
echo "${browser}" | grep -q 'US-031' || fail "browser doit citer US-031"
echo "${browser}" | grep -q 'Playwright' || fail "browser doit qualifier Playwright"

browser_fs_page="$(fetch /browser/fs)"
echo "${browser_fs_page}" | grep -q 'ASSIST-061' || fail "page /browser/fs"
echo "${browser_fs_page}" | grep -q 'ADMIN_TOKEN' || fail "fs UI sans ADMIN_TOKEN"

api_browser="$(fetch /api/browser)"
echo "${api_browser}" | grep -q '"harness":"dom_simulator"' || echo "${api_browser}" | grep -q '"harness": "dom_simulator"' || fail "api browser harness"
echo "${api_browser}" | grep -q '"phase3_complete":false' || echo "${api_browser}" | grep -q '"phase3_complete": false' || fail "phase3 doit rester false"
echo "${api_browser}" | grep -q '"us031_complete":false' || echo "${api_browser}" | grep -q '"us031_complete": false' || fail "us031 doit rester false"
echo "${api_browser}" | grep -q '/browser/fs' || fail "api browser urls fs"

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

REQ_ORIGIN="${BASE_URL}"
code="$(http_code POST "/api/sessions" '{"site_id":"smoke_same_origin"}')"
unset REQ_ORIGIN
[ "${code}" = "201" ] || fail "meme origine doit creer une session (got ${code})"
grep -q 'session_id' /tmp/mohhdy-smoke-body || fail "session same-origin sans session_id"
sid_same="$(python3 -c 'import json; print(json.load(open("/tmp/mohhdy-smoke-body"))["session_id"])')"
[ -n "${sid_same}" ] || fail "session_id same-origin vide"

REQ_ORIGIN="https://evil.example"
code="$(http_code POST "/api/sessions" '{"site_id":"smoke_same_origin"}')"
unset REQ_ORIGIN
[ "${code}" = "403" ] || fail "origine etrangere en create doit etre 403 (got ${code})"
grep -q 'origin_denied' /tmp/mohhdy-smoke-body || fail "create etrangere sans origin_denied"
grep -q 'request_id' /tmp/mohhdy-smoke-body || fail "create etrangere sans request_id"
if grep -q '"session_id"' /tmp/mohhdy-smoke-body; then
  fail "create refusee ne doit pas renvoyer session_id"
fi
if grep -qi 'api_key' /tmp/mohhdy-smoke-body; then
  fail "refus origin ne doit pas fuiter api_key"
fi

REQ_ORIGIN="https://evil.example"
code="$(http_code POST "/api/sessions/${sid_same}/messages" '{"content":"secret-origin-gamma"}')"
unset REQ_ORIGIN
[ "${code}" = "403" ] || fail "message origine etrangere doit etre 403 (got ${code})"
grep -q 'origin_denied' /tmp/mohhdy-smoke-body || fail "message etranger sans origin_denied"
if grep -q 'secret-origin-gamma' /tmp/mohhdy-smoke-body; then
  fail "refus message ne doit pas renvoyer le secret"
fi
got_same="$(fetch "/api/sessions/${sid_same}")"
if echo "${got_same}" | grep -q 'secret-origin-gamma'; then
  fail "message etranger stocke malgre origin_denied"
fi

fs_demo="$(curl -fsS "${admin_hdr[@]}" "${BASE_URL}/api/browser/fs?path=demo")"
echo "${fs_demo}" | grep -q 'demo-app.html' || fail "fs demo sans demo-app.html"
fs_file="$(curl -fsS "${admin_hdr[@]}" "${BASE_URL}/api/browser/fs?path=demo/fs-sandbox.txt")"
echo "${fs_file}" | grep -q 'ASSIST-061' || fail "lecture fs-sandbox.txt"
trav="$(curl -sS -o /tmp/mohhdy-smoke-body -w "%{http_code}" "${admin_hdr[@]}" \
  "${BASE_URL}/api/browser/fs?path=../server.py")"
[ "${trav}" = "403" ] || fail "traversal ../server.py doit etre 403 (got ${trav})"
grep -q 'path_denied' /tmp/mohhdy-smoke-body || fail "traversal sans path_denied"

code="$(http_code POST "/api/sessions/${sid_a}/tools" '{"tool":"dom.click"}')"
[ "${code}" = "403" ] || fail "outil revoque/absent doit etre 403 (got ${code})"
grep -q 'capability_denied' /tmp/mohhdy-smoke-body || fail "outil refuse sans capability_denied"
grep -q 'request_id' /tmp/mohhdy-smoke-body || fail "refus sans request_id"

curl -fsS "${admin_hdr[@]}" -X POST -H 'Content-Type: application/json' \
  -d '{"grant":["dom.click","mcp.invoice.create"]}' \
  "${BASE_URL}/api/admin/sessions/${sid_b}/capabilities" >/dev/null

origin_payload="$(printf '{"tool":"dom.click","origin":"%s","args":{"selector":"#menu-toggle"}}' "${BASE_URL}")"
code="$(http_code POST "/api/sessions/${sid_b}/tools" "${origin_payload}")"
[ "${code}" = "200" ] || fail "geste allowliste doit etre 200 (got ${code})"
grep -q 'dom_simulator' /tmp/mohhdy-smoke-body || fail "geste sans harness simulateur"

state="$(fetch /api/demo-app/state)"
echo "${state}" | grep -q '"menu_open":true' || echo "${state}" | grep -q '"menu_open": true' || fail "menu pas ouvert"

evil_payload='{"tool":"dom.click","origin":"https://evil.example","args":{"selector":"#menu-toggle"}}'
code="$(http_code POST "/api/sessions/${sid_b}/tools" "${evil_payload}")"
[ "${code}" = "403" ] || fail "origine etrangere doit etre 403 (got ${code})"
grep -q 'origin_denied' /tmp/mohhdy-smoke-body || fail "origine etrangere sans origin_denied"
grep -q 'request_id' /tmp/mohhdy-smoke-body || fail "origine etrangere sans request_id"

REQ_ORIGIN="https://evil.example"
code="$(http_code POST "/api/sessions/${sid_b}/tools" "${origin_payload}")"
unset REQ_ORIGIN
[ "${code}" = "403" ] || fail "en-tete Origin etranger sur outil doit etre 403 (got ${code})"
grep -q 'origin_denied' /tmp/mohhdy-smoke-body || fail "outil Origin etranger sans origin_denied"

inv_payload="$(printf '{"tool":"mcp.invoice.create","origin":"%s","args":{"customer":"Smoke","amount":"10.00"}}' "${BASE_URL}")"
code="$(http_code POST "/api/sessions/${sid_b}/tools" "${inv_payload}")"
[ "${code}" = "200" ] || fail "facture accordee doit etre 200 (got ${code})"
grep -q "${sid_b}" /tmp/mohhdy-smoke-body || fail "facture sans session_id"

invoices="$(fetch "/api/demo-app/invoices?session_id=${sid_b}")"
echo "${invoices}" | grep -q "${sid_b}" || fail "liste factures sans session B"

curl -fsS "${admin_hdr[@]}" -X POST -H 'Content-Type: application/json' \
  -d '{"revoke":["mcp.invoice.create","dom.click"]}' \
  "${BASE_URL}/api/admin/sessions/${sid_b}/capabilities" >/dev/null
code="$(http_code POST "/api/sessions/${sid_b}/tools" "${inv_payload}")"
[ "${code}" = "403" ] || fail "revoke mcp.invoice.create doit etre 403"
grep -q 'capability_denied' /tmp/mohhdy-smoke-body || fail "revoke sans capability_denied"

journal="$(curl -fsS "${admin_hdr[@]}" "${BASE_URL}/api/admin/journal")"
echo "${journal}" | grep -q 'origin_denied' || fail "journal sans origin_denied"

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

echo "OK health admin embed.js demo demo-app browser fs isolation revoke origin invoice escalate handoff origin-bind"
