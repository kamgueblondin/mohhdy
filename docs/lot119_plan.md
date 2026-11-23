# Lot 119 plan - container queries (@container, container-type, container-name, container), the container units cqw/cqh/cqi/cqb/cqmin/cqmax, size and inline-size containment, relative colors rgb/hsl/hwb(from ...)

Base de44f7d (main after PR 277, lot 118). US-031 stays partial,
phase3_complete stays false. One engine: userspace/webeng.c and its
.inc files (new: webeng_cq.inc). Kernel and rights unchanged; the engine
API grows (review point below). No Chromium or WebKit, no JS VM, no
font engine, no calc-size, anchor, typed attr() or margin collapsing.
Freestanding i386 Ring 3, no 64-bit division (checked with nm at
-O0/-O1/-O2/-O3/-Os).

## Why this scope

Lot 118 left as first gaps container queries with the cq* units, the
relative colors and the root html margins. Container queries and their
units are one feature (the units need the same query containers, the
same sizes and the same restyle), and the relative colors reuse the lot
118 math core (the channel keywords are numbers in that core). The root
html margins are a separate layout question (the viewport, the initial
containing block and the root's margins) with nothing in common with the
two others: they are left for a later lot rather than mixed in.

## Rows

@container (webeng_cq.inc, CSS Conditional 5):
- prelude: an optional name (an identifier other than none, and, or,
  not, default and the CSS-wide keywords) and a condition with the
  @media syntax without media types: not, and, or, parentheses; a name
  alone is valid;
- size features: width, height, inline-size, block-size (lengths in px,
  em of the container's font, rem, or 0; three decimals, at most
  100000), aspect-ratio (a ratio), orientation; the name: value,
  min-/max- and range forms (a op v, v op a, v1 op a op v2 in one
  direction); the boolean form is true unless the size is 0;
- a refused prelude drops the block with "container query refused";
  16 conditions per document ("container query table full" beyond);
- nested @container blocks all apply; @media and @supports nest inside
  and around; the rules keep their place in the cascade.

Query container: the nearest ancestor (never the element itself, never
the document node; for a pseudo-element its originating element too)
with the queried name (case-sensitive) and a container-type covering
the axes the features read: inline-size for width and inline-size,
size for height, block-size, aspect-ratio and
orientation. Without one, or before a layout gave its size, the
condition is unknown and its rules do not apply. A name alone queries
any ancestor with that name.

Sizes and restyle: the container's content box (borders and padding
out) after a layout. we_layout_viewport styles and lays the document out
again while a size that a style read is new or changed, at most
WE_CQ_PASSES (3) more times; beyond, "container query unsettled" and
the last layout stays. Each pass's style and layout diagnostics replace
the previous ones (no duplicates).

Properties: container-type (normal, size, inline-size), container-name
(none or a list of names), container (name [/ type]; a missing type is
normal). A refused value keeps the earlier one ("container type
refused", "container name refused", "container refused"). Not inherited.
@supports reads them.

Containment (CSS Contain 2, 3.1 and 3.2): container-type: size lays an
auto height out as if the box were empty; size and inline-size make the
intrinsic inline size (shrink-to-fit, floats, flex and grid
contributions) that of an empty box.

Units (CSS Conditional 5, 7): cqw and cqi are the inline size of the
nearest ancestor container (any type), cqh and cqb the block size of the
nearest size container, cqmin and cqmax the smaller and larger; without
one (or before its size is known) the viewport's. Everywhere the engine
reads a length unit (lengths, math, font-size, line-height).

Relative colors (CSS Color 5, 4):
- rgb(from c r g b [/ a]), hsl(from c h s l [/ a]), hwb(from c h w b
  [/ a]) and the rgba/hsla aliases, in the space syntax only;
- the origin c is any color the engine reads (named, hex, rgb, hsl,
  hwb, transparent, another relative color);
- the keywords are numbers: r, g, b in 0..255, h in degrees (0..360),
  s, l, w, b in 0..100, alpha in 0..1, usable alone or in math (calc(h
  + 120)); a number plus an angle is refused (h is a number);
- the origin is converted to hsl and hwb in integers (thousandths,
  truncated); an omitted alpha is the origin's;
- each channel also takes a number, a percentage, none or math as in
  the non-relative forms.

## Public API (review point)

- webeng.h: we_unit_ctx_t gains cqw and cqh (0: none, the viewport);
  we_node_t gains cq_type, cq_name_len, cq_known, cq_name_off, cq_w,
  cq_h; we_rule_t gains cq; WE_CQ_MAX, WE_CQ_PASSES, the WE_CQ_* types
  and axes, we_cq_t; we_doc_t gains cqs[16], ncq, cq_used, cq_restyles,
  style_diag, style_dropped. we_doc_t is 874684 bytes (was 872336,
  < 1 MiB), we_node_t 4688 (was 4676), we_rule_t 1088.
- we_layout_viewport may run we_style again (container queries only).

## Converted tests (review points; no assertion weakened)

The unit cqw, used since lot 118 as a still-invalid unit, is read now.
The cases keep their meaning with rex (CSS Values 5, not an engine
unit), and their assertions are unchanged:
- scripts/lot117_gen.py: the math leaf "3cqw" became "3rex";
  test_lot117_umath.c regenerated (4 lines, only the leaf changes; the
  generator reproduces the old file byte for byte before the change);
- webeng_lot58_width.inc (20cqw), webeng_lot59_limits.inc (1cqw,
  10cqw), webeng_lot59_insets.inc (1cqw), webeng_lot81_objpos.inc
  (10cqw), test_lot118_engine.c (calc(1cqw) through we_math_milli).

## Not done (first gaps for lot 120)

- style() and scroll-state() queries, container-type: scroll-state,
  the container's own pseudo-elements as query containers;
- vertical writing modes for the container axes (horizontal only);
- an inline (non-block) container: its content box is read from its
  laid-out box as for any element, no special inline handling;
- relative colors: the legacy comma syntax (refused, as in CSS),
  currentcolor and system colors as origins, color(), lab, lch, oklab,
  oklch and color-mix() in the engine, powerless hues kept as 0;
- root html margins against the viewport units; calc-size, anchor,
  typed attr(), margin collapsing (out of scope by decision).

## Tests

- tests/unit/userspace/test_lot119_engine.c (hand, 10 tests): the
  feature forms and range edges, em and rem, the content box, not/and/
  or, nesting with @media, the query container (nearest, named, name
  lists, name alone, types, hidden, not inherited), the size features
  and orientation and aspect-ratio, size and inline-size containment,
  the units with and without containers (0 px containers, the API),
  restyle passes and the bound, refused preludes and declarations and
  the table bound, @supports; relative colors: conversions to hsl and
  hwb against the explicit forms (values of a small model of the
  integer conversion, run once, written into the table), the channels
  as numbers, rgb forms, alpha, nesting, refusals, keyword scope.
- QEMU: tests/scripts/test_qemu_lot119.py, `make qemu-lot119` (one
  boot, offline: no server, no DNS): web-inspect of /web/lot119.html
  (seven boxes; the lot 118 values are refused), web-console. CI job
  lot119-engine-cq.

## Gates on the box

See FINAL_REPORT.txt of the lot (make all, test-all with
test_lot67_web_host.sh, ASan with its six baseline failures, host and
QEMU mutations, the QEMU pages of lots 119 and 118).
