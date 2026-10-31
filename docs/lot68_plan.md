# Lot 68 plan and result: readable text (UI scale), estimator-driven pacing, form factors, power

Branch `cursor/lot68`, base `origin/main` c83259b. PR #225.

## Part 1: text that can grow (owner request: text too small)

### Root causes found
- Every glyph is the fixed 8x8 font `kernel/gfx_font8.h`, drawn 1:1. On
  1920x1080 or 4K a glyph is 8 px high (about 1.5 mm on a 24" 4K panel).
- The web page was laid out on a fixed 640x480 viewport whatever the window.
- `userspace/webeng.c` has no font-size support (8 px cell, 10 px line).
- No setting, command or shortcut changed any of it.

### What is real now
- `include/os_ui_scale.h`: scale in quarters (4 = 1x, 6 = 1.5x, 8 = 2x,
  12 = 3x, up to 16 = 4x); 0 = auto from the framebuffer (shorter side
  >= 1000: 1.5x, >= 1400: 2x, >= 2000: 3x). Window zoom slots: web, chat,
  shell, pane, ide, app0..app5.
- Scene v8 (`os_fb_scene_t.ui_scale_q`, `zoom_q[12]`). The kernel renderer
  (`kernel/gfx_desktop.c`) scales glyphs, line height, title bars (chrome
  capped at 2x), chat, shell, pane, IDE and app windows. Glyphs are
  smoothed with Scale2x (2x) and Scale3x (3x), 4x is Scale2x doubled,
  other scales use nearest neighbour.
- Menu bar plan: at a larger scale the logo text, then the AI label, give
  way so all 8 menus stay visible (they used to disappear first).
- Chat window grows with its text scale (up to 4/5 of the screen) and
  wraps its fixed texts instead of cutting them.
- Web: browser-style zoom. The page is laid out on `640` CSS px at 1x and
  on `view_width * 4 / q` (at least 240) when zoomed, so it reflows instead
  of being cut; the kernel multiplies coordinates, glyphs and scroll and
  divides clicks. The kernel reports `web view PW PH Q FBW FBH`.
- Commands (one code path for console and GUI chat):
  `ui-scale [show|auto|1|1.5|2|3|150%|+|-|0]`,
  `text-size [web|chat|shell|pane|ide|app0..app5|browser|terminal|editor] N|+|-|0`.
  Saved in `/persist/ui.cfg` (`ui_scale=N zoom=a,b,...`), loaded at start.
- Shortcuts: Ctrl+'+' / Ctrl+'=' / keypad +, Ctrl+'-' / keypad -,
  Ctrl+'0' / keypad 0 zoom the focused window (web, editor, focused app,
  open pane, else chat). Both keyboard paths (Ring 3 ps2driver and the
  Ring 0 fallback) emit the zoom codes; Ctrl is now a reported modifier so
  a driver/Ring 0 hand-over keeps it. Root cause of a first failure: the
  kernel `keyboard_poll_char` filter dropped the new codes.

### Not done (honest)
- No per-element CSS `font-size` in the web engine: zoom scales the whole
  page. Headings are bold, not bigger.
- The status pill row, the Scene IA panel and the desktop icon labels stay
  1x (fixed box sizes in the layout).
- No EDID read: physical size is unknown unless given to `form-factor`.

### Proof
- Host: `tests/unit/userspace/test_ui_scale.c` (7), `test_gfx_desktop.c`
  +2 (EPX glyph shape at 2x/3x/4x for all 94 glyphs, scene scale, grid
  fit), `test_ps2_decode.c` +1 (Ctrl zoom keys, keypad, right Ctrl, Ctrl as
  modifier and seed), `test_osui_gui.c` scene v8.
- QEMU: `make qemu-gui-text-scale` (CI job `lot68-text-scale`, about 1
  min): `ui-scale 2` saved to `/persist/ui.cfg`; menu and chat glyph ink
  height 7 -> 14 px; Ctrl+'+' chat 14 -> 18 px; web page 2x then Ctrl+'+'
  reflows (`osui web reflow viewport 396`), dark ink 14 -> 18 px; Ctrl+'0'
  back. Screenshots (box): `/workspace/lot68-shots/desktop-1x.png`,
  `desktop-2x.png`, `desktop-2x-ctrl-plus.png`, `web-2x.png`,
  `web-2x-ctrl-plus.png`.

## Part 2: US-068/US-073 estimator drives the send rate (still partial)
- `userspace/p2p_pace.{h,c}`: token bucket pacing user data (messages, KV
  puts, shaping queue drain) at the estimator's pacing rate
  (gain x bottleneck bandwidth), floor 1 KB/s, ceiling 8 MB/s, operator
  cap, burst credit 4 datagrams. It replaces the per-second window budget
  once the estimator has samples; before that (or `p2p-pace off`) the old
  window applies. `p2p-limit` stays a hard cap. Puts over budget are now
  queued, not skipped. Control frames are never paced.
- `p2p-pace [status|on|off|auto|cap BPS|burst N]`; `p2p-bwe` shows the
  pacer source, rate and counters.
- Proof: `tests/unit/userspace/test_p2p_pace.c` (5: source and bounds,
  burst then spacing, 2 s throughput within 10 % at 1 KB/s..1 MB/s, no
  32-bit overflow, command). QEMU `make qemu-p2p` scenario 1b: source
  `estimator`, cap 2000 B/s, burst of 10 = 4 sent + 6 queued, all 10
  delivered. `deferred` counts every refused try of the drain loop, not
  messages.
- Why still partial: no per-packet acks for data (the estimator only
  learns from probe trains), single pacer per node (not per peer).

## Part 3: US-080 form factors (still partial)
- `userspace/plat_form.{h,c}`: plan from pixels and an optional diagonal
  (0.1 inch): form (phone/tablet/desktop/tv), ppi, text scale aiming at a
  2.4 mm (phone) / 2.8 mm / 6 mm (tv) glyph when the ppi is known, touch
  target (44 pt), chat mode, dock side, menu style, grid, web CSS width.
- `form-factor [apply] [auto|phone|tablet|desktop|tv] [WxH] [DIAG]`;
  `apply` sets the shared `ui-scale`. Partial: chat mode, dock side and
  menu style are reported, not yet applied by the renderer; no EDID.

## Part 4: US-081 power (still partial, battery simule)
- `userspace/plat_energy.{h,c}`: `power-apply PROFILE|auto|status`
  applies the profile to real values: P2P pacer cap (saver 64000 B/s,
  balanced 1 MB/s, performance none), background period, refresh budget.
- `power-sim start [CAP_MWH] [PCT] | tick MS [BUSY] | charge on|off | stop`:
  SIMULATED battery (draw model in mW from profile, load, refresh; auto
  switches to saver under 20 %). Every line says `battery simule`; without
  it `battery none (emulated pc, mains)`.
- Host tests `test_plat_form_energy.c` (6). QEMU steps in
  `qemu-gui-text-scale`.

## US-076
Unchanged (partial, armv7 core under qemu virt). Not worked on this lot.

## Size
About 2700 added lines (code, tests, docs), well under the 8000 asked:
the honest work that was finished and tested fits in that.
