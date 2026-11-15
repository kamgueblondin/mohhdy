# Lot 106 plan - the body as a view timeline subject, html inactive, Oklab default interpolation of colours

Base: 419085f (origin/main). PR 263 (the Lot 105 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot106.
One guest engine (userspace/webeng.c and its .inc files; layout results
unchanged, webeng.c itself not edited). Kernel, rights and ABI
unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and US-097
stay simule.

Scope taken from the Lot 105 gaps, in the preference order of the brief:
html and body geometry (gap 1, first part), the oklab default
interpolation of colours (gap 4, first part). Add composition carrying
missing components (gap 3) is not done: CSS Color 4 4.4 makes a missing
component 0 outside interpolation, and add and accumulate are not
interpolation, so the Lot 102 behaviour is the specified one (review
point 6). No layout change, no margin collapsing, no new layout engine.

A row is listed only when its code, tests and docs are in the branch.
Size: see review point 10.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Oklab default interpolation (wf_cssprop.c) | keyframes and transitions of colours were interpolated in premultiplied sRGB | CSS Color 4 12.1: Oklab unless both colours are legacy sRGB forms (hex, named colours and transparent, rgb(), rgba(), hsl(), hsla(), hwb(); currentcolor counts as legacy as the reader keeps it as an sRGB colour). The Oklab path is color-mix(in oklab, A (100 - t)%, B t%) through cp_mix_core (split out of cp_mix): premultiplied, missing components carried by analogous set, gamut-mapped back to sRGB, each channel rounded half away from zero. Legacy pairs, add and accumulate keep the Lot 97/102 sRGB path. Proof: test_lot106_interp.c (640 interpolations with the space noted, 80 lists, 120 add/accumulate), test_lot106_reader.c, `make qemu-lot106` (#o106 #8c53a2) |
| B | interpolation past the ends (wf_cssprop.c) | not reachable in Oklab | a weight below 0 (an easing that overshoots): the premultiplied components are divided by the weighted alpha sum as it is (CSS Color 4 12.3), the alpha is clamped to 0..1 afterwards; with a negative weight a component is kept within +-32 (no overflow). Proof: test_lot106_interp.c, test_lot106_reader.c (overshoot) |
| C | exact OKLab values (wf_cssprop.c) | an oklab() or oklch() colour went to Oklab again through linear sRGB in 1/2^40 | cp_rgba_t carries the OKLab values of an oklab()/oklch() colour and of a mix in oklab or oklch (ok, ok_ok); a mix in oklab takes them as they are (the round trip lost about 1e-3 of a channel near L = 0). color-mix results that used the round trip may move by one unit (none did in the Lot 100 to 105 suites) |
| D | the body record (wf_geom.h, webeng_geom.inc) | the reader view writes html and body bare, so they had no geometry | wf_geom_t gains body, filled by we_geom_feed from the layout's body element (the first element child of the root element named body), without a mark; its nearest scroller is the root |
| E | body and html as view timeline subjects (wf_sitecss.c) | a view-timeline on body or html had no box (inactive) | body (named body, at the top or a child of the root html) reads the body record: view() and named view timelines on body run against the root scroller; the root html element has no ancestor scroll container, so its view timeline is inactive and asks for no geometry (CSS Scroll-driven Animations 1, 3.1). geom_used counts the body record. Proof: test_lot106_sheet.c (280 sheets), test_lot106_geom.c (160 engine layouts), test_lot106_reader.c, test_lot106_engine.c, `make qemu-lot106` |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB; the engine structures are unchanged). wf_geom_t 12312 to 12376 bytes (one wf_geom_box_t of 64 bytes, body; one static in shell_fetch.c). wf_site_t unchanged (513828 bytes). cp_rgba_t (local to wf_cssprop.c, on the stack) gains ok[3] and ok_ok (28 bytes).
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3):
    - userspace/wf_cssprop.c: the Oklab default (cp_legacy, cp_mix_core), the alpha sum past the ends, the exact OKLab values.
    - userspace/wf_sitecss.c: site_body, geom_box, the html early return in the view clock, geom_used through geom_box.
    - userspace/wf_geom.h: the body record.
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added (the command registry is unchanged).
3. body's own animated style: body is a subject whose timeline its descendants read (the visible use, a named view timeline on body). Its own animated colour is computed but not written: the reader view writes a bare `<body>`. The QEMU page refuses a colour on body.
4. Legacy sRGB past the ends keeps the Lot 97 order (the alpha clamped before the division); only the Oklab path follows CSS Color 4 12.3. A legacy pair with an overshooting easing may differ from a browser by a few units where the alpha changes.
5. currentcolor counts as a legacy colour: the reader resolves it to an sRGB colour (wf_cssprop_set_current); a browser would also take the computed color's form.
6. Add composition with missing components (Lot 105 gap 3): not done, by the specification (CSS Color 4 4.4: a missing component is 0 when the colour is not interpolated). Add and accumulate stay sRGB.
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour, the old value is refused where it was asserted):
    - Generators converted through scripts/lot106_model.py (keep(): a case the old model left out stays out; a case whose Oklab answer is unsure gets a new t from a side stream seeded by the crc32 of the case, then an rgb() second colour, so the main stream does not move): scripts/lot97_animmodel.py, lot100_colorgen.py, lot101_colorgen.py, lot102_gen.py, lot103_gen.py. Regenerated: test_lot100_mix.c (23 inputs), test_lot100_anim.c, test_lot101_mix.c (47 inputs), test_lot101_scroll.c, test_lot102_none.c (20 inputs), test_lot102_sheet.c, test_lot103_hue.c (22 inputs). Expected values move where a non-legacy colour is interpolated.
    - scripts/lot104_model.py and lot105_model.py (view_clock: html inactive, body from its record), scripts/lot104_gen.py (the harness fills g_g.body): test_lot104_sheet.c (html cases now idle), test_lot105_sheet.c (2 values).
    - Hand: test_lot100_reader.c (rgb(198, 73, 109) with a legacy witness; #5147d2 was #4000bf; rgb(189, 1, 1) was rgb(192, 1, 1)), test_lot101_reader.c (185,185,185 and 140,83,162, with an rgb() witness), test_lot103_reader.c (198,180,180, with a magenta/lime witness).
    - tests/scripts/test_qemu_lot103.py: the keyframes colour #007e14 is now #006300 (Oklab), the old value refused.
8. CI: one new job, lot106-reader-body (make qemu-lot106, timeout 15 minutes).
9. Mutations: see the Gates section and /workspace/run/lot106/mutations.txt on the box.
10. Size: see the Gates section.

## Not done (first gaps for lot 107)

1. Geometry: the body's own animated style written by the reader; html and body as box scrollers of their own (overflow on the root is the viewport's); view timelines in section 1 and later; the console web-load path (no layout).
2. Box scrollers: heights relative to fonts, the viewport or the containing block, max-height, horizontal box scrolling, scroll-padding for auto insets.
3. Animations: a running clock and repaint, other reader properties than color and background-color, transition events.
4. Colours: legacy interpolation past the ends in the CSS Color 4 12.3 order, relative colour syntax (from), system colours.
5. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests (generated by scripts/lot106_gen.py, seeds 10601..10603, from the model scripts/lot106_model.py with the Lot 97 to 105 models; the models are written from the specifications and share no code with the C): test_lot106_interp.c (840 cases: 640 interpolations, each noted srgb or oklab, 80 `<color>+` lists, 120 add/accumulate; the current colour is #008080), test_lot106_sheet.c (280 sheets: a body record and boxes, the target #i or body, its style, counters and flags), test_lot106_geom.c (160 engine layouts with a styled body: the body record and the root state). Hand: test_lot106_reader.c (5 tests: the space choice, past the ends, add in sRGB, keyframes, the body subject), test_lot106_engine.c (2 tests: the body record from the engine, two passes with a named view timeline on body).
QEMU: tests/scripts/test_qemu_lot106.py, `make qemu-lot106` (one boot at 1x): /lot106/body.html from the Lot 67 server (640x480 view, body at 8 px, 946 px high, root scroll range 482 px): #b106 on --b106 at "web-css scroll 0, 50, 100" is #545454, #808080, #ababab; #h106 on html's --h106 has no colour; #o106 (color(srgb 1 0 0) to blue at 500 ms) is #8c53a2 in Oklab (sRGB #800080 refused); #s106 (red to blue) #800080; the css-anim line ends with "view 1 need 1 geom 1". CI job lot106-reader-body.

## Gates on the box

(filled at the end of the lot)
