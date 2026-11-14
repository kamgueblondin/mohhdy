# Lot 100 plan - reader colours: CSS Color 4 names, hwb(), color-mix(in srgb), translucent colours, colour animations and transitions

Base: 0c1b716 (origin/main). PR 257 (the Lot 99 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot100.
One guest engine (userspace/webeng.c, unchanged here) and one shell path.
Kernel, rights and ABI unchanged. US-031 stays partial. phase3_complete
stays false. US-068/US-073, US-076, US-080 and US-081 stay partial.
US-078 and US-097 stay simule.

Scope taken from the Lot 99 gaps (gap 2, colours, and gap 1, transitions
and animations of the reader's own properties): every CSS Color 4 named
colour, hwb(), color-mix() in srgb, colours with an alpha painted by the
reader (the engine composites them since Lot 62), and keyframes,
transitions and @starting-style on the reader's own color and
background-color.

A row is listed only when its code, tests and docs are in the branch.
Size: 7863 lines added and 166 removed outside docs (code 390 / 123 in wf_cssprop.c/.h, wf_sitecss.c/.h and wf_cssbg.c; models and generators 954; hand suite 425; generated suites 5861; QEMU page, server page, Makefile and CI 189; converted tests 43 / 36; run_all_tests.sh 1).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | The 148 named colours of CSS Color 4 | the reader painted 16 names (the Lot 90 table) and a registered <color> took the same few; any other name (orange, rebeccapurple, darkorange) was dropped, and with it the whole background shorthand | one table of the 148 names (CSS Color 4 section 6.1, checked against the X11 list for the known CSS/X11 differences: gray, green, maroon and purple keep the CSS values), letters in any case, used by the registered <color> check, by interpolation and by the reader's colour parser (color, background-color, the background shorthand colour, var() values). transparent is a colour of alpha 0; currentcolor is still not taken (it has no value of its own here). Proof: test_lot100_color.c (all 148 names, mixed case), test_lot100_reader.c (names), test_lot90_reader.c (converted), `make qemu-lot100` (#n100 RebeccaPurple, #g100 darkorange) |
| B | hwb() (CSS Color 4 section 8) | not a colour | hwb(H W B [/ A]), space syntax only (commas refused); the hue as for hsl() (deg, grad, rad, turn, a number of degrees, taken modulo 360deg); W and B percentages or numbers on the same scale, clamped to 0..100%; when W + B reaches 100% the colour is the grey W / (W + B); else each channel is the hsl(H 100% 50%) channel scaled by 1 - W - B, plus W, exact in fixed point and rounded once to 1/10000 (half away from zero), then to 0..255 when painted. Proof: test_lot100_color.c, test_lot100_mix.c, test_lot100_reader.c (hwb), `make qemu-lot100` (#c100) |
| C | color-mix(in srgb, ...) (CSS Color 5) | not a colour | color-mix(in srgb, C1 [P1], C2 [P2]) with "in" and "srgb" in any case, each percentage before or after its colour, 0..100%; P1 and P2 default to 50% each, or to 100% minus the other; their sum S must be over 0; the colours are mixed premultiplied: A = a1 P1 + a2 P2, each channel (c1 a1 P1 + c2 a2 P2) / A (0 when A is 0), the alpha A / S, or A / 100% when S is under 100%; each value rounded once. The colours may be any colour above, a color-mix() included, four color-mix() levels in all at most; a colour item of 128 bytes or more is refused. Other colour spaces (oklab, hsl, ...) are refused. Proof: test_lot100_color.c, test_lot100_mix.c (interpolation, add and match), test_lot100_reader.c (mix), `make qemu-lot100` (#m100, a translucent mix) |
| D | Translucent reader colours | a reader colour had to be opaque: rgba() took only alpha 1, hsla() with alpha under 1 and #rgba/#rrggbbaa were dropped | one parser, wf_cssprop_color, gives the colour and its alpha (0..255, rounded half away from zero) of a hex colour (3, 4, 6 or 8 digits), a name, transparent, rgb(), rgba(), hsl(), hsla(), hwb() or color-mix(); the text may have spaces around it and a following ';' or '!', nothing else, and is under 128 bytes. rgb() and rgba() take the CSS Color 4 grammar: numbers or percentages, clamped to 0..255, any alpha (clamped to 0..1), the space form with "/ A". The reader keeps the alpha in the top byte of its colour (255 - alpha, the engine's encoding since Lot 62) and writes #rrggbbaa when the colour is not opaque, which the engine composites. color: transparent is written #00000000; background-color: transparent (and the shorthand word transparent) still means no background. The background shorthand colour word takes every such colour (still under 32 bytes, and a word with '!' or ';' is refused). Proof: test_lot100_color.c, test_lot100_paint.c (420 painted styles), test_lot100_reader.c (alpha, bounds, paint), test_lot97_reader.c and test_lot99_reader.c (converted), test_lot99_sheet.c (regenerated), `make qemu-lot100` (#m100 bg #80008066) |
| E | Animations and transitions of color and background-color | only registered custom properties animated (Lot 97 to 99) | at the reader clock (web-css time N), @keyframes with color or background-color animate them as <color> values (premultiplied sRGB, the composition of registered colours: add and accumulate sum the premultiplied colours), with the timing, iteration, direction, fill, play-state, composition and timeline rules of Lots 97 to 99; the value below is the element's own colour, else for color the nearest ancestor colour that is set (else #000000), else for background-color transparent. An !important own colour is not animated. A transition (property color, background-color or all) runs from the @starting-style colour of the element (the most specific starting rule wins, wherever it is; color: initial starts from black and background-color: initial or transparent from transparent; revert and inherit are ignored there) to the element's colour; a transition also runs on an !important colour. web-css adds " color A T" to the css-anim line (A animated and T transitioned colours) when A or T is over 0. Proof: test_lot100_anim.c (360 sheets), test_lot100_reader.c (keyframes, starting), `make qemu-lot100` (#k100, #t100, css-anim ... color 1 1) |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB, webeng.c untouched). wf_site_t grows from 496112 to 499960 bytes (wf_site_node_t 1204 to 1224: the @starting-style colour and background, their set flags and ranks; the col_animated and col_transitioned counters). wf_site_rule_t keeps 1108 bytes.
2. New API: wf_cssprop_color (colour and alpha of any reader colour); wf_site_t.col_animated and .col_transitioned; the node fields scolor, sbg, scolor_set, sbg_set, scolor_rank, sbg_rank. The reader colour (uint32_t) now carries 255 - alpha in its top byte; a style colour may be written with 8 hex digits.
3. Wider grammar (on purpose, CSS Color 4): rgb() and rgba() take percentages, numbers above 255 (clamped), leading zeros, any alpha and the space form; #rgba and #rrggbbaa are reader colours; color: transparent is painted (#00000000) instead of dropped.
4. Narrower grammar: a word after a hex colour or a name is now refused ("#abc x" and "red x" were taken as #abc and red); the colour text must end at a space, ';', '!' or the end.
5. Choices: the 148 names use the CSS values (gray #808080, green #008000, maroon #800000, purple #800080); hwb() takes the space syntax only and plain numbers for W and B; color-mix() takes srgb only and four levels; the reader stores a starting colour only for color and background-color (no starting all); the colour animation base is the element's own colour, else the nearest set ancestor colour for color, else #000000 or transparent (never the engine's default colours).
6. Approximations: channels and alpha rounded once to 1/10000 then to 0..255; rad through the reader's pi (Lot 92); a colour word of the background shorthand is still under 32 bytes; the shell's sc[96] colour buffer is unchanged.
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - test_lot90_reader.c test_unknown_colour_drops_shorthand: orange is a colour now, so the shorthand is kept (new assertion: #c gets background-color:#ffa500;background-image:url(c.png)); the dropping witness uses the unknown word "orangy".
    - test_lot97_reader.c test_l97_rgb_color: rgb(00255,0,0) and rgb(256,0,0) give #ff0000, rgba(1,2,3,0) gives #01020300, rgb(1,2,3,1) and rgb(1 2 3) give #010203 (all five were refused and left the blue of the base rule).
    - test_lot99_reader.c test_l99_hsl_paint: hsl(... / 0.5) paints color #00ffff80 and the background shorthand with / 0.5 paints #ffff0080 (both were dropped).
    - test_lot99_sheet.c (regenerated by scripts/lot99_animgen.py, whose paint cases now take their colour from scripts/lot100_colormodel.py): 26 lines change, the translucent hsl() paint cases.
    - scripts/lot97_animmodel.py: colour_match and rgba delegate to scripts/lot100_colormodel.py (the old bodies are kept as colour_match97 and rgba97); no other generated suite changes.
8. Removed after the mutation runs (each shown equivalent, its mutant dropped): in wf_cssprop_color, the stop at the ')' that closes the first '(' (a text glued after it stays in the colour text and fails every form), the empty or unclosed check (every form fails them) and the lower-casing (cp_rgba is case-insensitive); in the color-mix() split, the refusal of a stray ')' or an unclosed '(' (the part that keeps it fails every colour form); in a color-mix() item, the result check of the percentage parse (a token that is not a number leaves the percentage at -1, refused); in cp_mix, a length check of the "in srgb" part (the part is followed by a comma); in the colour pass, the ancestor bound q < i, a cleared output byte and a cached '<color>' parse.
9. Mutations: 138 applied / 138 killed / 0 survived. Host: 136 mutants on wf_cssprop.c (89), wf_sitecss.c (44) and wf_cssbg.c (3), 3 of them checked with ASan (the color-mix() part count, the item buffer and the colour text buffer); the reader and engine suites of run_all_tests.sh plus test_webeng (79 suites, the Lot 100 suites first, baseline 0 failures). QEMU: 2 mutants on a git archive copy of the code commit (the " color A T" summary removed; the alpha dropped by the reader colour parser), both killed by `make qemu-lot100`. Earlier runs on the unfinished tree left 9 then 1 survivors; each got a witness in test_lot100_reader.c or its code was removed as equivalent (point 8); the last one survived only on stale ASan objects of the mutation script, which now clears them.
10. CI: one new job, lot100-reader-colors (make qemu-lot100, timeout 15 minutes).
11. ASan: make test-all with -fsanitize=address,undefined: base 0c1b716 22736 tests 5 failed, head c2cbf03 23040 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).

## Not done (first gaps for lot 101)

1. Colours: currentcolor; lab(), lch(), oklab(), oklch() and color(); color-mix() in other colour spaces and with hue interpolation; relative colour syntax (from); system colours; translucent colours in gradient stops; a background shorthand colour word of 32 bytes or more.
2. Animations: named timelines (stored since Lot 99, never resolved: no scroll or view timelines), a running clock and repaint, other reader properties than color and background-color (lengths, margins, opacity), the engine's default colours as the animation base, transition events.
3. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests (generated by scripts/lot100_colorgen.py, seeds 10001..10004, from the model scripts/lot100_colormodel.py with the Lot 95 to 99 models; the models use Fractions and share no code with the C): test_lot100_color.c (764 reader colours with their alpha and text bounds), test_lot100_mix.c (720 interpolations, additions and matches of registered colours), test_lot100_paint.c (420 painted styles), test_lot100_anim.c (360 sheets with colour keyframes and transitions). Hand suite: test_lot100_reader.c (8 tests).
QEMU: tests/scripts/test_qemu_lot100.py, `make qemu-lot100` (one boot at 1x): /lot100/color.html from the Lot 67 server (web-css at 500 ms and 2000 ms). CI job lot100-reader-colors. `make qemu-lot99` is run again unchanged.

## Gates on the box

Code commit c2cbf03 (the docs commit changes docs only), 14 November 2026, UTC+4:

- make all: pass; a fresh git archive make all: pass (28 s, the same 4 kernel warnings as the base).
- make test-all: pass, 23125 tests, 0 failed (426 s).
- make qemu-lot100: pass (66 s); make qemu-lot99: pass (65 s).
- roadmap-check, secrets-check, font-check, strict-image-check: pass.
- ASan/UBSan test-all: base 0c1b716 22736 tests 5 failed, head c2cbf03 23040 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Other QEMU gates: left to GitHub CI.
