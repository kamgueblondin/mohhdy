# Lot 62: alpha, transforms, paint effects and paint order (US-031 partial)

Base: origin/main b632e3d (PR #218, lot 61, merged; same tree as 960e54b).
Branch: feat/lot62-webeng-layout, stacked after #218. No merge or force-push.

One console/GUI engine in userspace/webeng.c, C freestanding i386 Ring 3.
Kernel, rights and the shared ABI (os_fb_web_item_t) are unchanged: every
new effect is resolved by the engine into opaque RECT, BORDER and TEXT
items. US-031 partial; phase3_complete=false. No Chromium, WebKit, network
loading, JavaScript VM, new font engine or margin collapsing.
US-059/060/063/074 unchanged; US-G19..US-G24 partial. Roadmap statuses and
the complete production non-done list stay intact
(prod-roadmap next US-076 US-078 US-080 US-081 US-097 US-G19 US-G20 US-G21
US-G22 US-G23 US-G24). we_doc_t is 710340 bytes (lot 61: 684100), under
1 MiB.

## Colours and compositing

- rgb()/rgba()/hsl()/hsla() (comma and space forms, / alpha, percentages),
  #rgb, #rgba, #rrggbb, #rrggbbaa and transparent. The transparency byte
  sits in bits 24..31 of the item colour (0 opaque) and is cleared before
  the list leaves the engine. hsl is 32-bit integer only (no __divdi3).
- composite_items() after the clip: each translucent item is split along
  the edges of what is painted under it and every piece takes source-over
  with straight alpha against that backdrop (white canvas at the root). A
  TEXT under a translucent RECT is repainted on top, cut to the piece.
- opacity groups (0 < opacity < 1) are composited deepest first, then the
  group alpha is applied. Over the fragment budget the colours fall back to
  opaque with "alpha display budget"; alpha 0 items are dropped.
- Invalid functional colours: "color refused".

## Borders

- groove, ridge, inset and outset are shaded (dark 2/3 of each channel,
  light c + (255 - c) / 3), with pinwheel corners, for separate frames and
  collapsed lines (inset reads as ridge and outset as groove when collapsed).
  The lot 61 "border style painted solid" diagnostic is gone.
- border-radius (1..4 radii, "/" vertical radii, longhands with 1 or 2
  values; px up to 128 or percent up to 100): corners are scaled down by
  the smallest side / radius sum, then backgrounds and solid borders are
  painted as row slices (3 rows per corner, ellipse inset at the middle
  row, rounded half up), "border-radius painted in steps". Round 2: double
  borders paint two rounded rings of (t + 1) / 3, groove/ridge two nested
  shaded rings, inset/outset shaded rings (light/dark halves split at the
  middle row); dashed and dotted with a radius stay square ("rounded effect
  painted square"). A gradient with a radius paints square ("gradient
  radius painted square").
- Round 2: overflow hidden/clip with a radius clips descendants to the
  rounded padding box (intersected bands, at most 29 per clip): rects are
  cut per band, glyphs fully inside one band are kept whole and other
  glyphs dropped. Over budget the square clip stays.

## Transforms

- transform: translate/translateX/translateY (px, em, %), scale/scaleX/
  scaleY, rotate in multiples of 90deg or 0.25turn, lists of up to 4.
  transform-origin keywords, px and percentages (default 50% 50%).
- Paint only: layout, hit testing and fixed positioning are unchanged. The
  monomial matrix keeps a..d in 1/1000 and e,f in 1/16 px; mapped edges
  round half up. A transformed box forms a stacking context and contains
  absolute descendants. Over the clip budget the item is dropped with
  "transform display budget". Other angles: "transform refused".

## Outline, shadows, text

- outline (style, width, colour, offset): painted after the z >= 0
  children of its context; inline boxes get none.
- box-shadow: 1 or 2 entries, offsets, blur (two layers, a / 2 then
  a * 255 / (510 - a)), spread, inset over the background and under the
  content, the first shadow on top. Round 2: with a radius, outer shadows
  follow the spread radii and inset shadows paint the padding shape then
  repaint the background over the inner shape; over the slot budget they
  stay square with "rounded effect painted square".
- text-decoration (shorthand and longhands): underline, overline,
  line-through; solid, double, dotted, dashed, wavy (painted solid, one
  diagnostic); colour and thickness 1..4 px. Lines propagate from the
  nearest declaring ancestor and stop after floats, absolute boxes and
  inline-blocks. UA: u/ins underline, s/strike/del line-through (links
  keep no UA underline, so older pages keep their item counts).
- text-transform (uppercase, lowercase, capitalize, none; inherited),
  text-shadow (2 entries, colour defaults to the text colour, blur painted
  sharp with one diagnostic), text-overflow ellipsis with overflow hidden
  (three ASCII dots; the run crossing the content edge, or the last fitting
  run before hidden ones).
- The property name buffer grows from 24 to 40 characters: 25-character
  longhands (text-decoration-thickness) were truncated and lost.

## Backgrounds and spacing

- linear-gradient: to side or a multiple of 90deg, 2..4 stops with
  optional percent positions (clamped up, missing ones spread), premultiplied
  interpolation, at most 8 bands over the origin box, cut to the clip box,
  "gradient painted in bands". background-image: none clears it.
- background-clip and background-origin (border-box, padding-box,
  content-box). The border area outside the origin box is not filled by
  repetition.
- Percentage margins and paddings (shorthands, longhands, logical pairs,
  tenths, negative margins) resolve against the containing block width at
  layout time, through inline boxes and table rows; they count 0 in
  intrinsic sizes (cyclic percentage rule) and again on each relayout.

- Round 2: letter-spacing (-4..16 px) and word-spacing (-8..32 px),
  inherited, "normal" resets; a letter-spaced run paints one item per
  glyph, advances count in wrapping and intrinsic sizes. text-align:
  justify spreads the free width over the spaces of soft-wrapped lines
  (lines with atoms stay start aligned); text-align-last (auto, start,
  end, left, right, center, justify) for the last line and lines before a
  forced break; text-justify none/auto/inter-word; start/end keywords are
  left/right (left-to-right only).
- Round 2: radial-gradient (circle or ellipse, the four extent keywords,
  "at center" only, opaque stops): the last stop fills the clip box, then
  3 nested ellipses (8 row slices each) in the colour at the ring middle.
- Round 2: grid item percentage margins/paddings resolve against the grid
  area width, absolute boxes against the padding box of the containing
  block.

## Paint order and lists

- CSS 2.1 appendix E inside the in-flow level of each context: block
  backgrounds and borders, then floats (whole), then inline content
  (inline-blocks whole), then positioned boxes with z-index auto. Without a
  stacking context the order changes only where two overlapping items
  disagree (stable topological order), so pages without conflicts keep
  their emission order.
- ol start, reversed, li value, ol/ul type; list-style-type circle,
  lower/upper-alpha (latin), lower/upper-roman, decimal-leading-zero; the
  list-style shorthand (type and outside). Alpha below 1 and roman above
  3999 fall back to decimal.

## Adapted fixtures (assertions kept or strengthened)

- lot61_stack: opacity tests now expect the composited colours (0x80007F,
  0xFF7F7F, 0x660099) and no diagnostic.
- lot61_bstyle: the 3D test checks shaded pixels and no diagnostic.
- lot61_collapse: ridge over inset asserts the shaded pixels.
- lot61_stack without z: a relative box (z auto) now paints over the later
  static block (appendix E step 8); a second render keeps the two static
  blocks in tree order.
- Round 2: lot62_page expects page_h 192 and 67 items (the page grew);
  no other fixture changed.
- lot58_logical padding percent: the refused value becomes 10%px (truly
  invalid); percentages are now accepted and tested in lot62_spacing.

## Remaining gaps (first one first)

1. Margin collapsing (excluded by the brief).
2. Within the brief: per-line collapsed band widths (collapsed tables keep
   one uniform band width for every grid line, shared by the lot 61 track
   geometry).
3. Dashed/dotted rounded borders, rounded clips under a transform,
   radial positions other than center and translucent radial stops.
4. Background images (none decoded by the guest engine), so repeat,
   position and size are not implemented.
5. Transforms other than quarter turns (skew, arbitrary rotate, matrix)
   and transformed hit testing.
6. Column subgrid baselines, vertical writing modes, bidi (start/end are
   left/right).

## Results

See /workspace/run/lot62 (gate logs, mutations.txt, PROGRESS.md) and the
PR body for the measured counts.
