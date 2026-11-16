# Lot 111 plan - lh against line-height, relative colours with missing origin components, nested mixed math, the body against html expressions, opacity in calc()/var()/all/revert

Base: f9668e2 (origin/main). PR 268 (the Lot 110 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot111.
One guest engine (userspace/webeng.c and its .inc files, unchanged in this
lot). Kernel, rights and ABI unchanged. US-031 stays partial.
phase3_complete stays false. US-068/US-073, US-076, US-080 and US-081 stay
partial. US-078 and US-097 stay simule.

Scope taken from the "Not done" gaps of docs/lot110_plan.md, in the order
of the brief: lh resolved against the element's line-height; none as an
origin channel in relative colours; more than one mixed min()/max()/clamp()
in one value (nested, or inside a sum or a product); the body's
percentages against an html height given as an expression and its
percentage widths against html's width; a percentage inside a math
function for a scroll-padding inset; opacity with calc(), var(), all and
revert; the QEMU page observes opacity values (the report's node lines).
No margin collapsing, no new layout or font engine, no JS VM.

A row is listed only when its code, tests and docs are in the branch.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | line-height and lh (wf_sitecss.c, wf_sitecss.h, wf_cssmath.c, wf_cssmath.h) | line-height was not read; lh was 10/16 em (line-height normal) everywhere | line-height is a reader declaration (kind WF_SITE_K_LH, the strongest wins; a token, lh_val): normal and initial; inherit and unset (it inherits); a non-negative number (1/1000, at most 1000); a percentage (1/1000 %); an absolute length; a font or viewport length or a math function that resolves without a percentage. A negative value or a math function with a percentage is dropped. The computed value (lh_resolve, CSS Inline 3 4.2, CSS Values 4 6.1.1) is resolved after the font size: a number stays a number (times the element's font size, inherited as the number), a percentage is of the element's font size and a length is absolute (both inherited as px), lh inside line-height is the parent's line height, normal is 10/16 em (the reader's font). Box scroller lengths, html's lengths and scroll-padding insets take lh against it (wf_css_math_box_lh). line-height itself is still not passed to the engine (it keeps its own line height). Proof: test_lot111_lh.c (200 sheets), test_lot111_reader.c (lh_values, lh_inherit, lh_refused, lh_edges, math_api), test_lot111_engine.c (lh_layout), `make qemu-lot111` (html line-height 20px, height max(20lh, ...)) |
| B | relative colours with missing origin components (wf_cssprop.c) | a missing (none) component of the origin was 0 | the origin's missing components are carried forward to the analogous components of the relative function's space (CSS Color 5 4.1, CSS Color 4 12.2: reds, greens, blues, lightness, colorfulness, hue, opponent a and b; cp_rel_miss): a channel keyword of such a component is none, so the result has it missing too and color-mix() takes the other colour's; inside a math function the keyword is 0; a left-out alpha is the origin's, missing included. Proof: test_lot111_rel.c (200 colours), test_lot111_reader.c (rel_none), `make qemu-lot111` (#t111 #c80000, #640000 before) |
| C | mixed min()/max()/clamp() nested or in an arithmetic (wf_cssmath.c, wf_cssmath.h) | one mixed function per value, not nested, not in a sum or a product (refused) | a box length mixing px and a percentage is a tree (mx_t, at most 24 nodes of 8 arguments): min()/max()/clamp() may nest, a min() of a min() is one min(), arguments of one part (px only or % only) are compared; a sum goes into the arguments (min(a, b) + c is min(a + c, b + c)), a factor or a divisor too (a negative one turns min into max; -clamp(a, b, c) is min(-a, max(-b, -c))). The value is written as the engine's math reads it, within its bounds: at most WE_MATH_NODES_MAX (24) nodes, WE_MATH_DEPTH_MAX (4) functions deep, 4 arguments per function (WF_CSSMATH_NODES, WF_CSSMATH_FN_DEPTH, WF_CSSMATH_ARGS), 63 bytes. Bug found: Lot 109 wrote a mixed function with 5 to 8 arguments, which the engine's math refuses (math_atom takes 4): such a value is now refused by the reader. Proof: test_lot111_mix.c (184 values), test_lot111_mflow.c (100 layouts through the reader and the engine), test_lot111_reader.c (mixed), test_lot111_engine.c (mixed_layout), `make qemu-lot111` (max(20lh, min(10%, 40px))) |
| D | the body's percentages against html's size (wf_sitecss.c) | an html height given as an expression left the body's percentage as it is; the body's percentage widths were not resolved | html's height or width given as an expression with a percentage is resolved against the viewport (the initial containing block) by reading the expression again with that basis (html_len, wf_css_math_box_at: cut toward zero, below 0 is 0, at most WF_SITE_HMAX); the body box scroller's width and max-width in % are of html's width as its heights are of html's height (CSS 2 10.2, 10.5; body_pct ax). html's width auto leaves the body's percentage to the engine. Proof: test_lot111_blen.c (160 sheets), test_lot111_reader.c (body), test_lot111_engine.c (body_width), `make qemu-lot111` (body 50% of 400 px) |
| E | a percentage inside a math function for a scroll-padding inset (wf_sitecss.c) | the inset was auto | the expression (stored "@N") is resolved against the scrollport along the timeline's axis (sp_inset view: vp_b or vp_i; below 0 is 0). Proof: test_lot111_pad.c (120 sheets, view timelines) |
| F | opacity in calc() and var() (wf_sitecss.c) | dropped | a math function of a number or a percentage (wf_cssprop_math_eval) is an opacity value; var() is substituted first (sheet and element custom properties, deferred as for the colour kinds); a value invalid after substitution is dropped (the reader's Lot 89 rule for var(), see review point 4). Proof: test_lot111_op.c (220 sheets), test_lot111_reader.c (opacity), test_lot111_engine.c (opacity) |
| G | opacity with all, revert and revert-layer (wf_sitecss.c) | all and revert did not touch opacity; revert and revert-layer were dropped | all sets opacity too (initial 1, inherit the parent's); revert rolls the author origin back (no user-agent opacity: 1), revert-layer the cascade layer (resolve_reverts, op_rv). The report's node line names the final opacity ("opacity 0.7500"), which the QEMU page reads. Proof: test_lot111_op.c, test_lot111_reader.c (opacity), `make qemu-lot111` (#o111 0.7500, #a111 0.4000, #t111 0.2000) |

## Review points

1. Structure sizes: we_doc_t 871464 bytes (unchanged, < 1 MiB), wf_geom_t 15464 bytes (unchanged), we_forms_t 9848 bytes (unchanged); wf_site_t 552428 bytes (was 547628: per node a line-height token, its rank, the revert mark of opacity, and three arrays of computed line heights).
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3; webeng.c itself is unchanged):
    - userspace/wf_sitecss.c, wf_sitecss.h: lh_val, lh_digits, lh_mul, lh_px_of, lh_resolve and WF_SITE_K_LH with the node and document fields; tok_box with the line height; html_len and body_pct ax; sp_inset view; opacity: the declaration (math, revert, revert-layer, deferred var()), paint_one, kind_rank, rv_at, reset_kind, all_kind, initial_value, paint_rule (all), resolve_reverts, the report's node line.
    - userspace/wf_cssmath.c, wf_cssmath.h: mp_t lh_px and basis, the mixed trees (mx_t, mx_new, mx_of, mx_val, mx_op, mx_clamp, mx_scale, mx_add, mixed, mx_cost, put_mx), box_run, wf_css_math_box_lh and wf_css_math_box_at (new public functions), WF_CSSMATH_NODES, WF_CSSMATH_FN_DEPTH, WF_CSSMATH_ARGS. wf_css_math, wf_css_math_px and wf_css_math_box keep their behaviour for values Lot 110 took.
    - userspace/wf_cssprop.c: cp_rel_miss and cp_rel (none for a keyword of a missing component).
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added.
3. lh_mul computes a * b / den in 32 bits (the freestanding build has no 64-bit division: a first draft with long long failed to link on __divdi3).
4. var() in opacity: a value invalid after substitution is dropped (the earlier declaration stays), the reader's Lot 89 rule for every var(); CSS Variables 1 says invalid at computed-value time (unset, so opacity 1). Not changed in this lot.
5. A valid mixed value the engine cannot take (5 arguments, past 24 nodes or 4 functions, past 63 bytes) is lost: the last declaration wins in the cascade (it is valid CSS) and its height is not passed, so a box scroller with no other height is not passed either (test_lot111_engine.c mixed_layout). Before Lot 111 the reader wrote a 5-argument min() and the engine dropped it.
6. Mixed trees: a sum distributed over arguments can grow (min(a, b) + min(c, d) has four arguments); the bounds above cut it.
7. The report's node line of an element with an opacity value now ends with " opacity N.NNNN" (also on the Lot 110 page, whose QEMU checks do not read those lines exactly).
8. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot109_model.py (math_box follows lot111_model; the old rule kept as math_box_lot109), scripts/lot110_model.py (op_decl with math, op_cascade with revert and revert-layer, body_pct with html expressions and widths; old functions kept as *_lot110), scripts/lot108_model.py (body widths through body_pct ax 1), scripts/lot107_model.py (sp_inset with the scrollport for "@N").
    - Regenerated from those models: test_lot109_math.c (4 values Lot 109 refused now have their values, one case added, the rest a shift of one case between test functions), test_lot109_box.c (6 lines: 3 styles), test_lot109_flow.c (24 lines: 3 layouts), test_lot110_op.c (2 lines: calc(0.5) is opacity:50%), test_lot110_blen.c (4 lines: html expression heights resolved), test_lot107_pad.c (2 lines), test_lot109_pad.c (1 line). The Lot 105 to 109 generators are run through /workspace/run regen.py (their scroller rule shimmed as in Lot 110); the Lot 110 generator reproduces directly.
    - Hand: test_lot109_reader.c (pct_refused: calc(min(50%, 10px) + 1px) and min(min(50%, 1px), 2px) now pass, checked with their new values; math_api: three mixed values in a product or a quotient from -1 to their values), test_lot110_reader.c (op_values: calc(0.5) moved to the values, revert removed from the refused list and checked as a rollback; body_pct: html calc(100% - 20px) gives 230px instead of 50%).
    - Not regenerated: test_lot108_rel.c (its generator now selects other cases under the Lot 111 models; the committed file still passes and the regenerated one passes too).
9. CI: one new job, lot111-reader-box (make qemu-lot111, timeout 15 minutes).
10. Mutations and size: see the PR body.

## Not done (first gaps for lot 112)

1. Line height: line-height is not passed to the engine (it keeps its own line height); line-height with a percentage inside a math function (dropped); rlh; all and revert on line-height (it is outside the kinds that all resets).
2. Math: a mixed value the engine cannot take (more than 4 arguments, 24 nodes, 4 functions deep or 63 bytes) is lost; partial comparison of comparable arguments inside a mixed function (min(10px, 20px, 5%) keeps all three).
3. Opacity: var() invalid at computed-value time (dropped, not unset); var() in opacity keyframes; opacity transitions from a var() value.
4. Box scrollers: ex and ch against the element's actual font (the reader's 8x8 metrics); an html width given as auto leaves the body's percentage width to the engine.
5. Geometry: view timelines in section 1 and later; the console web-load path (no layout); the engine's own y click without a relayout (Lot 74).
6. Colours: system colours, color() with a custom colour space; missing components through color-mix() inside a relative colour's origin of another space family (carried only to analogous components, as CSS Color 4 12.2 lists them).
7. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests generated by scripts/lot111_gen.py (seeds 11101..11107) from the model scripts/lot111_model.py with the Lot 97 to 110 models; the models are written from the specifications (CSS Color 4 and 5, CSS Values 4 and 5, CSS Inline 3, CSS Cascade 5, CSS 2 10.2 and 10.5, Scroll-driven Animations) and share no code with the C: test_lot111_rel.c (200 colours), test_lot111_op.c (220 sheets), test_lot111_lh.c (200 sheets), test_lot111_blen.c (160 sheets), test_lot111_pad.c (120 sheets), test_lot111_mix.c (184 values), test_lot111_mflow.c (100 layouts through the engine). Hand: test_lot111_reader.c (9 tests), test_lot111_engine.c (4 tests).
QEMU: tests/scripts/test_qemu_lot111.py, `make qemu-lot111` (one boot at 1x): /lot111/box.html from the Lot 67 server, the Lot 110 page (same content and geometry) with html's height max(20lh, min(10%, 40px)) and line-height 20px: 400 px whatever the viewport, so the body's 50% is 200px (range 634 px); #i111, #n111 and #v111 give the Lot 110 colours at 317, 602 and 63 px; the report's node lines read opacity 0.7500 (calc), 0.4000 (var) and 0.2000 (revert-layer), and #t111 color #c80000 (relative colour with a missing red, mixed). CI job lot111-reader-box.

## Gates on the box

Filled in by the final report (see the PR body).
