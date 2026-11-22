# Lot 117 plan - font-relative lengths in the engine (em of the element's own font, rem, ex, ch, cap, ic, lh, rlh in every engine length, in its math, in font-size and line-height), font sizes in hundredths of a px, line-height math, the line-clamp's carried sum

Base: 6aeb9b5 (origin/main after PR 274, Lot 116, and PR 275, the CI apt
fix). Branch cursor/lot117. No lot past 116 on main: the number 117 is
free. One guest engine (userspace/webeng.c and its .inc files; this lot
changes webeng.c and webeng.h, and one condition of the reader,
wf_sitecss.c). Kernel, rights and ABI unchanged. US-031 stays partial.
phase3_complete stays false. No Chromium or WebKit, no JavaScript VM, no
new font engine, no margin collapsing.

## Why this scope

The engine read every length through parse_len, which took "1.5em" as
15 px whatever the font (em = 10 px, one decimal), cut px decimals and
took any other suffix as px ("2rem" was 2 px). Every page written in em or
rem (most real pages: margins, paddings, widths, line heights) was laid
out with the wrong sizes by the engine as soon as the reader passed the
value raw, and the engine's math refused em outright. The Lot 116 "Not
done" list named the next line-height gaps: a number with a fractional
font size passed as px (the engine kept whole px fonts), line-clamp
counting whole px lines, and line-height math in the engine. They all
come from the same missing piece: the engine had no font context when it
read a length. This lot adds that context and uses it everywhere.

## Rows

A. The font pass (style_node, font_pass_run): font-size, then
   line-height, are cascaded before the element's other declarations (each
   with both importance passes, like the Lot 64 writing-mode pass), quietly
   (diagnostics and counters restored); the main pass applies them again
   to the same values. So `margin: 2em; font-size: 20px` is 40 px (CSS
   Values 4 6.1.1: em is the computed font-size of the element).
B. em of the element's own font in every engine length (parse_len and the
   parsers built on it: margins, paddings, borders, widths, min/max sizes,
   text-indent, letter/word-spacing, tab-size, outline, columns,
   text-underline-offset, vertical-align, scroll-margin/padding,
   transforms and radii), heights and tracks (parse_dim_max), position
   offsets (inset_declaration), object-position (position_length),
   margin-inline/padding-block (logical pairs), flex-basis and
   border-spacing. Decimals read to three places; the length is
   hundredths of a px rounded half away from zero, within +-2000 px. The
   legacy px reading is unchanged (px decimals cut, a number with another
   suffix read as px).
C. Font sizes in hundredths: we_node_t gains font100 (font_px stays the
   rounded value for the glyph scale). font-size reads px and pt with
   decimals, em and % of the parent's hundredths, the keywords,
   smaller/larger (1.2), the UA heading sizes, small and big from the
   parent's hundredths; within 4..64 px; under half a px refused (as 0 was).
   A number line height is the number times font100 (was times the
   rounded px). The reader passes a number line height as the number for a
   font size in hundredths within 4..64 px (Lot 116: whole px only;
   thousandths, as smaller gives, still go as px).
D. rem (the root element's font), ex (5 rows), ch (8 columns), cap (7
   rows) of the element's 8x8 glyph scale, ic (1em: the font has no
   ideograph), lh (its used line height), rlh (the root's), in every
   length above and in font-size (ex, ch, cap, ic, lh of the parent; CSS
   Values 4 6.1.1). In the root's own font-size and line-height, rem and
   rlh are the initial 16 px and 10 px (and stay so in the main pass:
   fs_root100, fs_rlh100).
E. Math (calc, min, max, clamp) leaves with those units, decimals allowed
   with them only (px rounded at the element); px and number leaves stay
   whole as before. In width, heights, offsets, gaps, spacings and font-size
   (percentages of the parent font, whole px).
F. line-height reads calc/min/max/clamp lengths (percentages of the
   element's font) and the units other than em (lh is the parent's line
   height there), with hundredths; em and % stay em multiples resolved at
   the end of the cascade with font100.
G. line-clamp: with a fractional line height the clamped height is the
   carried sum of the N line steps (two lines of 20.6 px: 41, was 40).
   we_inspect prints font100 and lh100 (the QEMU contract reads them).

## Public API (review point)

webeng.h: we_unit_ctx_t, we_unit_len100() and we_font_size100() (host
tests and future callers); we_parse_font_size() is unchanged (rem stays 16
there). we_node_t gains `uint16_t font100` (we_node_t 4672 -> 4676 bytes,
we_doc_t 871464 -> 872104 bytes, < 1 MiB).

## Converted tests (review points; no assertion weakened)

1. webeng_lot58_width.inc test_l58_width_invalid_9: calc(20em + 2px) is now
   a length; the refused value is calc(20vw + 2px) (vw: Not done).
2. webeng_lot59_limits.inc test_l59_limits_atomic_width_4: fit-content(1em)
   is now 16 px; the refused value is fit-content(1vw).
3. webeng_lot63_text.inc test_l63_text_tab_size_values: tab-size 1em is 16
   px (was 10, em = 10 px).
4. webeng_lot81_objpos.inc test_l81_obj_bad_8: object-position 10em is now
   a length; the refused value is 10vw.
5. test_lot116_engine.c test_l116_lh_carry_bounds: line-clamp 2 of 20.6 px
   is 41 (was 40, the Lot 116 Not done), the next box at 42.
6. test_lot116_reader.c test_l116_lh_pass: font-size 10.5px with
   line-height 2 passes "line-height:2" (was 21px); two cases added
   (smaller: 13.333 px, still px; 1.1em of 10.5px: 11.55, the number).
7. webeng_lot59_insets.inc test_l59_insets_atomic_11: inset 1em is now a
   length; the refused value is 1vw.

## Not done (first gaps for lot 118)

1. Viewport units (vw, vh, vmin, vmax, dvh...) in the engine: the viewport
   is known only at layout time, after the cascade (we_layout_viewport);
   such values are still read as px by the lenient legacy path (or refused
   in math and the suffix-checked properties).
2. The column steps (column_units) keep whole px line bottoms with a
   fractional line height (no observable case found: the fill reads the
   line tops); the fraction carry still does not cross blocks. With a
   fractional line height, line-clamp counts a 1 px gap before the next
   line (a 1 px margin between blocks) as the carry (two px stay out).
3. Math: leaves are rounded to px at the element (calc(0.4em + 0.4em) is
   two rounded leaves); numbers and px stay whole; line-height math gives
   lengths only (a number-valued calc() is refused); font-size math is
   whole px.
4. One 8x8 font: ex, ch, cap are its rows and columns at the scale q; ic
   is 1em. @property lengths with these units (wf_cssprop.c) and html's
   margins in em against the body width (reader) are as in Lot 116.
5. The legacy reading stays lenient for unknown suffixes ("3zz" is 3 px in
   margins and widths); tightening it would change many older pages.

## Tests

Host tests generated by scripts/lot117_gen.py (seeds 11701..11707) from
the model scripts/lot117_model.py (with the Lot 116 model for the math
grammar and the legacy line-height reading); the model is written from
the specifications (CSS Values 4 6.1.1 and 10, CSS Fonts 4 2.5, CSS Inline
3 4.2, CSS Overflow 4) and shares no code with the C:
test_lot117_units.c (420), test_lot117_fsize.c (640: 480 through
we_font_size100, 160 chains of four elements), test_lot117_umath.c (360,
291 taken), test_lot117_ulh.c (260), test_lot117_clamp.c (200),
test_lot117_props.c (480), test_lot117_box.c (400: padding and margin
shorthands, inset, border-spacing). Hand: test_lot117_engine.c (18 tests: the
declaration order, the root units, lh, the quiet pass, the cascade in the
font pass, hundredths, font-size units and math, line-height math, the
other parsers, inset and border-spacing, the px bounds, the refusals,
the root without html and we_inspect, the clamp, the API, math leaves, ::before, the reader).

QEMU: tests/scripts/test_qemu_lot117.py, `make qemu-lot117` (one boot at
1x, offline: no server, no DNS): web-inspect of /web/lot117.html (nine
boxes; the Lot 116 values are refused). CI job lot117-engine-units.

## Gates on the box

Filled in by the final report (see the PR body).
