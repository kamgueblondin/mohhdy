# Lot 97 plan - animated registered properties, font units, average and per-pixel gradients

Base: aa74fac (origin/main, merge of PR 254, the Lot 96 branch). Branch
cursor/lot97. One guest engine (userspace/webeng.c) and one shell path for
the console and the desktop. Kernel, rights and ABI unchanged. US-031
stays partial. phase3_complete stays false. US-068/US-073, US-076, US-080
and US-081 stay partial. US-078 and US-097 stay simule.

Scope taken from the Lot 96 gaps, in the order of the brief: transitions
and animations of @property-registered properties (Lot 96 gap 2), em,
rem, ex and ch (gap 3), per-pixel painting where needed, repeating radial
gradients in vertical-rl, the mirrored vertical-rl cell partition and a
period of 0 painted as the average colour (gap 1). Conic children of
vertical boxes, background-clip: text and the other gap 1 items are left
(see Not done).

A row is listed only when its code, tests and docs are in the branch.
Size: 9768 lines added and 429 removed outside docs: reader, engine and shell code 1362; Lot 97 models and generators 1505; hand suites 729 (test_lot97_reader.c, test_lot97_engine.c); generated suites 5285; QEMU page, script, server page, Makefile and CI job 260; converted tests, models and regenerated suites 625; run_all_tests.sh 2.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | em, rem, ex, ch in registered values (Lot 96 gap 3) | kept as text (no font context) | an own registered value with a font unit is computed on its element with that element's font size (rem: the root's; ex and ch are half an em, the reader has no font metrics). The child of an inheriting property takes the computed length, not the em. Unregistered custom properties keep their text. calc() mixes (calc(1em + 5px)) are computed too; a result outside the fixed point range (9999em on 16px) keeps its text. Proof: test_lot97_reader.c (font_units), test_lot97_sheet.c, test_lot95_sheet.c and test_lot92_props.c (converted), `make qemu-lot97` (#sp97: 2em on 20px inherited as 40px) |
| B | Animations of registered properties (Lot 96 gap 2) | @keyframes and animation were ignored | @keyframes (identifier or quoted name, from/to/N% with two decimals, at most 8 rules kept by name, the last of a name wins; at the top level and in applying @media, @supports, @layer). The animation shorthand (duration, timing function, delay, count or infinite, direction, fill mode, play state, name; up to 4 comma separated animations) is cascaded as one property (not inherited; none, initial and unset clear it; an invalid list is dropped). At the reader clock each animation, later ones on top, sets the registered properties its frames declare: frames that do not match the syntax, use var() or are !important are left out, the later block wins at one offset, the value under the animation fills a missing 0% or 100%, the segment progress is eased by the frame's animation-timing-function or the animation's. An !important own declaration keeps its value. Interpolation: numbers, lengths, percentages, length-percentage mixes, angles, times, resolutions and integers (rounded), item by item in '+' and '#' lists of equal length, colours in premultiplied sRGB (hex, named, rgb()/rgba() with commas or "/ alpha"); anything else (custom idents, hsl(), different alternatives) is discrete (flips at 50%). The reader properties are cascaded again when a value changed, so color: var(--c) follows the animation. Proof: test_lot97_ease.c, test_lot97_interp.c, test_lot97_timing.c, test_lot97_sheet.c, test_lot97_reader.c, `make qemu-lot97` (#a97, #q97, #i97 at 500 and 2000 ms) |
| C | Transitions of registered properties (Lot 96 gap 2) | ignored | @starting-style blocks give starting values of custom properties only (other declarations inside are dropped). The transition shorthand (property or all, duration, timing function, delay, normal or allow-discrete; up to 4) is cascaded like animation. While a transition runs (or waits in its delay) the value goes from the starting value to the cascaded one at the reader clock; same values, a value off the syntax, a duration plus delay of 0 or a discrete pair without allow-discrete give no transition. An animation of the same property wins. Proof: test_lot97_reader.c (starting_style, transitions), test_lot97_sheet.c, `make qemu-lot97` (#t97 background #00009b at 500 ms) |
| D | Reader clock and summary | none | wf_site_set_time(ms) (0..1e8, 0 by default) sets the clock of the next prepares; the shell command "web-css time N" sets it (a bad value prints "usage: web-css time <0..100000000>"); web-css prints "css-anim keyframes K animated A transitioned T font F at Nms [dropped D]" on a page that has keyframes, animation or transition lists or font-relative values. The reader colour (parse_color) takes rgb(R, G, B) and rgba(R, G, B, 1) with integers 0..255, the form an animated opaque <color> is written in. Proof: test_lot97_reader.c (clock, rgb_color, keyframes_table), `make qemu-lot97` (css-anim lines, usage lines) |
| E | Period of 0 (Lot 96 gap 1) | refused by the engine and the reader | the engine paints a repeating gradient whose period is 0 in its average colour: stops taken evenly spaced, the mean of the premultiplied linear interpolation (weights 1 at both ends, 2 between), each channel and the opacity rounded half up, all transparent stops give transparent black; it becomes a solid gradient along the y axis (180deg, two stops) and is painted as one rectangle. The reader (wf_cssbg.c) passes such gradients on, and a registered <image> with a period of 0 matches. Proof: test_lot97_engine.c (average), test_lot97_cells.c (80 cases), test_lot96_* (converted), `make qemu-lot97` (#avg97), `make qemu-lot96` (#flat96, #img96 converted) |
| F | Per-pixel axis painting (Lot 96 gap 1) | 32 flat cells | a gradient painted in cells whose angle is 0, 90, 180 or 270deg without a corner (every repeating linear gradient along a side or a quarter turn, and the period-0 average) is painted per pixel line across its axis: the colour at the pixel centre, equal neighbours merged into one rectangle. When that needs more rectangles than the 32 reserved cells, the cells are painted as before. Proof: test_lot97_engine.c (axis_runs), test_lot97_cells.c (130 cases), test_lot96_cells.c (regenerated), `make qemu-lot97` (#axis97) |
| G | vertical-rl cells and repeating radial (Lot 96 gap 1) | cells cut in the logical frame (mirrored physically); a repeating radial child refused | in a vertical-rl box the cells of a gradient at an angle (and of a repeating one) are cut with their rows from the logical bottom, which is the physical left, and a repeating radial centre is mirrored in the logical frame, so the box paints as the same box in horizontal flow. Two marks in grad_repeat (bit 2 mirror, bit 4 rl partition; bit 1 still means repeating) are set by the forward pass and cleared by the back pass. Proof: test_lot97_engine.c (vertical_rl: 8 gradients compared with horizontal flow), test_lot97_cells.c (190 cases), test_lot96_engine.c (converted), `make qemu-lot97` (#rl97) |

## Review points

1. Structure sizes: we_node_t keeps 4668 bytes and we_doc_t 870824 bytes (< 1 MiB, unchanged). wf_site_t grows from 447372 to 480744 bytes (wf_site_node_t 952 to 1124: starting bindings sbind/srank, the animation and transition list indexes and ranks; the @keyframes table of 8 entries; counters). wf_site_rule_t keeps 1108 bytes (the starting flag fills padding). The reader adds about 12 KB of static scratch (the frames of one property, the bindings before the animation pass, two bit masks per element).
2. New cascade kinds WF_SITE_K_ANIM (27) and WF_SITE_K_TRANS (28) in wf_sitecss.h, after the existing ones (no renumbering).
3. Approximations: ex and ch are half an em (no font metrics); cubic-bezier is solved by bisection on x in 1/2^30 and y is rounded half away from zero (within 0.5/10000 of the exact curve, checked by the generator against Fractions); hsl() and hsla() colours are valid <color> values but interpolate discretely; colours are written as rgb()/rgba() with integer channels; times are rounded to 1 ms; the clock is a value set by the shell, not a running timer (no repaint loop).
4. Not taken on purpose: the longhands animation-name, animation-duration and the others (only the shorthands are read), animation-composition, transitions of the reader's own properties (only registered custom properties animate), transitionrun-like events, @keyframes inside @scope or @starting-style.
5. The reader colour parse_color now takes rgb(R, G, B) and rgba(R, G, B, 1) with integer channels (needed for color: var(--c) to follow an animated <color>); other rgb() forms stay refused.
6. The vertical-rl partition change also applies to non-repeating gradients at an angle in rl boxes (their cells are now cut from the physical left); colours are still taken at the cell centres.
7. A period of 0 is now accepted by the reader and by the <image> match of a registered property (Lot 96 refused both).
8. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - test_lot92_props.c: calc(1em * 2) on a registered <length> is now computed to 32px (was kept as text).
    - scripts/lot95_propmodel.py and scripts/lot95_propgen.py: em, rem, ex and ch use the font context of the element (16px); regenerated test_lot95_sheet.c.
    - test_lot96_engine.c: a period of 0 is painted as the average (ANGLE 180deg, no repeat, one rectangle) for 6 values that were refused, the 4 malformed values stay refused; the 3x5 "to top" box is one rectangle of 3x5 (was 5 cells of the same colour); the repeating radial child of a vertical-rl box is painted in 33 rectangles, grad_dir RADIAL, grad_repeat 1 (was refused).
    - test_lot96_reader.c: a period of 0 is written by the reader (several asserts); one '<image>+' match now gives 1, and a new assertion with a one-stop gradient gives 0.
    - scripts/lot96_gradmodel.py and scripts/lot96_gradgen.py: a period of 0 is valid (reader) and painted as the average (engine); axis cases use the Lot 97 per-pixel model; regenerated test_lot96_cells.c, test_lot96_bgval.c, test_lot96_accept.c, test_lot96_sheet.c.
    - tests/scripts/test_qemu_lot96.py and its server page: #flat96 keeps its colour and gets the period-0 gradient, #img96 takes its own value (was refused, the colour alone and the initial value).
9. Removed while writing the mutations, since no result could change (each was a mutant that survived and was then shown equivalent):
    - two guards of the axis runs (the reserved count never passes 32 and the origin box is never empty there);
    - the empty-name test of the keyframes lookup (no kept @keyframes has an empty name);
    - the reset of the animation and transition lists before the second cascade (the same rules give the same lists; the per-kind reset loop stays);
    - the font size <= 0 test of the font units (the reader never gives a size of 0 or less);
    - the universal syntax test before an interpolation ('*' has no parsed alternative, so the missing alternative already goes discrete);
    - the x >= 1 end test of the bezier solver (x = 1 gives u = 2^30 and y exactly 1 through the bisection);
    - the duration plus delay <= 0 test of a transition value (such a transition ends at once in the elapsed test below);
    - the own-value mask (the font-relative value is computed wherever it is found, own or inherited text);
    - the bound inside the list item loop (folded into the top-level comma count before the loop);
    - the font-relative prefilter (wf_cssprop_font_relative): the font units are computed on every registered value, since wf_cssprop_compute_font gives 0 when the value does not change (a value without a font unit was already computed when it was cascaded, a unit inside a string or a name is not computed).
10. Mutations: 168 applied / 168 killed / 0 survived (166 host on wf_cssprop.c, wf_sitecss.c and webeng.c; 2 QEMU on the web-css time command of shell_fetch.c). Survivors of the earlier runs got witnesses or their code was removed (point 9). The full list is kept with the run artifacts (mutations.txt).
11. CI: one new job, lot97-reader-engine (make qemu-lot97, no guest clock option, timeout 15 minutes).
12. ASan: no new failure; the same five known suites fail on the base and on this branch (see Gates on the box).

## Not done (first gaps for lot 98)

1. Gradients: conic children of vertical boxes (refused), background-clip: text, a clip or origin per layer, background-attachment, fetched background images; per-pixel painting only along the axes (other angles keep 32 cells).
2. Animations: longhands, animation-composition, a running clock and repaint, transitions of non-custom properties, hsl() interpolation, @keyframes in @scope.
3. Fonts: no font metrics (ex and ch are 0.5em), no lh/rlh/cap units.
4. @property: no <frequency>, <ratio> or <flex>; more than 8 alternatives, 16 registrations or 127-byte values are refused (unchanged).
5. Math: min(), max(), clamp() of a length-percentage mix are kept; fixed point with 1/10000 steps (unchanged).
6. Autofill: three fixed summer time rules (no tz database) (unchanged).
7. PSL: 360 hand-written entries (unchanged).
8. NE2000: the FIFO holds 16 frames, the drain is skipped during an LLM session (unchanged).
9. Shaping: single node settings, no WAN (US-068/US-073 partial, unchanged).
10. US-080: no DDC, fixed safe area margin (unchanged).
11. US-081: no backlight, no watts, battery simulated (unchanged).

## Tests

Host tests (generated by scripts/lot97_animgen.py, seeds 9701..9704, from the model scripts/lot97_animmodel.py, and by scripts/lot97_gradgen.py, seed 9705, from scripts/lot97_gradmodel.py with the Lot 94 and Lot 96 models; the models use Fractions and share no code with the C): test_lot97_ease.c (400 timing functions with their values), test_lot97_interp.c (500 interpolations), test_lot97_timing.c (300 animation and transition lists with their progress), test_lot97_sheet.c (160 sheets), test_lot97_cells.c (400 boxes: 80 averages, 130 axis gradients, 90 vertical-rl repeating radial, 100 vertical-rl linear). Hand suites: test_lot97_reader.c (13 tests), test_lot97_engine.c (3 tests).
QEMU: tests/scripts/test_qemu_lot97.py, `make qemu-lot97` (one boot at 1x): /web/lot97.html (web-render, 37 rectangles checked against the models) and /lot97/anim.html from the Lot 67 server (web-css at 500 ms and 2000 ms, bad clock values). CI job lot97-reader-engine.

## Gates on the box

Run on 81f14c5 (the code of this lot; the docs commit after it changes docs only) on 14 November 2026 (UTC+4), one lot QEMU guest at a time beside the demo guest (separate sockets, -display none, no host forwarding); none was rerun on that commit. Local gates as set for this lot (make all, test-all, ASan, mutations, qemu-lot97, qemu-lot96, plus roadmap-check, secrets-check, font-check, strict-image-check); GitHub CI runs the rest.

| Gate | Result | Time |
|------|--------|------|
| make all | pass | 0 s (built) |
| make test-all | pass, 22308 tests, 0 failed (lot67-web-host 64/64) | 303 s |
| make qemu-lot97 | pass | 107 s |
| make qemu-lot96 | pass | 40 s |
| roadmap-check, secrets-check, font-check, strict-image-check | pass | 7 s |
| make all on a fresh git archive of 81f14c5 | pass, the same 4 kernel warnings as the base (none new) | 27 s |

ASan and UBSan (TEST_CC="gcc -fsanitize=address,undefined -fno-sanitize-recover=all", make test-all on git archive copies of each tree): base aa74fac 21924 tests, 5 failed (test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt), 1701 s; head 81f14c5 22223 tests, 5 failed (the same five), 908 s. All Lot 97 suites and the converted suites pass under ASan. (The ASan count is 85 below the plain test-all count on both trees, the same gap as in Lot 96: base 22009 plain, 21924 ASan.)

Mutations: 168 applied / 168 killed / 0 survived (166 host, 2 QEMU).
