# Lot 118 plan - CSS Values 4 math in the engine in thousandths (the functions, constants, typed arithmetic, scientific notation), viewport units, math in numbers, angles, whole-px lengths, transforms and colors, hwb() and the modern color forms, @property

Base 0f909e9 (main after PR 276, lot 117). US-031 stays partial,
phase3_complete stays false. One engine: userspace/webeng.c and its
.inc files (new: webeng_math.inc, webeng_prop.inc). Kernel and rights
unchanged; the engine API grows (review point below). No Chromium or
WebKit, no JS VM, no font engine, no margin collapsing. Freestanding
i386 Ring 3, no 64-bit division (checked with nm at -O0/-O2/-O3/-Os).

## Why this scope

Lot 117 left as first gaps the viewport units and the math the engine
did not read: numbers (order, z-index, flex factors, opacity), angles
(transforms), the whole-px lengths and the newer CSS Values 4 functions.
The engine's old math (lot 89..117) was a whole-px evaluator with a few
functions. This lot replaces it with one fixed-point core (thousandths in
int32, with infinities and NaN as sentinels) shared by every consumer,
and closes the neighbouring holes that the same core makes cheap: colors
with math channels, hwb(), and registered custom properties (@property)
whose computed values need the same length and number evaluation.

## Rows

Math core (webeng_math.inc, about 900 lines):
- values in thousandths in int32, finite range +-1e9, sentinels +inf,
  -inf and NaN (CSS Values 4 10.9); the used px is cut toward zero once,
  at use; arithmetic overflow refuses the declaration;
- calc, min, max, clamp, round (nearest, up, down, to-zero), mod, rem,
  abs, sign, pow, sqrt, hypot, log (1 or 2 arguments), exp, sin, cos,
  tan, asin, acos, atan, atan2; constants e, pi, infinity, -infinity,
  NaN (case-insensitive; only -infinity takes a sign);
- typed arithmetic: numbers, lengths, angles (deg, grad, rad, turn);
  a product takes one number, a quotient a number or the same type;
  a constant zero divisor is refused at parse, a percentage-dependent
  one gives the infinities or NaN at layout;
- trigonometry by CORDIC (Q29, micro-degrees), log2 by squaring (Q28),
  exp2 by a Q30 table; scripts/lot118_model.py is a port of the
  integer algorithms with a float check (all within about 1 milli);
- scientific notation in math literals (1e3, 2.5E-2, 1e+1: CSS Syntax 3
  4.3.12) for numbers, px and percentages, seven significant digits,
  exponents of at most two digits;
- bounds kept from lot 116: 32 nodes, 8 nested groups, 8 arguments,
  literals up to 2000.

Units: vw, vh, vi, vb, vmin, vmax and the s/l/d aliases (CSS Values 4
6.1.2) against the layout viewport (we_layout_viewport restyles when a
viewport unit was used and the size changed), else the media size; vi
and vb follow the root's writing mode.

Consumers:
- numbers: order, z-index, flex-grow, flex-shrink (rounded or clamped
  as CSS Values 4 10.9 says), opacity (clamped 0..1), line-height (a
  number expression is unitless, a length keeps hundredths), font-size;
- angles: rotate, skew; numbers: scale; translate and transform-origin
  take math lengths (no percentage); a comma inside a math argument no
  longer splits the transform arguments;
- every whole-px length read by parse_len takes math without a
  percentage (within +-2000 px);
- colors: rgb()/rgba()/hsl()/hsla() channels take math numbers, the hue
  an angle in any unit or a math angle, "none" (space form only) is 0;
  the space form mixes numbers and percentages in rgb() and takes numbers
  for the saturation and lightness of hsl(); hwb() (CSS Color 4 8);
- the API we_math_milli(text, len, ctx, basis, type, milli) tries a
  number, a length, an angle.

@property (webeng_prop.inc, CSS Properties and Values API 1):
- syntax '*' or up to 8 components joined by |: <length>, <number>,
  <integer>, <percentage>, <length-percentage>, <color>, <angle>,
  <time>, <custom-ident> or an identifier, each with an optional + or #
  multiplier (lists computed item by item, up to 160 characters);
- inherits required; initial-value required unless '*', computationally
  independent (font-relative and viewport units refused);
- the last rule of a name wins; 16 registrations; a value that does not
  match is invalid at computed-value time (inherited or initial);
  inherits: false starts every element at the initial value; var() of a
  registered property without a value gives the initial value;
- computed values: lengths in px at the declaring element (children
  inherit px), numbers, integers, angles in deg, times in s.

## Public API (review point)

- webeng.h: we_unit_ctx_t gains vw, vh, vertical; WE_MATH_NUMBER,
  WE_MATH_LENGTH, WE_MATH_ANGLE and we_math_milli(); we_prop_t and
  WE_PROPS_MAX; we_doc_t gains style_vw, style_vh, vp_used, vp_restyles,
  props[16], nprops. we_doc_t is 872336 bytes (was 872104, < 1 MiB),
  we_node_t 4676 (unchanged).

## Converted tests (review points; no assertion weakened)

- Regenerated with the new model: test_lot117_umath.c, test_lot117_ulh.c,
  test_lot117_units.c, test_lot117_box.c, test_lot117_clamp.c,
  test_lot117_fsize.c, test_lot117_props.c (the math leaf "3vw", now
  valid, became "3cqw", still refused; values cut once at use instead of
  rounded per leaf), test_lot109_flow.c (1 value 554 -> 553),
  test_lot111_mflow.c (833 -> 832), test_lot116_bflow.c (7 -> 6),
  test_lot116_elh.c (7 values: calc(2) is now a unitless number, 200).
- Hand edits: webeng_lot58_width.inc (calc(1.5px + 2) is refused as a
  sum of a length and a number; 20cqw for the still-invalid unit),
  webeng_lot59_limits.inc and webeng_lot59_insets.inc (1cqw, 10cqw),
  webeng_lot81_objpos.inc (10cqw), webeng_lot83_spctx.inc (x 96, w 54:
  the vw unit now resolves; comment in the file), test_lot116_engine.c
  (node[0].value 1000: thousandths), test_lot117_engine.c (b 3, c 15, one
  diagnostic).
- Models: scripts/lot117_model.py, lot116_model.py and lot109_model.py
  delegate the math to lot118_model.py (the old one kept as
  engine_px_lot109); scripts/lot117_gen.py uses the new parser.

## Not done (first gaps for lot 119)

- container queries and the cq* units (cqw stays an invalid unit);
- calc-size(), anchor(), typed attr(), relative colors (rgb(from ...)),
  color-mix() in the engine (the reader has them), the percentage math
  forms in color channels (calc(50%)), lab/lch/oklab/oklch in the engine;
- scientific notation outside math (a bare 1e3px) and with units other
  than px;
- @property: <resolution>, <image>, <url>, <transform-function> and
  <transform-list>; animation of registered properties; 'unset' and
  'initial' of custom properties stay the old behaviour (dropped: initial
  value), unset is not inherit;
- the radian keeps the engine's 57.3 degrees (tf_angle_milli) outside
  math; math in percentages of translate();
- html root margins against the viewport units.

## Tests

- tests/unit/userspace/test_lot118_engine.c (hand, 23 tests): the API
  table (333 rows, values of the model), node and depth bounds, zero
  bases, viewport units and restyles, logical units, whole-px lengths,
  number properties, opacity, line-height, font-size, transforms,
  @property rules, inheritance, syntax lists, colors, scientific
  notation, the single @property types and their bounds, var() of an
  initial value up to the 512-byte expansion buffer.
- tests/unit/userspace/test_lot118_math.c (GENERATED by
  scripts/lot118_gen.py from scripts/lot118_exprs.py and the model): 800
  random expressions (a fifth mangled) through we_math_milli.
- Differential check (not in the tree): 5 x 30000 random expressions and
  40000 scientific literals, C against the model, 0 mismatches; a UBSan
  build of the probe is clean.
- QEMU: tests/scripts/test_qemu_lot118.py, `make qemu-lot118` (one boot,
  offline: no server, no DNS): web-inspect of /web/lot118.html (eight
  boxes; the lot 117 values are refused), web-console. CI job
  lot118-engine-math.

## Gates on the box

See FINAL_REPORT.txt of the lot (make all, test-all with
test_lot67_web_host.sh, ASan with its six baseline failures, host and
QEMU mutations, the QEMU pages of lots 118 and 117).
