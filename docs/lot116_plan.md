# Lot 116 plan - line-height as the engine reads it (numbers inherited as numbers, px with hundredths, 1..400 px), the engine's math bounds raised (32 nodes, 8 deep, 8 arguments), var() invalid at computed-value time unset for the box lengths, overflow, timelines and animation lists, the body against html's margin and padding, cap, ic and the root font units

Base: 326cd7e (origin/main after PR 273, Lot 115). The work was started
as "lot 113" on 2268ae3 (after PR 270, the Lot 112 branch); while it was
open, PRs 271, 272 and 273 (Lots 113 to 115: the JavaScript subset, its
web APIs and the layout values of page scripts) were merged, so it was
moved onto 326cd7e and renamed Lot 116 (page /lot116/lh.html,
test_qemu_lot116.py, qemu-lot116, CI lot116-engine-lh, test_lot116_*,
mutants M116-*). Lots 113 to 115 stay as they are. Branch cursor/lot116.
One guest engine (userspace/webeng.c and its .inc files; this lot changes
webeng.c and webeng.h). Kernel, rights and ABI unchanged. US-031 stays
partial. phase3_complete stays false. US-068/US-073, US-076, US-080 and
US-081 stay partial. US-078 and US-097 stay simule. No Chromium or WebKit,
no new font engine, no margin collapsing; this lot adds no JavaScript code.

Overlap with Lots 113 to 115, checked: none of their rows touches
line-height, math functions, var(), font units or the body's widths
(docs/lot113_plan.md, lot114_plan.md, lot115_plan.md); in the engine they
add only we_client_box (webeng_geom.inc, read only). Nothing was dropped
as a duplicate. Lot 115 lays page copies out with webeng.c for the
layout values of page scripts (wf_jslayout.c), so this lot's line heights
reach those values too; its tests (test_lot115_layout.c and the others)
pass unchanged.

Scope taken from the "Not done" list of docs/lot112_plan.md, in its
order: ex/ch with fonts the engine does not have (here: cap, ic and the
root units rex, rch, rcap, ric, against the engine's one 8x8 font); line
height (a number passed as a number, the engine's bounds, fractions);
mixed math past the engine's bounds; var() invalid at computed-value time
for the properties outside the kinds; the body's percentages against
html auto. Engine work was in scope and was done.

A row is listed only when its code, tests and docs are in the branch.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | the engine's line-height (webeng.c, webeng.h) | whole px 10..40 or a whole number 1..4 times the 10 px normal line (Lot 56); a fraction cut | lh_parse reads normal, a number, px, em and % with up to three decimals and keeps hundredths: px up to WE_LINE_H_MAX 400 (line_height + lh_frac), a number up to WE_LH_NUMBER_MAX 40 kept as the number (lh_unitless 1: used value number x the element's font px, CSS Inline 3 4.2; it inherits as the number), em and % (up to 40em, 4000%) resolved to px of the element's own font at the end of its cascade (children inherit the px); 0 and 0px are kept as the smallest value; the used line height is 1..400 px (node_lh100); a fraction is carried from line to line (finish_line: the hundredths add up and a line of the plain height takes the whole px they make, so 20.5px gives 20, 21, 20, 21); an orthogonal (vertical) root's contribution is its estimated lines times the line height in hundredths, cut the same way (it took the whole px; found by mutation, M116-E09). Proof: test_lot116_elh.c (220 cases: the computed fields of three nested blocks, the height and line tops of four lines), test_lot116_engine.c (lh_fields, lh_inherit, lh_carry, lh_carry_bounds, lh_vertical, lh_bounds), converted webeng_lot52.inc and webeng_lot69.inc, `make qemu-lot116` |
| B | the reader passes the line height as the engine reads it (wf_sitecss.c, wf_sitecss.h) | whole px within 10..40, truncated; a number as the px it gives | lh_engine: a number (at most WF_SITE_LH_NUM_MAX 40, the element's font whole px) is passed as the number ("line-height:1.25", rounded half up to hundredths), anything else as px with two decimals (rounded half up, at most WF_SITE_LH_MAX 400); the report's node line ends with " lh num N" or " lh N" (N with up to two decimals). lh_val: a bare 0 is the number 0, a math function of numbers is a number (calc(1 - 2) was read as 0px). Proof: test_lot112_lh.c and test_lot112_lflow.c (regenerated with the Lot 116 lh_engine), test_lot116_reader.c (lh_pass, lh_round), test_lot116_engine.c (reader_lh), `make qemu-lot116` (lh num 1.25, lh 20.5, lh 45) |
| C | the engine's math bounds (webeng.c, webeng.h) | 24 nodes, 4 functions deep, 4 arguments | 32 nodes, 8 deep, 8 arguments (WE_MATH_NODES_MAX, WE_MATH_DEPTH_MAX, WE_MATH_ARGS_MAX); a node packs its operator and dimension in one byte, so we_length_t keeps 194 bytes and we_doc_t its size. Proof: test_lot116_emath.c (320 values, 219 taken, the used width or the refusal), test_lot116_engine.c (math_bounds), converted webeng_lot56b.inc, webeng_lot58_width.inc, webeng_lot58_composition.inc, webeng_lot59_limits.inc, webeng_lot59_insets.inc |
| D | the reader's mirror of those bounds (wf_cssmath.c, wf_cssmath.h, wf_sitecss.h) | 24 nodes, 4 deep, 4 arguments, a 64-byte expression | WF_CSSMATH_NODES 32, WF_CSSMATH_FN_DEPTH 8, WF_CSSMATH_ARGS 8, MX_NODES 32, WF_CSSMATH_DEPTH 9 (the parse nests the leaf inside 8 functions), WF_SITE_BOXEX 128: mixed values with 5 to 8 incomparable arguments, deeper or longer, reach the engine. Proof: test_lot116_mixbig.c (220 values, 214 written, checked against the engine's math too), test_lot116_bflow.c (100 box scroller heights laid out by the engine), test_lot116_reader.c (math_mirror), test_lot111_mix.c and test_lot111_mflow.c (regenerated), `make qemu-lot116` (#d116, five arguments, 45 px) |
| E | the style attribute's length in the engine (webeng.c, webeng.h) | any attribute value past 255 bytes was refused whole ("attribute exceeds limit") | a style value is read up to WE_STYLE_ATTR_MAX 1279 bytes (the reader writes box lengths with math there), the others up to WE_ATTR_MAX 255 as before; the buffer is static. Bug found: a reader style attribute with a long math height lost the whole style. Proof: test_lot116_engine.c (style_attr), converted webeng_lot52.inc |
| F | var() invalid at computed-value time outside the kinds (wf_sitecss.c, wf_sitecss.h) | dropped (the earlier declaration stayed, the Lot 89 rule) for height, width, overflow, the timelines, scroll-padding and the animation and transition lists | unset (CSS Variables 1 3.1) for the properties of unset_prop: overflow, overflow-x/y, height, min/max-height, width, max-width, the scroll and view timeline properties, timeline-scope, view-timeline-inset, every scroll-padding property, animation-range, animation, transition and their longhands; also when the substituted value does not fit (each handler retries with unset). A var() naming an element's custom property is read per element for these too (their kinds come from the unset form); dprop holds 28 bytes (scroll-padding-inline-start). Bug found: such a per-element var() was unset sheet-wide instead of being substituted per element. Proof: test_lot116_unset.c (200 sheets, 114 unset, each against its rewrite), test_lot116_reader.c (unset_props, unset_per_element, unset_taken), test_lot116_engine.c (var_box), `make qemu-lot116` (#v116 height unset: auto) |
| G | the body against html's margin and padding (wf_sitecss.c) | html width auto with an html margin or padding left the body's percentage width to the engine (which takes it of the viewport: html's style is not written) | the body's percentage width and max-width are of the viewport less html's left and right margins and padding (CSS 2 10.3.3), at least 0. html height auto leaving the body's percentage height auto is CSS 2 10.5 already (checked, not changed). Proof: test_lot116_bodyw.c (120 sheets), test_lot116_reader.c (body_width), test_lot116_engine.c (body) |
| H | cap, ic and the root font units (wf_cssmath.c, wf_sitecss.c) | cap, ic, rex, rch, rcap and ric were units the reader did not resolve (a height was auto, a font-size or line-height dropped) | against the engine's 8x8 font (kernel/gfx_font8.h): cap is the capitals' 7 inked rows, os_ui_px(7, q) of the font's scale q (ex 5 rows and ch the 8 px cell as in Lot 112); the font has no ideograph, so ic is 1em (CSS Values 4 6.1.1.1); rex, rch, rcap and ric are those of the root's font (CSS Values 4 6.1.1); without the engine's metrics (the older box APIs) the proportions 7/16 and 16/16. They are units in box scroller lengths, html's lengths, scroll-padding, line-height and font-size. Bug found: the root's own font-size was read a second time against its new size and line height (html { font-size: 4rex } gave 24px, not 20px; 10rch was dropped; 2rlh took html's own line-height); the first read is kept. Proof: test_lot116_units.c (180 sheets, each against the same sheet with the model's px and the sheet without it), test_lot116_reader.c (font_units, root_font, lh_round), `make qemu-lot116` (#c116 4cap 28px, #e116 2rcap 14px) |

## Review points

1. Structure sizes: we_doc_t 871464 bytes (unchanged, < 1 MiB), we_node_t 4672 (unchanged: lh_frac fits the padding), we_length_t 194 (unchanged: the math node packs op and dimension), wf_geom_t 15464, we_forms_t 9848 (unchanged); wf_site_t 555184 bytes (was 553584: lhx is int per node, 1200 bytes, and dprop 28 bytes, 400). The engine's attribute buffer is a static 1280-byte array (was 256 bytes on the stack).
2. Files changed outside userspace/webeng.c (each a review point; all userspace, Ring 3):
    - userspace/webeng.h: WE_ATTR_MAX, WE_STYLE_ATTR_MAX, WE_LINE_H_MAX 400, WE_LH_NUMBER_MAX, WE_MATH_NODES_MAX 32, WE_MATH_DEPTH_MAX 8, WE_MATH_ARGS_MAX, the packed we_math_node_t, we_node_t lh_frac.
    - userspace/wf_sitecss.c, wf_sitecss.h: app_hund, app_lhx, lh_engine, the report's " lh", lh_val (0, number math), unset_prop and the parse_decl retries, body_pct, the unit list of height_val, resolve_lengths (the root's font-size), WF_SITE_LH_MAX, WF_SITE_LH_NUM_MAX, WF_SITE_BOXEX 128, dprop 28, lhx int.
    - userspace/wf_cssmath.c, wf_cssmath.h: the bounds, cap, ic, rex, rch, rcap and ric in with_unit (includes include/os_ui_scale.h, header only).
    - scripts/lot67_web_server.py: the /lot116/lh.html page.
    - Tests and tooling: tests/scripts/run_all_tests.sh, tests/scripts/test_qemu_lot116.py (new), Makefile (qemu-lot116), .github/workflows/ci.yml (lot116-engine-lh), the models and generators below.
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added.
3. A number line height is passed as the number only when the element's font size is whole px (the engine multiplies by its whole px font); otherwise the reader passes the px. The engine's font px is the reader's rounded size, so on em chains it may differ by one (Lot 112 review point 13).
4. The fraction carry is per block (the inline flow of one block); a line box raised by a taller inline keeps its own height and does not carry. line-clamp and columns take the whole px line height (cut): with a fractional line height the clamped height counts whole px lines (two lines of 20.6px clamp to 40 px, the boxes laid out are 20 + 21 = 41); see Not done.
5. var() unset for the properties of unset_prop: their initial values are those the reader already writes for unset (visible, auto, none, 0); overflow-y unset beside overflow-x auto computes to auto (CSS Overflow 3 3.1), so the box stays a scroller.
6. A unit the reader does not know (cqh, 2caps) in a height is still an auto height, as before (the Lot 107 rule), not a drop.
7. The QEMU pages of Lot 111 and 112 are unchanged and still pass (their lines are 20 px px values; the host mirror of the guest path gives the same lines).
8. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - Engine: webeng_lot52.inc (a 280-space style value is now accepted: the refused case uses 1300 spaces; a number line-height is times the font, 2 x 16 = 32 and the field 200 in hundredths; 9px, 5 and 41px are now line heights, -9px, 41 and 401px refused), webeng_lot69.inc (line-height 2 at 32px is 64, was 40), webeng_lot56b.inc, webeng_lot58_width.inc, webeng_lot58_composition.inc, webeng_lot59_limits.inc, webeng_lot59_insets.inc (the over-bound expressions made longer, deeper or with more arguments than the new bounds; the refusal still checked).
    - Reader: test_lot111_reader.c and test_lot111_engine.c (five arguments are passed now: min(90%, ...) is 104 px; nine are still refused; eight passed), test_lot112_reader.c (line-height 2 passed as "2", 10vh as 48px, 20000px capped at 400px; 10.5px font: px; font-size: 2cap is 14px now, cqh stands for an unread unit), test_lot112_engine.c (line-height: 2 passed as the number; 1..400 px), test_lot109_reader.c and test_lot107_reader.c (10cap resolves now: the unread unit is cqh), test_lot112_dom.c (regenerated, the bounds).
    - Models and generators: scripts/lot111_model.py (bounds 32/8/8, pool 32, BOXEX 128), scripts/lot112_model.py (lh_engine, engine_lh with hundredths; the Lot 112 ones kept as *_lot112), scripts/lot111_gen.py and scripts/lot112_gen.py (buffers of BOXEX); regenerated test_lot111_mix.c, test_lot111_mflow.c, test_lot112_lh.c, test_lot112_lflow.c, test_lot112_dom.c.
9. CI: one new job, lot116-engine-lh (make qemu-lot116, timeout 15 minutes).
10. -Werror at -O0/-O1/-O2/-O3/-Os: webeng.c has the misleading-indentation warning and (at -O3) one array-bounds warning, wf_sitecss.c the -O3 skip_noise array-bounds warning, all identical on the base; wf_cssmath.c is clean. No 64-bit division.
11. Mutations: see the PR body and mutations.txt.

## Not done (first gaps for lot 117)

1. Fonts: the engine has one 8x8 font; font-family, cap and ex of other fonts, and the ideographic advance (ic falls back to 1em) are not read; registered property lengths (@property, wf_cssprop.c) keep the 16 px proportions for ex and ch and do not take cap, ic or the root units.
2. Line height: a number with a fractional font size is passed as px (the engine's font is whole px); the fraction carry does not cross blocks or a raised line box; line-clamp counts whole px lines (the last line's fraction is not added to the clamped height: 40 px, not 41, for two lines of 20.6px); line-height in a math function of em inside the engine (the reader resolves it first) is not read.
3. Math: values past 32 nodes, 8 deep or 8 arguments are still refused (and lost in the reader); the engine's integer arithmetic cuts toward zero.
4. var(): an animated registered property read by another one's keyframe is read below the animations (Lot 112); properties outside the kinds and unset_prop (none left that the reader keeps, but new ones would need an entry).
5. Box scrollers: an html height left auto leaves the body's percentage height auto (CSS-correct); html's margin and padding in other units than px are dropped by the reader (only px and unitless kept), so they do not reduce the body's width.
6. Geometry, colours, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: as in docs/lot112_plan.md.

## Tests

Host tests generated by scripts/lot116_gen.py (seeds 11601..11607) from the model scripts/lot116_model.py with the Lot 97 to 112 models; the models are written from the specifications (CSS Inline 3, CSS Values 4, CSS 2, CSS Variables 1, CSS Cascade 5) and share no code with the C: test_lot116_elh.c (220), test_lot116_emath.c (320, 219 taken), test_lot116_mixbig.c (220, 214 written), test_lot116_bflow.c (100), test_lot116_bodyw.c (120), test_lot116_unset.c (200, 114 unset), test_lot116_units.c (180). Hand: test_lot116_reader.c (9 tests), test_lot116_engine.c (13 tests, one of them the QEMU page through the guest's reader-layout-geometry path).
QEMU: tests/scripts/test_qemu_lot116.py, `make qemu-lot116` (one boot at 1x): /lot116/lh.html from the Lot 67 server: html line-height 15px and height calc(26rlh + 10px), the body 200 px with line-height 1.25 passed as a number (20 px lines at 16 px, h1 40, #b116 24px 30, #e116 2rcap 14px 17), #q116 20.5px two lines 41, #w116 45px, #c116 4cap 28px, #d116 a five-argument max() 45 px, #v116 var() unset height auto; the body's range is 890 (844 with the Lot 112 code); #i116/#n116 follow the bar, #r116 (view()) #393939 at 667 and #b9b9b9 at 778. CI job lot116-engine-lh.

## Gates on the box

Filled in by the final report (see the PR body).
