# Lot 86 plan - page 5 letters, browser session, settings, measured pages

Base: main 86a1080 (lot 85, PR #243). One guest engine. Kernel rights
unchanged. US-031 stays partial. phase3_complete stays false.
US-068/US-073 stay partial. US-076 stays partial. US-080 and US-081 stay
partial. US-078 and US-097 stay simule.

## Scope

| Id | Feature | Before | After |
|----|---------|--------|-------|
| F1 | 16 Vietnamese double marks | base letter, no free glyph byte | page 5, switched by page-1 byte 0xFF (draws nothing). 72 more Latin letters share that page. Fonts unchanged |
| F2 | Ligatures | '?' or one letter | ff fi fl ffi ffl st expand in the display text |
| F3 | History and bookmarks | none | session stack, visit ring, bookmarks, text save |
| F4 | Tabs | one page | eight tabs, same URL focuses the open one |
| F5 | Cookies | memory for the session | Expires and Max-Age use the guest clock; cookies save/load |
| F6 | HTTP cache | max-age, validators | Age counts against freshness; Vary * is not stored; other Vary forces revalidation; cache save/load of small bodies |
| F7 | Network switch | always on | settings net off makes fetch fail with "network off" before a socket |
| F8 | Settings | the window was read-only | scale, net and search engine, console and /settings buttons, one path |
| F9 | Reader tables | cells became paragraphs | web-read also prints an aligned table, a dl, details and a figure |
| F10 | Autofill | stored, not offered | web-fill lists and applies a remembered value in the form editor. Password and hidden are sent and not stored. The file is /persist/autofill.txt |
| F11 | Chrome | suite only | qemu-lot86 drag-resizes the full-screen web window and the editor with the tablet, then closes each |
| F12 | US-068/073 | one outstanding loss is 7/8 | eight open losses also cut a quarter. Still a lab estimator |
| F13 | US-080 | boot EDID extras | week, binary serial, serial text, white point, every detailed timing. Not DDC |
| F14 | US-081 | idle halves refresh | a second idle period quarters it. Battery stays simulated |
| F15 | Live pages | 2026-11-09 lot 85 | measured again the same day: google gbv=1 is 92874 bytes and still enablejs; Wikipedia API returned Harbor; example.com 577 bytes; HN 33820 bytes with a stories table |
| F16 | Form submit | search forms only | web-preview prints the GET URL or the POST body. web-load opens a saved page against a base URL. The body is parsed back as form-urlencoded pairs |
| F17 | Reader CSS | the reader ignored style | type, class, id, attribute, descendant, child, sibling, :first-child, :last-child, :nth-child(an+b), :link, :not(type). Cascade is user-agent, author, inline, then !important. display:none skips a subtree. color, weight, align, margin and padding are copied onto the kept tag. @media min-width and max-width use a viewport. No external stylesheet, no JavaScript |
| F18 | US-080 | week, serial, white, timings | also the extension count (byte 126, not parsed) and the best mode (preferred timing, else the largest established mode). Still not DDC |
| F19 | US-076 | U+1EB2 | also U+1EA8, the first page-5 byte |
| F20 | Border cap | lot 84 was 8 px | lot 85 already raised it to 64 px (WE_BORDER_CAP). Lot 86 leaves that cap |

## Review points

1. Page 5 is not a sixth selector. Page 1 byte 0xFF was unused. It switches the page and takes no cell. Byte 0xFF on page 0 is still y-umlaut.
2. Cookie and cache files are operator files under /persist. Values are in the cookie file. Cache bodies over 1536 bytes stay in memory only.
3. settings net off does not power the NIC down. It refuses wf_fetch.
4. The compat fixtures in the repo are the shape of the live pages, not the full Google script. The byte counts above are the live measurements.
5. we_node_t is unchanged. No JavaScript VM.
6. Site CSS in the reader is off for a normal fetch. web-load turns it on for that page. web-css on turns it on for the next fetch. A large sheet can change which nodes fit, so it is not the default.
7. The reader sheet is not the layout engine. The engine already has its own cascade. The reader copies a few computed properties into a style attribute so the engine paints them.

## Not done (first gaps for lot 87)

1. Google's HTML page, measured 2026-11-09, is still a script wall. There is no JavaScript VM. External stylesheets are not fetched.
2. Page 5 stops at the free bytes after 0xDC. Letters that do not decompose to a mark we paint stay '?' or the base letter.
3. US-068/US-073 remain a laboratory estimator. No WAN, no cross traffic.
4. US-080 still reads the EDID block published at boot. DDC/I2C is not driven.
5. The battery stays simulated. Deep idle does not blank a real panel.
6. Autofill is offered by web-fill. The form editor does not pop a list up by itself when a field takes the focus.

## Tests

Hand-written: tests/unit/userspace/test_lot86.c, test_lot86b.c, the page-5 checks in test_glyph_pages.c, the chrome suite in test_gfx_wm.c, the week/serial/white check in test_edid.c, the deep-idle check in test_plat_form_energy.c, the eight-loss check in test_p2p_bwe.c.
QEMU: initrd_content/web/lot86.html, tests/scripts/test_qemu_lot86.py, `make qemu-lot86`, CI job lot86-layout.
ARM: arm_port_marks now round-trips U+1EB2 on page 5.
