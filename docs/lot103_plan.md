# Lot 103 plan - reader gamut mapping, color-mix() in every CSS Color 5 space, hue methods, missing components by analogous sets

Base: aaca718 (origin/main). PR 260 (the Lot 102 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot103.
One guest engine (userspace/webeng.c, unchanged here) and one shell path.
Kernel, rights and ABI unchanged. US-031 stays partial. phase3_complete
stays false. US-068/US-073, US-076, US-080 and US-081 stay partial.
US-078 and US-097 stay simule.

Scope taken from the Lot 102 gaps (gap 1, colours): gamut mapping of
colours outside sRGB (CSS Color 4 section 13.2.2, in OKLCh with chroma
reduction and the deltaE OK JND) instead of per-channel clipping,
color-mix() in the other CSS Color 5 interpolation spaces with the hue
interpolation methods and powerless hues, and missing components carried
by analogous sets across spaces. View timelines were looked at first and
left out (see Not done: the reader's CSS pass has no box geometry).

A row is listed only when its code, tests and docs are in the branch.
Size: 10593 lines added and 4232 removed outside docs (code 330 / 30 in wf_cssprop.c/.h; model scripts/lot103_model.py 557 and generator scripts/lot103_gen.py 425, new; older models 61 / 21 converted; hand suite 225 and hand conversions 15 / 6; generated suites 4611; QEMU test, server page, Makefile and CI 194; converted generated suites 4174 / 4174, regenerated with the gamut-mapped expectations; run_all_tests.sh 1 / 1).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | gamut mapping (CSS Color 4 13.2.2) | lab(), lch(), oklab(), oklch(), color() outside sRGB and their mixes were clipped per channel | the colour is mapped to sRGB by the CSS Color 4 algorithm, in fixed point (1/2^40, 128-bit products): in gamut (every linear sRGB channel in [0, 1]) it is kept; OKLab L >= 1 gives white and L <= 0 black; else the clipped colour is taken when its deltaE OK to the origin is under the JND 0.02, else a binary search of the OKLCh chroma (lightness and hue kept, epsilon 0.0001) stops when the clipped candidate is within 0.0001 under the JND, and the last clipped candidate is painted. The OKLab of oklab() and oklch() is used as written (no round trip). Applies to every colour the reader paints (color, background-color, the background shorthand, var() values, registered <color>, keyframes and transitions endpoints). rgb(), hsl(), hwb() and color(srgb ...) keep their clamp at parse. Proof: test_lot103_gamut.c (720 colours in every space and edge cases), test_lot103_reader.c (gamut), converted lot101 suites, `make qemu-lot103` (#g103 color(display-p3 1 0 0) #ff0b0c, #o103 oklch(0.7 0.4 30) #ff5843) |
| B | color-mix() spaces and hue methods (CSS Color 5) | only in srgb (others refused) | in srgb, srgb-linear, display-p3, a98-rgb, prophoto-rgb, rec2020, lab, oklab, xyz, xyz-d50, xyz-d65, hsl, hwb, lch and oklch; the polar spaces take shorter (default), longer, increasing or decreasing hue, a hue method on another space is refused. Premultiplied interpolation in the space (the hue not premultiplied), result alpha and percentages as in Lot 100; hsl and hwb results are clamped like hsl()/hwb(), the others gamut-mapped (row A). An achromatic operand in a polar space has a powerless hue (lch chroma at most 0.0015, oklch chroma at most 0.000004, hsl and hwb of a grey: equal sRGB channels), taken as missing. Proof: test_lot103_mix.c (820 mixes), test_lot103_hue.c (760 hue cases), test_lot103_reader.c (mix_spaces, hue_methods, witnesses), test_lot100_reader.c (converted), `make qemu-lot103` (#m103 in oklch #b700be, #l103 in hsl longer hue #00ff00, #b103 srgb longer hue refused) |
| C | missing components by analogous sets (CSS Color 4 12.2) | only rgb()/color() channels and the alpha were kept missing | a none in hsl(), hwb(), lab(), lch(), oklab(), oklch() is kept missing too; in color-mix() a missing component is carried into the interpolation space by analogous set (red, green, blue; lightness; colourfulness: chroma and saturation; hue; opponent a; opponent b; alpha), then taken from the other colour as in Lot 102 (0 in both and still missing when both miss it). Colour keyframes and transitions keep the Lot 102 srgb carrying. Proof: test_lot103_hue.c, test_lot103_mix.c, test_lot103_reader.c (missing), test_lot102_none.c (converted), `make qemu-lot103` (#n103 lch none hue #009fda, #h103 hsl none hue #20df20) |
| D | colour keyframes from mapped colours | clipped endpoint | a keyframes or transition colour outside sRGB is mapped (row A) before the Lot 100 interpolation. Proof: test_lot103_sheet.c (300 sheets), converted lot101_paint, lot101_scroll and lot102_sheet suites, `make qemu-lot103` (#a103 from color(display-p3 0 1 0): #007e14 at 500 ms, no colour at 2000 ms) |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB, webeng.c untouched); wf_site_t keeps 508420 bytes. The internal cp_rgba_t of wf_cssprop.c gains the linear sRGB values before mapping (lin, lin_ok), so a mix in a non-srgb space starts from the unclipped colour; no public structure or header contract changes (wf_cssprop.h: comment only).
2. Gamut mapping changes painted colours: every lab(), lch(), oklab(), oklch() and color() colour outside sRGB paints differently from Lot 101/102 (for example color(display-p3 1 0 0) #ff0000 to #ff0b0c, oklab(0 -40 -20) to #000000). Colours inside sRGB do not change.
3. Choices and approximations: the mapping is the CSS Color 4 13.2.2 pseudo-code in fixed point, not a float port; the result is the last clipped candidate (as the spec's last step); hsl() and hwb() operands of a mix are converted from their sRGB channels, so the written hue of an achromatic hsl() is lost (it is powerless anyway, but hsl(120 0% 50%) in a mix carries no hue 120); color(srgb ...) keeps its parse clamp (as rgb()); color-mix() results in hsl and hwb are clamped to the sRGB cube; xyz is xyz-d65.
4. Models: scripts/lot103_model.py (float, with an exact path and a forked check on near decisions) marks a case unsure when the two paths differ or when linear magnitudes are ill-conditioned for float versus fixed point (over 64 for a written colour, over 8 for mix operands and results); generators skip unsure cases (they are not asserted).
5. Simplifications from the mutation runs: the hue reduction modulo 360 of the mixed hue, the hsl 360 wrap and the polar negative wraps were removed (the conversions take any turn), and a redundant lin_ok reset in cp_mix.
6. Converted tests (assertions changed, none weakened; each now checks the new behaviour; the generators are unchanged except the model functions they call):
    - test_lot101_color.c (regenerated): 328 expected colours changed (mapped instead of clipped), 76 cases drawn differently (the unsure set changed); 430 unchanged.
    - test_lot101_mix.c: 191 changed, 457 drawn differently, 112 unchanged.
    - test_lot101_paint.c: 1074 changed, 187 unchanged; test_lot101_scroll.c: 840 changed, 1242 unchanged.
    - test_lot102_none.c: 132 changed, 239 drawn differently, 629 unchanged; test_lot102_sheet.c: 865 changed, 1137 unchanged.
    - test_lot100_color.c: 1 case (color-mix(in hsl, ...) was refused, now a colour).
    - test_lot100_reader.c: color-mix(in hsl, ...) gives #ff00ff and in srgb-linear #bc00bc (were refused); "in hsv" and "in srgb-lin" are added as refused.
    - test_lot101_reader.c: oklab(0 -40 -20) gives #000000 (was clipped to a green-blue), plus a clamp check at L 0.5; color(display-p3 1 0 0) gives #ff0b0c.
    - Models: scripts/lot101_colormodel.py (encode maps with the Lot 103 model; the clip kept as encode_clip) and scripts/lot102_model.py (mixm delegates non-srgb spaces to the Lot 103 model).
7. CI: one new job, lot103-reader-gamut (make qemu-lot103, timeout 15 minutes).
8. Mutations: host 114 mutants on wf_cssprop.c, one checked with ASan (a component loop past its array in cp_mix); the reader and engine suites of run_all_tests.sh plus test_webeng (95 suites, the Lot 103 suites first, baseline 0 failures): 105 killed, 9 survived, all shown equivalent at channel precision: six on the in-gamut shortcut and the min_inGamut flag of the gamut search (0 channel differences over 60000 colours near the gamut boundary), the two zero guards of the fixed-point power (the unguarded power of 0 is about 5.5e-5, about 2e-13 linear after the round trip), and a 360 degree wrap of the hsl hue (the same turn). QEMU: 3 mutants on a git archive copy of the code (gamut mapping skipped, longer hue ignored, analogous sets not carried), all killed by `make qemu-lot103`. Earlier runs left 21 then 1 survivors; each got a witness in test_lot103_reader.c, its code was simplified (point 5), or its mutant was strengthened. Detail: /workspace/run/lot103/mutations.txt on the box.
9. Size: 6419 new lines (code, models, generator, hand and generated suites, QEMU test and CI) plus 4174 regenerated lines of converted suites (point 6).

## Not done (first gaps for lot 104)

1. Timelines: view timelines (view-timeline-name/axis/inset activity, view()), named ranges cover/contain/entry/exit on them and lengths in animation-range are not done: the reader computes styles in a CSS pass with no box geometry (no element positions or sizes in the scroll container), so a view progress cannot be computed; they stay stored and inactive as in Lot 102. They need a layout feedback from the engine (webeng.c) to the reader.
2. Colours: relative colour syntax (from), the oklab default interpolation space for keyframes and transitions (still premultiplied sRGB), add composition carrying missing components, gamut mapping for color(srgb) beyond the parse clamp, system colours.
3. Animations: a running clock and repaint, other reader properties than color and background-color, transition events.
4. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests (generated by scripts/lot103_gen.py, seeds 10301..10304, from the model scripts/lot103_model.py with the Lot 100 to 102 models; the models share no code with the C): test_lot103_gamut.c (720 colours), test_lot103_mix.c (820 mixes), test_lot103_hue.c (760 hue method and missing component cases), test_lot103_sheet.c (300 sheets). Hand suite: test_lot103_reader.c (6 tests).
QEMU: tests/scripts/test_qemu_lot103.py, `make qemu-lot103` (one boot at 1x): /lot103/gamut.html from the Lot 67 server (web-css at 500 and 2000 ms). CI job lot103-reader-gamut. `make qemu-lot102` is run again unchanged.

## Gates on the box

Code commit 60f180f (the later CI comment fixup and the docs commit change no code), 15 November 2026, UTC+4:

- make all: pass; a fresh git archive make all: pass (26 s).
- make test-all: pass, 24085 tests, 0 failed (368 s).
- make qemu-lot103: pass (65 s); make qemu-lot102: pass (101 s).
- roadmap-check, secrets-check, font-check, strict-image-check: pass.
- ASan/UBSan test-all: base aaca718 23751 tests 5 failed, head 60f180f 24000 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Mutations: host 114 applied / 105 killed / 9 survived (equivalent, above); QEMU 3 applied / 3 killed.
- Other QEMU gates: left to GitHub CI.
