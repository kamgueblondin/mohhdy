# Lot 85 plan - masonry auto columns, reader, glyphs, local search

Base: main 75ad5f5 (lot 84, PR #242). One guest engine in userspace/webeng.c.
Kernel rights unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073 stay partial. US-076 stays partial. US-080 and US-081 stay partial.
US-078 and US-097 stay simule.

## Scope (the lot 84 first gap, then real-page and desktop checks)

| Id | Feature | Before | After |
|----|---------|--------|-------|
| F1 | Masonry-rows auto columns | sized from the ordinary row-major cell | sized from the column masonry placed the item in, then placed again (at most three passes) |
| F2 | Percentage top/bottom on an auto-height masonry grid | the used padding height is the basis once the columns are packed | covered: 50% of an 80px packed grid is 40, not the token 50 |
| F3 | Reader root | nav and some classes were dropped, but a page with an article still spent nodes on the rest | the interior of article, main, role=main or a content id is kept when it holds the text. Boilerplate classes (toc, navbox, cookie banner, catlinks) are skipped |
| F4 | Glyphs | Vietnamese and horn letters were '?' | 78 letters are the base glyph plus a painted accent, on bytes the glyph pages left unused. 16 rarer combinations and a few IPA letters become the base letter. Fullwidth ASCII folds to ASCII. The bitmap fonts are unchanged |
| F5 | Page buffer | fetch body 256 KiB, reduced page 192 KiB | 384 KiB and 256 KiB, still one static buffer |
| F6 | Search | Wikipedia was last, after DuckDuckGo and Google | Wikipedia is the first fallback. search-engine local searches a built-in catalogue with no network |
| F7 | EDID | preferred timing and centimetre size | also digital input, gamma, monitor name, range limits, standard timings, established VESA modes, and the refresh when the timing has blanking |
| F8 | Power | profiles set the refresh | after the profile's idle time the refresh is halved (dim). The battery stays simulated |
| F9 | Estimator | a late ack still counted as a loss | the confirmed loss count drops when the ack arrives |
| F10 | Chrome | one title bar already | text-size accepts web, chat, shell, pane, ide and app0. A tablet click minimizes the full-screen web window |

## Review points

1. we_node_t is unchanged. The document stays under 1 MiB.
2. Composed letters use glyph indexes past OS_GLYPH_TOTAL. The desktop paints them from the 8x8 base plus the accent at every scale. The generated fonts are not edited.
3. A fetch body of 384 KiB is static storage in the shell, not a heap.
4. search-engine local never opens a socket. The catalogue is the commands and limits of this guest, not the public web.
5. The border width cap stays 8 px per side. Raising it would repaint every lot 84 border test.

## Not done (first gaps for lot 86)

1. Percentage border widths are still capped at 8 px per side. A wider painted border is not in this engine.
2. Sixteen Vietnamese combinations (hook above a circumflex, breve or horn) have no free page byte, so they show the base letter.
3. The reader still has no JavaScript, no site CSS, and no live re-measurement of google.com or wikipedia.org in this lot. Google's HTML page still asks for another browser.
4. US-068/US-073 remain a laboratory estimator: no WAN, no cross traffic. US-078 and US-097 stay simule. EDID is the base block only; the kernel does not drive DDC.

## Tests

Hand-written: tests/unit/userspace/webeng_lot85.inc, test_wf_lot85.c, the composed-letter checks in test_glyph_pages.c, the EDID timing checks in test_edid.c, the idle dim check in test_plat_form_energy.c, the local catalogue check in test_wsearch_api.c, and the late-ack loss count in test_p2p_bwe.c.
QEMU: initrd_content/web/lot85.html, tests/scripts/test_qemu_lot85.py, `make qemu-lot85`, CI job lot85-layout.
ARM: arm_port_marks (U+1EA0 round trip, base-letter fallback, fullwidth A).
