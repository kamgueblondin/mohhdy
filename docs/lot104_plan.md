# Lot 104 plan - view timelines fed by the layout geometry: view(), view-timeline-name/axis/inset, named ranges and lengths in animation-range

Base: 67c8f67 (origin/main). PR 261 (the Lot 103 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot104.
One guest engine (userspace/webeng.c and its .inc files; layout results
unchanged) and one shell path. Kernel, rights and ABI unchanged. US-031
stays partial. phase3_complete stays false. US-068/US-073, US-076,
US-080 and US-081 stay partial. US-078 and US-097 stay simule.

Scope taken from the Lot 103 gaps (gap 1, timelines): the reader's CSS
pass had no box geometry, so view timelines, view() and lengths in
animation-range stayed inactive. The layout now hands a small, bounded
geometry record per element back to the reader style engine, and the
reader computes view progress from it. No layout change, no margin
collapsing, no new layout engine.

A row is listed only when its code, tests and docs are in the branch.
Size: 11259 lines added and 130 removed outside docs (code 696 / 66 in userspace: wf_cssprop.c/.h, wf_sitecss.c/.h, the new webeng_geom.inc and wf_geom.h, webeng.c/.h, wf_html.c/.h, shell_fetch.c/.h, osui_gui.c/.h, shell.c; model scripts/lot104_model.py 394 and generator scripts/lot104_gen.py 899, new; lot102 model and generator converted 64 / 16; generated suites 8165; regenerated test_lot102_range.c 59 / 19; hand suites 746 (test_lot104_reader.c, test_lot104_engine.c, test_osui_gui.c) and hand conversions 5 / 5; QEMU test, server page, Makefile and CI 205 / 3; converted QEMU tests 24 / 19; run_all_tests.sh 2 / 2). The total is above the 8000-10000 aim because of the generated suites (8165); no case is padding.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | geometry record (wf_geom.h, webeng_geom.inc) | the layout kept its boxes to itself | we_geom gives an element's geometry from the boxes the existing layout left in the document (read only, the layout is not run or changed): offset and size along the block and inline axes of the nearest scroller, that scroller's viewport, scroll offset and scroll range, whether it is the root, and which axis is vertical. The nearest scroller is the nearest ancestor that scrolls vertically (overflow auto or scroll, Lot 74), else the root (we_layout_viewport). Axes follow the scroller's writing mode (the root element's for the root); in vertical-rl the block offset is taken from the block start (the right edge). we_geom_feed collects the boxes of the elements whose id is "<prefix><reader index>" into a wf_geom_t of WF_GEOM_MAX (192) boxes, 8468 bytes, plus the root viewport and scroll range. Proof: test_lot104_geom.c (240 generated layouts: stacked boxes with margins, borders and padding, a box scroller with children, a box holding a child, display none, marks), test_lot104_engine.c (node, marks, box scroller, vertical-rl, the document unchanged byte for byte by we_geom_feed, the two passes) |
| B | two passes in the shell (shell_fetch.c, osui_gui.c, wf_html.c) | one reader pass, one layout | when web-css is on and the page has a view timeline subject (or a scroll timeline with a length edge), the reader marks those subjects with id="wgN" (N its reader index), the GUI lays the page out, the new page op page_geom (osui_web_page_ops_t) gives the laid-out document to the shell, which reads the geometry (we_geom_feed), runs the reader again with it and returns section 0; the GUI lays that out. Colours do not move boxes, so the second layout has the same geometry. The geometry is cleared by the next page. Proof: test_osui_gui.c (test_lot104_page_geom), test_lot104_engine.c (two passes), `make qemu-lot104` |
| C | view timelines active (wf_sitecss.c) | view() and named view timelines stored and idle | view() (the element is the subject) and a name from view-timeline-name (or the view-timeline shorthand) on the element or an ancestor run against the root scroller: the subject's box along the axis (block, inline, x, y), the view, the root scroll range, the position from "web-css scroll N" (N% of the vertical root range; a horizontal axis stays at 0). view-timeline-inset (auto, absolute lengths, percentages of the view, a unitless 0; one value is both) and the insets of view() and of the view-timeline shorthand are taken. A subject in a box scroller, without a box (merged into another element, display none, outside section 0) or with no geometry yet has no effect. The css-anim line ends with "view V need N geom G" (animations on view timelines, elements whose geometry is needed, boxes found). Proof: test_lot104_sheet.c (320 sheets with geometry), test_lot104_reader.c, converted QEMU pages lot101 and lot102, `make qemu-lot104` |
| D | named ranges on view timelines (wf_cssprop.c) | stored, treated as normal | cover, contain, entry, exit, entry-crossing and exit-crossing per CSS Scroll-driven Animations 1 4.1, with the insets: cover [S - V + ie, E - is], contain between E - V + ie and S - is (the smaller first), entry [cover start, contain start], exit [contain end, cover end], entry-crossing [S - V + ie, E - V + ie], exit-crossing [S - is, E - is]; scroll [0, scroll range]. Edges: normal is cover 0% / 100%, a percentage of the named range (cover when none), positions in 1/1000000 px, floor; the time is (delay + duration x count) (P - start) / (end - start) floored and kept within +-2e8 ms. Proof: test_lot104_view.c (800 generated view cases), test_lot104_reader.c |
| E | lengths in animation-range (wf_cssprop.c, wf_sitecss.c) | refused | an absolute length (px, cm, mm, q, in, pt, pc; +-100000px) stands for the percentage of an edge, alone or after a range name: an offset in px from the start of the range. On a view timeline it is taken in the named range (cover when none); on a scroll timeline it needs the root scroll range from the geometry (the range names count as normal there, as in Lot 102) and without geometry the animation has no effect. wf_cssanim_range_ms (Lot 102, percentages) returns 0 for a length edge. Proof: test_lot104_parse.c (600 generated values), test_lot104_view.c, test_lot104_reader.c (scroll_px), test_lot102_range.c (regenerated) |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB; webeng.c only includes webeng_geom.inc and the engine structures are unchanged). wf_site_t 508420 to 509972 bytes (wf_site_node_t 1268 to 1276: the geo flag and the sixth timeline slot, WF_SITE_TLS 5 to 6), wf_cssanim_t 120 to 132 bytes (rsl, rel, vik, viv), wf_csstl_t 33 to 44 bytes (ik, iv). New: wf_geom_t 8468 bytes (one static in shell_fetch.c).
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3):
    - userspace/wf_cssprop.c/.h, wf_sitecss.c/.h: the reader style engine itself (parsing of insets and length edges, view range math, the view and scroll clocks, the counters); the brief's feature lives there.
    - userspace/wf_geom.h (new): the geometry record shared by the engine (producer) and the reader (consumer), so neither includes the other's headers.
    - userspace/webeng.h: declarations of we_geom and we_geom_feed (implemented in the new webeng_geom.inc, included by webeng.c); they only read a laid-out document.
    - userspace/wf_html.c/.h: the reduction writes id="wgN" on view timeline subjects when asked (wf_html_set_geommarks) and passes the geometry to the reader (wf_html_set_cssgeom, wf_html_sitecss_geom_need); the reader has no other way to name an element to the engine.
    - userspace/shell_fetch.c/.h: the two-pass glue (shell_page_geom): pass 1 with marks, then after the GUI layout the geometry is fed and the page reduced again; the geometry is cleared on the next page. Needed because only the shell holds the page body and the reader state.
    - userspace/osui_gui.c/.h: a new page op page_geom (osui_web_page_ops_t gains a last member; a null op keeps one pass) and a second osui_web_pipeline run when it returns a page; the GUI owns the laid-out document. The g_wops declaration moved up so the web load can see it.
    - userspace/shell.c: registers shell_page_geom in its page ops initializer (the only initializer of osui_web_page_ops_t).
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change (the new op is an in-process userspace function pointer).
3. GUI cost: the GUI web load runs the pipeline a second time when the op returns a page (one extra reduction, parse, style and layout, only for pages with view timelines and web-css on).
4. Reader and layout coupling: the reader only reads the root scroller; positions are those of the reader scroll setting ("web-css scroll N", a percentage of the vertical root range), not of the GUI scroll. The reader passes no overflow, height or writing-mode to the engine, so reader pages have no box scrollers and stay horizontal; the box scroller and vertical-rl paths of the geometry are proven on engine documents (test_lot104_geom.c, test_lot104_engine.c). The root scroll range is the page height minus the viewport along y and 0 along x (the engine page width is the viewport width, it does not track a wider overflow), so a vertical-rl page has no root block scroll range.
5. Merged elements: a reader div that holds blocks is written as an empty paragraph before them (Lot 67 reduction), so its box is a 0 px point at its start; a subject with inline content has its real box. html and body have no tag of their own in section 0 and no geometry. The console web-load path has no layout and no feed (view timelines stay idle there).
6. Choices: auto insets are 0 (the reader has no scroll-padding); a percentage inset is of the view; lengths in view-timeline-inset and ranges are absolute only (no font or viewport context at parse); the view timeline axis inline/x/y follows the scroller's writing mode; a subject without a box is idle (no effect), not an error.
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - test_lot102_range.c (regenerated by scripts/lot102_gen.py): length edges, refused before, are now valid; their expected edges carry " px" and their text caps come from a separate random stream (seed 10402) so the other cases keep theirs; 19 lines replaced, 59 written.
    - scripts/lot102_model.py: view() keeps its insets, range_len and the length edges, range_ms gives no effect for a length (the Lot 104 model takes it).
    - test_lot101_reader.c: wf_csstl_parse(4) is now WF_CSSTL_INSETS, the out-of-range kind checked is 6.
    - test_lot102_reader.c: "10px" as an edge is valid, the refused length is "1em" (twice); WF_CSSTL_SCOPE + 1 is valid, the witness is WF_CSSTL_VSHORT + 1; WF_SITE_TLS is 6.
    - tests/scripts/test_qemu_lot101.py and test_qemu_lot102.py: #v101 (view-timeline) and #v102 (view()) were idle and are now active: #747474 at every position (the pages do not scroll; the subject point is at 262 px in the 480 px view), "color" counts one more and the css-anim line ends with "view 1 need 1 geom 1".
8. CI: one new job, lot104-reader-view (make qemu-lot104, timeout 15 minutes).
9. Mutations: host 156 mutants in a first run (wf_cssprop.c, wf_sitecss.c, webeng_geom.inc, wf_html.c, osui_gui.c) on the reader and engine suites of run_all_tests.sh plus test_webeng (102 suites, the Lot 104 suites first, baseline 0 failures) and test_osui_gui for the GUI mutants: 15 survived; each got a witness (an auto range edge, the clamp after the floor, an unfilled box record, a horizontal scroll range, the node past the last one, text before html, a partial id prefix, a long index, a failed render) or its code was simplified as unreachable or redundant (the auto test of cp_range_len, the empty insets item, the insets of a scroll timeline, the scroll range test of scroll_clock, the viewport fallbacks and the root x range of the geometry, the digit cap of the marks, the text node test of the feed). The four GUI verdicts of that run came from a wrong build line and were not counted. Second run: 149 applied, 148 killed, 1 survived (a page_geom call after a failed render), killed in a third run by its witness. No equivalent mutant is left. QEMU: 4 mutants on a git archive copy (geometry never awaited, geometry not fed, pass-1 marks off, page_geom op unset), all killed by `make qemu-lot104`. Detail: /workspace/run/lot104/mutations.txt on the box.
10. Size: about 3100 new lines of code, models, generator, hand suites, QEMU test and CI, 8165 generated suite lines, and 107 / 59 lines of converted tests and models (point 7).

## Not done (first gaps for lot 105)

1. Timelines: box scrollers in reader pages (the reader passes no overflow or height to the engine), the GUI scroll position as the timeline position (a repaint on scroll), view timelines in section 1 and later, html and body as subjects, the console web-load path.
2. Lengths relative to fonts or the viewport (em, rem, vw, vh, %) in view-timeline-inset and animation-range, auto insets from scroll-padding, the timeline range names on scroll timelines with lengths.
3. Colours: relative colour syntax (from), the oklab default interpolation space for keyframes and transitions, system colours.
4. Animations: a running clock and repaint, other reader properties than color and background-color, transition events.
5. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests (generated by scripts/lot104_gen.py, seeds 10401..10404, from the model scripts/lot104_model.py with the Lot 99 to 103 models; the models share no code with the C): test_lot104_view.c (800 view progress cases), test_lot104_parse.c (600 values of view(), view-timeline-inset, view-timeline and animation-range), test_lot104_sheet.c (320 sheets with a geometry record), test_lot104_geom.c (240 engine layouts). Hand suites: test_lot104_reader.c (8 tests), test_lot104_engine.c (6 tests), test_osui_gui.c (test_lot104_page_geom).
QEMU: tests/scripts/test_qemu_lot104.py, `make qemu-lot104` (one boot at 1x): /lot104/view.html from the Lot 67 server at scroll 0, 50 and 75%. CI job lot104-reader-view. `make qemu-lot103` is run again unchanged; `make qemu-lot102` and `make qemu-lot101` are run with their converted checks.

## Gates on the box

Code commit c42c69e (the suites, QEMU and docs commits after it change no code), 15 November 2026, UTC+4, tree ab922ba:

- make all: pass; a fresh git archive make all: pass (27 s).
- make test-all: pass, 24320 tests, 0 failed (624 s).
- make qemu-lot104: pass (99 s); make qemu-lot103: pass (67 s); make qemu-lot102: pass (100 s, converted checks); make qemu-lot101: pass (361 s under load, converted checks).
- roadmap-check, secrets-check, font-check, strict-image-check: pass.
- ASan/UBSan test-all: base 67c8f67 24000 tests 5 failed, head ab922ba 24235 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Mutations: host 149 distinct mutants, all killed (after 3 runs, above); QEMU 4 applied / 4 killed.
- Other QEMU gates: left to GitHub CI.
