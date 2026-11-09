# Lot 87 plan - web forms and sheets, script walls, CEA EDID, delay trend

Base: main a3fc7fd (lot 86, PR #244). Branch lot87-webforms-chrome, PR
#245. One guest engine. Kernel rights unchanged. US-031 stays partial.
phase3_complete stays false. US-068/US-073, US-076, US-080 and US-081 stay
partial. US-078 and US-097 stay simule.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Reader stylesheet (US-031) | 40 nodes, 32 rules, 3 KiB of CSS; type/class/id/attr, combinators, a few pseudo-classes | 192 nodes, 128 rules, 16 KiB of CSS. Attribute operators ^= $= *= \|= and the i flag. :first-of-type, :last-of-type, :only-child, :only-of-type, :nth-last-child, :nth-of-type, :nth-last-of-type, :empty, :root, :not() of a class/id/attr/tag, :is() and :where() with two simple alternatives. font-style, text-decoration, visibility. Custom properties on :root/html/body read by var() with a fallback. @media lists (and, width, height, orientation, prefers-color-scheme, print), @supports |
| B | External stylesheets (US-031) | `<link rel=stylesheet>` and @import ignored | wf_csslink lists them (media checked against the viewport, print dropped), the shell fetches up to WF_CSSL_SHEETS = 4 (attempts, not successes), @import is followed one level, and the text goes to wf_site_prepare_ext. A saved page (web-load) reads its sheets from the same directory by base name only ('/' and '..' refused). The status prints "css-links found F fetched N failed M media D". wf_http resolves dot segments (RFC 3986 5.2.4) and keeps the query |
| B2 | Cascade layers (US-031) | @layer blocks read as plain rules | @layer statements and blocks, named, nested ("outer.inner") and anonymous, up to 12 layers. Order: first appearance among siblings; inside a layer its sublayers are weaker than its own rules ("a.b" registers "a" first). Normal declarations: a later layer beats an earlier one, unlayered beats every layer. !important: reversed, the first layer wins and every layer beats unlayered. `@import url(x) layer(name)`, bare `layer` and `supports(cond)` (not fetched when false); leading `@layer a, b;` statements of a fetched sheet are written before its imports. The status adds " layered L unsupported U" when not zero. A full layer table keeps the rules unlayered and counts them |
| B3 | Cascade order fix | rank = spec * 10 + (order & 1023) | a rule late in the sheet beat a selector with one more type (order up to 1023 against a step of 10). Specificity is now packed (ids, classes, types) above the order |
| C | Script walls | "enablejs" or nothing | wf_compat names the wall: enablejs, noscript, challenge, blocked (4xx/5xx block page), spa (empty root), browser (unsupported browser). The status prints "web-compat VERDICT: message [fallback URL]". The fallback is a `<noscript>` link, a meta refresh, or a known lite page. A captcha only counts as a challenge on a short page (under 600 text bytes) |
| C2 | Compat set re-measured 2026-11-09 | lot 86 numbers | google gbv=1 93419 B (enablejs), DuckDuckGo html 30706 B, DuckDuckGo lite 22535 B (0 scripts), bing 90781 B (20 scripts), text.npr.org 6079 B, lobste.rs 55595 B, old.reddit 403 23406 B ("blocked by network security"), github 503539 B, wikipedia.org 122421 B. Fixtures for ddg lite, npr, reddit, a challenge page and an SPA shell |
| D | CEA-861 EDID extension (US-080) | count only (byte 126) | edid_cea_parse: revision, underscan/audio/YCbCr flags, SVDs with the native bit, audio blocks, HDMI vendor block (physical address), HDR static metadata, detailed timings. edid_cea_best: native SVD, then the first DTD, then the largest known VIC. Boot prints "[GFX] edid ext ..." (the summary, "tag 0xNN not parsed" or "none"). SYS_DISPLAY_INFO carries a fixed summary (edid_cea_summarize: status, revision, flags, first 8 VICs, native VIC, audio channels, best mode and where it came from, HDMI address, TMDS clock, TV sink guess) and display-info prints "display cea ..." plus "display tv-hint scale_q N (not applied)" for a TV sink. Still the EDID published at boot, not DDC |
| F1 | Autofill on the rendered page | offered only on the reader's mohhdy-form links | a click or Tab onto a form2 text control asks the shell (page_offer) for the remembered value of that host and field name and prints "osui web autofill offer name=N key=Down"; Down fills it. Not offered when the field already holds it. Cleared on a new page |
| F2 | Autofill list | one value per key, no drawn list | a key keeps its newest value and two older ones (WF_AF_HIST 3). The same value typed again moves to the front. The session file writes them oldest first, so a load rebuilds the order. web-fill lists them all, newest first. The desktop draws the list under the field (a box, a border, a highlight row, up to three rows) as the last web items of the scene. Down fills the next one ("pick i/n"), Up the previous one, a tablet click on a row fills that row and closes the list ("pick i/n click"), a click on the list padding is swallowed (the page under it is not clicked), Esc closes the list and keeps the text; one value keeps the lot 87 first-chunk behaviour (Down fills and closes). wf_formfill_offers read only the first a->n slots, so a host forgotten before another host hid its values: it now reads every used slot |
| E | Delay-gradient detector (US-068/US-073) | delivery-rate estimate only | p2p_trend: GCC-style trendline over 5 ms probe groups (window 20, smoothed accumulated delay, least-squares slope, adaptive threshold 6..600 ms). Overuse caps the pacer at 85 % of the acked rate, normal raises the cap 8 %/s up to 1.5x acked. A cap older than 2 s is ignored. Fed by the probe PONG; "p2p trend ..." line in the bwe report. Integer only. Still the lab path, not a WAN |
| G | ARM self-tests (US-076) | 9 checks | arm_port_trend (overuse after 60 rising probes, cap 15515 B/s, the same number on i386 and x86-64) and arm_port_cea (native 1080p SVD, HDMI 1.0.0.0, damaged block refused). The ARM boot test expects both lines |
| H | qemu-lot87 | none | EDID extension line, and display-info's cea line with the same revision, SVD count and best mode; web-css on + /lot87/page.html fetches main.css and its @import, never print.css, the missing import fails; web-read hides the banner and side note; /lot87/layers.html imports reset.css into a layer, skips gone.css (supports() false) and the unlayered rule wins over the more specific layered one; the wall and block pages are named; two remembered values, the GUI click offers both, the list is found in a screendump, Down/Down/Up pick ada, grace, ada, a tablet click on the second row picks grace, Enter posts user=grace. CI job lot87-layout |

## Review points

1. The GUI and the console share one store (shell_desk g_fill) and one formfill code path. The GUI asks through osui_web_page_ops_t (page_offer, page_offer_at); osui_gui.c does not call shell code directly.
2. The autofill value is not printed with the offer line, only the field name. The drawn list shows the values on screen.
3. The list is drawn by the compositor like any web item (RECT, BORDER, TEXT). When a page fills all 96 scene items, the last page items give way to the list.
4. External sheets are fetched with the same guest fetch and cookies as the page. A failed sheet is counted, the page still renders.
5. QEMU's std VGA EDID has a CEA extension. "best" there is the largest VIC listed (for example 4096x2160@50), not the panel mode: QEMU marks no native SVD. The mode set is unchanged; the line is informational.
6. p2p_trend sees the queue of the probe path only. It does not see cross traffic on a real WAN. The cap only lowers the pacer; it never raises it past the bwe estimate.
7. The CI runners use GCC 15 (C23). No -Wno-* was added.

## Not done (first gaps for lot 88)

1. No JavaScript VM. Pages behind an enablejs, challenge or spa wall (google gbv=1 included, measured 2026-11-09) are named and, when there is one, the fallback URL is printed; they are not run.
2. Autofill: three values per key, 32 keys, exact host match (no subdomain sharing), no per-entry delete from the GUI (autofill forget HOST only).
3. External sheets: one @import level, WF_CSSL_SHEETS attempts. Layers: 12 names; revert-layer and @scope are not handled; a layer statement after the first rule of a fetched sheet is not moved in front of its imports.
4. US-068/US-073 remain a laboratory estimator: no WAN, no cross traffic, the trend only sees the probe train.
5. US-080 still reads the EDID block published at boot. DDC/I2C is not driven; the CEA best mode and the TV scale hint are reported, not applied. The TV guess (HDMI and underscan, a CTA TV native VIC, or no physical size) is a heuristic.
6. US-081: the battery stays simulated; deep idle does not blank a real panel. Nothing changed in lot 87.
7. GUI chrome: the window types (app, chat, full, pane) already have chrome; lot 87 added no new QEMU tablet test for them.

## Tests

Hand-written host tests: tests/unit/userspace/test_lot87_css.c (13), test_lot87_layer.c (11), test_lot87_link.c (5), test_lot87_web.c (8), test_lot87_fill.c (5), test_p2p_trend.c (8), tests/unit/kernel/test_edid_cea.c (12), test_lot87_display.c (3), test_osui_gui.c (test_lot87_form2_autofill, test_lot87_offer_list), test_p2p.c (trend line), test_arm_portable.c (trend and cea).
QEMU: scripts/lot67_web_server.py /lot87/ routes, tests/scripts/test_qemu_lot87.py, `make qemu-lot87`, CI job lot87-layout. ARM: tests/integration/test_qemu_arm_boot.py expects the two new lines.

## Gates on the box (2026-11-09)

See the PR body for the final list; filled in when the gates ran.
