# Lot 69 plan and result: per-element font size, larger text everywhere, Internet search, form factors, per-peer pacing, ARM

Branch `cursor/lot69`, PR #226, base `origin/main` 9bf1eca (Lot 68 merge).

## 1. Web font-size and line-height (US-031, partial)
- `userspace/webeng.c`: `font-size` in px, em, rem, %, pt, the keywords
  (xx-small..xxx-large) and smaller/larger. The user agent gives h1-h6
  2/1.5/1.17/1/.83/.67 em, plus `small` and `big`. A unitless
  `line-height` is relative to the element font. A block's line height
  grows with the largest inline text on that line.
- The display list carries a per-item scale: each TEXT item's `pad` holds
  q in quarters (4 = 1x, from round(font_px / 4), clamped 4..16). The
  kernel multiplies it by the window's web zoom.
- Honest limit: there is a single 8x8 bitmap cell. Sizes below 16 px
  render at the 8 px cell (q = 4); larger sizes are the same glyphs scaled
  up (Scale2x/3x or the native raster fonts, see `ui-font`). There is no
  outline font and no per-pixel size.
- Tests: `tests/unit/userspace/webeng_lot69.inc` (16 tests). One older
  expectation changed on purpose (`webeng_lot63_gen.inc`: h2 is now 1.5em,
  so the line is 15 px; the comment says why).

## 2. Text scale in the dock tooltip, dialogs, explorer and editor
- The status row, desktop icon labels, Scene IA headings, stage body,
  file tree rows, media pill and dock tooltip follow `ui-scale`, with the
  chrome capped at 2x. Host tests are in `tests/unit/kernel/test_gfx_desktop.c`
  (`test_lot69_*`).
- Native square bitmap fonts (IBM Plex Mono raster, OFL) for 1.5x-4x.
  `ui-font native|smooth` switches between them and Scale2x/3x.

## 3. QEMU pixel test and screenshots
- `make qemu-gui-text-scale` reads EDID and `display-info`, applies a
  tablet plan (dock-left pixel check), and opens `/web/lot69.html`.
  Its font-size check passed: median text run height 3 px against a
  tallest run of 18 px. It also switches `ui-font` between smooth and
  native.
- Screenshots in `/workspace/lot69-shots`:
  - Before/after: `desktop-1x`, `desktop-2x-smooth` (the Lot 68 style)
    and `desktop-2x` (native font).
  - `desktop-2x-ctrl-plus`, `desktop-tablet-dock-left`.
  - `web-2x`, `web-2x-ctrl-plus`, `web-font-size-2x`.
  - `search-gui`, `search-address-bar`, `search-chat`.

## 4. Internet search (owner request)
- One path for the shell and the GUI:
  - `search WORDS`, `search-open N` and `search-engine ddg|ddg-html|ddg-lite|URL`.
  - The `/browser` address bar searches any text that is not a URL.
  - `/search` in the chat; `/help` lists it.
  - The commands are registered in `shell.c` and in the parity table.
- The query is URL-encoded and fetched with the Lot 67 HTTP/TLS stack
  from `html.duckduckgo.com` (no JavaScript). If that gives an error or
  no result, the fetch falls back to `lite.duckduckgo.com`.
- Parsing (`userspace/wsearch.c`, pure and bounded to 10 results):
  - The DuckDuckGo html and lite layouts, plus a generic link fallback
    when the markup changes.
  - `uddg=` redirect links are decoded.
  - A bot-check page is detected and reported as `blocked bot-check`
    instead of an empty list.
- Results are numbered in the console. In the GUI they are a generated
  HTML page; a click or `search-open N` opens a result in `/browser`.
- Tests:
  - Host: `test_wsearch.c` (11 tests), on pages saved from the real
    engine on 2026-10-31 (`tests/fixtures/search/`).
  - QEMU, offline: `make qemu-lot67-web` against a fake engine over
    slirp (`scripts/lot67_web_server.py` `/html/?q=`, `/block/?q=`).
  - Manual: `LOT69_REAL=1` runs the real engine (not in CI). Results are
    in the "Real engine" section below.
- Real engine result (manual run, 2026-10-31 about 20:50 Mauritius,
  UTC+4): `search ok results 0 layout none status 202 bytes 14270
  fallback ddg-lite blocked bot-check`. DuckDuckGo answered the QEMU
  guest with an HTTP 202 bot-check page on both html and lite, so
  there are no real results from inside Mohhdy right now. Mohhdy detects
  this and reports it. The same session fetched https://example.com/ over
  real TLS (status 200). The GUI step of that manual run could not type
  its line after closing the page, so there is no real-engine screenshot.
- Fixed: a bare `/search` again opens the Recherche panel (parity
  family). Only `/search WORDS` is the Internet search.
- Honest: the real engine can change its markup or answer with a bot
  check at any time. Mohhdy then prints `blocked bot-check` or uses the
  generic fallback; it does not run JavaScript.

## 5. US-080 (partial): the form-factor plan is applied, physical size from EDID
- `form-factor apply` now also sets the layout through `ui-layout`:
  - dock left or bottom;
  - compact or full menu names;
  - full-screen chat.
- These are kept in `/persist/ui.cfg` (`layout=N`) and drawn from scene
  `scale_pad[1]`.
- EDID is read at boot from the QEMU std VGA (1234:1111, BAR2 offset 0)
  and parsed by `kernel/edid.c`. The new read-only syscall
  `SYS_DISPLAY_INFO` (164) returns it, and `display-info` prints
  `edid ok RHT 1280x800 325x203mm 15.1in`. `form-factor` with no size
  uses that diagonal.
- Still partial: real monitors give EDID over DDC, which this kernel does
  not drive, and there is no touch input or rotation.

## 6. US-068/US-073 (partial): per-peer pacers and per-packet data acks
- `userspace/p2p_flow.c` (pure, 32-bit division only) keeps one flow per
  peer. Each user data datagram (message or KV put) is tracked by the
  sealed counter already in its header, so the data format is unchanged.
- The receiver acknowledges every accepted data datagram by its counter.
  Acks are batched: a new inner type `P2P_I_DACK` carries up to 8 counters
  and is sent when the batch is full or at the next tick.
- Each ack gives:
  - an RTT;
  - a delivery-rate sample (bytes delivered between the send and the ack,
    as in BBR).
- A datagram not acked within 4 x srtt (bounded to 0.4..3 s) is counted
  as lost.
- The peer's own token bucket runs at 5/4 x the windowed maximum of the
  last 8 samples. After a loss it is capped at 7/8 of the current rate;
  a faster sample lifts the cap. Before the first sample, at most 8
  datagrams are in flight.
- `admit()` checks the peer's pacer first, then the node-wide
  estimator/window, so a slow or cut peer defers only its own traffic.
- New command `p2p-flow [PEER|reset]` (shell, GUI parity).
- Tests:
  - `test_p2p_flow.c` (12 tests).
  - `test_p2p.c` `test_lot69_per_peer_flow_and_data_acks`: a cut peer
    loses data and backs off while the healthy peer has 0 losses and
    every datagram acked.
  - QEMU: `make qemu-p2p` scenario 1b checks that alpha's flow for beta
    has every paced message acked, 0 lost, samples and a rate, and that
    beta sent the acks in DACK frames.
- Three older p2p tests changed their counts on purpose, because the
  acks are real traffic:
  - Relayed counts include the relayed DACK.
  - The replay test keeps the message datagram, since the last frame on
    the wire is now beta's ack.
- In QEMU the guest clock reads the ack RTT as 0 ms (below one
  tick), so the rate samples there are bounded by the 1 ms floor.
- Still partial: no retransmission (the mesh stays best effort, losses
  are only measured) and no ECN.

## 7. US-076 (partial): ARM port progress
- The ARM kernel now also compiles unchanged and self-tests at boot:
  - `kernel/aes_gcm.c`;
  - `kernel/edid.c`;
  - `userspace/p2p_flow.c` + `p2p_pace.c`.
- The expected values (`arch/arm/arm_portable.c`) were computed with the
  same sources on x86, and `test_arm_portable` checks them on the host.
  So the ARM boot log lines show bit-identical results:
  - `arm aes-gcm self-test ok`
  - `arm edid self-test ok 1280x800 325x203mm 15.1in`
  - `arm p2p flow self-test ok rate 62500 backoff 54687`
- These lines are checked by `make qemu-arm-boot` in the `arm-port` CI
  job. `qemu-system-arm` could not be installed on the dev box (Debian
  mirror error 500), so the ARM boot proof is the CI job only. The ARM
  image was built locally with Arm GNU Toolchain 13.3.
- Still not ported: drivers, filesystem, shell, OS-UI, AI engine.

## 8. Owner test on the box (address bar, first boot scale, windows)

The owner ran the box build and reported that the changes were not
visible. Root causes and fixes:

- Browser-OS address bar: the Browser-OS pane now draws a white address
  field ("Adresse (https://...) ou mots a chercher, puis Entree") with
  "<", ">" and "Aller" buttons. Enter on a non-slash line in that pane
  opens a URL (web ...) or runs a search (search ...) for plain words.
  Tests: test_gfx_desktop test_lot69_browser_address_bar, test_osui_gui
  test_lot69_browser_pane_address_line.
- First boot scale: the auto rule now gives 1.5x when the shorter screen
  side is >= 760 px (1024x768 included), with the native HD font. A host
  can pin it with the fw_cfg file opt/mohhdy/ui-scale (read by the
  kernel, used only without /persist/ui.cfg). The pixel QEMU tests pin
  1x this way; test_qemu_gui_text_scale checks the unpinned 1.5x boot.
- "//center": a desktop click used to type "/center" into the input
  line, after a half-typed "/". Desktop clicks are now an event
  ("cmd LINE") run apart from the typed line, which is kept. Only the
  shell terminal still receives typed text. Test:
  test_lot69_desktop_cmd_event_keeps_typed_line.
- make: the kernel and userspace objects now use compiler generated
  header deps (-MMD -MP); the hand rules missed gfx_font_hd.h and
  os_ui_scale.h, so a header edit could leave a stale image.
- Windows: one chrome path (kernel/gfx_wm.c) for every window type.
  The buttons were working but tiny (14 px at any scale) and the full
  screen web / editor windows used other "traffic" dots with only a
  close. Now: button size follows the chrome scale (1.5x: 21 px, capped
  by the title bar), the chat, pane and app windows resize from the
  corner grip and the right / bottom edges (minimum 200x120, clamped to
  the work area), and the full screen web / editor windows have the
  same three buttons: close closes, minimize shows the desktop and the
  Browser-OS dock icon restores it, maximize is a no-op (they are
  already full screen). Tests: test_gfx_wm
  test_lot69_scale_and_resize, test_gfx_desktop
  test_lot69_every_window_shared_chrome, QEMU
  tests/scripts/test_qemu_gui_lot69_windows.py (make
  qemu-gui-lot69-windows: tablet clicks on the pane, chat and web
  windows). App windows and the editor are clicked only in host tests.

## Other US
- US-078 and US-097 stay "simule".
- Phases 6-8 are unchanged apart from the items above.

## Size
About 5600 added lines against `origin/main` (code, tests, docs). That is
below the 8000 asked: this is the finished and tested work, not padded.
