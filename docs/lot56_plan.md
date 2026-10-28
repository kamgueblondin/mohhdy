# Lot 56: absolute, flex, table and text layout (US-031, partial)

Base: origin/main c7c4d9a (merged PR #212, lot 55).
Branch: cursor/lot56-webeng-d639. No merge and no force-push.

One guest increment in userspace/webeng.c. The same console and GUI shell
calls it. Kernel, rights and the syscall ABI are unchanged. US-031 stays
partial. phase3_complete stays false. No Chromium, WebKit, JavaScript VM,
network loader or new font. US-059, US-060, US-063 and US-074 are unchanged.
US-G19..US-G24 stay partial. Roadmap statuses do not move, so the production
next line stays the lot 55 string.

The useful addition is real layout code and geometry tests. It is smaller
than the lot 55 band of about 4300 useful lines. No copied function and no
comment padding was added to chase that band.

## Implemented behavior and explicit limits

- position:absolute is removed from flow, flex, grid and table collection.
  The containing block is the nearest ancestor with position relative or
  absolute, otherwise the initial containing block. left/top/right/bottom
  are pixel insets. Both left and right, with width auto, stretch the margin
  box to the padding edge. A specified width keeps left and ignores the extra
  right inset. Right-only and bottom-only use the margin-box size. Percentage
  width and height use the padding-edge size. No insets use the padding
  origin. That is not the CSS static position. Absolute wins over float.
  Absolutes do not increase the parent height. They are placed once, at the
  end of the containing block and before its relative offset, so a relative
  offset moves them. A vertical relative offset still applies only when the
  parent height is definite. position:fixed is refused and the previous
  position is kept. position:static clears absolute and relative.
- z-index -8..8 orders absolute boxes inside that slice. Ties keep source
  order. auto clears it. A value outside the range is diagnosed and the
  previous value stays. Negative z-index still paints after the in-flow
  content of the same block. z-index on a static or relative box is stored
  and does not reorder in-flow paint.
- flex-direction row-reverse and column-reverse mirror the main start after
  auto margins are resolved, so physical left stays left. order is an integer
  -8..8 with a stable source-order tie. An empty order declaration is skipped
  and produces no diagnostic. Out of range and non-integers are diagnosed and
  keep the previous order.
- flex-wrap:wrap-reverse uses the existing wrap, then mirrors the cross
  start. A definite cross size is required for the mirror to move a single
  line; an automatic cross size only swaps multiple lines. align-content is
  packed first and then mirrored. nowrap clears it. An invalid token is
  diagnosed and the previous wrap is kept. Column wrap still requires a
  definite height.
- margin:auto on the flex main axis only. Cross-axis auto is ignored. The
  free space is shared once per line. The remainder goes to the earlier auto
  in source order. Autos consume positive free space, so justify-content does
  nothing for that line. A later length clears that auto bit. The legacy
  equal-split path remains when the container sets none of the modern flags
  (justify, align, gaps, reverse, wrap-reverse, auto margins).
- table rowspan 1..8 occupies later rows. A span past the last row is clamped
  and diagnosed. A span outside 1..8 refuses the table. A rowspan taller than
  the rows it covers grows the last spanned row. Single-row cells are not
  stretched to the row unless vertical-align is middle or bottom.
- vertical-align top, middle and bottom applies to table cells. Top is the
  default and keeps the content-sized box. Middle and bottom stretch the cell
  box to the row, or to the rowspan, and shift in-flow descendants. The cell
  background stays put. baseline is refused. This is not inline vertical-align.
- border-spacing accepts one value or two. One value sets both axes. Two
  values are horizontal then vertical, each 0..64, with an optional px suffix.
  border-collapse forces both axes to 0. It does not merge adjoining borders.
  A bad second value keeps both previous axes.
- white-space:pre keeps spaces and newlines and does not wrap. pre-wrap keeps
  them and wraps on spaces. pre-line preserves newlines, collapses spaces,
  and wraps. normal still collapses newlines to a space. break-spaces and
  pre-line's old refusal token are diagnosed. A tab in pre is one space.
  There is no shaping and no new font.
- line-height is 10..40 px, or a unitless 1..4 meaning that multiple of 10.
  0 and normal use the 10px line. The value is inherited. Glyphs stay at the
  top of the line box. The glyph hit box stays 10px tall. There is no baseline.
- text-indent is 0..400 px on the first line only, after float shortening.
  It is inherited. Percent and negative values are refused. A block child is
  not shifted by its parent's indent. max-content and min-content include the
  indent when the element has content. A br consumes the first line, so the
  following line is not indented.
- clear:left, right, both and none moves a block or a float below floats on
  those sides. none restores the previous beside-float placement. An invalid
  token is diagnosed and the previous clear is kept.
- display:inline-block is an atomic box on the line. Width auto uses
  max-content, capped by the remaining line. It wraps to the next line when
  it does not fit beside pending content. Horizontal margin is outside the
  border box. The box top matches the line top. text-align is not applied on
  a line that contains an inline-block. There is no baseline alignment.
- Still absent: JavaScript, network loading, font loading, scrollbars, dense
  grid, named areas, baselines, cross-axis auto margins, CSS static position,
  position:fixed, white-space:break-spaces, border-collapse that merges
  borders, margin collapsing, and full replaced-element intrinsic sizing.

## Fixed storage

WE_MAX_NODES=160, WE_DEPTH_MAX=24, WE_ORDER_MAX=8, WE_SPACING_MAX=64,
WE_Z_MAX=8, WE_LINE_H_MAX=40, WE_INDENT_MAX=400. The caller owns we_doc_t.
No heap is added. The one imgdec workspace is unchanged. The layout scratch
assertion still caps static scratch at 512 KiB.

## Tests

Host geometry lives in webeng_lot56.inc and webeng_lot56b.inc, on the
existing test_webeng binary. Former refusals of rowspan, absolute and the
pre token were replaced by stricter geometry. The invalid white-space token
is now break-spaces, so the refusal diagnostic remains. /web/lot56.html
combines reverse+order, an absolute inset, spaced table cells, pre, then
wrap-reverse, clear and pre-line at line-height 20. The QEMU contract keeps
every previous page and adds those boxes.

## Measured local results

Host webeng cases, run from /workspace on 28 October 2026:

```text
Tests Run: 589
Tests Passed: 589
Tests Failed: 0
Tests Ignored: 0
```

That is 455 cases on lot 55 plus 134 lot 56 cases. The full make test-all
count, the QEMU web-layout log and the other gates are recorded in this
file after those commands finish. They are not invented here.

git diff origin/main --numstat, with the three untracked files counted as
additions: 2493 lines added and 150 lines removed. Of the additions, 790 are
engine/header (726 code, 42 comment, 22 blank) and 1678 are tests, the guest
page and the test build rule (1537 code, 5 comment, 136 blank). Docs are 25
lines and are not in the useful count. Useful code is 2263 lines. That is
below the 4000..6000 band. The shortfall is stated here instead of filled
with copies.
