# Lot 120: the root box and the initial containing block, container style queries, angles on the hue only

Base: origin/main bf47b2a (PR 278, Lot 119). One engine: userspace/webeng.c
and its .inc files. Rights and ABI unchanged; one kernel constant grows
(fs/initrd.c INITRD_FILE_MAX 144 -> 152, review point below).

## Scope and choice

The theme proposed for the lot is kept: the root box and the initial
containing block are engine code that every page with an html element
goes through, and the probes at the start of the lot showed four real
gaps there (below). Container style queries are coherent with it: they
reuse the Lot 119 @container machinery (prelude, container selection,
nested rules) and the Lot 73 / Lot 118 custom properties, and need no
layout pass. The Lot 119 report's bug (rgb(... 10deg ...) read as 10) is
fixed where it was: the shared number reader of the color channels.

What the probes found on bf47b2a (640 x 480 viewport):

- html { height: 100% } was an indefinite percentage ("percent height
  indefinite"), so body { height: 100% } and everything under it was
  auto. html { margin, padding, border } and body in html were already
  laid out as boxes (no change needed there, kept by tests).
- position: absolute without a positioned ancestor used the document
  box as its containing block: bottom: 0 went to the end of a tall page,
  or to the top of a short one, and height: 100% was the document height.
- The root's background color painted the html box only, and body's
  background stayed on the body box: the canvas had none.
- rgb(10deg 20 30) was rgb(10 20 30): the color number reader dropped a
  "deg" suffix on every channel. The same reader served the matrix()
  translation numbers, so matrix(1,0,0,1,10deg,0) was accepted too.

Not taken: calc-size, anchor positioning, typed attr() and margin
collapsing (excluded by the brief).

## The root element and the initial containing block

- The root element is an html element child of the document node
  (we_doc_t.root_html, 0 for a fragment: a page without an html tag keeps
  the document as its block, as the implicit auto-height body of a
  browser would). A text node before it is skipped.
- Its containing block is the initial containing block: the viewport at
  the canvas origin (CSS 2 10.1). layout_once sets the root's
  containing_height to the viewport height, so height, min-height and
  max-height percentages of html resolve against it (content box, or
  border box with box-sizing: border-box), and body { height: 100% }
  inside is definite. html without a height keeps body's percentage
  indefinite (no quirks mode).
- An absolutely positioned box without a positioned ancestor is placed
  in the initial containing block: its height is the viewport height (the
  document box is already at 0 0 and viewport wide), whatever the page
  height. A positioned ancestor (html included) is still the block;
  auto insets keep the static position; fixed boxes are unchanged.

## The canvas background (CSS Backgrounds 3, 2.11.2)

- The element whose background paints the canvas (we_doc_t.canvas_from)
  is the root when it has a background, else, when the root has no
  background at all (no color, gradient or image), its body child. Only a
  background color goes to the canvas: canvas_from is 0 when that element
  has none, or when the root or that body is display: none.
- The canvas is the first display item, owned by the document node: a
  rectangle at 0 0, the viewport wide, as tall as the page or the
  viewport when the page is shorter, in that color (alpha kept). The
  element whose color went to the canvas does not paint it on its own box
  (its gradients, images, borders and shadows stay on its box).
- web-inspect prints " canvas #rrggbb" on that element.

## Container style queries (CSS Conditional 5, 7.2)

- style(--x: v) and style(--x) in an @container condition, alone, in
  parentheses, with not / and / or inside style() or around it, with a
  container name, and mixed with size features.
- Every element is a style container: a condition made of style features
  only selects the nearest ancestor (for a pseudo-element its originating
  element first), or the nearest with the name, and reads no size (no
  layout pass, no restyle). With size features the same container must
  also have the type the features need (Lot 119).
- The value is the container's computed custom property (inherited ones
  included), compared with v as text once trimmed, each run of white space
  as one space, case-sensitive. var() in v is substituted with the
  container's properties (a missing one without fallback: false).
  style(--x) is true when the property has a value that is not its
  initial one; v = initial is true when the property is unset, or equal
  to its registered initial value (an unset registered property has its
  initial value).
- Refused (the rule is dropped, "container query refused"): a feature that
  is not a custom property name (style(color: red), style(t: x),
  style(-tt: x), style(--)), an empty value, style() nested in style(),
  size features inside style(), broken syntax.

## Angles on the hue only

- The color number reader takes no unit: "deg" (and every other angle
  unit) is read only on the hue channel of hsl() and hwb() (and their
  relative forms), by the angle reader. rgb(10deg 20 30), hsl(120
  100deg 50%) and rgb(from red 10deg g b) are refused; hsl(120deg ...)
  and hwb(120deg ...) are unchanged.
- matrix() translations and scale() numbers are unitless: a deg there
  refuses the transform (scale(1deg) was already refused, its explicit
  check is now the reader's).

## Public API changes (review points)

- we_doc_t: root_html, canvas_from (int16_t), canvas_bg (uint32_t).
  sizeof(we_doc_t) 874692 bytes (was 874684, < 1 MiB); we_node_t 4688
  (unchanged).
- The display list of a page whose root or body has a background color
  starts with the canvas item (one item more).
- web-inspect: " canvas #rrggbb" at the end of the line of that element.

- fs/initrd.c: INITRD_FILE_MAX 144 -> 152. lot120.html is the 137th
  initrd entry and tests/scripts/check_initrd_capacity.py keeps 8 spare
  (137 + 8 > 144); Lot 86 grew the table the same way. Two static arrays
  of 8 more entries, no other change.
- Guest pipeline gap: userspace/osui_gui.c runs webrt_css_resolve_vars
  (Lot 7) on the style sheet before the engine; it substitutes var() and
  drops every --x declaration of the sheet, so in the guest a style query
  only sees custom properties set in style attributes. The host engine
  and its tests read the sheet's own. Not changed in this lot.

## Converted tests (review points)

- tests/unit/userspace/test_lot118_engine.c: rgb(0deg 0 0) was accepted
  as black; it is now refused (the 0x010203 sentinel). No assertion is
  removed.

## Tests

- tests/unit/userspace/test_lot120_engine.c: root heights (100%, 25% of
  another viewport, min/max, padding, border, box-sizing, body in an auto
  html, fragments, hidden root), absolute boxes (short and tall pages,
  stretch, percentages, html margins, positioned ancestors, static
  position), the canvas (html, body, tall page, transparent html, head
  first, gradient and image roots, hidden boxes, fragments, alpha,
  web-inspect), style queries (values, spaces, not/and/or, var(),
  registered properties, inheritance, names, pseudo-elements, mixed
  conditions, no restyle, refusals) and the hue-only angles.
- QEMU: initrd_content/web/lot120.html, tests/scripts/test_qemu_lot120.py
  (`make qemu-lot120`, CI job lot120-engine-root).

## Not done (gaps for later lots)

- The root's or body's background images and gradients stay on their
  box (only the color goes to the canvas); background-clip/origin and
  positioning against the root's box for the canvas.
- Overflow propagation from html/body to the viewport, a vertical root
  writing mode for the initial containing block, display: contents on
  the root, quirks mode body height.
- In the guest, the page pipeline (userspace/osui_gui.c through
  webrt_css_resolve_vars, Lot 7) still resolves and drops the custom
  properties of the style sheet before the engine: there, a style query
  reads the custom properties set in style attributes (the QEMU page does
  that). Handing the sheet's custom properties to the engine is left to a
  later lot (it changes every page with variables).
- Style queries on standard properties, registered property values
  compared after computing the query value (style(--len: 1em) against
  16px), scroll-state() queries.
- Found, not fixed (outside the root box): flex: 1 in a column flex
  container of definite height does not grow (a body of flex: 1 in an
  html of height: 100% stays 0 high).
- calc-size, anchor positioning, typed attr(), margin collapsing.
