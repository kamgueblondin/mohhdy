# Lot 110 plan - the body as a box scroller of its own, its percentage heights against html, animated opacity

Base: 46704f5 (origin/main). PR 267 (the Lot 109 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot110.
One guest engine (userspace/webeng.c and its .inc files). Kernel, rights
and ABI unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and US-097
stay simule.

Scope taken from the Lot 109 gaps, in the order of the brief: (a) html and
body as box scrollers of their own (CSS Overflow 3, 3.3): html's overflow
always goes to the viewport, and the body's goes there only while html's
is visible; otherwise the body is a box scroller like any element, with
its timelines (scroll(), named scroll timelines, view()) and its
percentage heights taken against html's height; (b) one animated reader
property other than the colours: opacity (a value, inherit, keyframes,
@starting-style transitions). Part (c) (lh against line-height, relative
colours with none as an origin channel, more than one mixed
min()/max()/clamp() per value) is not done: the size of (a) and (b) is
already past the brief's range (see the gaps). No margin collapsing, no
new layout or font engine.

A row is listed only when its code, tests and docs are in the branch.
Size: see review point 10.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | the body as a box scroller (wf_sitecss.c) | the body was never a box scroller: its overflow and its timelines were the root's | wf_site_box_scroller: html never is one (its overflow is the viewport's); the body element under the root html element is one when html's computed overflow is not visible along an axis (ov_axis: a visible axis next to a non-visible one is auto) and the body's own value is a box value (body_own). Its style is passed as for any box scroller ("overflow-y:auto;height:Npx"); scroll(nearest), scroll(), a named scroll timeline on the body and view() of an element inside it run on the body's own scroll state: scroll_clock and view_clock read the geometry's body record (Lot 106) and the engine's WF_GEOM_SM_BODY mark (below). While html's overflow is visible nothing changes (the body's overflow goes to the viewport). Proof: test_lot110_body.c (280 sheets: which element is the box scroller, scroller_of, the timelines' colours, the need/geom counts), test_lot110_reader.c (body_own, body_top, body_scroll, body_x, body_view), `make qemu-lot110` |
| B | the body's percentage heights (wf_sitecss.c) | a percentage height of a box scroller is passed as it is (the engine takes it against the containing block) | the body box scroller's height, max-height and min-height in % are of html's height (CSS 2.1 10.5): html's height in px, or in % of the viewport height the reader is given (the initial containing block); truncated, at most WF_SITE_HMAX (30000); passed in px (body_pct). An html height that is auto or an expression leaves the percentage as it is (the engine reads it as auto). Proof: test_lot110_blen.c (220 sheets), test_lot110_reader.c (body_pct), `make qemu-lot110` (height: 50% of html's 400px: 200px) |
| C | the body record in the engine (webeng_geom.inc, wf_geom.h, webeng_scroll.inc) | we_geom_feed gave sm 0 (the root) to a box in an unmarked box scroller; a box's offset was kept in the forms by its id only | a box whose nearest box scroller is the body element (unmarked) has sm WF_GEOM_SM_BODY (WF_GEOM_MAX + 1); a marked body keeps its mark. The body's offset is kept in the forms under the key " body" when the body has no id (an id never holds a space), so it survives the GUI's relayout after a click (osui web-bscroll). Proof: test_lot110_geom.c (150 layouts: clicks on the body's bar, the record and the marks), test_lot110_engine.c (body_layout, geom_sm, round_trip, persist), `make qemu-lot110` |
| D | opacity (wf_sitecss.c, wf_sitecss.h) | not read | opacity is a reader property (kind WF_SITE_K_OP, outside the kinds that all and revert reset): a number or a percentage, in 1/10000 truncated toward zero and clamped to 0..1 (CSS Color 4: the computed value is clamped); initial and unset 1; inherit the parent's final value; anything else (calc(), var(), units) is dropped. The strongest declaration wins (importance, then order); @starting-style takes a value (not inherit). Keyframes of opacity are clamped when taken (a <number> registration) and interpolated as numbers; an !important own value is not animated; a transition runs from the @starting-style value. The style passes "opacity:N%" (whole percent, truncated: the engine keeps whole percents) and the css-anim line adds "opacity A T" (animated, transitioned) when one of them is not 0. Proof: test_lot110_op.c (360 sheets: values, cascades, inherit, keyframes, transitions), test_lot110_reader.c (op_values, op_inherit, op_anim), test_lot110_engine.c (opacity: the engine's transparency), `make qemu-lot110` (css-anim "opacity 1 1") |

## Review points

1. Structure sizes: we_doc_t 871464 bytes (unchanged, < 1 MiB), wf_geom_t 15464 bytes (unchanged), we_forms_t 9848 bytes (unchanged); wf_site_t 547628 bytes (was 543780: four ints and two bytes of opacity per node, two counters).
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3):
    - userspace/wf_sitecss.c, wf_sitecss.h: body_own, wf_site_box_scroller, scroll_clock through geom_box, view_clock's body mark, body_pct; opacity: op_num, op_text, the declaration, add_rule (@starting-style), paint_one, op_prop and kf_take, op_anim in col_pass, the style and the css-anim text; WF_SITE_K_OP and the node fields.
    - userspace/wf_geom.h: WF_GEOM_SM_BODY and the sm comment.
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added.
3. CSS Overflow 3 propagation: the reader only sees author styles, so "html's overflow is not visible" means a declared value other than visible (on either axis; clip next to visible keeps visible, as ov_axis computes it). The body's overflow then stays on the body; html's own overflow still goes to the viewport (html is never a box scroller).
4. A body box scroller without a height (or with a percentage of an auto html) has nothing to scroll: the reader passes nothing for it (as for any box scroller without a height) and the timelines that find it are inactive; the engine lays the page out on the root as before.
5. The forms key " body" is also taken by a nested body element without an id (the engine's parser keeps one inside a div); only the body under html is the reader's body box scroller.
6. Opacity is written in whole percents, truncated (0.255 is opacity:25%); the engine's transparency is 100 minus that. Keyframes and transitions interpolate the 1/10000 values; the result is truncated again.
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot105_model.py (box_scroller follows lot110_model; the old rule kept as box_scroller_lot105; scroll_clock and view_clock read the body record and the body mark), scripts/lot106_model.py and scripts/lot107_model.py (the clocks pass the nodes), scripts/lot108_model.py (the body's percentage heights through lot110_model.body_pct), scripts/lot105_gen.py (the geometry count leaves out the body, whose record is its own: Lot 106).
    - Regenerated from those models: test_lot105_overflow.c (4 lines), test_lot105_sheet.c (counts, flags and style of 2 cases), test_lot106_sheet.c (2 lines), test_lot107_pad.c (1 line): the cases whose html has overflow other than visible, where the body is now a box scroller.
    - Hand: test_lot105_reader.c (with html's overflow auto the body is a box scroller: wf_site_box_scroller 2 is WF_SITE_OV_SCROLL, scroller_of 3 and 7 is the body), test_lot108_reader.c (the body is flagged and its style is passed: " style=\"overflow-y:auto;height:100px\"").
8. CI: one new job, lot110-reader-body (make qemu-lot110, timeout 15 minutes).
9. Mutations: see the PR body (host run on the code commit and QEMU run on a git archive).
10. Size: see the PR body.

## Not done (first gaps for lot 111)

1. Box scrollers: the body's percentage heights against an html height given as an expression (left as a percentage); html's own width as the body's percentage width (widths are not resolved); ex/ch/lh against the element's line-height (lh reads line-height normal); a percentage inside a math function for a scroll-padding inset (auto); more than one mixed min()/max()/clamp() in one value, or one inside an arithmetic.
2. Geometry: view timelines in section 1 and later; the console web-load path (no layout); the engine's own y click without a relayout (Lot 74).
3. Animations: opacity in calc() or var(), all and revert on opacity, other reader properties (transform, widths), a running clock and repaint, transition events.
4. Colours: relative colours with none as an origin channel's value (the origin's missing components are 0), system colours, color() with a custom colour space.
5. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests generated by scripts/lot110_gen.py (seeds 11001..11004) from the model scripts/lot110_model.py with the Lot 95 to 109 models; the models are written from the specifications (CSS Overflow 3, CSS 2.1 10.5, Scroll-driven Animations, CSS Color 4, CSS Transitions 2) and share no code with the C: test_lot110_body.c (280 sheets), test_lot110_blen.c (220 sheets), test_lot110_op.c (360 sheets), test_lot110_geom.c (150 layouts through the engine). Hand: test_lot110_reader.c (10 tests), test_lot110_engine.c (5 tests).
QEMU: tests/scripts/test_qemu_lot110.py, `make qemu-lot110` (one boot at 1x): /lot110/body.html from the Lot 67 server: html has overflow: hidden and height: 400px, the body overflow: auto and height: 50% (200 px, range 634 px, bar at x 634..640); #i110 (scroll(nearest)) and #n110 (the body's named timeline) start at #000000, #v110 (view()) has no colour; "web-bscroll 637 100" gives #808080 and #v110 #848484 (317 of 634 px), "web-bscroll 637 190" #f2f2f2 (602), "web-bscroll 637 20" #191919 (63); opacity: css-anim "opacity 1 1" (#a110 animated, #t110 transitioned). CI job lot110-reader-body.

## Gates on the box

Filled in by the final report (see the PR body).
