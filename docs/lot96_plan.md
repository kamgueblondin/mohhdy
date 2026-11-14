# Lot 96 plan - fractional degrees and repeating gradients (engine and reader)

Base: fc0fdc9 (head of PR 253, branch lot95-property-limits-computed; its
tree is the tree of eedf5da, the local Lot 95 head, plus one line of
.github/workflows/ci.yml: the coverage job timeout 15 to 25 minutes).
Branch cursor/lot96. One guest engine (userspace/webeng.c) and one shell
path for the console and the desktop. Kernel, rights and ABI unchanged.
US-031 stays partial. phase3_complete stays false. US-068/US-073, US-076,
US-080 and US-081 stay partial. US-078 and US-097 stay simule.

Scope taken from the Lot 95 first gaps, in the order of the brief: gap 1
(fractional degrees and repeating-linear/radial/conic gradients, conic
"from" in radians) and the <image> stop check of gap 2 are taken.
Transitions and animations of registered properties and em/rem/ex/ch are
left for lot 97 because the size budget is reached (see Not done).

A row is listed only when its code, tests and docs are in the branch.
Size: about 9900 added lines (1920 removed) against fc0fdc9 before these
docs, of which about 5850 are generated host test suites
(tests/unit/userspace/test_lot96_{cells,bgval,accept,sheet}.c, written by
scripts/lot96_gradgen.py from the model scripts/lot96_gradmodel.py and the
Lot 94 model scripts/lot94_gradmodel.py, which share no code with the C
under test, fixed seeds 9601 to 9603), about 1810 are regenerated
converted suites (review point 6) and about 2200 are written by hand (about
330 lines of product code in webeng.c, webeng.h, wf_cssbg.c and
wf_cssprop.c, the models and generators, the two hand suites, the QEMU
test, page and server page, the CI job, docs).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Fractional degrees in the engine (Lot 95 gap 1) | linear angles were whole degrees (tf_angle rounded to 1deg) | angles are kept in 1/100 degree (we_node_t grad_angle in degrees plus grad_frac 0..99, no change of size): the value is read in 1/1000 of its unit (one helper tf_angle_milli for transforms and gradients), times the unit factor (turn 360, grad 9/10, rad 57.3, truncated), rounded half away from zero to 1/100 degree. The cells use sin/cos interpolated between the whole-degree table entries (rounded half up). Quarter turns keep their bands; a side keeps angle 0. Vertical boxes map the fractional angle (lr: 630deg - a, rl: a + 270deg and back). Conic stops in degrees take two decimals. Proof: test_lot96_engine.c (fractional_angles, vertical), test_lot96_cells.c, test_lot94_engine.c (converted), `make qemu-lot96` (#frac96 at 30.25deg, cells checked against the model) |
| B | Repeating gradients in the engine (Lot 95 gap 1) | repeating-* made the background refused | repeating-linear-gradient (always painted in cells along its angle; a side is its quarter turn, a quarter turn without a corner puts the 32 cells in one row or one column), repeating-radial-gradient (cells; each takes the colour at its centre from the elliptic distance p = isqrt(64 N) / 8R, 64-bit, no division helper), repeating-conic-gradient (cells). Positions are mapped into the first to last stop (pos0 + (p - pos0) mod period). A period of 0 (the last stop, after raising, not past the first) refuses the gradient as before. Guards: a radius past 8191 px or a doubled centre offset past 32767 px gives "repeating gradient too large" and no cell. A repeating radial child of a vertical-rl box is refused ("repeating radial refused in rl"); vertical-lr keeps it. Proof: test_lot96_engine.c (repeat_by_hand, repeat_parse, guards, vertical), test_lot96_cells.c (800 boxes), test_lot96_sheet.c, `make qemu-lot96` (#rep96, #rad96) |
| C | Reader (Lot 95 gap 1) | the reader wrote radians as whole degrees, refused conic "from" in radians and repeating-* | wf_cssbg.c passes repeating-linear/radial/conic-gradient on (same checks as the plain ones, plus the period check of the engine), writes radians in 1/100 degree with trailing zeros trimmed (1rad as 57.3deg, the factor 0.5729577951 in 1e-10 units), takes a conic "from" in radians (written as degrees) and conic stops in degrees with up to two decimals (0..360). The background shorthand takes the repeating names as images. Proof: test_lot96_bgval.c (1336 values), test_lot96_accept.c, test_lot96_reader.c, test_lot91/92/94 suites (converted), `make qemu-lot96` (/lot96/grad.html) |
| D | <image> stops (Lot 95 gap 2, review point 4 of Lot 95) | a registered <image> checked only the gradient name and balanced parentheses | a gradient value of a registered <image> is checked by the background reader (wf_cssbg_gradient): a value the background reader refuses does not match, so the declaration falls back to the initial value. Proof: test_lot96_bgval.c (match cases), test_lot95_props.c and test_lot95_match.c (converted), `make qemu-lot96` (#img96) |

## Review points

1. Branch base. cursor/lot96 starts at fc0fdc9 (head of PR 253), whose tree is the tree of eedf5da (local Lot 95 head) plus the coverage timeout line (15 to 25 minutes), checked with git diff before branching. The mbox holds only the Lot 96 commits (fc0fdc9..HEAD).
2. Structure sizes: we_node_t keeps 4668 bytes and we_doc_t 870824 bytes (< 1 MiB): grad_frac and grad_repeat fill padding. wf_site_t is unchanged (447372 bytes). New constants WE_GRAD_RADIUS_MAX 8191 and WE_GRAD_OFFSET_MAX 32767 in webeng.h; no ABI structure (os_fb_web_item_t, syscalls) changes.
3. Approximations: 32 flat cells per repeating gradient (each takes the colour at its centre; a short period shows as stripes of cell size, not a smooth repeat); sin and cos interpolated linearly between whole degrees; the radial distance uses an integer square root (the generator asserts, for every generated cell, that the model's integer position is within 1 + 10000 / (16 rx ry) of the exact one, in 1/10000); 1rad is 57.3deg in the engine (as the transforms before), while the reader writes radians with the exact factor; decimals past the third are dropped before the unit factor.
4. Refusals kept on purpose: a period of 0 (as browsers paint the average colour, the gradient is refused here and the earlier background stays); a repeating radial gradient in a vertical-rl box (its cells are painted in the logical frame and rotated, the centre would need the physical width, as for the conic refusal of lot 95); a radius or centre offset that would pass 32-bit products in the cell distance.
5. Vertical-rl repeating linear "to right": the cells are partitioned in the logical frame, so the partition is mirrored; the colours are the colours at the cell centres (documented approximation, test_lot96_engine.c vertical).
6. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot94_gradmodel.py: sin65536 interpolates for fractional angles; decimal_degrees keeps 1/100 degree (truncated to three decimals as the engine reads them).
    - scripts/lot94_gradgen.py (same seeds and random sequence): fractional angles are accepted in cells, "red 45.5deg" became "red 45.555deg" (three decimals stay refused), "from 1rad" became "from 1.rad" (still refused) and a radian "from" is valid and written as degrees; regenerated test_lot94_accept.c, test_lot94_bgval.c, test_lot94_cells.c, test_lot94_sheet.c.
    - scripts/lot91_bggen.py and scripts/lot92_bggen.py: radians are written in 1/100 degree (test_lot91_bgradial.c 2 lines, test_lot92_bgrad.c and test_lot92_bgsheet.c regenerated).
    - scripts/lot95_propmodel.py: <image> gradients use the Lot 96 reader model; regenerated test_lot95_compute.c, test_lot95_match.c, test_lot95_sheet.c.
    - test_lot94_engine.c: an angle of 44.5deg is now 44deg plus 50/100 (was rounded to 45).
    - test_lot95_props.c: "repeating-conic-gradient(from 1deg, red, (blue))" no longer matches <image> (the stop "(blue)" is refused by the background reader), and a new assertion checks that the same value with "blue" matches; two values with a quote inside a gradient no longer match.
    - tests/scripts/test_qemu_lot92.py: #radbad92 expects 57.3deg (was 57deg).
7. Removed while writing the mutations, since no result could change: the repeat test of the axis grid (only a repeating gradient keeps a quarter turn as an angle without a corner), the period check of grad_repeat_p (the parser refuses a period of 0), the back-pass test of the rl refusal (the first pass already cleared the gradient), the "g < 0" test of a conic "from" (a bad radian is not an angle() either) and the "10000 > top" test of the reader period (no stop is past 100%) and the percentage test of the angle reader (a value ending in a digit is never a percentage; it came with the unit table of tf_angle). tf_angle and tf_angle_c now share tf_angle_milli (no duplicate unit table).
8. Mutations: 92 applied / 92 killed / 0 survived counted: 91 host mutations of rows A to D (run on the tree of 3c664eb, logs/mut1.txt) and one QEMU mutation (grad_frac stored as 0: make qemu-lot96 failed on the #frac96 cell colours). The run applied 92 host mutations; M96-w-tf-percent survived because the percentage test of the angle reader could not change a result; the test was removed (point 7) and the mutation is not counted.
9. CI: one new job, lot96-engine (make qemu-lot96, no guest clock option, timeout 15 minutes; about 40 s locally). The coverage job keeps the 25 minutes of fc0fdc9.
10. ASan: base and head fail the same 5 suites, no new failure (see Gates).
11. Size: about 9900 added lines before the docs, inside the 8000 to 10000 target; transitions/animations and em/rem/ex/ch were therefore not started (Not done 2 and 3).

## Not done (first gaps for lot 97)

1. Gradients: per-pixel painting (32 cells), a repeating radial gradient in vertical-rl (refused) and the mirrored rl partition of a repeating linear gradient, conic children of vertical boxes, the vertical-rl radial centre, background-clip: text, a clip or origin per layer, background-attachment, fetched background images, a period of 0 painted as the average colour.
2. @property: transitions and animations of registered properties (not started); no <frequency>, <ratio> or <flex>; more than 8 alternatives, 16 registrations or 127-byte values are refused.
3. Math: em, rem, ex and ch are kept as text (no font context in the sheet; not started); min(), max(), clamp() and the other functions of a length-percentage mix are kept; fixed point with 1/10000 steps; radians past 100 are kept; infinity and nan are never known.
4. Autofill: three fixed summer time rules (no tz database) (unchanged).
5. PSL: 360 hand-written entries (unchanged).
6. NE2000: the FIFO holds 16 frames, the drain is skipped during an LLM session (unchanged).
7. Shaping: single node settings, no WAN (US-068/US-073 partial, unchanged).
8. US-080: no DDC, fixed safe area margin (unchanged).
9. US-081: no backlight, no watts, battery simulated (unchanged).

## Tests

Host tests: tests/unit/userspace/test_lot96_cells.c (200 tests, 800 boxes), test_lot96_bgval.c (167 tests, 1336 values), test_lot96_accept.c (69 tests), test_lot96_sheet.c (40 tests, 160 sheets), test_lot96_engine.c (5 hand tests), test_lot96_reader.c (4 hand tests). Generator scripts/lot96_gradgen.py (seeds 9601..9603) with the independent model scripts/lot96_gradmodel.py (Decimal and Fractions). Converted generators: scripts/lot91_bggen.py, scripts/lot92_bggen.py, scripts/lot94_gradgen.py, scripts/lot94_gradmodel.py, scripts/lot95_propmodel.py.
QEMU: tests/scripts/test_qemu_lot96.py, `make qemu-lot96` (one boot at 1x): /web/lot96.html (web-render, 94 rectangles checked against the models) and /lot96/grad.html from the Lot 67 server (web-css). CI job lot96-engine.

## Gates on the box

Run on 284ee43 (the code of this lot; the later commit changes docs only) on 14 November 2026 (UTC+4), gate starts 07:14:14 to 10:06:29 (qemu-lot72, stopped at 10:09), one QEMU guest at a time (scripts in /workspace/run/lot96: gates.sh). Every gate that ran passed; none was rerun. On the user's request GitHub CI is the full gate: gates.sh was stopped at 10:09 during qemu-lot72 (its process group killed, no QEMU left; qemu-lot72 gave no result), so qemu-lot72, qemu-lot71, qemu-web-layout and qemu-production were not run locally and are left to GitHub CI. tree-clean was run alone after the stop (10:10, pass).

| Gate | Result | Time |
|------|--------|------|
| make-all | pass | 1 s |
| test-all | pass | 744 s |
| roadmap-check | pass | 0 s |
| font-check | pass | 1 s |
| secrets | pass | 6 s |
| strict-image | pass | 1 s |
| qemu-lot96 | pass | 40 s |
| qemu-lot95 | pass | 32 s |
| qemu-lot94 | pass | 40 s |
| qemu-lot93 | pass | 31 s |
| qemu-lot92 | pass | 60 s |
| qemu-lot91 | pass | 896 s |
| qemu-lot90 | pass | 846 s |
| qemu-gui-text-scale | pass | 166 s |
| qemu-gui-parity | pass | 417 s |
| qemu-lot89 | pass | 686 s |
| qemu-p2p | pass | 1002 s |
| qemu-p2p-mesh | pass | 499 s |
| qemu-p2p-phase5 | pass | 204 s |
| qemu-net-wire | pass | 83 s |
| qemu-lot67-web | pass | 1016 s |
| qemu-lot88-desk | pass | 639 s |
| qemu-lot88-tv | pass | 450 s |
| qemu-lot87 | pass | 208 s |
| qemu-lot86 | pass | 591 s |
| qemu-lot85 | pass | 152 s |
| qemu-lot84 | pass | 145 s |
| qemu-lot83 | pass | 107 s |
| qemu-lot82 | pass | 97 s |
| qemu-lot81 | pass | 117 s |
| qemu-lot80 | pass | 97 s |
| qemu-lot79 | pass | 302 s |
| qemu-lot78 | pass | 40 s |
| qemu-lot77 | pass | 24 s |
| qemu-lot76 | pass | 21 s |
| qemu-lot75 | pass | 90 s |
| qemu-lot74 | pass | 96 s |
| qemu-lot73 | pass | 388 s |
| qemu-lot72 | not run (stopped; left to GitHub CI) | - |
| qemu-lot71 | not run (left to GitHub CI) | - |
| qemu-web-layout | not run (left to GitHub CI) | - |
| qemu-production | not run (left to GitHub CI) | - |
| tree-clean | pass | 0 s |

make test-all: 22009 tests, 22009 passed, 0 failed (Lot 95 head: 21520).

ASan and UBSan (TEST_CC="gcc -fsanitize=address,undefined -fno-sanitize-recover=all", make test-all on git archive copies of each tree): base fc0fdc9 21435 tests, 5 failed (test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt), 754 s; head 284ee43 21924 tests, 5 failed (the same five), 1508 s. All Lot 96 suites and the converted suites pass under ASan; no new failure. The ASan runs overlapped the mutation run and the first gates.

Mutations: 92 applied / 92 killed / 0 survived counted (91 host, 1 QEMU); one more host mutation (M96-w-tf-percent) survived because its check could not change a result, the check was removed and the mutation is not counted (mutations.txt).
