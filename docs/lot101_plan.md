# Lot 101 plan - reader colour spaces (lab(), lch(), oklab(), oklch(), color()), currentcolor, named scroll timelines

Base: 9dd02b6 (origin/main). PR 258 (the Lot 100 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot101.
One guest engine (userspace/webeng.c, unchanged here) and one shell path.
Kernel, rights and ABI unchanged. US-031 stays partial. phase3_complete
stays false. US-068/US-073, US-076, US-080 and US-081 stay partial.
US-078 and US-097 stay simule.

Scope taken from the Lot 100 gaps (gap 1, colours: currentcolor, lab(),
lch(), oklab(), oklch() and color(); gap 2, animations: the named
timelines stored since Lot 99 and never resolved): the CSS Color 4
colour functions of the other spaces, currentcolor in the reader paint,
and the scroll-timeline and view-timeline properties with the scroll
timeline of the root element driving animations at a reader scroll
position.

A row is listed only when its code, tests and docs are in the branch.
Size: 11149 lines added and 42 removed outside docs (code 600 / 30 in wf_cssprop.c/.h, wf_sitecss.c/.h, wf_cssbg.c, wf_html.c/.h and shell_fetch.c; models and generators 1184 new, 45 / 2 converted; hand suite 586; generated suites 8527; QEMU page, server page, Makefile and CI 197; converted tests 9 / 9; run_all_tests.sh 1 / 1).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | lab(), lch(), oklab(), oklch() (CSS Color 4 section 9) | not a colour | lab(L a b [/ A]), lch(L C H [/ A]) and their OK forms, space syntax only (a comma refuses the colour), letters in any case. L a number or a percentage of 100 (1 for the OK forms), clamped to 0..100 (0..1); a and b numbers or percentages of 125 (0.4); C a number or a percentage of 150 (0.4), at least 0; H as for hsl() (deg, grad, rad, turn or a number of degrees, any value); a, b and C kept within 10000 (10 for the OK forms). CIE Lab with the D50 white, Bradford to D65 (Oklab through LMS), then linear sRGB clipped to 0..1 and the sRGB curve, all in 1/2^40 fixed point (ln, exp and powers computed by series), each channel rounded once to 1/10000 of 0..255. Proof: test_lot101_color.c (834 colours), test_lot101_mix.c, test_lot101_reader.c (lab, oklab), `make qemu-lot101` (#l101 lab(50 0 0) #777777, #o101 oklch(0.5 0 0) bg #636363) |
| B | color() (CSS Color 4 section 10) | not a colour | color(space c1 c2 c3 [/ A]) with srgb, srgb-linear, display-p3, a98-rgb, prophoto-rgb, rec2020, xyz, xyz-d65 and xyz-d50 (letters in any case), components numbers or percentages of 1 kept within 100; srgb gives its channels directly (clamped to 0..1); the others go through their transfer function (odd around 0) and matrix to linear sRGB (prophoto-rgb and xyz-d50 through Bradford), clipped and encoded as in A. Proof: test_lot101_color.c, test_lot101_reader.c (color_fn), `make qemu-lot101` (#a101 color(a98-rgb 0.5 0.5 0.5) #818181) |
| C | currentcolor in the reader paint | refused everywhere but as a registered <color> keyword (Lot 97); a value with it was dropped | color: currentcolor is color: inherit; in color (color-mix() included) it is the parent's final colour, in background-color and the background shorthand colour the element's own final colour (black at the top); an inherited background-color keeps currentcolor and takes the inheriting element's colour; in keyframes and @starting-style colours it is the same as in the declaration of that property (Lot 100 animations). The colours are resolved once the cascade is done, parents first. A registered <color> still keeps currentcolor as a keyword. Proof: test_lot101_paint.c (420 painted styles), test_lot101_reader.c (current_api, current_paint, current_anim, tl_cascade), test_lot100_paint.c and test_lot100_anim.c (converted), `make qemu-lot101` (#u101 bg #4000bf) |
| D | scroll-timeline-name, scroll-timeline-axis, view-timeline-name, view-timeline-axis, scroll-timeline, view-timeline (CSS Scroll-driven Animations 1) | not taken (animation-timeline: <dashed-ident> was stored since Lot 99 and the animation counted idle) | the names ([none \| <dashed-ident>]#), the axes ([block \| inline \| x \| y]#) and the shorthands ([<name> <axis>?]#, both lists, axis block when left out; view-timeline-inset is not taken); at most 4 items, names under 32 bytes, letters lower-cased; initial and unset give no list; any other invalid value (inherit too) drops the declaration; per longhand the strongest declaration wins (wf_csstl_parse, wf_csstl_coord, wf_csstl_text). animation-timeline: <dashed-ident> takes the timeline of the nearest element, the animated one or an ancestor (no timeline-scope), whose scroll-timeline-name, else view-timeline-name, has the name (the last one of its list). Only a scroll timeline of the root html element with axis block or y runs: the reader scroll position pm (0..10000, wf_site_set_scroll, shell web-css scroll N in percent) gives the local time floor(pm (delay + duration x count) / 10000) (wf_cssanim_scroll_ms; no effect when the count is infinite or the total is 0 or over 10^8 ms); the reader clock does not move it. A view timeline, an inline or x axis, another element or an unknown name gives no effect (idle as before). web-css adds " scroll N pos P" to the css-anim line (N animations on a scroll timeline, P the position). Proof: test_lot101_tl.c (800 declarations and cascades), test_lot101_scroll.c (520 sheets), test_lot101_reader.c (tl_api, tl_cascade, scroll_ms, scroll_reader), `make qemu-lot101` (#s101 at 25%, 75% and 100%, #v101 idle) |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB, webeng.c untouched). wf_site_t grows from 499960 to 505348 bytes (wf_site_node_t 1224 to 1252: the currentcolor value indices color_cur, bg_cur, scolor_cur, sbg_cur, the color_inh flag, the four timeline lists tl[4] and their ranks; wf_site_t: cur_resolved, anim_scroll, scroll_pm). wf_site_rule_t keeps 1108 bytes. Four new rule kinds from WF_SITE_K_TL0 = 44.
2. New API: wf_cssprop_set_current, wf_cssprop_color_cur, wf_csstl_parse, wf_csstl_coord, wf_csstl_text, wf_cssanim_scroll_ms, wf_site_set_scroll, wf_html_set_cssscroll; shell command web-css scroll N (0..100, "usage: web-css scroll <0..100>" otherwise).
3. Approximations: no gamut mapping (a colour outside sRGB is clipped per channel, CSS Color 4 asks for a gamut-mapped result); a, b and C clamped (10000, 10 for Oklab) and color() components kept within 100, so extreme values saturate earlier than a browser would; color-mix() and keyframes still interpolate in premultiplied sRGB whatever the spaces of the colours.
4. Choices: the scroll range of the root is taken as non-zero (any position 0..100% is reachable); at 100% a scroll-driven animation is in its after phase (no effect without fill-mode forwards or both); timeline names are lower-cased with the rest of the value, so they match whatever the case (CSS idents are case-sensitive); inherit is dropped for the timeline properties (they do not inherit); a view timeline is never active (the reader has no layout).
5. Gaps kept: scroll() and view() in animation-timeline stay refused (so the Lot 99 and Lot 100 tests are unchanged), timeline-scope, animation-range and view-timeline-inset are not taken; `none` components and relative colour syntax are not taken; a background shorthand colour word is still under 32 bytes.
6. The shell's web-css summary buffer sc grows from 96 to 128 bytes (the css-anim line with " scroll N pos P" and " color A T").
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - test_lot100_paint.c (regenerated by scripts/lot100_colorgen.py): 3 lines, the cases with currentcolor that were dropped and now paint: background-color: currentcolor gives #000000 (the element's colour, black), color-mix(in srgb, currentcolor, deeppink) gives #800a4a, color-mix(in srgb, currentcolor, hwb(...)) in color gives #403c00 (the parent's colour, black).
    - test_lot100_anim.c (regenerated): 6 lines, keyframes with currentcolor frames that were left out: background-color: currentcolor frames now paint (#00000000, #14281e, #6aff429f), a color frame keeps the element's colour (#000000), and two colour keyframes now take the currentcolor frame (#f500d0, #9af0f0).
    - scripts/lot100_colormodel.py: rgba() and colour_match() take currentcolor (lot100_colormodel.CUR) and the Lot 101 functions from scripts/lot101_colormodel.py; scripts/lot100_colorgen.py keeps the currentcolor frames; no other generated suite changes.
8. Removed after the mutation runs (each shown equivalent, its mutant dropped): a hue reduction before the cosine (cp_cossin takes any angle), a count check in wf_csstl_text, the anim_scroll and cur_resolved resets (the document is cleared by wf_site_prepare_ext), set-flag conditions in the currentcolor pass, a color_inh reset in the paint (reset_kind clears it), a null value check in the currentcolor parse, the null checks of wf_csstl_coord (wf_csstl_parse refuses both).
9. Mutations: 259 applied / 259 killed / 0 survived. Host: 257 mutants on wf_cssprop.c (185), wf_sitecss.c (70), wf_cssbg.c (1) and wf_sitecss.h (1), three of them checked with ASan (the srgb-linear shortcut, which keeps the matrix index in range; a timeline list longer than the item arrays; the separator of wf_csstl_text); the reader and engine suites of run_all_tests.sh plus test_webeng (85 suites, the Lot 101 suites first, baseline 0 failures). QEMU: 2 mutants on a git archive copy of the code commit (the shell's percent scale of web-css scroll, the wf_html_set_cssscroll forward), both killed by `make qemu-lot101`. Earlier runs left 18 then 2 then 1 survivors; each got a witness in test_lot101_reader.c, its code was removed as equivalent (point 8), or its mutant was shown equivalent (equal ranks cannot happen: the rule order is part of the rank). Two guards are kept without a killing mutant: the zero test before the a98-rgb power (the series at 0 gives 0 too, the guard avoids relying on it) and the empty-list test of the timeline lookup (its mutant reads inside wf_site_t, which ASan does not see).
10. CI: one new job, lot101-reader-spaces (make qemu-lot101, timeout 15 minutes).
11. ASan: make test-all with -fsanitize=address,undefined: base 9dd02b6 23040 tests 5 failed, head 6feb37c 23463 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).

## Not done (first gaps for lot 102)

1. Colours: gamut mapping; color-mix() in other colour spaces and with hue interpolation; relative colour syntax (from); `none` components; system colours; translucent colours in gradient stops; a background shorthand colour word of 32 bytes or more.
2. Timelines: scroll() and view() functions, timeline-scope, animation-range, view-timeline-inset, view timelines and scrollers other than the root (they need layout), case-sensitive names, a running scroll position from the reader's real scrolling and repaint.
3. Animations: a running clock and repaint, other reader properties than color and background-color, transition events.
4. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests (generated by scripts/lot101_colorgen.py, seeds 10101..10105, from the model scripts/lot101_colormodel.py with the Lot 95 to 100 models; the models use floating point with an unsure margin and Fractions and share no code with the C): test_lot101_color.c (834 colours of the five functions), test_lot101_mix.c (760 interpolations and color-mix() with them), test_lot101_tl.c (800 timeline declarations, coordinations and cascades), test_lot101_paint.c (420 painted styles with currentcolor), test_lot101_scroll.c (520 sheets on scroll timelines). Hand suite: test_lot101_reader.c (10 tests).
QEMU: tests/scripts/test_qemu_lot101.py, `make qemu-lot101` (one boot at 1x): /lot101/space.html from the Lot 67 server (web-css scroll at 25%, 75% and 100% with the reader clock at 500 ms, and an out-of-range position refused). CI job lot101-reader-spaces. `make qemu-lot100` is run again unchanged.

## Gates on the box

Code commit 6feb37c (the docs commit changes docs only), 15 November 2026, UTC+4:

- make all: pass; a fresh git archive make all: pass (27 s, the same 4 kernel warnings as the base).
- make test-all: pass, 23548 tests, 0 failed (770 s).
- make qemu-lot101: pass (107 s); make qemu-lot100: pass (491 s with the image rebuild, 111 s for the boot).
- roadmap-check, secrets-check, font-check, strict-image-check: pass.
- ASan/UBSan test-all: base 9dd02b6 23040 tests 5 failed, head 6feb37c 23463 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Other QEMU gates: left to GitHub CI.
