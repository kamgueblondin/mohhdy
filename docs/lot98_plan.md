# Lot 98 plan - animation and transition longhands, animation-composition, hsl() interpolation, ex and ch from the engine font

Base: bec0eba (head of PR 255, the Lot 97 branch). PR 255 is not merged
into origin/main (aa74fac when this lot started), so the branch starts from
its head. Branch cursor/lot98. One guest engine (userspace/webeng.c,
unchanged here) and one shell path. Kernel, rights and ABI unchanged.
US-031 stays partial. phase3_complete stays false. US-068/US-073, US-076,
US-080 and US-081 stay partial. US-078 and US-097 stay simule.

Scope taken from the Lot 97 gaps (gap 2 and gap 3), in the order of the
brief: the animation and transition longhands, animation-composition,
@keyframes inside @scope, hsl() interpolated instead of switching at 50%,
and ex and ch from the font the engine draws instead of half an em.

A row is listed only when its code, tests and docs are in the branch.
Size: 8217 lines added and 110 removed outside docs (code 546 / 96: wf_cssprop.c/.h, wf_sitecss.c/.h; models and generator 1034; hand suite 456; generated suites 5975; QEMU page, server page, Makefile and CI 196; converted tests 9; run_all_tests.sh 1).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Animation longhands (Lot 97 gap 2) | only the animation shorthand was read | animation-name (none or an identifier, not a CSS-wide keyword, under 32 bytes), animation-duration (a time not negative, or auto = 0s), animation-timing-function, animation-delay, animation-iteration-count (infinite or a number 0..100000), animation-direction, animation-fill-mode, animation-play-state and animation-composition are read as comma lists (one token per item, at most 4, no empty item); an invalid list is dropped, initial and unset reset the longhand (inherit is not taken, as for the shorthands). Each longhand is cascaded on its own (specificity, then order): the animation shorthand sets the first eight (not animation-composition), so a later longhand changes one field and a later shorthand sets them all again. The lists are coordinated (CSS Values 4): animation-name gives the number of animations, the other lists repeat cyclically, a list not set is its initial value. Proof: test_lot98_longs.c (360 lists and 220 coordinations), test_lot98_sheet.c, test_lot98_reader.c (longhands, long_api), `make qemu-lot98` (#a98) |
| B | Transition longhands (Lot 97 gap 2) | only the transition shorthand | transition-property (all, none alone, or an identifier), transition-duration, transition-timing-function, transition-delay and transition-behavior, cascaded and coordinated the same way (transition-property gives the count; the last item naming the property or all applies). transition: none is now kept as a list whose property is none (Lot 97 cleared it), so a later transition-duration does not start transitions; animation: none is kept too (animation-name none animates nothing, as when Lot 97 cleared the list). A list of 128 bytes or more is dropped, never cut (the Lot 90 value limit, which the model applies too). Proof: test_lot98_longs.c, test_lot98_sheet.c, test_lot98_reader.c (transition_longhands), `make qemu-lot98` (#h98) |
| C | animation-composition (Lot 97 gap 2) | not read | replace, add or accumulate per animation, and per keyframe (an animation-composition declared in a keyframe block wins for that keyframe). With add or accumulate each keyframe value given in the rule is put on the value below (wf_cssprop_add): numbers, lengths, percentages, length-percentage mixes, angles, times, resolutions and integers are summed, '+' and '#' lists item by item (equal lengths), colours by their premultiplied sRGB channels (alpha clamped to 1, channels to 0..255); any other value replaces. The 0% and 100% keyframes filled in from the value below stay that value. Animations stack in list order: the value below an animation is the result of the ones before it. accumulate is treated as add (Web Animations 1 only tells them apart for values this reader keeps discrete). The css-anim line gets " composed C" (the number of element properties with a composed keyframe) when C > 0. Proof: test_lot98_add.c (560 sums), test_lot98_sheet.c, test_lot98_reader.c (add_api, composition), `make qemu-lot98` (#b98, css-anim ... composed 1) |
| D | @keyframes inside @scope (Lot 97 gap 2) | dropped | @keyframes inside @scope (also nested and under @media, @supports and @layer, four at-rule levels at most) are taken like top-level ones: keyframe names are global, the scope root does not limit them. @property inside @scope is still not taken. Proof: test_lot98_reader.c (scope), test_lot98_sheet.c (keyframes wrapped in @scope), `make qemu-lot98` (#a98 uses keyframes declared in @scope, registered 3 with an @property in @scope) |
| E | hsl() interpolation (Lot 97 gap 2) | hsl() and hsla() were valid <color> values but flipped at 50% | hsl(H, S%, L%[, A]) and hsla() with commas (S and L must be percentages there), or hsl(H S L[ / A]) with spaces (S and L percentages or numbers): H is a number of degrees or a deg angle taken modulo 360, S and L clamped to 0..100%, A as for rgb(). The colour is converted to sRGB with CSS Color 4 hslToRgb, exactly in fixed point (each channel rounded half away from zero to 1/10000 of 0..255), then interpolated (and added) like any rgb() colour in premultiplied sRGB; the result is written rgb()/rgba(). Proof: test_lot98_hsl.c (480 cases, 12 of them chosen so that a channel rounds up at its last 1/10000), test_lot98_reader.c (hsl), test_lot97_interp.c (converted), `make qemu-lot98` (#h98 background #800080) |
| F | ex and ch from the engine font (Lot 97 gap 3) | half an em | the engine draws text with the 8x8 glyphs of kernel/gfx_font8.h in an 8 px advance for a 16 px font (WE_CHAR_W 8, WE_FONT_PX_DEFAULT 16): the 'x' glyph has 5 inked rows, so ex is 5/16 em; the advance of '0' is 8 px, so ch is 1/2 em (WF_CSSPROP_EX_NUM/DEN, WF_CSSPROP_CH_NUM/DEN in wf_cssprop.h). A host test reads the glyph rows and the engine constants so the ratios cannot drift from the font. The HD fonts (gfx_font_hd*) only redraw glyphs inside the same cells and do not change the layout, so they do not change ex or ch. Proof: test_lot98_reader.c (font_metrics), test_lot97_reader.c (converted), `make qemu-lot98` (#sp98: calc(2ex + 1ch) on 16px is 18px) |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB, webeng.c untouched). wf_site_t grows from 480744 to 494572 bytes (wf_site_node_t 1124 to 1196: the two list indexes and ranks of Lot 97 become 14 indexes, 14 shorthand flags and 14 ranks, one per longhand; a counter). wf_site_rule_t (1108), wf_cssanim_t (72, the composition byte fills padding) and wf_csstrans_t (68) keep their sizes.
2. New cascade kinds WF_SITE_K_LONG0 (29) .. 42 in wf_sitecss.h, after the existing ones (no renumbering). The 14 longhands share the Lot 90 list pool (32 values per page, deduplicated) with the background lists.
3. New API: wf_cssprop_add, wf_cssanim_long, wf_csstrans_long, wf_cssanim_coord, wf_csstrans_coord, the WF_CSSANIM_L_* and WF_CSSTRANS_L_* indexes, WF_CSSANIM_REPLACE/ADD/ACCUMULATE and wf_cssanim_t.comp.
4. Choices: animation-composition is not set by the animation shorthand (its grammar in CSS Animations 2 does not include it); add and accumulate are the same here; a value that cannot be added replaces (Web Animations falls back the same way for discrete values); the filled 0%/100% keyframes are not composed (they are the value below already); keyframe names are global across @scope; transition: none is kept as transition-property: none (CSS Transitions 1), Lot 97 treated it like initial.
5. Approximations: ex and ch follow the 8x8 font of the engine at every font size (linear scaling, no per-size metrics); hsl() hues take numbers and deg only (turn, rad and grad are not part of the <color> value check of the reader, unchanged since Lot 95); an hsl() value is still not a reader colour (color: var(--c) with an hsl() value is not painted; an animated or composed hsl() value is written rgb() and is painted).
6. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - test_lot97_reader.c: 1ex on 20px is 6.25px (was 10px); 1.0001ex on 15px is 4.688px (was 7.5008px), still checking the rounding half away from zero.
    - test_lot97_interp.c (regenerated from scripts/lot97_animmodel.py, which now reads hsl() through scripts/lot98_animmodel.py): 4 lines with hsl(120, 50%, 50%) are interpolated (code 2 and an rgb()/rgba() result) where they flipped at 50% (code 1).
    - scripts/lot95_propmodel.py: ex is 5/16 em (was 1/2); no generated suite changes (the Lot 95 sheets use no ex with a font context).
    - scripts/lot97_animmodel.py: four space separated arguments without "/" are refused (the C refused them already; no generated line changed).
7. Removed while writing: the "rank > 0 ? rank : 1" store of the Lot 97 list rank (a rule rank is never 0, and storing the rank itself keeps "later wins on a tie" for any rank); the full initialisation of the coordinated output and of every list in wf_cssanim_coord / wf_csstrans_coord (every field is copied from a list item, and an unset list gets its initial item); a second clamp of the summed premultiplied channels (the final 0..255 clamp covers it). Removed after the mutation run (each shown equivalent, its mutant dropped): the ')' test in the colour argument splitter (cp_color checks it first), the empty-argument test (an empty token fails its own parse), the "3 or 4 arguments" test for rgb() (the splitter bounds the count), max < 1 and c < 1 in wf_cssanim_long / wf_csstrans_long (cp_list_items returns -1 for max < 1 and never 0), the hue wrap by subtraction (a 32-bit modulo now; the sum stays under 6e6), the animation: none special case in the shorthand (the list with name none animates nothing either), the pool bound in node_lists (an index always comes from long_intern), the 100000 cap of animation-iteration-count (the number reader already refuses more than CP_LIM, 100000). The hue copy now tests its length itself (no space to trim), so its mutant is an ASan one. One boundary mutant was not kept: "<" against "<=" for the longhand rank, since two declarations never share a rank (the order is part of it) and the second cascade pass sets the same value again.
8. Mutations: 196 applied / 196 killed / 0 survived. Host: 194 mutants on wf_cssprop.c (152), wf_sitecss.c (40) and wf_cssprop.h (2), 4 of them checked with ASan (the argument and list bounds, the hue length); the reader and engine suites of run_all_tests.sh plus test_webeng (69 suites, the Lot 98 and Lot 97 suites first, baseline 0 failures). QEMU: 2 mutants on a git archive copy of the code commit (the " composed" summary removed; hsl() not recognised), both killed by `make qemu-lot98`. A first run on the unfinished tree left 28 survivors: each got a witness in test_lot98_reader.c or the generator, or its code was removed as equivalent (point 7).
9. CI: one new job, lot98-reader-longhands (make qemu-lot98, no guest clock option, timeout 15 minutes).
10. ASan: make test-all with -fsanitize=address,undefined: base bec0eba 22223 tests 5 failed, head 90ff9f4 22475 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).

## Not done (first gaps for lot 99)

1. Animations: a running clock and repaint (the clock is still a value set by "web-css time N"), transitions and animations of the reader's own properties (only registered custom properties animate), hsl() as a reader colour, turn/rad/grad hues, animation-timeline and the other Animations 2 longhands, transition events.
2. Fonts: one font metric set (the 8x8 engine font); no lh/rlh/cap/ic units.
3. Gradients: conic children of vertical boxes (refused), background-clip: text, a clip or origin per layer, background-attachment, fetched background images; per-pixel painting only along the axes (unchanged).
4. @property: no <frequency>, <ratio> or <flex>; more than 8 alternatives, 16 registrations or 127-byte values are refused (unchanged).
5. Math: min(), max(), clamp() of a length-percentage mix are kept; fixed point with 1/10000 steps (unchanged).
6. Autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot97_plan.md).

## Tests

Host tests (generated by scripts/lot98_animgen.py, seeds 9801..9804, from the model scripts/lot98_animmodel.py with scripts/lot97_animmodel.py and scripts/lot95_propmodel.py; the models use Fractions and share no code with the C): test_lot98_hsl.c (480 interpolations with hsl()), test_lot98_add.c (560 sums), test_lot98_longs.c (360 longhand lists and 220 coordinated lists), test_lot98_sheet.c (300 sheets with longhands, compositions and @keyframes in @scope). Hand suite: test_lot98_reader.c (8 tests).
QEMU: tests/scripts/test_qemu_lot98.py, `make qemu-lot98` (one boot at 1x): /lot98/anim.html from the Lot 67 server (web-css at 500 ms and 2000 ms). CI job lot98-reader-longhands. `make qemu-lot97` is run unchanged.

## Gates on the box

Code commit 90ff9f4 (the docs commit changes docs only), 14 November 2026, UTC+4:

- make all: pass; a fresh git archive make all: pass (26 s, the same 4 kernel warnings as the base).
- make test-all: pass, 22560 tests, 0 failed (359 s).
- make qemu-lot98: pass (66 s); make qemu-lot97: pass (106 s).
- roadmap-check, secrets-check, font-check, strict-image-check: pass.
- ASan/UBSan test-all: base bec0eba 22223 tests 5 failed, head 90ff9f4 22475 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Other QEMU gates: left to GitHub CI.
