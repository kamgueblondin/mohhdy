# Lot 63: generated content, text values, tabs, radial positions, rotate/skew, rounded dashes and clips (US-031 partial)

Base: origin/main f808d46 (merge of PR #219, lot 62; contains ab540be).
Branch: feat/lot63-webeng-layout. No push, no merge, no force-push.

One console/GUI engine in userspace/webeng.c, C freestanding i386 Ring 3.
Kernel, rights and the shared ABI (os_fb_web_item_t) are unchanged; every
effect still resolves to opaque RECT, BORDER and TEXT items. US-031
partial; phase3_complete=false. No Chromium, WebKit, network loading,
JavaScript VM, new font engine or margin collapsing. US-059/060/063/074
unchanged; US-G19..US-G24 partial. Roadmap statuses and the complete
production non-done list stay intact (prod-roadmap next US-076 US-078
US-080 US-081 US-097 US-G19 US-G20 US-G21 US-G22 US-G23 US-G24).
we_doc_t stays under 1 MiB (see the PR body for the measured size).

## A. ::before / ::after, content, counters, quotes

- Selectors: ::before, ::after, :before and :after, only as the last part
  of a compound selector; any other "::name" refuses the selector. Rules
  keep their pseudo-element and the cascade matches each pseudo-element
  only against its own rules.
- content: none, normal, or a list of strings (CSS escapes), attr(id |
  class | href | src | alt), counter(name[, style]), counters(name,
  "sep"[, style]), open-quote, close-quote, no-open-quote,
  no-close-quote; any other token refuses the declaration. At most
  WE_CONTENT_TEXT_MAX (96) characters are generated per box.
- quotes: auto (double quotes, then single quotes when nested), none, or
  1-2 pairs of strings of up to 3 characters each; levels past the list
  reuse the last pair, a close-quote at depth 0 paints nothing. The UA
  gives q open-quote/close-quote.
- counter-reset, counter-increment, counter-set: up to 4 name/value pairs
  each, values saturate at +-999999. Order per element: reset, increment,
  set (CSS Lists 3). A reset on a sibling replaces that sibling's
  instance, otherwise it nests; an increment without a reset instantiates
  the counter on the element. counter(list-item) uses the li ordinal.
  display:none elements do not count, visibility:hidden ones do.
- Generated boxes are real nodes (tag "::before"/"::after") styled in a
  scratch node first, inserted as first/last child; replaced elements
  (img, input, br, textarea, select, iframe, video, audio, canvas, object)
  get none. A second we_style() drops the generated nodes and rebuilds
  them from the source nodes, so pool and node counts are stable.
- list markers use the same counter text (disc '*', circle 'o', square
  '#', decimal, alpha, roman).

## B. Text values

- Character references: numeric (decimal, hex, up to 7 digits) and the
  named set amp lt gt quot apos nbsp shy mdash ndash hellip lsquo rsquo
  ldquo rdquo laquo raquo copy reg trade times middot bull ensp emsp
  thinsp euro, mapped to ASCII for the 8x8 font; soft hyphen to nothing;
  other code points to '?'.
- overflow takes one or two values, overflow-x/overflow-y set one axis;
  hidden, clip, auto and scroll all clip that axis (no scrolling UI).
- white-space-collapse, text-wrap-mode and text-wrap (balance, pretty and
  stable wrap as wrap) map onto the white-space modes.
- word-wrap aliases overflow-wrap; word-break accepts keep-all, and
  break-word resets break-all and wraps anywhere.

## C. radial-gradient positions and sizes

- circle/ellipse, an extent keyword, or explicit sizes (circle: one
  length; ellipse: two lengths or percentages), and "at <position>" with
  1 or 2 keywords, lengths or percentages (keywords may swap). A circle
  refuses two sizes or a percentage, an ellipse refuses one size; one
  length alone means a circle.
- Extents are measured from the centre: closest/farthest side per axis,
  corners as sqrt(sx^2 + sy^2) for circles and side * sqrt(2) for
  ellipses. A centre outside the box keeps its distances.

## D. Transforms: any whole-degree rotate, skew, composition

- Angles in deg, grad, rad or turn (or a bare 0), rounded to whole
  degrees; rotate keeps 0..359, skew/skewX/skewY keep -80..80 (an angle
  is taken modulo 180deg; 90deg has no tangent and is refused).
  linear-gradient angles share the parser and stay quarter turns.
- The matrix uses a 91-entry sine table (1/1000). Nested transforms with
  no overflow clip between them compose into one matrix before painting,
  so a box rotated by +45deg inside -45deg paints upright and exact.
- A rectangle under a matrix that is not axis-aligned becomes a
  parallelogram painted as at most 8 row bands of equal height, each
  spanning the shape at its middle row; corners round to half pixels and
  clamp to +-8000 px. Quarter turns and scales keep the exact lot 62 path.
- Text under such a matrix stays upright at its mapped centre ("transform
  text upright"). Over the fragment budget: "transform display budget".

## E. Rounded dashed and dotted borders

- The rounded ring is painted, then kept only inside the corner boxes
  (rx x ry), so arcs are solid; the straight parts between the arcs take
  the lot 61 dash/dot pattern, each side restarting it. A square corner
  keeps the horizontal side's t x t square. "rounded effect painted
  square" no longer applies to dashed and dotted borders.

## F. Rounded clip under a transform

- Under a transform, each overflow clip with a radius applies its bands in
  its box's own space, after the rectangle clip and before that box's
  transform maps the pieces (round_clip_node shared with the untransformed
  path).

## G. Tabs, tab-size and break-spaces

- Text keeps U+0009 (also from &#9;). Under pre, pre-wrap and
  break-spaces a tab advances to the next multiple of tab-size measured
  from the line start (painted as at most 32 space advances); a tab that
  does not fit on a wrapping line starts the next line. Collapsing modes
  read a tab as a space.
- tab-size: an integer 0..31 (space advances of 8 px) or a px/em length of
  at most 255 px, inherited, default 8; anything else gives "tab size
  refused". 0 removes the tabs.
- white-space: break-spaces and white-space-collapse: break-spaces keep
  spaces and wrap them like letters: a run fills the line and continues
  on the next one instead of hanging; text-wrap-mode:nowrap turns it into
  pre. The intrinsic widths treat each break-spaces space as a break
  opportunity.

## Round 2

### H. Collapsed band widths per grid line

- A collapsed table no longer uses one band width B for every grid line.
  Each column line and each row line takes the widest resolved edge along
  it (hidden edges count 0); a line where no edge is drawn has a 0 band.
  Cells are placed with those bands, paint their half-edges inside the
  band of their own line (edge_offset), and the cell insets and the
  intrinsic widths read the same per-line values.
- border-spacing is ignored for a collapsed table (as before); the bands
  live in the table slot as int8 arrays so the layout workspace stays
  under WE_LAYOUT_WORKSPACE_MAX.

### I. Soft hyphens and hyphens

- &shy; and &#173; become a soft hyphen (no glyph, not a space). With
  hyphens:manual (default) or auto (read as manual: no dictionary) a word
  may break at a soft hyphen; the line then ends with a '-' glued to the
  piece before the break. A piece that fits with its hyphen stays on the
  line.
- hyphens:none, nowrap and word-break:break-all do not break at soft
  hyphens; they stay invisible. Inherited; anything else gives "hyphens
  refused". The intrinsic minimum width counts a soft hyphen break with
  its hyphen.

### J. transform: matrix()

- matrix(a, b, c, d, e, f) with exactly six unitless numbers; a..d in
  -8..8 (1/1000 precision), e and f in px within +-4000, rounded half
  away from zero. It takes three of the four transform slots and composes
  with the other functions in list order (same row-band painting as
  rotate/skew). A list that would need more than four slots is refused.
- Translucent linear stops already blend since lot 62. Translucent radial
  stops are not done: the disjoint annulus pieces over a shared cut set
  would need about 70-119 items for one box, beyond the 96 item list.

### L. Background images

- background-image: url(path) (quotes optional) on any non-<img> box with
  a block layout. The path follows the <img src> rules (absolute, no
  "..", no scheme) and is read through the existing image reader and
  decoder (initrd only, no network); failures give the image diagnostics.
- background-repeat: repeat, no-repeat, repeat-x, repeat-y or one
  repeat/no-repeat per axis (space and round refused).
  background-position: the transform-origin grammar (keywords, px,
  percent of the free space). background-size: auto, contain, cover, or
  one or two px/percent values (auto keeps the ratio).
- Tiles are positioned in background-origin and cut to background-clip
  (lot 62 values), sampled at the tile size and merged into rectangles
  like an <img>; the runs are inserted after the box colour and gradient,
  before its inset shadow and children. When the whole set does not fit
  in the remaining display list it is refused whole ("background image
  budget"). A later background shorthand or gradient replaces the url.
- The guest page sanitizer (webrt_sanitize_page, US-039) used to drop
  every declaration with url(). It now keeps a url() whose argument is a
  local absolute path (no scheme, no "//", no "..", only [A-Za-z0-9._/-],
  optional matching quotes): such a url names an initrd file and fetches
  nothing. Any other url() still drops its declaration (test_webrt
  test_sanitize_page; 12 mutants of the exception killed).

### Not done in round 2

- K vertical writing modes: the line builder, the intrinsic sizes and the
  block layout all assume a horizontal inline axis; a bounded subset
  would touch every layout path, not done this round.
- Hit testing of transformed boxes: the engine has no hit test.

## QEMU

- /web/lot63.html combines A-F; tests/scripts/test_qemu_web_layout.py
  checks its console items (web-render) and its pixels in the GUI pane;
  the same page is asserted on the host (webeng_lot63_page.inc).

## Adapted fixtures (assertions kept or strengthened)

- lot57_selectors bad_selector_67: div::before is now valid, the refused
  selector becomes div::marker.
- lot55_paint clip_invalid_overflow: overflow:scroll now clips, the
  refused value becomes "scrolling".
- lot59_composition word_break_invalid: keep-all is now valid, the
  refused value becomes keep-everything.
- lot62_radial refusals: "circle at left" and "circle at center center"
  are now valid; the refusals become "circle at middle" and "circle at
  center center center".
- lot62_transform refusals: rotate(45deg) and skewX(10deg) are now valid;
  the refusals become rotate(45dag) and skewX(90deg) (same count).
- lot62_radius and lot62_round: dashed and dotted rounded borders now
  leave the outer corner pixel unpainted and give no square diagnostic;
  the rounded clip under translateX(1px) is now rounded (two bands
  asserted instead of the square).
- lot55_text, lot56b and lot63_text: break-spaces is now valid; the
  refused values become break-everything, break-all (white-space) and
  discard (white-space-collapse).
- lot56 tabs: a pre tab now advances to the next 64 px stop ("a       b",
  width 72) instead of being one space.
- Round 2, lot61_collapse and lot61_table: each grid line now has its own
  band (widest edge on that line, 0 where no edge is drawn), so the
  expected boxes move to the spec geometry: wider_edge (right band 2 px,
  blue at 38-39), row_group_boundary, rowspan, edge_lists,
  one_edge_shorthand, inset_odd_line and spans_and_intrinsic (table 37x37).
  The same properties are asserted, with the new coordinates.
- Round 2, lot62_gradient refusals: background-image:url(x.png) now
  parses; the relative path is refused at paint ("image local path
  refused", count 1) and nothing is painted, as before.

## Remaining gaps (first one first)

1. Margin collapsing (excluded by the brief).
2. Within the brief: vertical writing modes (vertical-lr/vertical-rl).
3. Translucent radial stops (display list budget), dots and dashes along
   the corner arcs (arcs are solid).
4. 3D transforms, rotated text glyphs, a hit test (none exists).
5. hyphens:auto with a dictionary, bidi; background images on inline
   boxes, several layers, space/round repeats.
6. Column subgrid baselines, floats inside the negative z layer.

## Results

See /workspace/run/lot63 (gate logs, mutations.txt, PROGRESS.md) and the
PR body for the measured counts.
