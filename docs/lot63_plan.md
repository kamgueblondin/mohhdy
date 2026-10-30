# Lot 63: generated content, text values, radial positions, rotate/skew, rounded dashes and clips (US-031 partial)

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

## Remaining gaps (first one first)

1. Margin collapsing (excluded by the brief).
2. Within the brief: per-row collapsed band widths (collapsed tables keep
   one band width B for every grid line).
3. Dots and dashes along the corner arcs (arcs are solid), translucent
   radial stops, background images (repeat/position/size).
4. matrix()/3D transforms, rotated text glyphs, transformed hit testing.
5. tab-size and break-spaces (tabs become spaces at parse time), soft
   hyphen breaks and hyphens, vertical writing modes, bidi.
6. Column subgrid baselines, floats inside the negative z layer.

## Results

See /workspace/run/lot63 (gate logs, mutations.txt, PROGRESS.md) and the
PR body for the measured counts.
