# Lot 109 plan - box scroller lengths in ex, ch and lh, percentages inside math functions, heights past 2000 px, relative lab(), lch(), hwb() and color(), unclamped origins

Base: bbe75f5 (origin/main). PR 266 (the Lot 108 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot109.
One guest engine (userspace/webeng.c and its .inc files). Kernel, rights
and ABI unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and US-097
stay simule.

Scope taken from the Lot 108 gaps, in the order of the brief: (a) box
scroller lengths: ex, ch and lh units with the font metrics of the 8x8
font, a percentage inside calc()/min()/max()/clamp() resolved by the
engine against the containing block, and the engine's 2000 px limit on a
parsed height raised to the layout bound (16000 px); (b) relative colour
syntax for lab(), lch(), hwb() and color(), and an origin outside the
sRGB gamut keeping its unclamped channels (rgb(), hsl(), hwb() and
color(srgb)). Part (c) (html and body as box scrollers, other animated
properties) is not done (see the gaps). No margin collapsing, no new
layout or font engine.

A row is listed only when its code, tests and docs are in the branch.
Size: see review point 10.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | ex, ch and lh (wf_cssmath.c, wf_cssmath.h, wf_sitecss.c) | a box scroller length in ex, ch or lh was dropped | wf_css_math_box (new) reads a box scroller length: ex is 5/16 em (the inked rows of "x" in the 8x8 font, WF_CSSPROP_EX_NUM), ch 8/16 em (WE_CHAR_W at 16 px), lh 10/16 em (WE_LINE_H, line-height normal), against the element's font size; in px thousandths, cut as the other units of wf_css_math_px. The reader keeps "Nex", "Nch" and "Nlh" as font tokens ("~L") and resolves them when the style is written; scroll-padding takes them too. Font sizes do not (wf_css_math_px is unchanged). Proof: test_lot109_math.c (420 values), test_lot109_box.c (200 cascades), test_lot109_pad.c (200 sheets), test_lot109_reader.c (font_units, math_api, padding), `make qemu-lot109` (#o109 height: 20ex, 100 px; #p109 height: 12lh, 120 px) |
| B | percentages inside math functions (wf_cssmath.c, wf_sitecss.c) | a math function with a percentage was dropped (wf_css_math_px has no containing block) | wf_css_math_box keeps a percentage as its own part (CSS Values 4: the containing block is unknown to the reader) and writes the value as the engine's math reads it: "N%", "calc(N% + Mpx)" (integers, half away from zero, at most 2000: WF_CSSMATH_LIT = WE_MATH_LITERAL_MAX), or min()/max()/clamp() of those when it mixes the parts (one such function per value, not nested, not inside an arithmetic: the engine's math takes no more). The reader stores it as "@N" (pool) and passes the expression for height, max-height, min-height, width and max-width (at most 63 bytes, WF_SITE_BOXEX; resolved again at the element's font size); a scroll-padding inset with a percentage stays auto. The engine resolves it against the containing block (Lot 92 math; a percentage of an auto block makes the height auto). Proof: test_lot109_math.c, test_lot109_box.c, test_lot109_flow.c (160 layouts through the reader and the engine), test_lot109_reader.c (pct_math, pct_refused), test_lot109_engine.c (pct_flow, width_flow), `make qemu-lot109` (#q109 clamp(20%, calc(50% + 2ch), 90%) inside a 120 px box scroller: 76 px) |
| C | heights past 2000 px (webeng.c) | height, min-height and max-height in px were refused past 2000 (parse_dimension) | parse_height takes them up to WE_LAYOUT_MAX (16000, the int16 layout bound: positions and sizes are clamped there, and a math height could already reach it); a percentage keeps its 2000 limit; the other lengths keep 2000. Proof: test_lot109_tall.c (200 stacks in style attributes and sheets), test_lot109_engine.c (tall_heights, math_then_px, stack_clamp) |
| D | relative lab(), lch(), hwb() and color() (wf_cssprop.c) | invalid | lab(), lch() and hwb() from an origin colour with the keywords l a b, l c h, h w b (the origin's values as color-mix() takes them: hwb from its sRGB channels, lab and lch from its linear values in CIE Lab D50, the lch hue 0 when the chroma is powerless), and color(from C space ...) for the rgb and xyz spaces of color() (r g b or x y z, the origin's values in that space; a space color() does not take is refused by the absolute colour). Proof: test_lot109_rel.c (420 colours), test_lot109_reader.c (rel_spaces), `make qemu-lot109` (#l109 #336699, #h109 #755e29, #w109 #959595, #x109 #ff0000, keyframes between lab() and lch() #636363) |
| E | unclamped origins (wf_cssprop.c) | rgb(from ...) and hsl(from ...) read the origin's gamut-mapped channels | the keywords of rgb(), hsl(), hwb() and color(srgb) come from the origin's linear sRGB values sRGB-encoded without clamping (CSS Color 5: the origin is converted, not gamut-mapped; inside the gamut these are its channels); hsl() and hwb() values from those channels by the CSS Color 4 sample code, a negative saturation (lightness past 0 or 100%) turning the hue by 180 deg and taken as its absolute value. The result is the absolute colour written with those values (clamped and gamut-mapped as that one is). Proof: test_lot109_ucol.c (300 colours from origins mostly outside the gamut), test_lot109_reader.c (rel_unclamped, rel_hsl_unclamped), `make qemu-lot109` (#u109 rgb(from color(display-p3 1 0 0) calc(r - 40) g b) #ef0000, #d70000 if clamped first) |

## Review points

1. Structure sizes: unchanged. we_doc_t 871464 bytes (< 1 MiB), wf_site_t 543780 bytes, wf_geom_t 15464 bytes, we_forms_t 9848 bytes. The 64-byte expression buffers (WF_SITE_BOXEX) are on the stack of the reader functions.
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3):
    - userspace/wf_cssmath.c, wf_cssmath.h: box mode (mv_t.mix, mp_t.box/out), ex/ch/lh, the percentage part, the writer of the engine's expression, wf_css_math_box and WF_CSSMATH_LIT. wf_css_math and wf_css_math_px are unchanged in behaviour.
    - userspace/wf_sitecss.c, wf_sitecss.h: tok_box, the "@N" token, ex/ch/lh as font tokens, box_len/box_app writing an expression, the height/max-height/min-height loop, scroll-padding through tok_box; WF_SITE_BOXEX.
    - userspace/wf_cssprop.c: cp_relf, cp_rel_kw, cp_rel_kv, cp_rel (lab, lch, hwb, color(); unclamped origins), and in cp_mix_comps the negative saturation of hsl and hwb from unclamped channels (it cannot happen for channels in 0..255, so color-mix() is unchanged).
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added.
3. The font metrics are the 8x8 font's, scaled from 16 px: ex 5/16 em, ch 8/16 em, lh 10/16 em (line-height normal; the reader does not read line-height for lh).
4. An expression is written with integer literals (half away from zero): calc(12.5% + 3px) becomes calc(13% + 3px). The engine truncates a plain percentage height (Lot 108 point 5) and evaluates its math in whole px.
5. A percentage inside a math function only resolves when the containing block has a definite height: in the reader view that is a box scroller parent with a height the reader passes (the reader drops other author heights). Otherwise the height is auto (the content), as CSS 2.1 10.5 says for a percentage.
6. hsl() and hwb() origins: the values come from the unclamped channels; a grey has the hue 0 (powerless), not turned.
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot107_model.py (ex, ch and lh units; math through lot109_model.math_box, an expression kind), scripts/lot108_model.py (relative colours through lot109_model). Regenerated test_lot107_height.c (11 assertions: math with a percentage now passed as an expression, ex/ch/lh heights), test_lot108_width.c and test_lot108_xclock.c (14 assertions together). test_lot107_pad.c regenerates unchanged.
    - test_lot108_rel.c: 20 assertions patched in place with values checked by the model (13 for the new functions and spaces, now valid, 7 for hsl() from an origin outside the gamut); rerunning scripts/lot108_gen.py would draw other cases (the generator calls the updated model).
    - Hand: test_lot107_reader.c (calc(50% + 10px) is now passed; 10ex replaced by 10cap for the refused unit and a 10ex case of 50px added; the "@" token), test_lot108_reader.c (hwb(from is valid: the refused case is now hwba(from), tests/unit/userspace/webeng_lot54.inc (min-height 2001px is accepted: the refused case is now 16001px).
8. CI: one new job, lot109-reader-box (make qemu-lot109, timeout 15 minutes).
9. Mutations: see the PR body (host run on the code commit and QEMU run on a git archive).
10. Size: see the PR body.

## Not done (first gaps for lot 110)

1. Box scrollers: html and body as box scrollers of their own (part (c)); ex/ch/lh against the element's line-height (lh reads line-height normal); a percentage inside a math function for a scroll-padding inset (auto); more than one mixed min()/max()/clamp() in one value, or one inside an arithmetic (the engine's math has no such node); the engine's own y click without a relayout (Lot 74).
2. Geometry: view timelines in section 1 and later; the console web-load path (no layout).
3. Animations: other reader properties than color and background-color (part (c)), a running clock and repaint, transition events.
4. Colours: relative colours with none as an origin channel's value (the origin's missing components are 0), system colours, color() with a custom colour space.
5. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests generated by scripts/lot109_gen.py (seeds 10901..10907) from the model scripts/lot109_model.py with the Lot 95 to 108 models; the models are written from the specifications (CSS Values 4, CSS Box Sizing 3, CSS 2.1 10.5 and 10.7, CSS Color 4 and 5) and share no code with the C: test_lot109_math.c (420 values through wf_css_math_box), test_lot109_box.c (200 cascades: the Lot 108 width writer with ex/ch/lh and percentages in math functions), test_lot109_pad.c (200 sheets: the Lot 107 scroll-padding writer with them), test_lot109_flow.c (160 layouts of nested box scrollers through the reader and the engine), test_lot109_rel.c (420 relative colours), test_lot109_ucol.c (300 colours from origins outside the gamut), test_lot109_tall.c (200 stacks with heights past 2000 px). Hand: test_lot109_reader.c (8 tests), test_lot109_engine.c (5 tests).
QEMU: tests/scripts/test_qemu_lot109.py, `make qemu-lot109` (one boot at 1x): /lot109/box.html from the Lot 67 server: #o109 (height: 20ex, 100 px), #p109 (height: 12lh, 120 px, a named timeline) and #q109 inside it (clamp(20%, calc(50% + 2ch), 90%), 76 px) start at #000000; "web-bscroll 629 200" gives #s109 #bcbcbc (187 of 254 px), "web-bscroll 629 250" #t109 #e1e1e1 (252 of 286 px), "web-bscroll 629 100" #i109 #8e8e8e (128 of 230 px); #l109 #336699, #h109 #755e29, #w109 #959595, #x109 #ff0000, #u109 #ef0000 (unclamped origin; #d70000 refused), #c109 #636363 at 500 ms. CI job lot109-reader-box.

## Gates on the box

Filled in by the final report (see the PR body).
