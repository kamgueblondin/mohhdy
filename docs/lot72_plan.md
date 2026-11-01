# Lot 72 plan and result: browser images, Google without JavaScript, TLS AES-256 / ChaCha20, constant-time P-256, keep-alive, Greek and Cyrillic glyphs, page charsets

Branch `cursor/lot72`, base `origin/main` 61d9d91 (after PR #228, Lot 71).
The first ten commits (9cf63b8..4f4ffe1) were already on the branch and are
kept as they are; the later commits complete the lot.

## 1. Images in the browser
- Decoders in Ring 3 (`userspace/imgdec.c`): PNG (with RGBA, gray+alpha
  and palette tRNS transparency, composed over white), JPEG baseline and
  progressive, scaled at 1/2..1/8 for large files (`imgdec_open_scaled`),
  GIF 87a/89a (LZW, interlace, transparency, first frame of an animation,
  `imgdec_gif.inc`), ICO with PNG or DIB entries (`imgdec_ico.inc`), BMP.
- `userspace/wf_webimg.c`: the `<img>` tags of the current section are
  fetched (8 per section, 512 KiB per file), decoded, scaled to their box
  (at most 480x360) and stored in the web atlas; the tag becomes
  `mohhdy-img:K`. A failed image shows its alt text. `web-images on|off`
  and `web-images` (found / shown / failed counts).
- Favicon: `<link rel=icon>` or `/favicon.ico`, slot 0 of the atlas, drawn
  at the start of the web window title and (Lot 72 audit fix) at the start
  of the Browser-OS address field.
- ABI: `OS_FB_WEB_IMAGE` scene item and `SYS_VGA_BLIT` sub-call 6 (the
  atlas), both added by commit 3eb3842. See section 9.

## 2. Google without JavaScript
- The Google form is sent to the basic HTML version (`gbv=1`). Measured
  from the build box (2026-11-01): Google answers with its "update your
  browser" / `enablejs` page, so no results. Mohhdy detects that answer,
  says so, and moves to the fallback engines (Lot 71 chain) with a note.
  This is a Google limit, not a form bug.

## 3. TLS
- `userspace/wf_aead.c`: AES-128/256-GCM and ChaCha20-Poly1305 (FIPS-197,
  SP 800-38D, RFC 8439 known answers and OpenSSL vectors). GHASH, ChaCha20
  and Poly1305 have no secret-dependent branch or table; the AES rounds
  still use the S-box table (cache timing on shared hardware not
  hardened, documented in the header).
- Suites: TLS 1.3 `AES256-GCM-SHA384` (SHA-384 key schedule) and
  `CHACHA20-POLY1305-SHA256`; TLS 1.2 ECDHE-ECDSA / ECDHE-RSA with
  AES-256-GCM and ChaCha20. `tls-suite NAME|auto` forces one suite.
- `userspace/wf_p256.c`: constant-time P-256 ladder (complete addition
  formulas, masked swap, same work for every scalar), checked against
  OpenSSL and by an equal-work test.
- HTTP keep-alive: the page, its favicon and its images reuse one TCP/TLS
  connection; `http-keepalive on|off` (counts opened / reused).
- Proof: host tests against OpenSSL, `make qemu-lot72` (each suite forced
  from the guest; the server sees one client port for page + images, one
  port per request when keep-alive is off).

## 4. More characters: glyph pages
- The display charset stays one byte per cell (CP1252, Lot 71). The five
  CP1252 bytes with no character select a glyph page for the next bytes
  and take no cell: 0x81 page 0 (CP1252), 0x8D page 1 (Latin Extended-A:
  the 122 letters CP1252 lacks: Polish, Czech, Hungarian, Turkish...),
  0x8F page 2 (Greek, ISO-8859-7 layout), 0x90 page 3 (Cyrillic,
  ISO-8859-5 layout plus Ukrainian, Belarusian and Kazakh letters and the
  numero sign), 0x9D page 4 (arrows, math, box drawing, blocks, shapes,
  check mark, Romanian comma letters, currency signs). ASCII is the same
  in every page; every string starts in page 0.
- `include/os_charset.h` (API) and `include/os_charset_pages.h`
  (generated): UTF-8 to display (`os_text_to_display`, idempotent: after a
  selector, high bytes are copied as they are), display to UTF-8, cells,
  cut by cells, CP437 for the VGA console.
- Fonts: 616 glyphs (was 223) in `kernel/gfx_font8.h` and
  `kernel/gfx_font_hd.h` (12/16/24/32 px), from IBM Plex Mono Medium, and
  Noto Sans Mono weight 500 for the code points Plex lacks (mostly Greek).
  The 223 earlier glyphs are kept byte for byte. Box drawing and blocks
  are drawn by rule; in the 8x8 font, Greek and Cyrillic letters shaped
  like Latin ones reuse the ASCII strokes, marked letters are composed.
  `make font-check` (CI) regenerates and compares. No new font engine.
- Desktop: GUI strings (chat, titles, terminal rows, input line) switch
  pages on the in-band selectors; web TEXT items carry their page in
  `pad` bits 8..10 (the low byte stays the Lot 69 font scale). The VGA
  console maps page bytes to CP437 (Greek, math, look-alikes).
- Web: network pages (`wf_html`, `wf_entity`), search results
  (`wsearch`) and local UTF-8 pages (`/web/lot72.html`, converted run by
  run in `osui_gui`) give paged display text; the web engine keeps one
  page per text node and splits a chunk when the page changes. Numeric
  references and the HTML 4 names with a glyph (`&alpha;`, `&rarr;`,
  `&le;`, 216 names) reach their page.
- Not covered: CJK, Arabic, Hebrew, Devanagari, combining marks, emoji
  (still `?`). A raw CP1252 byte after a Greek run converted in place,
  with no room left for a selector, becomes `?` (documented, tested).

## 5. Page charsets
- `windows-1251`, `koi8-r`, `koi8-u`, `iso-8859-5`, `iso-8859-7`,
  `windows-1253` (header or meta tag) are decoded through generated
  tables (`scripts/gen_charset_tables.py`, `wf_charset_table.inc`, checked
  by `make font-check`).
- Forms are sent in the page charset; a character the charset lacks is
  sent as `&#N;` as browsers do. This also fixes a Latin-1 page with Greek
  typed in a field (the page byte was sent raw).
- `fetch` prints the title in UTF-8 and names a charset other than UTF-8.

## 6. Partial user stories
- US-031 partial: images, glyph pages and page charsets in the web
  engine. `phase3_complete` stays false.
- US-076 partial: the ARM boot self-tests add `userspace/wf_aead.c`
  (AES-256-GCM, ChaCha20-Poly1305, bytes equal to OpenSSL) and the glyph
  pages, compiled unchanged (CI `arm-port`; host test
  `test_arm_portable`). The ARM image was not booted on the build box
  (no `arm-none-eabi-gcc` / `qemu-system-arm`: the Debian mirror answered
  500); the two sources were compiled for ARMv7-A with another compiler
  (`zig cc`) without warning and without libc symbol.
- US-068/US-073, US-080, US-081 unchanged; US-078/US-097 still simule.
  No roadmap status changes: none of these stories is complete.

## 7. Tests and proofs
- Unit: `test_glyph_pages.c`, `webeng_lot72.inc`, `test_gfx_desktop.c`
  (glyph pages, web item page, title selectors, wrap by cells, input tail,
  favicon in the address bar), `test_wf_text.c`, `test_wf_form.c`,
  `test_wsearch_api.c`, `test_osui_gui.c`, `test_arm_portable.c`, plus the
  image, AEAD, P-256 and TLS tests of the first commits.
- `make test-all` now builds `test_wf_webimg` (it was a
  COMPILATION_ERROR in the runner, audit fix).
- `make qemu-lot72`: images, favicon, every TLS suite, keep-alive; the
  local UTF-8 page in `web-render` (page and UTF-8 of each item); the GUI
  glyphs matched pixel for pixel in the page and in the title bar; the
  windows-1251 and ISO-8859-7 pages over HTTP, and the windows-1251 form
  sent back as `q=%CC%E8%F0`.

## 8. Rules kept
- Freestanding i386 C in Ring 3; new code and docs in 7-bit ASCII (the
  test page `/web/lot72.html` is UTF-8 on purpose: it is the input).
- No assertion weakened; the one adapted expectation is in the delivery
  notes (ADAPTED).

## 9. ABI and security points for review
- `OS_FB_WEB_IMAGE` and `SYS_VGA_BLIT` 6 (commit 3eb3842).
- `os_fb_web_item_t.pad` bits 8..10 now carry the glyph page of a TEXT
  item (semantic extension of an existing field, size unchanged; older
  producers send 0 = page 0).
- In-band page selectors in GUI strings use the five undefined CP1252
  bytes; a string from an older producer has none.
- AES uses an S-box table (not cache-timing hardened).
