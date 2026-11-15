# Lot 105 plan - box scrollers as scroll and view timeline sources, the GUI scroll position, divs kept as boxes

Base: da0e8fc (origin/main). PR 262 (the Lot 104 branch) is merged into
origin/main, so the branch starts from origin/main. Branch cursor/lot105.
One guest engine (userspace/webeng.c and its .inc files; layout results
unchanged, webeng.c itself not edited) and one shell path. Kernel, rights
and ABI unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and US-097
stay simule.

Scope taken from the Lot 104 gaps, in the preference order of the brief:
gap 1 (timelines: box scrollers in reader pages, the GUI scroll position
as the timeline position with a restyle on scroll) in full, and the part
of gap 2 that box scrollers need (a reader div that holds blocks keeps a
box of its own when it is a box scroller a timeline reads, or a view
timeline subject). html and body geometry, add composition with missing
components and the oklab default interpolation stay for later lots. No
layout change, no margin collapsing, no new layout engine.

A row is listed only when its code, tests and docs are in the branch.
Size: see review point 10.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| A | overflow, overflow-x, overflow-y and height in the reader (wf_sitecss.c/.h, wf_cssprop.c/.h) | not read | three cascade slots (WF_SITE_K_SC0, importance, origin, layer, specificity, order; in a style attribute the later declaration wins): visible, hidden, clip, scroll, auto (overlay is auto); the overflow shorthand takes one or two keywords (x then y); initial and unset are visible; inherit, revert, an unknown keyword or a third keyword drop the declaration. height: an absolute length (px, cm, mm, q, in, pt, pc, a unitless 0; wf_csslen_abs, new) is kept in px (floor, at most WF_SITE_HMAX 30000); auto, min-content, max-content, fit-content, fit-content(), percentages, lengths in any other unit (em, vh, ...), calc(), min(), max(), clamp(), lengths above 100000px, initial and unset are kept as "not passed"; negative, unitless or broken values are dropped. Proof: test_lot105_overflow.c (160 generated sheets), test_lot105_reader.c |
| B | scroll containers (wf_sitecss.c) | the reader had no box scroller | CSS Overflow 3: visible next to hidden, auto or scroll is auto, clip next to them is hidden; an element is a scroll container when an axis is hidden, auto or scroll; html and body are the root, never a box scroller. wf_site_box_scroller gives the computed y value of a container, wf_site_scroller_of the nearest container ancestor (-1: the root). Proof: test_lot105_overflow.c, test_lot105_reader.c |
| C | box scrollers as timeline sources (wf_sitecss.c, wf_geom.h, webeng_geom.inc) | scroll(nearest) always read the root; a view subject in a box scroller had no effect | scroll(nearest) reads the nearest container (the root when none; the root element itself has none), scroll(self) the element when it is a container (the root on the root element), scroll(root) the root; a scroll-timeline-name works on a container or the root element; the axis is block or y. A box timeline runs on the box's own scroll state from the layout (the geometry record gains, per box, sv: the box scrolls along y, s_vp, s_sc, s_rg: its scrollport, scroll offset and scroll range, and sm: the mark of the nearest box scroller, plus sc_y, the root scroll offset). A view() subject inside a box scroller runs when the layout puts it in that marked box, at the box's scroll offset. A hidden container is a container without a scroll box (no effect). The css-anim line ends with " box B" (animations on box timelines). Proof: test_lot105_sheet.c (240 sheets with geometry), test_lot105_geom.c (150 generated engine layouts with box scrollers, nested scrollers and track clicks), test_lot105_reader.c, test_lot105_engine.c |
| D | the box scroller passed on (wf_sitecss.c, wf_html.c) | the reader passed no overflow or height | a box scroller read by a timeline, auto or scroll along y, with a passed height, gets ` overflow-y:auto|scroll;height:Npx` in its style attribute, so the engine lays out a real box scroller; it is written as a `<div>` (a div stack of 32 open div-like elements, matched by name) and so is a view subject holding blocks (wf_site_geom_block), so their boxes are their own and not an empty paragraph; marks id="wgN" as in Lot 104. Proof: test_lot105_engine.c (reduce, the two passes with a box scroller), test_lot105_reader.c |
| E | the GUI scroll position (shell_fetch.c, osui_gui.c, shell.c, wf_html.c) | the timeline position was the "web-css scroll N" setting only | "web-css scroll gui": the root position is the layout's root scroll offset (sc_y) and root scroll timelines need the geometry; "web-css scroll N" leaves it. When a page has timelines that read real scroll offsets (a box scroller, or the GUI position), the GUI styles the page again after it scrolls: arrow keys, the new commands "web-scroll Y" (osui web scroll Y[ restyled]) and "web-bscroll X Y" (a click on a box scrollbar: osui web bscroll hit|none[ restyled]) and a desktop click on a box scrollbar. shell_page_geom runs again on a live page (section 0 shown), keeps the reader form state (a byte copy) and box offsets persist through the forms state by id. The css-anim line shows "pos gui" and " gui Y". Proof: test_osui_gui.c (test_lot105_scroll_refresh), test_lot105_engine.c (two passes at a GUI offset), test_lot105_reader.c, `make qemu-lot105` |

## Review points

1. Structure sizes: we_doc_t keeps 870824 bytes (< 1 MiB; the engine structures are unchanged). wf_geom_box_t 44 to 64 bytes and wf_geom_t 8468 to 12312 bytes (sv, sm, s_vp, s_sc, s_rg per box, sc_y; one static in shell_fetch.c); wf_site_t 509972 to 513828 bytes (wf_site_node_t 1276 to 1296: ov[2], hpx, scrank[3], scr; the site gains scroll_gui, anim_box, geom_live, gui_y). New statics: a wf_form_page_t copy in shell_page_geom (26408 bytes), the div stack in wf_html_reduce_ex (32 x 17 bytes).
2. Files changed outside userspace/webeng.c and its .inc files (each a review point; all userspace, Ring 3):
    - userspace/wf_sitecss.c/.h, wf_cssprop.c/.h: the reader style engine (overflow and height parsing and cascade, scroll containers, the box timeline clocks, the GUI position, counters, the style passed on); wf_csslen_abs is a new helper on the existing length tokens.
    - userspace/wf_geom.h: the geometry record gains the box scroll state and the root scroll offset.
    - userspace/wf_html.c/.h: a div stack so a box scroller or a view subject holding blocks is written as a div; wf_html_set_cssscroll_gui and wf_html_sitecss_geom_live pass through to the reader.
    - userspace/shell_fetch.c: "web-css scroll gui"; shell_page_geom runs again on a live page and keeps the reader form state (fcopy: no memcpy in this freestanding file).
    - userspace/osui_gui.c: web_geom_refresh, the web-scroll and web-bscroll commands, the restyle after arrow keys and after a desktop click on a box scrollbar.
    - userspace/shell.c: registers web-scroll and web-bscroll (builtin list and the web command dispatch, same code as the GUI line).
    - shared/multiboot_shell_commands.json and userspace/mohhdy_osui_bridge.h: regenerated by scripts/extract_guest_commands.py for the two new commands (495 to 497 names); test-all checks they match shell.c.
    - No change under kernel/, include/, fs/ or the syscall table: no new syscall, no rights change, no ABI change.
3. GUI cost: on a page whose timelines read real scroll offsets (web-css on), each GUI scroll step that moves runs the reader and the pipeline again (one reduction, parse, style and layout). Pages without such timelines are not touched.
4. "web-css scroll N" still drives the root position (the Lot 101 setting) unless "web-css scroll gui" is given; box scrollers always use their real offsets from the layout. The usage text of web-css scroll is unchanged ("usage: web-css scroll <0..100>", checked by test_qemu_lot101.py); "gui" is an extra word.
5. Heights in %, em, vh or a math function are not passed on (the reader has no containing block size at parse), so such a box scroller is read by its timelines but laid out as a plain block (its scroll range is 0: no effect). Only overflow-y auto or scroll is passed; a hidden container is a container without a scroll box. An x-only scroller is still a container (a nearest scroll() finds it) but has no vertical range (no effect).
6. Merged divs: a div-like element is kept as a div only when it is a box scroller a timeline reads or a view subject holding blocks, and only while fewer than 32 are open (an end tag must find its start); others stay Lot 67 paragraphs.
7. Converted tests (assertions changed, none weakened; each now checks the new behaviour):
    - tests/scripts/test_qemu_lot101.py and test_qemu_lot102.py: #v101 and #v102 are view timeline subjects that are divs holding a paragraph. Lot 104 saw them as the empty paragraph the reduction wrote (a 0 px point at 262 px: #747474); Lot 105 keeps them as divs (row D), so their box is their own (254 px, 26 px high: cover [-226, 280] in the 480 px view, progress 226/506 at the unscrolled page) and they paint #727272. Three lines each (two assertions and a message) and the docstrings. Every host suite of Lots 101 to 104 passes unchanged.
8. CI: one new job, lot105-reader-scroll (make qemu-lot105, timeout 15 minutes).
9. Mutations: host, 3 runs on wf_sitecss.c, wf_cssprop.c, webeng_geom.inc, wf_html.c and osui_gui.c (107 suites plus test_osui_gui): run 1 144 applied / 17 survived (each got a witness, or the check was removed as redundant: the comma test of the overflow keywords, an x-only test in ov_axis, the site CSS test before wf_site_geom_block, the active test of web_geom_refresh); run 2 142 / 2 survived (an unread val for height: removed; equal ranks in a style attribute: witness test_l105_inline); run 3 141 applied, 141 killed. QEMU: 8 mutants on a git archive copy, make qemu-lot105: a no-op control mutant (a comment changed) survived as it must, so the harness is not vacuous; live page flag, live refresh guard, "web-css scroll gui" ignored, gui not left by "web-css scroll N", form state not kept, web-scroll/web-bscroll not dispatched: killed; the is_builtin entries survived the first contract and were killed by the "which web-scroll" check added to it. Detail: /workspace/run/lot105/mutations.txt on the box.
10. Size: 10135 lines added and 61 removed outside docs: code 536 / 51 (userspace, incl. the regenerated bridge header 3 / 1), registry json 4, model scripts/lot105_model.py and generator scripts/lot105_gen.py 951 (new), generated suites 7529, hand suites 864 (test_lot105_reader.c, test_lot105_engine.c, test_osui_gui.c), QEMU test, server page, Makefile, CI and run_all_tests.sh 241 / 2, converted QEMU checks 10 / 8. The total is slightly above the 8000-10000 aim because of the generated suites (cut to 160 / 240 / 150 cases); no case is padding.

## Not done (first gaps for lot 106)

1. Geometry: html and body as subjects or scrollers with boxes of their own, view timelines in section 1 and later, the console web-load path (no layout).
2. Box scrollers: heights relative to fonts, the viewport or the containing block, max-height, horizontal box scrolling (overflow-x passed on), scroll-padding for auto insets.
3. Animations: add and accumulate composition carrying missing components, a running clock and repaint, other reader properties than color and background-color, transition events.
4. Colours: the oklab default interpolation space for keyframes and transitions, relative colour syntax (from), system colours.
5. Fonts, gradients, @property, autofill, PSL, NE2000, shaping, US-080 and US-081: unchanged (see docs/lot99_plan.md).

## Tests

Host tests (generated by scripts/lot105_gen.py, seeds 10501..10503, from the model scripts/lot105_model.py with the Lot 99 to 104 models; the models are written from the specifications and share no code with the C): test_lot105_overflow.c (160 sheets: overflow and height values, cascade, containers, the style passed on), test_lot105_sheet.c (240 sheets with a geometry record: box and root scroll timelines, box view timelines, the GUI position), test_lot105_geom.c (150 engine layouts with box scrollers). Hand suites: test_lot105_reader.c (9 tests), test_lot105_engine.c (4 tests), test_osui_gui.c (test_lot105_scroll_refresh).
QEMU: tests/scripts/test_qemu_lot105.py, `make qemu-lot105` (one boot at 1x): /lot105/scroll.html from the Lot 67 server, box offsets 0, 61 and 183 px through web-bscroll, the GUI root offset 200 and 0 through web-scroll, a field value kept through a restyle, back to "web-css scroll 50". CI job lot105-reader-scroll. `make qemu-lot104` is run again unchanged.

## Gates on the box

Code commit ee1802b (the suites, QEMU and docs commits after it change no code), 15 November 2026, UTC+4:

- make all: pass; a fresh git archive make all: pass (31 s).
- make test-all: pass, 24446 tests, 0 failed, registry, GUI parity and the other post checks pass (426 s).
- make qemu-lot105: pass (158 s, final contract); make qemu-lot104: pass (99 s); make qemu-lot102: pass (508 s under load, converted checks); make qemu-lot101: pass (108 s, converted checks); make qemu-lot103: pass (207 s, before the conversions, unaffected).
- roadmap-check, secrets-check, font-check, strict-image-check: pass.
- ASan/UBSan test-all: base da0e8fc 24235 tests 5 failed, head (code ee1802b) 24361 tests 5 failed (the same known five: test_gpt2_infer, test_pmm, test_vfs_service, test_lot87_web, test_webrt).
- Mutations: host 141 distinct mutants, all killed (after 3 runs); QEMU 7 killed, 1 control survived (review point 9).
- Other QEMU gates: left to GitHub CI.
