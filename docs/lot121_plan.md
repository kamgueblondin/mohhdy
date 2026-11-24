# Lot 121: flex items (the flex shorthand, flex-flow, automatic minimum of column items, columns on the flex algorithm) and the style sheet's custom properties in the guest

Base: origin/main 8056dbe (PR 279, Lot 120). One engine: userspace/webeng.c
and its .inc files; the guest page pipeline (userspace/osui_gui.c) changes
one step. Kernel, rights and ABI unchanged. US-031 partial,
phase3_complete=false.

## Scope and choice

The two Lot 120 findings are taken together: "flex: 1 in a column flex
container of definite height does not grow" and the guest pipeline gap
(webrt_css_resolve_vars drops the style sheet's custom properties). The
probes on 8056dbe (640 x 480) showed the flex finding was larger:

- the flex shorthand was an unknown property (silently ignored, like
  flex-flow): flex: 1, flex: none, flex: auto did nothing anywhere;
- a column container took its old stacking path unless an item had a
  flex-basis or flex-shrink: grow read only a px height of the container
  (not a percentage or the html/body chain of Lot 120) and counted auto
  items as 0, so a flex-grow item took the whole height and the next
  ones overflowed;
- column items with a 0 basis (what flex: 1 means) collapsed to 0 in an
  auto-height column: no automatic minimum size (CSS Flexbox 4.5);
- min-height: auto and max-height: none were refused ("height limit
  refused").

Fixing only the shorthand would have collapsed every flex: 1 column item
of auto height; so the lot is "flex items": shorthand, flex-flow,
automatic minimum, columns on the flex algorithm. The guest pipeline fix
is one step and lets Lot 120's style queries read sheet custom
properties (the Lot 120 QEMU page now declares them in the sheet).

Not taken: calc-size, anchor positioning, typed attr(), margin collapsing.

## The flex shorthand (CSS Flexbox 7.1.1)

- none = 0 0 auto, auto = 1 1 auto, initial = 0 1 auto.
- <grow> [<shrink>] || <basis>: factors are integers 0..1000 (as
  flex-grow and flex-shrink read them; calc() integers too), next to each
  other; an omitted shrink is 1, an omitted grow 1 (flex: 10px is 1 1
  10px), an omitted basis 0 (flex: 1 is 1 1 0). A unitless 0 is a factor
  until two factors are read, then a basis; any other unitless number as
  a basis is refused. The basis is any flex-basis value (length,
  percentage, auto, content, math, intrinsic keywords): it goes through
  the flex-basis declaration.
- Refused (nothing set, "flex refused", the flex-basis diagnostic taken
  back): more than 3 parts, two bases, a factor between the basis and
  the other factor (1 10px 2), a third factor, a refused basis, none or
  auto with other parts, inherit and other CSS-wide keywords.
- The shorthand resets earlier longhands of the same element, as in CSS.

## flex-flow (CSS Flexbox 5.3)

<flex-direction> || <flex-wrap>, one of each at most, any order; an
omitted part is row or nowrap. Anything else: "flex-flow refused".

## Columns on the flex algorithm

- Every column container (flex-direction: column / column-reverse) takes
  the flex algorithm the row containers with sizing take (Lot 59 tracks
  path): the main size is the container's definite height (px,
  percentage of a definite parent, the html/body chain of Lot 120, min
  and max), the default flex-shrink is 1, an auto basis is the item's
  content height (it was 0 for an item with only flex-grow). The old
  stacking path is removed.
- Rows are unchanged (their legacy equal slots without explicit grow
  stay).

## Automatic minimum size of column items (CSS Flexbox 4.5)

- A column item with min-height auto (unset or "auto"), that is not a
  scroll container (overflow other than visible), has as minimum its
  content height, at most its definite height and its max-height; the
  outer size includes padding, borders and margins.
- The hypothetical size (line breaking, auto-height containers, the
  choice between growing and shrinking) is the base clamped to that
  minimum. Growing starts from the bases; a grower that would end below
  its minimum is frozen there and the line grows again without it (9.7
  step 4, min violations; max violations were already water-filled).
  Shrinking stops at the minimum ("flex minimum prevents fit" when the
  line still overflows).
- min-height: 0 (or any length, percentage or math) turns it off.
- Storage: the flex base size and the minimum are kept, until the cross
  pass of their line, in the item's cross[] and baseline[] slots of the
  per-depth track work (the layout workspace has 212 bytes left under
  WE_LAYOUT_WORKSPACE_MAX, no room for two more arrays per depth).

## min-height auto, max-height none

Accepted as the unset values (were "height limit refused"); max-height
auto and min-height none stay refused.

## The guest pipeline

userspace/osui_gui.c gives the engine the style sheet after the US-040
@media flattening, with its --x declarations and var() untouched: the
engine resolves var() per element (Lot 73 inheritance, fallbacks, Lot 118
@property) and Lot 120 style() queries read them. The US-041 pass
(webrt_css_resolve_vars) still runs, only to count the sheet's custom
properties for the status line ("vars N"). Before, it kept one global
value per name (the last one, case-insensitive, 32 names of 64
characters) and dropped the declarations.

## Review points

- userspace/osui_gui.c (guest pipeline, outside the engine): the sheet's
  custom properties go to the engine; status counters unchanged.
- Behavior changes on pages: flex, flex-flow, min-height auto and
  max-height none are read now; column containers always take the flex
  algorithm (default shrink 1, auto bases from content, automatic
  minimums). On the 43 initrd pages, compared through a host copy of the
  guest pipeline (Lot 120 engine and pipeline against Lot 121), no
  display item or box changes; lot120.html (converted, below) and
  lot121.html (new) aside.
- The track work's cross[] and baseline[] slots hold the base and the
  minimum before the cross pass (FLEX_BASE, FLEX_FLOOR).
- Converted test (no assertion changed): webeng_lot64_vertical.inc, a
  vertical root flex item had flex: none, which was never applied (the
  shorthand was unknown); it is dropped from the markup so the test keeps
  the path it was written for. With flex: none the item takes its
  max-content estimate (40 px, the Lot 64 cut estimate) instead of the
  laid-out 30 px: see "Found, not fixed".
- Converted QEMU page: initrd_content/web/lot120.html declares --theme and
  --size in the style sheet instead of a style attribute; the Lot 120
  contract is unchanged (docstring updated).
- test_osui_gui.c gains test_gui_web_sheet_custom_properties.
- we_doc_t and we_node_t sizes unchanged (no new field).

## Tests

- tests/unit/userspace/test_lot121_engine.c: shorthand values and
  refusals (diagnostic taken back, full diagnostic list), flex-flow,
  column growth (html/body chain, percent container, shares, max-height,
  gaps, padding, margins, auto bases), automatic minimums (auto-height
  columns, min-height 0 / percentage / math / auto, overflow hidden,
  frozen growers, padding and borders, shrinking, definite height and
  max-height caps, overflow diagnostic), rows unchanged, height keywords.
- tests/unit/userspace/test_osui_gui.c: the guest pipeline.
- QEMU: initrd_content/web/lot121.html, tests/scripts/test_qemu_lot121.py
  (`make qemu-lot121`, CI job lot121-engine-flex); `make qemu-lot120`
  with the converted page.

## Not done (gaps for later lots)

- The automatic minimum of row items (min-content width); aspect-ratio
  items' transferred minimum.
- Fractional flex factors (flex: 0.5, flex-grow: 1.5 are refused as
  before); flex: inherit / unset / revert.
- The hypothetical size of a grower already above its base because of an
  explicit min-height still counts in the free space as before (Lot 59
  model).
- osui_runtime's web-theme command (the older webrt renderer) still uses
  the US-041 resolver.
- Found, not fixed (outside this lot): the max-content estimate of a
  vertical root (Lot 64) is the text cut in inline sizes (40 px for "ab
  cd ef" at 16 px) while layout gives 30; it shows with flex: none.
- calc-size, anchor positioning, typed attr(), margin collapsing.
