# Lot 69 - real web site compatibility (branch cursor/lot69-web)

User report: "https://google.com does not open". This lot runs a real-site
campaign, root-causes every failure and fixes it in the shared Ring 3 web
stack (wf_*), used by the shell `fetch` and the GUI `web` / Browser-OS.

## How it was measured

- Host: `tests/host/wf_fetch_host.c` (same wf_* code over POSIX sockets).
- Guest: `LOT69_WEB_REAL=1 python3 tests/scripts/lot69_web_campaign.py`
  (headless QEMU, -netdev user + ne2k_isa + -cpu max, shell `fetch`).
  Manual only, needs Internet; writes test_logs/lot69-web-campaign.md.
  `LOT69_WEB_SHOTS=1` also renders google and wikipedia in the GUI.
- Date of the campaign: 2026-10-31. Sites change; this is a snapshot.

## Per-site table (before = cursor/lot69 at c73f761, after = this branch, in the guest)

| Site | Before | After |
|---|---|---|
| https://google.com | FAIL host name mismatch | OK 200, redirect to www.google.com, TLS 1.3, rendered |
| https://www.google.com | OK (TLS 1.2) | OK 200, TLS 1.3, gzip, rendered (screenshot) |
| https://wikipedia.org | FAIL host name mismatch | OK 200 (www.wikipedia.org) |
| https://fr.wikipedia.org | fetch OK, GUI "render error" (too many nodes) | OK 200, gzip, rendered (reader view, cut at node budget) |
| https://en.wikipedia.org/wiki/Operating_system | OK, truncated | OK, truncated 1 (page > 256 KiB decoded) |
| https://github.com | OK, truncated | OK, truncated 1 |
| https://duckduckgo.com | OK | OK (JS page: little text) |
| https://html.duckduckgo.com/html/?q= | 202 bot check | 202 bot check (engine policy, not a stack bug) |
| https://www.bing.com | OK | OK, TLS 1.3 |
| https://www.cloudflare.com | OK, truncated | OK, truncated 1 |
| https://www.mozilla.org | OK | OK, TLS 1.3 |
| https://www.lemonde.fr | OK, truncated (full 900 KiB download) | OK, gzip, truncated 1, stops reading when the buffer is full |
| https://www.bbc.com | OK, truncated | OK, truncated 1 |
| https://example.com | OK | OK, TLS 1.3 |
| https://httpbin.org/get | OK | OK |
| https://httpbin.org/gzip | FAIL compressed body not supported | OK (gzip decoded) |
| https://httpbin.org/redirect/3 | OK | OK, 3 redirects |
| https://www.youtube.com | FAIL host name mismatch, then headers too large | OK, truncated 1 |
| https://www.amazon.com | FAIL connection closed by peer | OK |
| https://www.reddit.com | FAIL connection closed by peer | OK |
| https://news.ycombinator.com, apple, microsoft, x.com, gov.uk | OK | OK |
| https://rsa2048 / rsa4096 / ecc256 / ecc384.badssl.com | FAIL no extended master secret | OK |
| https://expired / wrong.host / self-signed.badssl.com | rejected | rejected (correct) |
| https://www.lexpress.mu | unknown issuer | unknown issuer: server omits its intermediate (openssl fails too); no AIA fetching |
| http://example.com, http://neverssl.com, http://httpbin.org | 500 | 500: produced by the egress proxy of the test box (curl gets the same 500), not by Mohhdy; plain HTTP is covered by the offline CI test |

## Root causes fixed

1. Host name mismatch on google.com / wikipedia.org / youtube.com: only the
   first 24 dNSName SANs were kept; these certificates carry 100+. The
   host check now scans the whole SAN extension.
2. No TLS 1.3: added a TLS 1.3 client (TLS_AES_128_GCM_SHA256, X25519,
   HKDF, encrypted extensions, CertificateVerify with ECDSA P-256/P-384
   and RSA-PSS, Finished, KeyUpdate, tickets ignored, HelloRetryRequest
   and downgrade sentinel detected). TLS 1.2 stays as fallback.
3. amazon.com / reddit.com closed the connection: the ClientHello record
   used version 0x0303; it is now 0x0301 like every browser.
4. TLS 1.2 servers without extended master secret were refused: RFC 5246
   master secret is used then (as browsers do).
5. TLS 1.2 servers without X25519: P-256 ECDHE added (wf_ecdh_p256).
6. Root store: whole Mozilla TLS set (148 roots), PEM decoded in place.
7. Content-Encoding: bounded in-place gzip/deflate decoder (wf_inflate.c);
   Accept-Encoding: gzip, deflate. Wire bytes limited to a third of the
   buffer; reading stops when the buffer is full.
8. Headers > 8 KiB (youtube): limit 32 KiB.
9. GUI render error on real pages: the engine holds 160 nodes; the reader
   view now stops at a 140-node budget ("[page tronquee]"), drops empty
   paragraphs, navigation, hidden blocks and language pickers.
10. Accents showed as '?': UTF-8 Latin-1 letters shown as base letters.
11. A failed load left the browser closed: an error page now says which
    step failed (DNS, TCP, TLS, HTTP).
12. ALPN http/1.1 sent.

## Tests

- Host unit: RFC 8448 key schedule, RFC 5903 P-256 ECDH, SAN beyond cap,
  in-place root store (148 roots, 0 skipped), gzip/zlib/raw/fixed/stored
  vectors and truncation, Content-Encoding parsing, real-page shapes.
- Host end-to-end (test_lot67_web_host.sh): 23 checks incl. TLS 1.3
  ECDSA and RSA-PSS, P-256, gzip, deflate, big gzip, http -> https.
- QEMU offline (make qemu-lot67-web, CI job lot67-web, about 5 min):
  12 fetch checks incl. TLS 1.3, P-256, gzip, cross-scheme redirect, big
  page, plus the GUI error page.

## Honest limits

- P-256 ECDH scalar multiplication is not hardened against timing.
- Only TLS_AES_128_GCM_SHA256 in TLS 1.3 (no ChaCha20, no AES-256); a
  server requiring another suite or a P-256 key share fails clearly.
- No AIA intermediate fetching, no OCSP / revocation.
- Reader view only: no JavaScript, pages are cut at 256 KiB decoded and
  at 140 layout nodes. No cookies.
- Fonts are ASCII: non-Latin scripts show '?'.
