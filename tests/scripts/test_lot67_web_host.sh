#!/bin/sh
# Lot 67: end-to-end host test of the shared Ring 3 web stack (wf_*) over
# POSIX sockets against the offline test server (scripts/lot67_web_server.py)
# with a throw-away test CA. No Internet access is used.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
W="$ROOT/build/lot67/host"
mkdir -p "$W"
cd "$W" || exit 1
python3 "$ROOT/scripts/lot67_gen_pki.py" "$W/pki" >/dev/null || { echo "FAIL: pki"; exit 1; }
gcc -O1 -m32 -Wall -Werror -I"$ROOT/userspace" -I"$ROOT/kernel" -I"$ROOT/include" -o wf_fetch_host \
    "$ROOT/tests/host/wf_fetch_host.c" \
    "$ROOT"/userspace/wf_http.c "$ROOT"/userspace/wf_dns.c "$ROOT"/userspace/wf_tls.c "$ROOT"/userspace/wf_fetch.c "$ROOT"/userspace/wf_cookie.c "$ROOT"/userspace/wf_inflate.c \
    "$ROOT"/userspace/wf_x509.c "$ROOT"/userspace/wf_bn.c "$ROOT"/userspace/wf_sha512.c "$ROOT"/userspace/wf_ec.c \
    "$ROOT"/userspace/wf_rsa.c "$ROOT"/userspace/wf_html.c "$ROOT"/userspace/wf_entity.c "$ROOT"/userspace/wf_form.c \
    "$ROOT"/userspace/wf_section.c "$ROOT"/userspace/wsearch_api.c "$ROOT"/userspace/wsearch.c "$ROOT"/kernel/sha256.c "$ROOT"/kernel/aes_gcm.c "$ROOT"/kernel/x25519.c || { echo "FAIL: build"; exit 1; }
HP=$((20000 + $$ % 20000)); SP=$((HP + 1))
python3 "$ROOT/scripts/lot67_web_server.py" "$W/pki" $HP $SP > server.log 2>&1 &
SRV=$!
trap 'kill $SRV 2>/dev/null' EXIT
i=0; while ! grep -q READY server.log 2>/dev/null; do i=$((i+1)); [ $i -gt 50 ] && { echo "FAIL: server"; cat server.log; exit 1; }; sleep 0.1; done
pass=0; fail=0
check() {  # name expect-regex args...
    name=$1; want=$2; shift 2
    out=$(timeout 30 ./wf_fetch_host "$@" 2>&1)
    if printf '%s\n' "$out" | grep -Eq "$want"; then pass=$((pass+1)); echo "  ok   $name"
    else fail=$((fail+1)); echo "  FAIL $name"; printf '%s\n' "$out" | sed 's/^/       /' | head -20; fi
}
CA="$W/pki/test_ca.pem"
check "http content-length"  'FETCH OK status=200 bytes=17 '           "$CA" "http://127.0.0.1:$HP/hello"
check "http chunked"         'chunk-one chunk-two chunk-three'          "$CA" "http://127.0.0.1:$HP/chunked"
check "https page"           'FETCH OK status=200.*page.html'           "$CA" "https://mohhdy.test:$SP/page.html" now 127.0.0.1
check "https chain + suite"  'TLS suite=ECDHE-ECDSA-AES128-GCM-SHA256 leaf=mohhdy.test anchor=MOHHDY Test Root' "$CA" "https://mohhdy.test:$SP/hello" now 127.0.0.1
check "https wildcard SAN"   'FETCH OK status=200'                      "$CA" "https://www.mohhdy.test:$SP/chunked" now 127.0.0.1
check "https redirect"       'redirects=1 url=https://mohhdy.test:[0-9]+/page.html' "$CA" "https://mohhdy.test:$SP/redirect" now 127.0.0.1
check "redirect limit"       'FETCH FAIL: too many redirects'           "$CA" "https://mohhdy.test:$SP/loop" now 127.0.0.1
check "200 KiB body"         'FETCH OK status=200 bytes=204800 '        "$CA" "https://mohhdy.test:$SP/big" now 127.0.0.1
check "host mismatch"        'certificate rejected: host name mismatch' "$CA" "https://evil.test:$SP/hello" now 127.0.0.1
check "unknown CA"           'certificate rejected: unknown issuer'     "$ROOT/initrd_content/etc/ca-roots.pem" "https://mohhdy.test:$SP/hello" now 127.0.0.1
check "clock after expiry"   'certificate rejected: certificate expired' "$CA" "https://mohhdy.test:$SP/hello" 20400101000000 127.0.0.1
check "no clock is reported" 'time_checked=0'                           "$CA" "https://mohhdy.test:$SP/hello" noclock 127.0.0.1
check "404 is a response"    'FETCH OK status=404'                      "$CA" "http://127.0.0.1:$HP/nope"
# Lot 69: TLS 1.3, P-256 ECDHE, Content-Encoding, cross-scheme redirect
T13=$((SP + 1)); T13R=$((SP + 2)); P256=$((SP + 3))
check "tls13 ecdsa"          'TLS suite=TLS13-AES128-GCM-SHA256 leaf=mohhdy.test anchor=MOHHDY Test Root' "$CA" "https://mohhdy.test:$T13/hello" now 127.0.0.1
check "tls13 rsa-pss"        'TLS suite=TLS13-AES128-GCM-SHA256 leaf=mohhdy.test'  "$CA" "https://mohhdy.test:$T13R/page.html" now 127.0.0.1
check "tls13 host mismatch"  'certificate rejected: host name mismatch' "$CA" "https://evil.test:$T13/hello" now 127.0.0.1
check "tls13 unknown CA"     'certificate rejected: unknown issuer'     "$ROOT/initrd_content/etc/ca-roots.pem" "https://mohhdy.test:$T13/hello" now 127.0.0.1
check "tls12 p256 ecdhe-rsa" 'TLS suite=ECDHE-RSA-AES128-GCM-SHA256 leaf=mohhdy.test' "$CA" "https://mohhdy.test:$P256/hello" now 127.0.0.1
check "gzip body"            'ENCODING gzip'                            "$CA" "http://127.0.0.1:$HP/gzip"
check "gzip decoded"         'fetched over the network by Mohhdy'       "$CA" "https://mohhdy.test:$T13/gzip" now 127.0.0.1
check "deflate decoded"      'fetched over the network by Mohhdy'       "$CA" "http://127.0.0.1:$HP/deflate"
check "big gzip bounded"     'FETCH OK status=200 bytes=[0-9]+ truncated=1' "$CA" "http://127.0.0.1:$HP/biggz"
check "http -> https tls13"  "redirects=1 url=https://mohhdy.test:$T13/page.html" "$CA" "http://mohhdy.test:$HP/to?https%3A%2F%2Fmohhdy.test%3A$T13%2Fpage.html" now 127.0.0.1
# Lot 71: forms (GET, POST, 303 after POST), charsets, long pages, SearXNG
U="http://127.0.0.1:$HP"
check "form page parsed"     "PAGE title=Formulaire $(printf '\351')t$(printf '\351') .*forms=3 fields=8" "$CA" "$U/form.html" now - page
check "form fields"          'FIELD 1 type=4 name=lang value=fr'        "$CA" "$U/form.html" now - page
check "form GET submit"      'SUBMIT method=GET url=http://127.0.0.1:[0-9]+/echo\?q=caf%C3%A9\+cr%C3%A8me&lang=fr' "$CA" "$U/form.html" now - "submit:0=caf$(printf '\351') cr$(printf '\350')me"
check "form GET answer"      'method=GET</p><p>query=q=caf%C3%A9\+cr%C3%A8me&amp;lang=fr' "$CA" "$U/form.html" now - "submit:0=caf$(printf '\351') cr$(printf '\350')me"
check "form POST submit"     'SUBMIT method=POST url=http://127.0.0.1:[0-9]+/echo body=user=moi&msg=bonjour' "$CA" "$U/form.html" now - "submit:3=moi"
check "form POST answer"     'method=POST</p><p>query=</p><p>body=user=moi&amp;msg=bonjour' "$CA" "$U/form.html" now - "submit:3=moi"
check "POST then 303 GET"    'ANSWER status=200 redirects=1 url=http://127.0.0.1:[0-9]+/echo\?after=post' "$CA" "$U/form.html" now - "submit:6=1"
check "latin-1 page"         "text=$(printf '\303\251')t$(printf '\303\251') $(printf '\303\240') la plage, $(printf '\303\251')l$(printf '\303\250')ve $(printf '\303\251') $(printf '\305\223')uvre $(printf '\342\202\254')" "$CA" "$U/latin1.html" now - page
check "long page sections"   'PAGE title=Longue page nodes=[0-9]+ bytes=[0-9]+ sections=(1[3-9]|[2-9][0-9])' "$CA" "$U/long.html" now - page
check "long page end"        'SECTION 26 bytes=[0-9]+ tail=.*1500 de la longue page\. +FIN-DE-PAGE +\[section 26/26 - fin de la page\]'                       "$CA" "$U/long.html" now - page
check "searx html"           'RESULT 1 title=R.sultat pour mohhdy os url=https://example.org/1' "$CA" "$U/searx" now - "search:searx:mohhdy os"
check "searx json"           'RESULT 2 title=Second & co url=https://example.org/2\?a=1&b=2' "$CA" "$U/searx" now - "search:searx-json:mohhdy"
check "controls defaults"    'SUBMIT method=GET url=http://127.0.0.1:[0-9]+/echo\?t=web&lr=lang_fr body=' "$CA" "$U/controls.html" now - "submit:1=on"
check "checkbox on"          'url=http://127.0.0.1:[0-9]+/echo\?safe=on&t=web&lr=lang_fr'  "$CA" "$U/controls.html" now - "submit:0=on"
check "radio moves"          'url=http://127.0.0.1:[0-9]+/echo\?t=img&lr=lang_fr'          "$CA" "$U/controls.html" now - "submit:2=on"
check "select by label"      'url=http://127.0.0.1:[0-9]+/echo\?t=web&lr=lang_en'          "$CA" "$U/controls.html" now - "submit:3=anglais"
check "cookie after redirect" 'COOKIES set=2 sent=2 refused=1 kept=2'   "$CA" "$U/cookie/login" now -
check "cookie sent"          'cookie=sid=s3cr3t-session; theme=dark'    "$CA" "$U/cookie/login" now -
check "cookie values hidden" '127.0.0.1 /cookie sid len=14 host-only httponly' "$CA" "$U/cookie/login" now -
echo "[lot67-web-host] $pass passed, $fail failed"
[ $fail -eq 0 ]
