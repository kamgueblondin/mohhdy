# Lot 131: display: contents

Base: origin/main 0b15d23 (PR 289, Lot 130). One engine: userspace/webeng.c,
with two additions to userspace/webeng.h (a flag in we_node_t, the DOM link
table in we_doc_t). Kernel, rights and ABI unchanged. we_node_t is still 4692
bytes (the flag fits in padding); we_doc_t grows from 875332 to 876296 bytes,
under the 1 MiB bound. US-031 partial, phase3_complete=false.

## Scope and choice

The engine refused `display: contents` ("display refused"). It is common in
component libraries (a wrapper element that must not be a flex item, a grid
cell or a styled box) and the Lot 120 notes name it for the root only as not done. The
two other open points of Lot 130 were checked first and set aside: nothing
outside the tests calls we_render, so @media is evaluated once per page in
every real path (the guest flattens @media itself before the engine sees the
sheet), and the calc() followed by a keyword quirk probed fine in Lot 130.

Probed on 0b15d23 (640 x 480, 8 px glyphs, 10 px lines):

1. `<div id=f style="display:flex;width:300px"><div style="display:contents;
   background:red;padding:20px"><div id=a w50 h10>..<div id=b w60 h10>..
   </div><div id=c w30 h10></div></div>`: the wrapper stayed one flex item
   with its padding and background (a at 20 20, b at 20 30, f 60 high, one
   "display refused" diagnostic); expected a 0 0, b 50 0, f 10 high.
2. A grid with 100px 100px columns: the wrapper took one cell with its two
   children stacked inside (b at 0 10, c in the second column at 100 0);
   expected a 0 0, b 100 0, c 0 10.
3. `x y <span style="display:contents;padding:9px;border:1px solid red">ab
   <i>cd</i></span> ef` in a 200 px paragraph: the span kept its edges, "ab"
   at x 42, "cd" at 66, "ef" at 100 (expected 32, 56, 80).
4. `height: 50%` under a contents wrapper resolved against the wrapper's
   own height (5 of 10 instead of 20 of 40 for the grandparent); an
   absolute child (right 0, bottom 0 in a relative 200 x 60 box with padding 5) sat in the relative wrapper, at 185 2 instead of 190 60.

## How it works

- style_node (display): `contents` sets display none and the new flag
  `we_node_t.contents`. Replaced elements and controls (img, input, select,
  textarea, iframe, video, audio, canvas, object) and br compute to display
  none with the flag clear: they have no children to hand over (CSS Display
  3, 2.5). The root element keeps refusing it, with the diagnostic
  "contents refused on the root" (a document node above it would have to
  become the root). Any later accepted display clears the flag; a refused
  value keeps the earlier one. The reset block of style_node clears the
  flag, so a second pass starts clean.
- The element's box properties compute as usual (children inherit), but no
  box exists: its margin, padding, border, background, size, position,
  float, transform, opacity, overflow, outline, shadow, z-index and flex or
  grid item properties apply to nothing.
- display none is propagated to the descendants of a none element by the
  end of we_style, and by style_node to the children. Both now skip a
  contents parent: the children are hidden by the nearest ancestor that has
  a box (a contents element inside display none stays hidden).
- splice_contents, at the end of we_style: the children of a contents
  element hang from the nearest ancestor with a box, in place and in order,
  nested wrappers included. The element itself stays in the child list
  as a display none leaf just before its children, because html_space (the
  white space that preceded the element) is read from it by the inline flow:
  `a <span style=display:contents>b</span>` keeps its space. Layout,
  geometry, scrolling, percentages, containing blocks, flex and grid
  items, inline runs and table sections all work on the box tree with no
  second code path.
- Generated content and counters run through a contents element:
  gen_allowed and gen_walk let it through, so its ::before and ::after are
  boxes of its parent and its counter operations apply.
- The DOM links (parent, first child, next sibling of every node) are kept in
  we_doc_t.dom_link while spliced is 1 (last is not kept: drop_generated
  sets it again, and the splice leaves it right for the box tree). we_style restores them first
  (unsplice_contents): selectors, inheritance and drop_generated read the
  DOM. A page without a contents element is never spliced (the table is
  filled, nothing else changes).
- list_ordinal reads the DOM owner (dom_parent, dom_first, dom_next): an
  `ol start=3 reversed` with display: contents numbers its items as before.
- stack_items no longer counts a contents element as a stacking context
  (transform, opacity, z-index on it painted the items in another order).
- we_inspect and we_dump_dom print `display contents` / ` contents` for the
  flag.

## Tests

- Host: tests/unit/userspace/test_lot131_engine.c (18 tests). Most compare
  the page with a wrapper (carrying margin, padding, border, background,
  size, position, float, flex and grid properties, transform, opacity,
  outline, shadow) against the same page without it (or with a plain span in
  inline flow): the box of each id and the painted display list must be the
  same, and explicit values are checked: flex row, flex column wrap, grid
  with placement, inline flow with white space around the wrapper, percentage
  heights, relative wrapper and absolute child, nested wrappers, display none
  ancestors, replaced elements, the root (and an html element met inside the
  body), the cascade (later display, id rule, style attribute, !important,
  refused value), the DOM links before and after a second we_style, the box
  tree invariant (parents, child lists, last) on six pages, generated boxes
  dropped by the second pass, ::before / ::after and counters, list
  numbering (also an li inside a wrapper and at the top level), paint order
  with an absolute z-index 0, a full node table (160 nodes: the splice reads
  and writes nothing past the table, a second pass leaves the document
  unchanged), table sections, stacking, we_inspect and we_dump_dom, a
  container query that stops and starts making the wrapper contents, a
  viewport-unit re-style. 16 of 18 fail on 0b15d23 (table sections and the
  absolute z-index 0 paint order are regression guards: both pass on the
  base).
- Differential fuzz (outside the repository, tools in the run directory):
  random box trees of flex, column-wrap flex, grid and block containers with
  wrappers (display: contents plus 26 kinds of ignored properties) nested up
  to 3 deep, in block, flex, grid and inline flow, against the same tree with
  the wrappers removed (or plain spans): box of every id, every painted item,
  page height: 40000 cases (5 seeds of 8000), 0 mismatches on this tree; on
  0b15d23 more than half of the cases differ (176 and 169 of 300 for two
  seeds).
- QEMU: /web/lot131.html, `make qemu-lot131`
  (tests/scripts/test_qemu_lot131.py), CI job lot131-engine-display-contents:
  a flex row, a grid, a percentage height and an absolute child under
  contents wrappers; each wrapper is `display contents`, box 0 0 0x0, with
  its style computed; none of the Lot 130 values; web-console reports no
  diagnostic.
- No existing page changes (every initrd web page renders byte-identical
  through the host pipeline, base vs this lot).

## Not covered (honest)

- `display: contents` on the root element is refused (diagnostic). A tr
  with it leaves its cells directly under the table: "table child refused"
  (no anonymous row); tbody, thead and tfoot work.
- Properties inherited through a contents element are inherited as computed
  (correct), but a writing-mode or direction set on the contents element
  itself is not seen by the box parent's wmode checks.
- A scroller, sticky or fixed position set on a contents element does not
  apply; overflow on it is ignored (as in CSS), geometry queries that walk
  the DOM parent chain of a spliced child see the box tree.
- we_dump_dom shows the box tree (children of a contents element appear
  under its parent).
- @media is evaluated once per parse (nothing outside the tests re-styles
  after we_set_media); revert-layer; the earlier lots' list (vertical writing
  modes for left / right and self-*, legacy keywords, grid overflow default,
  wrap-reverse column baselines, aspect-ratio transferred minimum).
