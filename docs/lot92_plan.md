# Lot 92 plan - math functions and computed registered values; radian gradient angles

Base: 4def6ce (Lot 91 head, the same tree as f870156 of open PR 249,
branch cursor/lot91-reader-property-dst-822e; main is bf79580, Lot 89).
Branch cursor/lot92. One guest engine (userspace/webeng.c, unchanged in
this lot) and one shell path for the console and the desktop. Kernel,
rights and ABI unchanged. US-031 stays partial. phase3_complete stays
false. US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and
US-097 stay simule.

Scope taken from the Lot 91 first gaps: gap 3 (calc() typed but not
simplified; no sign(), abs(), round(), mod() or trigonometric functions)
and the radian part of gap 1 (rad angles). Gaps 2 and 4 to 9 are left as
they were (see Not done).

A row is listed only when its code, tests and docs are in the branch.
Size: about 10200 added lines, of which about 8060 are generated host
test suites (tests/unit/userspace/test_lot92_{mathtype,matheval,compute,
propsheet,bgrad,bgsheet}.c and the regenerated test_lot91_bgradial.c,
written by scripts/lot92_mathgen.py, lot92_bggen.py and lot91_bggen.py
from Python models that do not share code with the C under test, fixed
seeds) and about 2100 are written by hand (about 470 lines of product
code in wf_cssprop, wf_sitecss and wf_cssbg, generators, hand tests,
QEMU test, docs).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Math functions and computed values of registered properties (US-031, Lot 91 gap 3) | calc(), min(), max() and clamp() were type checked only; abs(), sign(), round(), mod(), rem() and the trigonometric functions made a value invalid; a registered value kept its text, so calc(20px * 2) reached a property as text and every reader value that takes a plain length (a radial radius, a gradient angle) refused it | wf_cssprop: the math parser carries a value with each type, in 1/10000 of the canonical unit (px, deg, s, %, number); every conversion and every * and / rounds half away from zero; a value past 100000 units is not known; 64-bit products and a shift-subtract division (no __divdi3). Units: in 96px, cm 4800/127 px, mm 480/127, q 120/127, pt 4/3, pc 16; grad 9/10 deg, turn 360, rad 1800000000/31415927; ms 1/1000 s. abs() keeps the type, sign() gives a number, round() takes nearest (half toward +infinity), up, down or to-zero and a step B of the same type (B left out only when A is a number), mod() takes the sign of B and rem() the sign of A; these are typed and simplified. sin(), cos(), tan() (a number or an angle in, a number out), asin(), acos(), atan() (a number in, an angle out) and atan2() (two values of one type) are typed and kept as text. wf_cssprop_math_eval gives the value in its canonical unit; wf_cssprop_compute gives the computed value of a registered value that is one math function whose first matching alternative is one numeric type without a multiplier (<integer> rounds half toward +infinity). wf_sitecss computes such a value where it is declared (descendants inherit the computed value) and computes a math initial value; the css-props line ends with " simplified N" only when N > 0. Relative lengths (em, rem, ex, ch, vw, vh, vmin, vmax), a length mixed with a percentage, a zero divisor or step and the trigonometric functions keep the text. Proof: test_lot92_mathtype.c (840 generated values), test_lot92_matheval.c (1190), test_lot92_compute.c (900), test_lot92_propsheet.c (170 sheets), test_lot92_props.c (9), test_lot92_engine.c (a computed radius and angle drawn), `make qemu-lot92` (/lot92/props.html, /lot92/bg.html) |
| B | Radian angles of linear-gradient (US-031, part of Lot 91 gap 1) | the reader refused every radian angle; the engine's tf_angle reads rad (with 57.3 deg per rad) but draws only multiples of 90deg | wf_cssbg: a radian angle with at most two integer digits and four decimals, within half a degree of a multiple of pi/2 (\|x - k pi/2\| <= 0.0087266 rad, \|k\| <= 40), is taken and written back as whole degrees (1.5708rad -> 90deg, -3.1416rad -> -180deg), so the engine draws the same quarter turn with its exact deg path. Other radian values still refuse the gradient. A registered <angle> computed by row A (mod(450deg, 360deg) -> 90deg) is a written angle for the gradient. Proof: test_lot92_bgrad.c (670 generated cases), test_lot92_bgsheet.c (200 sheets), test_lot91_bgradial.c (converted), test_lot91_reader.c (converted), test_lot92_engine.c, `make qemu-lot92` (/lot92/bg.html) |

## Review points

1. Branch base. cursor/lot92 starts at 4def6ce (Lot 91 head; its tree is f870156 of PR 249, not merged). A PR against main (bf79580) also carries the Lot 90 commits, the qemu-gui-text-scale fix and the Lot 91 commits; those are not rewritten. The mbox holds only the Lot 92 commits.
2. Registered values are now computed values. A registered numeric value that is one math function is replaced by its value in the canonical unit when the value is known (calc(2px * 3 + 4px) -> 10px, round(up, 21px, 4px) -> 24px, calc(1in / 8) -> 12px), and descendants inherit that text. Lot 91 kept the declared text; no existing test asserted the text of a computed math value (test-all passes unchanged except the conversions in point 6). Plain values (12px, 1in) are not canonicalised (as before).
3. The css-props status line gains " simplified N" when at least one value was computed (the Lot 90/91 prefix is unchanged; with no computed value the line is byte for byte the Lot 91 one). The shell prints it in a 96-byte buffer (shell_fetch.c, Lot 90); a line with every counter in four digits would be cut, as before for the other counters.
4. wf_site_t grows from 416520 to 416524 bytes (int prop_simplified). wf_cssprop_t (164 bytes) and we_doc_t are unchanged; webeng.c is unchanged. When the 64-entry evar table is full, the computed text cannot be interned: the declared text stays bound and dropped_vars counts it.
5. Math typing: abs(), sign(), round(), mod(), rem(), sin(), cos(), tan(), asin(), acos(), atan() and atan2() are now math functions (Lot 91 refused them), so a registered <length> takes abs(-3px) and a <number> takes sin(30deg). Constants (pi, e, infinity, nan) and pow(), sqrt(), hypot(), log(), exp() stay invalid.
6. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot91_bggen.py and test_lot91_bgradial.c (regenerated, same seed and the same random sequence, 840 cases): the model takes 0rad, 1.5708rad, 3.1416rad and 6.2832rad as 0deg, 90deg, 180deg and 360deg (were refused); 1rad stays refused. 23 assertions changed, the other 817 are identical.
    - test_lot91_reader.c test_gradians: linear-gradient(1.5708rad, red, blue) gives linear-gradient(90deg, red, blue) (was refused).
7. Rounding model. Values are fixed point (1/10000), each step rounded; pi is 3.1415927 for rad. The generated suites check these exact rules with Python fractions; they are not the double precision of a browser, so the fourth decimal can differ from one (for example calc(1px / 3 * 3) is 0.9999px).
8. Mutations: 50 host mutations (rows A and B) and one QEMU mutation (no computation of registered values), all killed. M-rad-int-digits first survived: the two-digit bound also keeps the 32-bit arithmetic of rad_quarters from wrapping (429.4967rad); chunk 4 adds ten fixed edge cases to lot92_bggen.py and the mutant is killed. Two redundant checks found while writing the mutations were removed, since mutating them could not change any result (a length-percentage is never a known value; '*' needs no special case in wf_cssprop_compute, its alternatives list is empty and the one-type path does not take it).
9. CI: one new job, lot92-reader (make qemu-lot92, no guest clock option). The pull_request trigger keeps no branches filter.
10. ASan (gcc -fsanitize=address,undefined): the base and the head fail the same 5 suites (test_gpt2_infer, test_pmm, test_vfs_service, test_webrt, test_lot87_web); no new failure (see Gates).

## Not done (first gaps for lot 93)

1. Reader backgrounds: conic gradients, angles that are not a multiple of 90deg (the engine draws quarter turns only), background-clip: text, a clip or origin per layer, background-attachment; the engine does not fetch background images.
2. @property: at most four alternatives, no <url>, <image>, <transform-function> or <resolution> types, no transitions or animations of registered properties, 24 names and 64 values of 63 bytes (unchanged).
3. Math: sin(), cos(), tan(), asin(), acos(), atan() and atan2() are typed but not simplified; no constants (pi, e, infinity, nan), no pow(), sqrt(), hypot(), log(), exp(); relative lengths and length-percentage mixes are kept as text; list values ('+', '#') and plain dimensions (1in) are not computed; fixed point with 1/10000 steps, not doubles.
4. Autofill: three fixed summer time rules (no tz database, no historical changes); expiry before offers needs a new local day (unchanged).
5. PSL: 360 hand-written entries, not the full list (unchanged).
6. NE2000: the FIFO holds 16 frames and the drain is skipped during an LLM session (unchanged).
7. Shaping: single node settings, no WAN (US-068/US-073 partial, unchanged).
8. US-080: no DDC, the mode set is unchanged, the safe area margin is fixed at 2.5% (unchanged).
9. US-081 unchanged: no backlight, no watts, battery simulated.

## Tests

Host tests: tests/unit/userspace/test_lot92_mathtype.c (140 tests, 840 values), test_lot92_matheval.c (199 tests, 1190 values), test_lot92_compute.c (150 tests, 900 values), test_lot92_propsheet.c (170 tests, 170 sheets), test_lot92_props.c (9), test_lot92_bgrad.c (112 tests, 670 cases), test_lot92_bgsheet.c (40 tests, 200 sheets), test_lot92_engine.c (3). Generators: scripts/lot92_mathgen.py (seeds 9201..9204), lot92_bggen.py (9211, 9212); lot91_bggen.py (converted, same seeds).
QEMU: tests/scripts/test_qemu_lot92.py, `make qemu-lot92` (one boot at 1x): /lot92/bg.html, /lot92/props.html. CI job lot92-reader.

## Gates on the box

Run on 0445a67 (the code of this lot with its first docs commit) on 13 November 2026 (UTC+4), one QEMU guest at a time (scripts in /workspace/run/lot92, gates.sh), 10:27 to 12:22. No gate failed and none was rerun. The ASan runs (below) ran at the same time as the first QEMU gates, which made those guests slower than when run alone (qemu-lot92 took 59 s alone before the gate run).

| Gate | Result | Time |
|------|--------|------|
| make-all | pass | 1 s |
| test-all | pass | 228 s |
| roadmap-check | pass | 0 s |
| font-check | pass | 0 s |
| secrets | pass | 7 s |
| strict-image | pass | 0 s |
| qemu-lot92 | pass | 275 s |
| qemu-lot91 | pass | 454 s |
| qemu-lot90 | pass | 427 s |
| qemu-gui-text-scale | pass | 166 s |
| qemu-gui-parity | pass | 416 s |
| qemu-lot89 | pass | 276 s |
| qemu-p2p | pass | 584 s |
| qemu-p2p-mesh | pass | 107 s |
| qemu-p2p-phase5 | pass | 203 s |
| qemu-net-wire | pass | 83 s |
| qemu-lot67-web | pass | 277 s |
| qemu-lot88-desk | pass | 416 s |
| qemu-lot88-tv | pass | 222 s |
| qemu-lot87 | pass | 208 s |
| qemu-lot86 | pass | 174 s |
| qemu-lot85 | pass | 151 s |
| qemu-lot84 | pass | 146 s |
| qemu-lot83 | pass | 107 s |
| qemu-lot82 | pass | 97 s |
| qemu-lot81 | pass | 117 s |
| qemu-lot80 | pass | 97 s |
| qemu-lot79 | pass | 59 s |
| qemu-lot78 | pass | 39 s |
| qemu-lot77 | pass | 25 s |
| qemu-lot76 | pass | 20 s |
| qemu-lot75 | pass | 91 s |
| qemu-lot74 | pass | 96 s |
| qemu-lot73 | pass | 170 s |
| qemu-lot72 | pass | 267 s |
| qemu-lot71 | pass | 285 s |
| qemu-web-layout | pass | 537 s |
| qemu-production | pass | 72 s |
| tree-clean | pass | 0 s |

make test-all: 19879 tests, 19879 passed, 0 failed (Lot 91 head: 19056).

ASan and UBSan (TEST_CC="gcc -fsanitize=address,undefined -fno-sanitize-recover=all", make test-all on copies of each tree): base 4def6ce 18971 tests, 5 failed (test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt), 754 s; head 0445a67 19794 tests, 5 failed (the same five), 666 s. All Lot 92 suites and the converted Lot 91 suites pass under ASan; no new failure.

Mutations: 51 applied / 51 killed / 0 survived (50 host, 1 QEMU), see review point 8.
