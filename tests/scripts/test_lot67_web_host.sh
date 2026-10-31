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
    "$ROOT"/userspace/wf_http.c "$ROOT"/userspace/wf_dns.c "$ROOT"/userspace/wf_tls.c "$ROOT"/userspace/wf_fetch.c "$ROOT"/userspace/wf_inflate.c \
    "$ROOT"/userspace/wf_x509.c "$ROOT"/userspace/wf_bn.c "$ROOT"/userspace/wf_sha512.c "$ROOT"/userspace/wf_ec.c \
    "$ROOT"/userspace/wf_rsa.c "$ROOT"/kernel/sha256.c "$ROOT"/kernel/aes_gcm.c "$ROOT"/kernel/x25519.c || { echo "FAIL: build"; exit 1; }
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
echo "[lot67-web-host] $pass passed, $fail failed"
[ $fail -eq 0 ]
