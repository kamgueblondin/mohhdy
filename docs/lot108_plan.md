# Lot 108 plan - horizontal box scrolling, inline-axis scroll timelines, width, max-width and min-height of box scrollers, relative colour syntax

Base: ae7df5a (origin/main). PR 265 (the Lot 107 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot108.
One guest engine (userspace/webeng.c and its .inc files). Kernel, rights
and ABI unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and US-097
stay simule.

Scope taken from the Lot 107 gaps, in the preference order of the brief:
(a) box scrollers: horizontal box scrolling in the engine (box_scroll_x),
overflow-x auto/scroll with width and max-width passed by the reader,
web-bscroll along x, scroll and view timelines on the inline (x) axis of
a box scroller, min-height; (b) relative colour syntax (CSS Color 5:
rgb(), hsl(), oklab(), oklch() from an origin colour, the channel
keywords and calc()). Not done: ex, ch and lh units, a percentage inside a
math function, the engine's 2000 px limit and part (c) (see the gaps). No
margin collapsing, no new layout engine.

A row is listed only when its code, tests and docs are in the branch.
Size: see review point 10.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | horizontal box scrolling (webeng_scroll.inc, webeng.h, webeng.c) | a box with overflow-x auto or scroll was clipped along x only (Lot 74 scrolled boxes along y) | a box scrolls along x when overflow-x is auto or scroll (bit 1 of overflow_scroll): box_scroll_x shifts its children, box_cw keeps its content width, measured by scroll_apply before the display list is clipped (CSS Overflow 3: the in-flow descendant boxes from the content edge; a display none, absolute or fixed box and the insides of a box clipping along x do not count; text runs count by their own box, display items such as shadows do not). The scrollport is the content box (as along y). A 6 px bar (WE_SCROLL_W) is painted at the bottom inside the border edge, stopping left of a painted vertical bar (the corner is the vertical track's). The axes stay independent: overflow-y visible stays visible. The offsets of both axes persist per box id in we_forms_t (scroll_x next to scroll_y). Proof: test_lot108_xflow.c (150 layouts: content width, range, bars, clicks), test_lot108_engine.c, `make qemu-lot108` |
| B | web-bscroll along x (webeng_scroll.inc) | a click on a box's bottom edge did nothing | a click in the horizontal track sets box_scroll_x to range * (x - track left) / track width, rounded down (the vertical bar's rule of Lot 74); the shell command web-bscroll is unchanged and reaches it through the engine. Proof: test_lot108_xflow.c, test_lot108_engine.c (both_axes: the corner click hits the vertical track), `make qemu-lot108` ("web-bscroll 158 66", "web-bscroll 68 146") |
| C | inline-axis timelines on box scrollers (webeng_geom.inc, wf_geom.h, wf_sitecss.c) | a scroll timeline on the inline or x axis of a box scroller had a 0 range, and geom_scroller took only vertical scrollers | we_geom gives a box scroller's own offset and range along x (svx, x_vp, x_sc, x_rg); the nearest scroller of an element is its nearest ancestor scrolling along either axis; view timelines on the inline axis take the x offset into account (r[0] from the content start moved back by box_scroll_x, r[4], r[6], r[8] from geom_own_x). The reader runs scroll(), named scroll timelines and view timelines on the inline or x axis of a box scroller from svx, x_rg and x_sc (the inline axis is x: the reader has no writing mode). The root scrolls along y only: scroll(root inline) has no effect. Proof: test_lot108_xclock.c (200 sheets with geometry), test_lot108_reader.c (xbox, named_x), test_lot108_engine.c (flow_inline), `make qemu-lot108` |
| D | width, max-width and min-height (wf_sitecss.c, wf_sitecss.h) | not read | three new scroller slots (WF_SITE_K_SX0, WF_SITE_SXS 3) with the values of height (max-width takes none). A box scroller along x a timeline reads is passed as "overflow-x:auto;width:Npx;max-width:Mpx" (a percentage as N%, the integer part); min-height is passed together with a box passed along y ("overflow-y:auto;height:32px;min-height:80px"). Proof: test_lot108_width.c (200 cascades), test_lot108_xflow.c, `make qemu-lot108` (#m108: min-height 5em wins over height 2em, max-width 15em gives 240 px) |
| E | relative colour syntax (wf_cssprop.c) | rgb(from ...) and the others were invalid | rgb(), rgba(), hsl(), hsla(), oklab() and oklch() with "from" and an origin colour (any colour wf_cssprop parses, currentcolor and nested relative colours, at most 3 levels deep) are rewritten into an absolute function string and parsed as such (so clamping and conversion are the existing code). The keywords: r g b (the origin's channels, 0..255), h s l (CSS Color 4 sample code, the hue 0 for a grey), oklab l a b and oklch l c h (the origin's own values for an oklab() or oklch() origin, else from its linear sRGB values; the oklch hue normalised to 0..360, 0 when the chroma is powerless), alpha; a math argument has the keywords substituted (a negative value in parentheses) and is evaluated by the existing calc() code. The alpha is the origin's when left out. Commas at the top level are invalid. Proof: test_lot108_rel.c (400 colours), test_lot108_reader.c (rel_basic, rel_invalid, rel_nest, rel_reader), `make qemu-lot108` (#r108 #336689, #h108 #00cc00, #c108 #800080) |

## Review points

1. Structure sizes: we_doc_t 870824 to 871464 bytes (< 1 MiB; box_scroll_x and box_cw, 2 x int16_t per node). we_forms_t 9832 to 9848 bytes (scroll_x). wf_site_node_t 1392 to 1452 bytes (xpx[3], xtok[3], sxrank[3]), so wf_site_t 532260 to 543780 bytes (192 nodes; the reader's static site in wf_html.c, Ring 3 BSS). wf_geom_box_t +16 bytes, wf_geom_t 12376 to 15464 bytes.
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3):
    - userspace/webeng.h: box_scroll_x and box_cw in we_node_t, scroll_x in we_forms_t.
    - userspace/wf_geom.h: svx, x_vp, x_sc, x_rg.
    - userspace/wf_sitecss.c, wf_sitecss.h: the width, max-width and min-height slots, box_len for them, the style passed along x, the inline and x axes of scroll and view clocks on box scrollers.
    - userspace/wf_cssprop.c: cp_rel_head, cp_rel_kw, cp_rel_kv, cp_rel_arg, cp_rel (relative colours); in cp_mix_comps the powerless chroma test of oklch and lch is compared without the product (Lot 103 code: v[1] * 1000000 overflowed for a chroma past about 8.4, which a relative colour's origin such as oklch(0.5 10 40) reaches; found by UBSan on test_lot108_rel, witness in test_lot108_reader.c).
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added (web-bscroll reaches the x track through the engine).
3. The inline axis is x (horizontal-tb); the reader has no writing mode. The root scrolls along y only: a scroll timeline on the root's inline axis stays inactive.
4. The x scrollport is the content box (as the y one of Lot 74), not the padding box of CSS Overflow 3; the content width is measured from the content edge.
5. The engine truncates a fractional percentage (12.5% is 12%); the reader passes the integer part, the generated suites use whole percentages.
6. rgb(from ...) reads the origin's gamut-mapped sRGB channels (CSS Color 5 keeps an out-of-gamut origin's channels unclamped); hsl() keywords come from them too.
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot105_model.py, lot106_model.py, lot107_model.py: a scroll or view clock on the inline or x axis of a box scroller reads svx, x_rg and x_sc (it was the inactive 0 range); lot105_model.sc_cascade and passed go through scripts/lot108_model.py. Regenerated test_lot105_sheet.c (19 lines: a box scroller along x is now passed with its width). test_lot104 to test_lot107 other suites regenerate unchanged.
    - Hand: test_lot105_reader.c, the inline/x case now expects scr == 1 and geom_live == 1 (the box scroller is read along x), the written style still "" (no width).
8. CI: one new job, lot108-reader-x (make qemu-lot108, timeout 15 minutes).
9. Mutations (/workspace/run/lot108/mutations.txt on the box): host, final run on the code commit: 120 mutants on webeng_scroll.inc, webeng_geom.inc, webeng.c, wf_sitecss.c, wf_sitecss.h and wf_cssprop.c (2 of them with ASan), 119 killed and the no-op control survives. Run 1 left 27 survivors and run 2 left 4: fixed by witnesses (test_lot108_engine.c: shift, clamp, corner, restyle, child ranges, narrow padding, a box left of the page; test_lot108_reader.c: relative colour forms, max-width none, html and body, an origin longer than a value buffer) or by removing equivalent code (the scrollport clamp, the unused return value of scroll_paint_x, the click cap, the display items loop of the content width, the overflow-x tests in geom_own_x and scroll_paint_x, the box scroller test in box_passed, the comma and ')' checks in cp_rel). QEMU: 9 mutants on a git archive with make qemu-lot108 (see Gates).
10. Size: 10826 lines added and 104 removed outside docs: code 434/59 (webeng_scroll.inc 164/24, wf_cssprop.c 157/2, wf_sitecss.c 68/21, webeng_geom.inc 23/9, wf_sitecss.h 10/1, wf_geom.h 8/1, webeng.h 3, webeng.c 1/1), model and generator 1073, model conversions 33/22, generated suites 8459 (width 1576, xclock 3514, xflow 2386, rel 983), hand suites 584 (test_lot108_engine 360, test_lot108_reader 224), converted suites 23/21, QEMU test, server, Makefile, CI and run_all 220/2. About 800 lines over the 8000-10000 target, all in the generated suites.

## Not done (first gaps for lot 109)

1. Box scrollers: ex, ch and lh units for heights and widths (wf_cssmath.c has no font context for them), a percentage inside a math function, the engine's 2000 px limit on a parsed height (parse_dimension), scroll-padding sides in vertical writing modes and rtl, the inline axis of a vertical writing mode, the engine's own y click without a relayout (Lot 74 behaviour: the content is counted with the offset applied).
2. Geometry: html and body as box scrollers of their own (overflow on the root is the viewport's; the root scrolls along y only); view timelines in section 1 and later; the console web-load path (no layout).
3. Animations: a running clock and repaint, other reader properties than color and background-color, transition events.
4. Colours: relative colour syntax for lab(), lch(), hwb() and color(); an origin out of the sRGB gamut keeps its unclamped channels; system colours.
5. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests generated by scripts/lot108_gen.py (seeds 10801..10804) from the model scripts/lot108_model.py with the Lot 95 to 107 models; the models are written from the specifications (CSS Overflow 3, CSS Box Sizing 3, CSS Scroll-driven Animations 1, CSS Color 4 and 5) and share no code with the C: test_lot108_width.c (200 cascades of overflow, width, max-width, min-height, height: the slots and the style written), test_lot108_xclock.c (200 sheets with geometry: scroll and view timelines on the inline and x axes of box scrollers and the root), test_lot108_xflow.c (150 layouts: the content width, range, bars and clicks along x), test_lot108_rel.c (400 relative colours, through wf_cssprop_color and wf_cssprop_match). Hand: test_lot108_reader.c (6 tests), test_lot108_engine.c (5 tests: both axes and the corner, persistence, an inline flow end to end, max-width with a percentage, the content width).
QEMU: tests/scripts/test_qemu_lot108.py, `make qemu-lot108` (one boot at 1x): /lot108/x.html from the Lot 67 server: #o108 (overflow-x: auto, width: 300px) and #m108 (overflow: auto, max-width: 15em, height: 2em, min-height: 5em, scroll-timeline --x108 x) start at #000000; "web-bscroll 68 100" (where #m108's bar would be without min-height) gives none; "web-bscroll 158 66" gives #i108 #808080 (190 of 380 px); "web-bscroll 68 146" gives #n108 #404040 (110 of 440 px); #v108, a row of #m108 on view(inline), is #808080 at offset 0 and #bababa at 110 px (#808080 refused: the contents must move along x); #r108 #336689, #h108 #00cc00, #c108 #800080 at 500 ms. CI job lot108-reader-x.

## Gates on the box

Filled in by the final report (see the PR body).
