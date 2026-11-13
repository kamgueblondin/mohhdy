# Lot 93 plan - math functions simplified: trigonometric functions, pow, sqrt, hypot, log, exp and constants

Base: ebb64fc (Lot 92 head, the same tree as c5d1e1d of open PR 250,
branch cursor/lot92-math-computed-rad, not merged; main is bf79580, Lot
89). Branch cursor/lot93. One guest engine (userspace/webeng.c, unchanged
in this lot) and one shell path for the console and the desktop. Kernel,
rights and ABI unchanged. US-031 stays partial. phase3_complete stays
false. US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and
US-097 stay simule.

Scope taken from the Lot 92 first gaps, in order: gap 1 (reader
backgrounds: conic gradients and angles that are not quarter turns) needs
an engine drawing change and is left for lot 94; gap 2 (@property limits)
is left; gap 3 (math) is taken for its function part: sin(), cos(),
tan(), asin(), acos(), atan() and atan2() are now simplified, the
constants e, pi, infinity, -infinity and nan are read, and pow(), sqrt(),
hypot(), log() and exp() are typed and simplified. The rest of gap 3
(relative lengths, length-percentage mixes, list values, plain
dimensions, the 1/10000 step) and gaps 4 to 9 are left as they were (see
Not done).

A row is listed only when its code, tests and docs are in the branch.
Size: about 9900 added lines, of which 7555 are generated host test
suites (tests/unit/userspace/test_lot93_{trig,powlog,mtype,nested,
compute,propsheet}.c, written by scripts/lot93_mathgen.py from the
Decimal model scripts/lot93_mathmodel.py, which shares no code with the C
under test, fixed seeds 9301 to 9306) and about 2350 are written by hand
(about 455 lines of product code in wf_cssprop.c and wf_cssprop.h, the
model and generators, hand tests, the QEMU test and page, the CI job,
docs). The regenerated Lot 92 suites change 68 assertions (review point
6).

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | Trigonometric functions simplified (US-031, Lot 92 gap 3) | sin(), cos(), tan(), asin(), acos(), atan() and atan2() were typed and kept as text, so calc(sin(30deg) * 10px) did not reach font-size | wf_cssprop: an angle (1/10000 deg) is reduced exactly modulo 360deg to the nearest quarter turn, so sin(30deg) is 0.5, cos(90deg) is 0 and sin(390deg) is 0.5 with no rounding error; a number is radians, known when \|x\| <= 100 (sin(100.0001) is kept), reduced by pi/2 in 62-bit fixed point. sin and cos use a 14-term series on \|r\| <= pi/4; tan() at an odd multiple of 90deg is infinite and kept (tan(90deg), tan(-90deg), tan(100grad)). asin(x) = atan2(x, sqrt(1 - x^2)) and acos(x) = atan2(sqrt(1 - x^2), x) with an exact integer square root; past [-1, 1] they are NaN and kept. atan() uses an odd series with one reduction at tan(pi/8); atan2(A, B) takes two values of one known type (atan2(1px, 1in), atan2(10%, -30%); atan2(1px, 1em) kept) and gives the angle in all four quadrants, atan2(0, 0) = 0deg. Results are numbers (sin, cos, tan) or angles in deg (asin, acos, atan, atan2), rounded half away from zero to 1/10000. Proof: test_lot93_trig.c (1000 generated values), test_lot93_nested.c (800 nested trees), test_lot93_math.c (hand edge cases), test_lot93_engine.c, `make qemu-lot93` (/lot93/math.html) |
| B | pow(), sqrt(), hypot(), log(), exp() (US-031, Lot 92 gap 3) | these names made a value invalid | pow(A, B), sqrt(A), log(A), log(A, B) and exp(A) take numbers and give a number; hypot(A, ...) takes values of one type and gives that type (hypot(3px, 4%) is a length-percentage and is kept). sqrt() and hypot() are exact (integer square root of the 1/10^8 sum, rounded half away). pow() with an integer exponent from 1 to 127 is exact while A^n fits 127 bits (decimals past the fourth truncated, then rounded once), from -1 to -8 it is an exact 128-bit division; other exponents use exp(B ln \|A\|) with the sign from the parity. pow(0, 0) = 1, pow(0, -1) and pow(-2, 0.5) are kept (infinity, NaN). log(A) is ln, log(A, B) is ln A / ln B; A or B <= 0 and B = 1 are kept. exp(A) is not computed past A = 12 and is 0 below -12; as every result, it is kept past 100000 units (exp(11.5) is 98715.771, exp(12) is kept). Any result past 100000 units is kept. Proof: test_lot93_powlog.c (1100 generated values), test_lot93_mtype.c (800 typed values, 280 of them broken), test_lot93_nested.c, test_lot93_math.c, test_lot93_engine.c, `make qemu-lot93` |
| C | Constants (US-031, Lot 92 gap 3) | e, pi, infinity, -infinity and nan made a value invalid | e and pi are numbers known as 2.7183 and 3.1416 (1/10000, like every value); infinity, -infinity and nan are numbers that are never known, so a value that uses them is typed and kept. Names are matched lower case as whole identifiers; -pi, 2pi and pie are invalid (calc(-1 * pi) is the way to write it). Proof: test_lot93_math.c, test_lot93_mtype.c, test_lot92_props.c (converted), `make qemu-lot93` (round(calc(pi * 5px), 1px) -> 16px) |
| D | Computed registered values with the new functions (US-031) | a registered value with a trigonometric function kept its text | the Lot 92 computation (one math function, one numeric alternative) takes rows A to C: a registered <angle> atan2(1, 0) is 90deg and draws a 90deg gradient, acos(-1) is 180deg, hypot(30px, 40px) is a 50px radial radius, pow(), pi, sqrt(), log() and exp() font sizes are computed (16px, 16px, 12px from an inherited initial value, 21px), tan(90deg) is kept. css-props counts them in " simplified N". Proof: test_lot93_compute.c (520 values), test_lot93_propsheet.c (120 sheets), test_lot93_engine.c (atan2 and acos angles drawn, hypot, sqrt and pow radii), `make qemu-lot93` |

## Review points

1. Branch base. cursor/lot93 starts at ebb64fc (Lot 92 head; its tree is c5d1e1d of PR 250, not merged). A PR against main (bf79580) also carries the Lot 90, Lot 91 and Lot 92 commits while PR 250 is open; once it is merged GitHub shows only the Lot 93 commits. The mbox holds only the Lot 93 commits.
2. Precision contract. Results that are not exact are computed in 62-bit fixed point (1/2^62) with 128-bit two's-complement words and shift-subtract divisions (freestanding i386: no libm, no __divdi3; nm shows no libgcc helper in wf_cssprop.o). The header states "within about 1e-12 (relative) of the exact value before the final rounding". The model (Decimal, 60 digits; pi by Machin, its own series) drops a case that is within 1e-11 of a rounding boundary (relative to max(1, \|y\|)); no generated case was dropped. A fuzz of 100000 random cases and 18382 random trees against the model found no difference (9 cases near a boundary skipped). A browser computes in doubles; the fourth decimal can differ from it near a boundary.
3. pi and e are 3.1416 and 2.7183 as values (1/10000 like every other value), so calc(pi * 10000) is 31416, cos(pi) is -1 and sin(pi) is 0 after rounding (the radian argument is the rounded constant 3.1416, whose sine is -0.0000073).
4. Trigonometric functions are numbers now. A registered <number> takes sin(30deg) as 0.5 (Lot 92 kept it); a <length> calc(sin(30deg) * 10px) is 5px; font-size takes it (qemu-lot92 converted, point 6).
5. Removed while writing the mutations, since no result could change: the low words of pi/2 and ln 2 (CP_PIO2_LO, CP_LN2_LO; the rounded 62-bit constants already meet point 2), the limit checks in cp_sqrtw and cp_exp (the final cp_fits check of cp_margs refuses the same values), and the floor mode of cp_wdivu for negative values (only called with values >= 0; it is now truncation). Guards kept although mutating them cannot change a result, because they avoid a division by zero or document a bound: tan() at a pole (den == 0), atan2(0, 0), the isqrt comparison, half-away ties of 1/2^62 quotients, the n <= 127 loop guard, the k range of cp_ln, the quadrant rounding offsets (see mutations).
6. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot92_mathgen.py and its four suites (regenerated, same seeds and the same random sequence, same counts 840/1190/900/170): the trigonometric values use the Lot 93 model; pow(), log() and hypot() of the right arity and the constants (except -pi) are valid. Assertions changed: test_lot92_mathtype.c 23, test_lot92_matheval.c 29, test_lot92_compute.c 9, test_lot92_propsheet.c 7; the others are identical.
    - test_lot92_props.c: ev("sin(30deg)") gives 0.5 (was kept); pow(2, 3) and calc(pi) are numbers (were invalid); calc(-pi) is invalid (new); cv('<number>', sin(30deg)) gives 0.5 (was kept); tan(90deg) is kept (new).
    - tests/scripts/test_qemu_lot92.py: #trig92 has "size 5px" (was no size) and css-props ends with " simplified 3" (was 2).
7. webeng.c, the kernel, the ABI, wf_site_t, wf_cssprop_t and we_doc_t are unchanged. The engine still draws quarter turns only; a computed angle that is not a multiple of 90deg (atan2(1, 1) = 45deg) is not drawn, as for a written one (/lot93/math.html #init93).
8. Mutations: 64 host mutations (rows A to D) and one QEMU mutation (atan2() ignores a negative x: #acos93 drew 0deg), all killed. The first host run (69 mutations) left 14 survivors; chunk 3 adds witnesses (a short sin/cos series, exact ties of integer exponents, a power whose low word is 0, pow() of two lengths, double rounding of integer powers), removes the redundant code of point 5 and adds a mutation on the final limit check. Four mutations are equivalent and are not counted (mutations.txt).
9. CI: one new job, lot93-reader (make qemu-lot93, no guest clock option). The pull_request trigger keeps no branches filter.
10. ASan (gcc -fsanitize=address,undefined): see Gates.

## Not done (first gaps for lot 94)

1. Reader backgrounds: conic gradients, angles that are not a multiple of 90deg (the engine draws quarter turns only; webeng.c is unchanged), background-clip: text, a clip or origin per layer, background-attachment; the engine does not fetch background images.
2. @property: at most four alternatives, no <url>, <image>, <transform-function> or <resolution> types, no transitions or animations of registered properties, 24 names and 64 values of 63 bytes (unchanged).
3. Math: relative lengths and length-percentage mixes are kept as text; list values ('+', '#') and plain dimensions (1in) are not computed; fixed point with 1/10000 steps, not doubles; radians past 100 are kept; infinity and nan are typed but never known (no infinite or NaN result is carried through calc()).
4. Autofill: three fixed summer time rules (no tz database, no historical changes); expiry before offers needs a new local day (unchanged).
5. PSL: 360 hand-written entries, not the full list (unchanged).
6. NE2000: the FIFO holds 16 frames and the drain is skipped during an LLM session (unchanged).
7. Shaping: single node settings, no WAN (US-068/US-073 partial, unchanged).
8. US-080: no DDC, the mode set is unchanged, the safe area margin is fixed at 2.5% (unchanged).
9. US-081 unchanged: no backlight, no watts, battery simulated.

## Tests

Host tests: tests/unit/userspace/test_lot93_trig.c (125 tests, 1000 values), test_lot93_powlog.c (138 tests, 1100 values), test_lot93_mtype.c (100 tests, 800 values), test_lot93_nested.c (100 tests, 800 trees), test_lot93_compute.c (65 tests, 520 values), test_lot93_propsheet.c (120 tests, 120 sheets), test_lot93_math.c (14 tests, hand edge cases with expectations from the model), test_lot93_engine.c (2). Generators: scripts/lot93_mathgen.py (seeds 9301..9306) with the model scripts/lot93_mathmodel.py; scripts/lot92_mathgen.py (converted, same seeds).
QEMU: tests/scripts/test_qemu_lot93.py, `make qemu-lot93` (one boot at 1x): /lot93/math.html. CI job lot93-reader.

## Gates on the box

Run on the commit that adds this plan; the results are added by the next commit.
