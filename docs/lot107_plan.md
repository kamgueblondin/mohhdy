# Lot 107 plan - relative heights, max-height and scroll-padding insets of box scrollers, the body's own style, legacy interpolation past the ends

Base: 1ba29cf (origin/main). PR 264 (the Lot 106 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot107.
One guest engine (userspace/webeng.c and its .inc files; not edited:
the engine already lays out height, max-height and percentage heights).
Kernel, rights and ABI unchanged. US-031 stays partial. phase3_complete
stays false. US-068/US-073, US-076, US-080 and US-081 stay partial.
US-078 and US-097 stay simule.

Scope taken from the Lot 106 gaps, in the preference order of the brief:
(a) box scrollers: heights relative to fonts, the viewport and the
containing block, max-height, scroll-padding for auto insets (gap 2,
horizontal box scrolling left, see below); (b) colours: legacy
interpolation past the ends in the CSS Color 4 12.3 order (gap 4, first
part); (c) the body's own animated style written by the reader (gap 1,
first part). No layout change, no margin collapsing, no new layout
engine.

A row is listed only when its code, tests and docs are in the branch.
Size: see review point 10.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | relative heights (wf_sitecss.c) | the reader passed a box scroller's height only as an absolute length; a percentage, a font or viewport unit or calc() won the cascade but passed nothing | height keeps a token next to hpx: "%N" for a percentage (passed as N%, the integer part, at most 30000), "~L" for a length in em, rem, vw, vh, vmin or vmax (CSS Values 4: em the element's font size, rem the root's, the viewport 640x480), "=N" (the long value pool) for calc(), min(), max() and clamp() without a percentage that resolve. Lengths are resolved to whole px (floor) when the style is written; a negative math result is 0; past 10000 px a length does not resolve (wf_css_math_px). fit-content(), a math function with a percentage and other units (ex, ch, lh) still win the cascade and pass nothing. Proof: test_lot107_height.c (220 cascades), test_lot107_flow.c (150 reader to engine layouts), test_lot107_reader.c, test_lot107_engine.c, `make qemu-lot107` |
| B | max-height (wf_sitecss.c, wf_sitecss.h) | not read | a fourth scroller slot (WF_SITE_SCS 3 to 4): max-height with the values of height (none instead of auto; auto is not valid). A box scroller a timeline reads is passed on with a height, a max-height or both ("overflow-y:auto;height:Npx;max-height:Mpx"); the engine makes the scrollport min(height, max-height) or min(content, max-height). Proof: as A, `make qemu-lot107` (#m107, max-height: 5em, 80 px scrollport) |
| C | scroll-padding as the auto insets of view timelines (wf_sitecss.c, wf_sitecss.h) | an auto view-timeline-inset was 0 | scroll-padding (one to four values: top, right, bottom, left), scroll-padding-block and -inline (one or two: start, end), the logical longhands and the physical ones are read per logical side (WF_SITE_K_SP0: block-start, block-end, inline-start, inline-end; a physical side as in a horizontal-tb ltr scroller), each side auto or a non-negative length-percentage ("#Q" an absolute length in 1/10000 px, else the tokens of A). An auto inset of a view timeline takes the side of its scroller on its axis (CSS Scroll-driven Animations 1, view-timeline-inset: auto is the scroll container's scroll-padding): the box scroller's own, the root html element's for the root. Proof: test_lot107_pad.c (300 sheets with geometry), test_lot107_reader.c, test_lot107_engine.c, `make qemu-lot107` (#v107) |
| D | the body's own style (wf_html.c, wf_sitecss.c) | the reader view wrote a bare body | wf_site_body_start gives the body element's offset (site_body: at the top or a child of the root html element); wf_html writes its style attribute (an animated colour too) on `<body>`, without a mark. The GUI sections (wf_section.c) find a body start tag with attributes (they looked for a literal `<body>`, so a page whose body had a style was not presented) and every section reopens the body with its tag as the page has it (wf_sections_t.body_tag). Proof: test_lot107_engine.c, test_lot107_reader.c, test_wf_section.c (test_body_with_style), `make qemu-lot107` (body #808080) |
| E | legacy interpolation past the ends (wf_cssprop.c) | the alpha was clamped to 0..1 before the premultiplied channels were divided by it (Lot 97 order) | CSS Color 4 12.3 as the Oklab path since Lot 106: the channels are divided by the interpolated alpha as it is, the alpha is clamped afterwards; an alpha of exactly 0 gives channels 0. Add and accumulate unchanged. Proof: test_lot107_interp.c (400 legacy pairs past 0 and 1), test_lot107_reader.c, `make qemu-lot107` (#e107 #eee8aa, was #ffffd5) |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB; the engine is not edited). wf_site_node_t 1296 to 1392 bytes (mpx, htok and mtok, sptok[4] and sprank[4]), so wf_site_t 513828 to 532260 bytes (192 nodes; the reader's static site in wf_html.c, Ring 3 BSS). wf_geom_t unchanged (12376 bytes). wf_sections_t +4 bytes (body_tag; one static in shell_fetch.c).
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3):
    - userspace/wf_sitecss.c: height_val (tokens), sp_val, tok_pool, box_len, the max-height and scroll-padding declarations and cascade slots, sp_inset in the view clock, wf_site_body_start.
    - userspace/wf_sitecss.h: WF_SITE_SCS 4, WF_SITE_K_SP0 and WF_SITE_SPS, the node fields, wf_site_body_start.
    - userspace/wf_cssprop.c: the legacy order past the ends.
    - userspace/wf_html.c: the body's style attribute.
    - userspace/wf_section.c, wf_section.h: the body start tag with attributes (s_body, body_tag), kept in every rendered section.
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change. No shell command added.
3. Percentage heights are passed as such: the engine takes a percentage of its containing block (an auto-height block resolves it as the engine always did); the reader does not check that the containing block has a definite height.
4. Physical scroll-padding sides (scroll-padding, -top, -bottom, -left, -right) are mapped as in a horizontal-tb, ltr scroller; the reader has no writing mode. Logical longhands are exact.
5. Font and viewport heights are floored to whole px when the style is written (the engine takes integer px); a value between two px may differ by one px from a browser's layout.
6. The body's own style is written without a geometry mark: the body box keeps coming from its own record (Lot 106). A body style the reader computes from its own animations or properties now reaches the engine (the text colour of the page may change where a page styles body).
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - scripts/lot97_animmodel.py (interp_colour legacy path: the 12.3 order, flag lot107_model.ORDER123): regenerated test_lot97_interp.c (2 values), test_lot98_sheet.c (2 values), test_lot106_interp.c (10 values), e.g. red to transparent at 1.25 is rgba(255, 0, 0, 0) (was rgba(0, 0, 0, 0)), transparent to palegoldenrod at 2 is rgb(238, 232, 170) (was rgb(255, 255, 255)).
    - scripts/lot105_model.py (sc_cascade and passed go through scripts/lot107_model.py; the Lot 105 functions kept as sc_cascade_lot105 and passed_lot105): regenerated test_lot105_overflow.c (19) and test_lot105_sheet.c (13 style assertions: relative heights now passed, e.g. 10em as height:160px, 50% as height:50%).
    - Hand: test_lot105_reader.c, #b with height: 50% is now passed ("overflow-y:auto;height:50%", was ""); a fit-content case keeps the "not passed" check.
8. CI: one new job, lot107-reader-box (make qemu-lot107, timeout 15 minutes).
9. Mutations (/workspace/run/lot107/mutations.txt on the box): host 98 mutants on wf_sitecss.c, wf_sitecss.h, wf_cssprop.c and wf_html.c, 97 killed and the no-op control survives (run 1: 13 survivors, fixed by witnesses in test_lot107_reader.c or by removing equivalent code: the f < 4 test in height_val, auto and none in sp_val, the ok test in sp_inset, the g_sitecss test around the body style in wf_html.c); wf_section.c 12 mutants (test_wf_section, plain and ASan), 11 killed and the control survives; QEMU 8 mutants on a git archive copy with make qemu-lot107, 7 killed and the control survives (the body mutant survived a first page whose checks read only the reader's styles: the page now gives the body font-size: 20px, which only the written body style brings to the layout).
9b. GUI sections (wf_section.c, wf_section.h, outside the engine): a reduced page whose body tag carries a style was not indexed (wf_section_index looked for a literal `<body>`), so the page was not presented in the GUI; the body tag with attributes is found and copied into every rendered section. wf_sections_t gains body_tag (+4 bytes).
10. Size: 8950 lines added and 87 removed outside docs: code 319/35 (wf_sitecss.c 259/25, wf_sitecss.h 23/3, wf_section.c 20/3, wf_section.h 1, wf_html.c 10/1, wf_cssprop.c 6/3), model and generator 1103, generator conversions 25/4, generated suites 6618, hand suites 603 (test_lot107_reader 370, test_lot107_engine 177, test_wf_section 56), regenerated or converted older suites 52/46, QEMU test, server, Makefile, CI and run_all 230/2; in the 8000-10000 target.

## Not done (first gaps for lot 108)

1. Box scrollers: horizontal box scrolling (the engine scrolls boxes along y only: no box_scroll_x); heights with a percentage inside a math function, ex, ch and lh units; the engine's 2000 px limit on a parsed height (parse_dimension); min-height; scroll-padding sides in vertical writing modes and rtl.
2. Geometry: html and body as box scrollers of their own (overflow on the root is the viewport's); view timelines in section 1 and later; the console web-load path (no layout).
3. Animations: a running clock and repaint, other reader properties than color and background-color, transition events.
4. Colours: relative colour syntax (from), system colours.
5. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests generated by scripts/lot107_gen.py (seeds 10701..10704) from the model scripts/lot107_model.py with the Lot 97 to 106 models; the models are written from the specifications (CSS Values 4 units and math functions, CSS Overflow 3, CSS Scroll Snap 1 scroll-padding, CSS Scroll-driven Animations 1 insets, CSS Color 4 12.3) and share no code with the C: test_lot107_height.c (220 cascades of overflow, height, max-height and font-size: the box scrollers, hpx, mpx, the font size and the style written), test_lot107_pad.c (300 sheets: scroll-padding on html, body, #o, #b, view timelines with auto and explicit insets against box scrollers or the root, the style and counters), test_lot107_interp.c (400 legacy pairs past the ends), test_lot107_flow.c (150 reader to engine layouts: the style the reader writes on #o and the scrollport the engine gives). Hand: test_lot107_reader.c (6 tests: the legacy order, heights, scroll-padding parsing, root insets, box insets on the inline axis, the body offset), test_lot107_engine.c (4 tests: the body style, an em, vh and calc() box scroller, max-height, the root's scroll-padding in two passes).
QEMU: tests/scripts/test_qemu_lot107.py, `make qemu-lot107` (one boot at 1x): /lot107/box.html from the Lot 67 server: #o107 (height: 10em at 10 px) gets a 100 px scrollport and "web-bscroll 630 94" gives #i107 #717171 (56 of 126 px); #m107 (max-height: 5em at the body's 20 px) a 100 px one and "web-bscroll 630 164" gives #n107 #242424 (22 of 153 px); #v107 on view() with html's scroll-padding-block: 100px 50px has no colour at 55% (#0e0e0e without the insets), is #101010 at 60% (#252525 without them and #191919 without the body style, both refused) and has no colour at 100% (#e1e1e1 without them); the page fits one section (under 120 nodes); body #808080 with font-size: 20px written on the body element, so the page is laid out at 20 px; #e107 (transparent to palegoldenrod, cubic-bezier overshoot 1.25) #eee8aa (#ffffd5 refused). CI job lot107-reader-box.

## Gates on the box

Local, Mauritius time (UTC+4), 16 November 2026, code commit (suites and QEMU page) dac0607, code tree as b290d8f:
- make all: OK. make all on a fresh git archive: OK.
- test-all: 24796 tests, 0 failures (00:39-00:47, code b290d8f; dac0607 only changes the QEMU page and its test).
- qemu-lot107: OK at dac0607 (01:11). qemu-lot106 and qemu-lot105: OK at b290d8f.
- roadmap-check, secrets-check, font-check, strict-image-check: OK.
- ASan/UBSan: base 1ba29cf 24529 tests, 5 failures; head (code b290d8f) 24711 tests, 5 failures: the 5 known ones (test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Mutations: review point 9.
- The rest: GitHub CI.
