# Lot 88 plan - window chrome, site autofill, @scope, panel key trace

Base: main 6a56c54 (lot 87, PR #245). Branch lot88-chrome-scope. One guest
engine and one shell path for the console and the desktop. Kernel rights
unchanged. US-031 stays partial. phase3_complete stays false. US-068/US-073,
US-076, US-080 and US-081 stay partial. US-078 and US-097 stay simule.

This file is updated with each chunk pushed to the PR; a row is listed
only when its code, tests and docs are in the branch.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Panel key trace (Lot 46 CI flake) | the desktop panels took keys blind: the QEMU tests sent Down / text / Enter with a fixed delay and only learnt about a lost key from a missing "osui panel armed" line 60 s later (CI failure on 83af6e5: timeout waiting for "osui panel armed cmd=collab-work solo 2"); qemu-gui-mesh wrote its serial log under the parity name, which the Lot 46 job did not keep | every key a desktop panel (osui_parity families) takes prints "osui panel key sel=S ilen=L"; Enter prints "osui panel enter sel=S input=TEXT" after what the run logged. The trace has its own buffer: the first run of qemu-gui-panels with the trace inside the 400-byte log showed a long run output cutting it ("input=/web/demo.h"). The panel tests (test_qemu_gui_parity run_entry, test_qemu_gui_panels run_entry, so qemu-gui-mesh too) wait for the trace of each key before the next one and fail naming the key and the guest's last panel lines. qemu-gui-mesh writes test_logs/qemu-gui-mesh-serial.log, kept by the gui-panels-logs artifact |
| B | Autofill per site | exact host only | wf_regdom: registrable domain of a host from a short hand-written list of multi-label public suffixes (country second levels, hosting suffixes such as github.io where subdomains are different owners) and NAME.TLD otherwise. IP literals, one-label hosts and public suffixes have none and share with nobody. The offers of a field (console web-fill and the desktop list) are the host's own values first, then the other hosts of the site, without repeats; web-fill marks those "site HOST". Delete in the desktop list (and autofill forget from the shell) can forget a value held by another host of the site. autofill add HOST NAME VALUE and autofill offers URL NAME (console) use the same store and list |
| C | @scope and more properties (US-031) | @scope blocks were dropped; the reader kept nine properties; CSS-wide keywords only on them, no all | @scope (start) to (end), @scope to (end), @scope (start) and @scope with no prelude (root: the parent element of the <style> block; a sheet in head therefore reaches nothing of body, as in a browser). Limits leave the scope with what is under them. :scope and & (specificity of a class / 0). Rules are relative to the root (":scope p" implied, or a leading > + ~); with :scope or & written, the rest of the selector may match outside. Declarations right in the block apply to the root. Nested @scope (4 levels) inside @media, @supports and @layer, and those inside @scope. Scope proximity in the cascade (after specificity, before order; unscoped = infinite hops). 8 scopes, more are counted and their rules dropped, never widened. The all shorthand (initial, inherit, unset, revert, revert-layer) on every reader property, style attribute included. text-transform and white-space are kept (inherit, every keyword) and passed in the style attribute to the engine. web-css prints "css-scope scopes N rules R hits H" |
| D | Window chrome on every window type (QEMU tablet) | the chrome had host tests (gfx_wm) and per-window QEMU tests from lots 69/85/86, never one pass over every window type at more than one scale; nothing told a test where the guest put a window | the desktop prints "gfx wm hit WHO x= y= w= h= title= btn= q=" for the window a click lands in (chat, pane, app slot, full). tests/scripts/test_qemu_lot88_chrome.py opens the chat, a pane (/status), an app (/files) and the full-screen web window (/browse) and, on each, clicks the title, finds the drawn red close button in a screendump and checks it against the hit test (same side, same place), then maximize and restore (or restore and maximize again for the full window, which opens maximized), a grip drag past the screen (clamped), a 100x60 shrink that must give exactly 100x60 less, a drag back to the first size, minimize and dock restore (1x), and close. It runs at 1x and at 1.5x (from the TV hint, row E); every window type must report the same q. Found and fixed: a grip drag that grew a window past the work area kept the unclamped size in the window state (dw/dh), so the next shrink was swallowed by the excess. gfx_wm_mouse now stores the size the window was placed at (host test test_lot88_resize_from_visible_size fails on the old code) |
| E | TV mode hints applied where safe (US-080) | display-info printed a TV scale hint marked "not applied" | plat_form_tv_safe_q: with a CEA TV sink (no ui.cfg, no user pin), the wanted scale (2x for 1080p, 3x for 4K, 1.5x for 720p) is stepped down 3x, 2x, 1.5x until the framebuffer still holds 640x360 logical pixels; it is applied only on a first boot (no /persist/ui.cfg) and only through the existing scale path. display-info prints "display tv-hint scale_q W safe_q S (why, applied Q)" or the reason it is not applied, and "display tv-hint overscan likely" when the sink does not declare underscan support (a note only: no overscan margin is applied). The kernel takes a test EDID from fw_cfg file opt/mohhdy/edid (128 or 256 bytes) before the BAR2 block, so QEMU can boot as a 1080p HDMI TV; std VGA has no CEA block of its own. Proof: second boot of qemu-lot88 (CI job lot88-tv) with a hand-written 1080p TV EDID: 1.5x taken on 1024x768, then the chrome pass of row D at that scale |

## Review points

1. The panel trace is one serial line per key in a panel only (not in the chat, the editor or the web pane).
2. Scope proximity counts generations only and is capped at 6 hops (7 levels in 3 bits of the rank); the rule order now uses 7 bits (WF_SITE_RULES is 128, so it still fits).
3. The fw_cfg EDID file is a test harness hook like opt/mohhdy/ui-scale: it is read only when QEMU passes it and only replaces what the BAR2 block would have given.
4. The title bar keeps its height (30/34 px) at every scale; the buttons grow with q up to title - 6 (24 px for the app and pane bars at 2x, 28 for the chat). The title text is capped at the chrome scale as before.
5. The suffix list is not the Public Suffix List. A private hosting suffix that is not listed lets its subdomains share autofill values. See the gaps.

## Not done (first gaps for lot 89)

1. @scope: no @import scope(), no sibling hops in proximity, :scope cannot share a compound with another pseudo-class, the start/end selectors take one selector (a list keeps the first, counted as an error). Variables (--x) declared inside @scope are not read.
2. Chrome: the title bar height does not follow the scale (buttons capped at title - 6); keyboard window moves and snapping are not tested; the chrome test does not cover the editor and terminal windows of earlier lots (they have their own QEMU tests).
3. TV: no overscan margin is drawn (only reported); the mode set is unchanged (no CEA mode is programmed, no DDC/I2C); the hint applies on a first boot only.
4. Public suffixes: 126 hand-written entries, not the PSL (thousands of entries, private ones change often); no IDN (punycode) handling.

## Tests

Hand-written host tests: tests/unit/userspace/test_lot88_site.c (7), test_lot88_scope.c (25), test_osui_parity.c (test_panel_key_trace).
Host tests added with chunk 2: test_gfx_wm.c (test_lot88_resize_from_visible_size), test_lot87_display.c (test_lot88_tv_safe_q, test_lot88_tv_line).
QEMU: tests/scripts/test_qemu_lot88.py and test_qemu_lot88_chrome.py, `make qemu-lot88` (both boots), `make qemu-lot88-desk` (CI job lot88-chrome) and `make qemu-lot88-tv` (CI job lot88-tv). The two boots are two jobs so each stays under 15 minutes (the two boots took 600 s together on the box). qemu-gui-panels and qemu-gui-mesh use the key trace.

## Gates on the box

Chunk 1 (10 November 2026): make all; make test-all (15699 tests, userspace 73/73, lot67-web-host 62/62); roadmap-check, strict-image-check, secrets-check; tree-clean-check after the commit; make qemu-lot88 (182.9 s); make qemu-gui-panels (66 entries, 499 panel keys, slowest trace 3.3 s); qemu-gui-mesh passed with the first version of the trace (rerun with the final one listed with the next chunk).

The first panels run, with two QEMUs on the box, saw one key trace come after 8 s and then arrive: the slow key is late, not lost. The wait is 25 s and every run prints "panel keys N, slowest trace S".

Chunk 2 (rows D and E, 10 November 2026): make all; make test-all (15702 tests, userspace 73/73, lot67-web-host 62/62); roadmap-check, strict-image-check, secrets-check; make qemu-lot88-desk (387.9 s) and make qemu-lot88-tv (179.2 s). The first TV run failed honestly: the hint was read at shell start, before the desktop set its mode, so the framebuffer size was 0 and the TV scale was refused; the safe scale now uses the desktop's 1024x768 mode when no framebuffer is on yet (host test added), and the rerun passed. Both boots in one run took 600.5 s on the box, hence two CI jobs.
