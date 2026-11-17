# Lot 112 plan - line-height passed to the engine, line-height math with percentages and numbers, rlh, all/revert on line-height, comparable arguments of mixed min()/max(), var() in keyframes, var() invalid at computed-value time

Base: 48709b8 (origin/main). PR 269 (the Lot 111 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot112.
One guest engine (userspace/webeng.c and its .inc files, unchanged in this
lot: the engine already reads line-height from a style attribute). Kernel,
rights and ABI unchanged. US-031 stays partial. phase3_complete stays
false. US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and
US-097 stay simule.

Scope taken from the "Not done" gaps of docs/lot111_plan.md, in the order
of the brief: line-height passed to the engine; line-height with a
percentage inside a math function; rlh; all and revert on line-height;
partial comparison of comparable arguments in mixed functions (so more
values fit the engine's bounds); var() in opacity keyframes and opacity
transitions from a var() value; var() invalid at computed-value time for
opacity (and line-height). Added: var() in color and background-color
keyframes. ex and ch against the element's actual font are not done (see
Not done). No margin collapsing, no new layout or font engine, no JS VM.

A row is listed only when its code, tests and docs are in the branch.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | line-height passed to the engine (wf_sitecss.c, wf_sitecss.h) | the reader computed line-height (Lot 111) for lh only; the engine kept its own line height | the reader writes line-height in the element's style attribute (lh_engine): a computed line height as whole px within the engine's bounds (WE_LINE_H 10 .. WE_LINE_H_MAX 40, truncated; a number is passed as the px it gives for the element's font, the engine inheriting px), normal as "line-height:normal" when the element's own or an ancestor's line height is not normal (so the engine does not inherit an ancestor's px), nothing when the element and all its ancestors are normal (the engine's default). The report's node line ends with " lh N" or " lh normal". Proof: test_lot112_lh.c (180 sheets), test_lot112_lflow.c (100 layouts through the engine), test_lot112_engine.c (lh_layout), `make qemu-lot112` (20 px lines, the body's range 952) |
| B | line-height math with a percentage or numbers (wf_sitecss.c) | a math function with a percentage was dropped; a number math function was dropped | a math function with a percentage is resolved against the element's font size (lh_resolve with the font as basis, wf_css_math_box_ctx); a math function of numbers is a number and one of absolute lengths the box math does not read (in, cm, mm, q, pt, pc) is a length (wf_cssprop_math_eval; below 0 is 0, CSS Values 4 10.12); another type (an angle, a time) is dropped. Proof: test_lot112_lh.c, test_lot112_reader.c (lh_math), `make qemu-lot112` (calc(100% + 4px)) |
| C | rlh (wf_cssmath.c, wf_cssmath.h, wf_sitecss.c) | rlh was not a unit | rlh is the root's line height (wf_css_box_ctx_t rlh_px; in the root's own line-height and with no root line height it is normal at 16px, 10 px); it is a unit in box scroller lengths, html's lengths, scroll-padding insets and line-height itself (height_val's unit list, box_of). Not in font-size (Not done). Proof: test_lot112_lh.c, test_lot112_reader.c (rlh), test_lot112_engine.c (rlh_layout), `make qemu-lot112` (html calc(26rlh + 10px) and #r112 2rlh) |
| D | all, revert and revert-layer on line-height (wf_sitecss.c) | all did not touch line-height; revert and revert-layer were dropped | all sets line-height too (initial normal, inherit and unset inherit); revert rolls the author origin back (no user-agent line-height: inherited), revert-layer the cascade layer (kind_rank, rv_at, reset_kind, kind_inherits, initial_value, all_kind, paint_one, parse_decl). Proof: test_lot112_lh.c, test_lot112_reader.c (lh_cascade), test_lot112_engine.c, `make qemu-lot112` (#t112 revert-layer, lh 18) |
| E | comparable arguments of a mixed min()/max() (wf_cssmath.c) | min(10px, 20px, 5%) kept all three; five or more arguments were refused | after flattening, an argument another one covers is dropped (mx_cover: in a min(), a covers b when its px part and its % part are both at most b's; in a max() at least; of equal arguments the first stays). More values fit the engine's 4 arguments: min(50%, 120px, 80%, 90px, 150px) is min(50%, 90px). Proof: test_lot112_dom.c (145 values, checked against the engine's math too), test_lot111_mix.c and test_lot111_mflow.c (regenerated), test_lot112_reader.c (dominance), test_lot112_engine.c (dom_layout) |
| F | var() and math in opacity keyframes (wf_sitecss.c) | a keyframe with var() was left out; calc() was left out | var() in an opacity keyframe is the animated element's (op_anim sets the element for kf_take; invalid after substitution: the frame is left out); a math function of a number or a percentage is a value. Opacity transitions from a var() value (in @starting-style or as the end value) work since the Lot 111 deferral and are proven here. Proof: test_lot112_opk.c (120 sheets), test_lot112_reader.c (op_frames), test_lot112_engine.c (op_frames), `make qemu-lot112` (#k112 opacity 0.4000) |
| G | var() in color and background-color keyframes (wf_sitecss.c) | left out | as in F: col_pass sets the element for kf_take, the substituted text must be a colour (currentcolor included), else the frame is left out. A registered property's keyframes still leave var() out. Proof: test_lot112_ckf.c (120 sheets), test_lot112_reader.c (op_frames), `make qemu-lot112` (#k112 color #ff8080) |
| H | var() invalid at computed-value time for opacity and line-height (wf_sitecss.c) | dropped (the earlier declaration stayed; Lot 89 rule) | unset (CSS Variables 1 3.1): opacity 1, line-height inherited; also when the substituted value is not an opacity or a line height. Bug found (Lot 111): a per-element var() whose sheet-wide substitution is no opacity was dropped instead of being read per element; now it is (and for line-height). Proof: test_lot111_op.c (regenerated), test_lot112_lh.c, test_lot112_reader.c (var_unset), `make qemu-lot112` (#u112 lh 20) |

## Review points

1. Structure sizes: we_doc_t 871464 bytes (unchanged, < 1 MiB), wf_geom_t 15464 bytes (unchanged), we_forms_t 9848 bytes (unchanged); wf_site_t 553584 bytes (was 552428: per node the revert mark of line-height; per document the root's line height and two arrays: what each element passes the engine, and whether an ancestor's line height is not normal).
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3; webeng.c itself is unchanged):
    - userspace/wf_sitecss.c, wf_sitecss.h: lh_engine, the style attribute's line-height and the report's " lh"; box_of and tok_box with a basis; lh_val with percentages and number math; rlh (root_lh, height_val); line-height in kind_rank, rv_at, reset_kind, kind_inherits, initial_value, all_kind, paint_one, parse_decl; var() unset for opacity and line-height; the deferred var() kept when its sheet-wide value is invalid; kf_take with var() (g_kfdoc), op_anim, col_pass.
    - userspace/wf_cssmath.c, wf_cssmath.h: mx_cover and mx_op (dominance), mp_t rlh_px, the rlh unit, wf_css_box_ctx_t and wf_css_math_box_ctx (new public function); box_run takes the context. wf_css_math_box_lh and wf_css_math_box_at keep their behaviour (rlh as normal).
    - scripts/lot67_web_server.py: the /lot112/lh.html page; the /lot111/box.html page gets "line-height: normal" on the body (see 6).
    - Tests and tooling: tests/scripts/run_all_tests.sh, tests/scripts/test_qemu_lot112.py (new), tests/scripts/test_qemu_lot111.py (converted), Makefile (qemu-lot112), .github/workflows/ci.yml (lot112-reader-lh), the models and generators below.
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added.
3. The engine's bounds: a line height past 40 px is passed as 40, below 10 as 10 (the engine's WE_LINE_H_MAX and WE_LINE_H); a fraction of a px is cut. A unitless number is passed as px (the engine has no unitless line-height that inherits as a number); the reader resolves it per element, so each element carries its own px.
4. var() invalid at computed-value time is unset for opacity and line-height only; the other properties keep the Lot 89 rule (dropped).
5. Dominance (E) changes what the reader writes for mixed min()/max(): arguments another one covers are no longer written, so some values the engine refused (5 to 8 arguments) are now passed.
6. The Lot 111 QEMU page: html's line-height 20px now reaches the engine, which would change the Lot 110 geometry the page checks; the body gets line-height: normal (scripts/lot67_web_server.py), and the checks read " lh normal" at the end of the node lines (tests/scripts/test_qemu_lot111.py, converted).
7. The report's node lines end with " lh N" or " lh normal" when the reader passes a line-height.
8. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot111_model.py (_Mix.op with dominance, the old rule kept as op_lot111; lh_decl and lh_comp with percentage and number math and rlh, kept as *_lot111; op_value with var() unset, kept as op_value_lot111), scripts/lot110_model.py (op_kf; op_cascade with var() unset), scripts/lot110_gen.py (keyframe frames through op_kf), scripts/lot109_model.py (rlh).
    - Regenerated from those models: test_lot111_mix.c, test_lot111_mflow.c (dominance), test_lot111_lh.c (18 lines: percentage and number math), test_lot111_op.c (var() unset), test_lot110_op.c (4 lines: calc() keyframes), test_lot109_box.c, test_lot109_flow.c, test_lot109_math.c (dominance; the Lot 109 generator run through /workspace/run regen.py as in Lot 111).
    - Hand: test_lot111_reader.c (the style strings now carry line-height; calc(100% + 2px) moved from the refused list to an explicit value, calc(1px + 1) and calc(2 * 3px * 4px) added as refused; the mixed bound cases replaced by incomparable arguments and the old values checked with their new results), test_lot109_reader.c (2 values with comparable arguments now written shorter, incomparable cases added), test_lot110_reader.c (a calc(1) keyframe is now a value; the left-out case uses 1px), test_lot111_engine.c (the five-argument case made incomparable; min(90%, 1px, 80%, 2px, 70%) is height:min(1px, 70%), 1 px).
9. CI: one new job, lot112-reader-lh (make qemu-lot112, timeout 15 minutes).
10. The models (lot111_model.py lh_decl) know math of numbers and percentages; math of absolute lengths the box math does not read is checked by hand tests only (test_lot112_reader.c lh_math), the generated sheets do not draw it.
11. -O3 -Werror: wf_sitecss.c's skip_noise has an array-bounds warning at -O3 on the base too (not a regression of this lot).
12. Mutations and size: see the PR body.

## Not done (first gaps for lot 113)

1. Fonts: ex and ch against the element's actual font (the reader still uses its 8x8 metrics); rlh and lh in font-size.
2. Line height: a unitless number is passed as px, not as a number the engine would inherit; the engine's bounds (10..40 px) cut larger or smaller line heights; fractions of a px are cut.
3. Math: a mixed value past the engine's bounds after dominance (more than 4 incomparable arguments, 24 nodes, 4 functions deep, 63 bytes) is still lost.
4. var(): invalid at computed-value time is unset for opacity and line-height only; var() in registered properties' keyframes and in other keyframe properties is left out; the deferred var() with an invalid sheet-wide value is fixed for opacity and line-height (color and background-color were not affected in the probes; the other kinds were not audited).
5. Box scrollers: an html width given as auto leaves the body's percentage width to the engine.
6. Geometry, colours, fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: as in docs/lot111_plan.md.

## Tests

Host tests generated by scripts/lot112_gen.py (seeds 11201..11205) from the model scripts/lot112_model.py with the Lot 97 to 111 models; the models are written from the specifications (CSS Inline 3, CSS Values 4, CSS Cascade 5, CSS Variables 1, CSS Animations 1, CSS Color 4) and share no code with the C: test_lot112_lh.c (180 sheets), test_lot112_lflow.c (100 layouts through the engine), test_lot112_dom.c (145 values), test_lot112_opk.c (120 sheets), test_lot112_ckf.c (120 sheets). Hand: test_lot112_reader.c (8 tests), test_lot112_engine.c (4 tests).
QEMU: tests/scripts/test_qemu_lot112.py, `make qemu-lot112` (one boot at 1x): /lot112/lh.html from the Lot 67 server: html line-height 15px and height calc(26rlh + 10px) (400px), the body's 50% is 200px and its line-height calc(100% + 4px) is 20px on each line; the body's range is 952 (634 with normal lines); #i112 and #n112 follow the bar, #r112 (2rlh, 30 px, view()) is in view only with the new range: #8e8e8e at 476, #f7f7f7 at 571; #u112 lh 20 (var() unset), #t112 lh 18 (revert-layer), #k112 opacity 0.4000 and color #ff8080 (var() in keyframes). CI job lot112-reader-lh.

## Gates on the box

Filled in by the final report (see the PR body).
