# Lot 73 plan and result: animated GIF, constant-time AES, multi-column fragmentation, line clamp, inside list markers, small-caps, underline offset, srcset and picture, HTTP cache, inline box edges and box-decoration-break, GIF in the viewer, WebP lossless, form controls

Branch `cursor/lot73`, base `origin/main` d2ca116 (after PR #229, Lot 72).
One engine (`userspace/webeng.c`), freestanding i386 C in Ring 3, no new
syscall, no ABI change; one kernel source change for review (section 7).

## 1. Constant-time AES (cache timing)
- `include/os_aes_ct.h`: the AES S-box computed with the Boyar-Peralta
  circuit on bit planes (as BearSSL `aes_ct`): up to 32 bytes per pass are
  transposed into 8 planes of 32 bits, put through 113 AND/XOR/XNOR gates
  and transposed back. No table, no secret-dependent branch or index.
  `os_aes_ct_xtime` multiplies by x in GF(2^8) with a mask.
- `userspace/wf_aead.c` (TLS AES-128/256-GCM) and `kernel/aes_gcm.c`
  (kernel AES-128-GCM) drop their S-box tables: SubBytes and the key
  schedule use `os_aes_ct_sub16` / `os_aes_ct_sub4`. The kernel GHASH
  multiply uses masks instead of `if (bit)` (the user space GHASH was
  already masked).
- Proof: `tests/unit/userspace/test_aes_ct.c`: the 256 S-box values
  against a GF(2^8) inverse plus affine map reference, every lane of the
  16 and 4 byte passes, xtime; FIPS-197 C.1 and C.3, key schedule words
  (FIPS-197 A.1 and A.3), SP 800-38A ECB-AES256, GCM test case 14
  (AES-256) and the kernel GCM test cases 2 and 4 with a tampered tag
  refused (-3). The Lot 72 AEAD/TLS tests pass unchanged.

## 2. Animated GIF
- `userspace/imgdec_gif.inc` / `imgdec.h`: `imgdec_anim_t`,
  `imgdec_anim_need`, `imgdec_anim_begin`, `imgdec_anim_next`: one
  composed RGB canvas per frame, with the frame delay (cs), the disposal
  (0/1 keep, 2 background, 3 previous) and the NETSCAPE2.0 / ANIMEXTS1.0
  loop count (-1 none, 0 forever, N). The disposal of a frame is applied
  only when a next frame exists. A cut stream ends the animation after
  its last whole frame.
- `userspace/wf_webimg.c`: up to 4 animated images per section, 32
  frames each, 192 K pixels in all (`anim_cut` counts what does not
  fit); frames are scaled to the image box. `wf_webimg_tick(now)` (100 Hz
  ticks) copies the due frame into the image atlas slot and bumps the
  atlas generation: the existing `SYS_VGA_BLIT` 6 path sends it to the
  kernel, so the GUI needs no new call. Delays 0 and 1 cs play at 10 cs
  (as browsers); no loop extension plays once, loop N plays N + 1 times,
  then the last frame stays; after a long pause the next change is one
  delay after now (no burst).
- GUI: new last member `anim_tick` of `osui_web_page_ops_t`, called with
  the image sync of the web window. Shell: `web-anim` (advance, then one
  line per animation), `web-images` adds `animated`, `frames`,
  `anim_cut`.
- Fixtures: `scripts/gen_gif_anim_fixtures.py` (own LZW encoder and
  compositor model) writes `tests/unit/userspace/imgdec_anim_samples.h`;
  `--check` is part of `make font-check`.
- Proof: `test_imgdec_anim.c`, `test_wf_webimg_anim.c`, the GUI op test in
  `test_osui_gui.c`, `make qemu-lot73` (section 6).

## 3. Multi-column fragmentation
- A block child of a multi-column container made of lines only (no
  painted box, no block descendant, not `column-span: all`, not
  `break-inside: avoid`) is cut between its lines. `orphans` and
  `widows` (inherited, default 2, 1..99) keep the first and last lines
  together; a block with fewer lines than orphans + widows stays whole.
  The first fragment carries the top margin and `break-before`, the last
  one the bottom margin and `break-after`; the box node follows its first
  fragment.

## 4. Line clamp, flow-root, -webkit-box
- `-webkit-line-clamp: N` and `line-clamp: N` (1..99, or none): the block
  shows its first N line boxes (nested blocks included, in order), the
  items of later lines are dropped, the last run of line N ends with
  "..." (three ASCII dots, appended when they fit, else the run is cut),
  and the content height ends at line N. Diag "line clamp".
- `display: -webkit-box` and `flow-root` are blocks,
  `-webkit-inline-box` an inline block; `-webkit-box-orient` is accepted
  (no effect). Values out of range: "line clamp refused", "box orient
  refused".

## 5. Text and lists
- `list-style-position: inside` (and `inside` in `list-style`,
  inherited): the marker is painted at the start of the content box (the
  end in rtl) and the first line is indented by the marker and a space;
  an image marker or `none` takes no room.
- `font-variant` / `font-variant-caps: small-caps | all-small-caps`:
  with one fixed-width face, letters are painted as capitals of the same
  size (diag "small-caps painted uppercase"); an explicit
  `text-transform` wins; `normal` resets.
- `text-underline-offset: auto | 0..16px` (inherited) moves the
  underline down.

## 5b. Round 2 features
- srcset and `<picture>` (`userspace/wf_srcset.c`, `wf_html.c`):
  x and w descriptors (smallest density >= the device pixel ratio, else
  the largest; first wins ties; bad candidates skipped), `sizes` with
  media conditions (min-width / max-width, and, comma lists) in px, vw,
  em, rem; `<source type>` limited to the decoded formats (png, jpeg,
  gif, bmp, ico; `image/webp` is refused because most WebP files are
  lossy); `<source media>`. Viewport 640 px and ratio 1 by default
  (`wf_html_set_viewport`). Stats `images_srcset`, `sources_skipped`.
- HTTP cache (`userspace/wf_cache.c`, `wf_fetch.c`): plain GETs only, 16
  entries, 512 KiB pool (one body at most a quarter), LRU eviction.
  Cache-Control no-store / no-cache / max-age (quoted too), Pragma
  no-cache only without Cache-Control, ETag and Last-Modified; an answer
  with nothing reusable is not stored. A fresh entry is served without a
  request; a stale one sends If-None-Match / If-Modified-Since (dropped on
  a redirect); 304 serves the cached body as 200 and refreshes the entry,
  another status drops it. Shell `http-cache [on|off|clear]`, `fetch`
  prints "fetch cache fresh" / "fetch cache revalidated".
- Inline box edges and `box-decoration-break` (webeng.c): the horizontal
  margin, border and padding of an inline element take room before its
  first word and after its last; borders and the padded background paint
  per line fragment (vertical padding and borders paint outside the line
  box, CSS 2 10.6.1). `slice` (default): start edge on the first
  fragment, end edge on the last; `clone` (and `-webkit-`): every
  fragment gets both edges and the room for them. Words go left to right
  in rtl too (lot 64), so the start edge is the left one.
- Image viewer: a GIF of two frames or more plays (`osui_tool_anim_tick`
  from the desktop image sync, 1/100 s clock), delays under 2 cs as 10,
  NETSCAPE2.0 loop count (none: one play); space pauses, resumes or
  replays a finished one; zoom restarts it.
- WebP lossless (`userspace/imgdec_webp.inc`, RFC 9649): RIFF with VP8L
  alone or inside a still VP8X; predictor (14 modes), cross color,
  subtract green and color indexing (1/2/4/8 bit packing) transforms,
  color cache, meta prefix codes, LZ77 with the 120-entry distance map;
  RGBA out (composed over white on web pages). Lossy VP8 and animated
  files are refused with their size. Scratch: pixels + 256 KiB tables +
  sub-images. The viewer and web pages show .webp.
- Form controls painted static (`userspace/webeng_forms.inc`, included
  by webeng.c): input text / password (stars) / search / email / url /
  tel / number with value or grey placeholder and `size`, checkbox and
  radio ("x", "*" when checked), submit / reset / button, hidden;
  button; select (the first selected option, else the first, plus " v";
  `size` > 1 or `multiple`: a list box with the selected rows filled);
  textarea (rows, cols, pre-wrap, placeholder); progress and meter bars
  (value, min, max, low, high); details / summary ("> " closed, "v "
  open, a "Details" summary when missing; closed content hidden);
  fieldset (groove) and legend; `disabled` greys. UA styles first, then
  author CSS. No input or submission (no event loop for pages); unknown
  input types show as text ("input type taken as text").
- CSS conditional rules and selectors (`userspace/webeng_cssq.inc`):
  `@media` (types all / screen / print / speech, not, only, comma lists,
  and / or / not conditions, width / height with min- / max- and the
  range forms, orientation, aspect-ratio, resolution, color, monochrome,
  prefers-color-scheme light, prefers-reduced-motion no-preference,
  hover, pointer fine; px / em / rem; the media size is the layout width
  or `we_set_media`, 640 x 480 when unset), `@supports` (a declaration is
  supported when the engine applies it with no diag on a scratch node,
  every side effect undone; `selector()`), `@layer` blocks kept in source
  order (one diag: layer priority not modelled), nesting up to 4 levels.
  Selectors: `:not()`, `:is()` / `:matches()` / `:-webkit-any()`,
  `:where()` (specificity rules), `:has()` with relative selectors (a
  simplification: an argument chain may match outside the element),
  `:link`, `:any-link`, `:checked`, `:disabled`, `:enabled`,
  `:placeholder-shown`, `:indeterminate`, `:open`, the `*-of-type`
  forms; `:hover`, `:focus`, `:visited`... parse and never match.
  Custom properties `--x` (inherited, 12 per element, resolved where
  declared, cycles invalid) and `var()` with fallbacks.
- HTML presentational hints (`userspace/webeng_hints.inc`, HTML 15.3):
  width, height, align, valign (rows and groups pass it to cells),
  bgcolor, text, font color / size, border (table outset grey, cells 1px
  inset), cellpadding, cellspacing, nowrap, hspace, vspace, hidden,
  `<center>`; author level before every rule. More UA styles: dl, dd,
  figure, address, menu / dir, mark, sub / sup, abbr[title], xmp.
- Tables: `<caption>` above or below (`caption-side`, caption
  align=top|bottom), the first thead first and the first tfoot last, col
  and colgroup widths as column minimums.
- `vertical-align: sub | super | <length>` shifts inline text (the line
  box does not grow).

## 6. Tests and proofs
- Unit: `test_aes_ct.c` (8), `test_imgdec_anim.c` (8),
  `test_wf_webimg_anim.c` (8), `test_osui_gui.c` (+1),
  `webeng_lot73.inc` (15, in `test_webeng.c`).
- Round 2: `test_wf_srcset.c` (8), `test_wf_cache.c` (6),
  `webeng_lot73.inc` (+10: inline edges, box-decoration-break, forms),
  `test_osui_tools.c` (+5: viewer GIF, WebP), `test_imgdec_webp.c` (12:
  Pillow fixtures compared byte for byte with libwebp's decode, plus
  crafted streams for code lengths, distances, the color cache, the
  half predictor, color indexing and chunk padding).
- `webeng_lot73_cssq.inc` (14) and `webeng_lot73_hints.inc` (7) in
  `test_webeng.c`.
- Lot 57 / 58 / 59 / 63 cases that asserted the refusal of `:hover`,
  `:not(.x)` and `var()` now assert the new behaviour (`:not` matches,
  `:hover` parses and never matches, an undefined `var()` gives "css
  variable undefined"); review point.
- `parse_sel` now clears `pseudo_el` of a reused rule slot (a refused
  `::before` selector could leave it set).
- One Lot 64 case (an inline span with padding and border and a
  content-box background origin) moved: since the inline edges take room
  its text and image start 5 px right; its left border is now asserted.
- One Lot 62 list refusal case (`list-style: inside square`, now valid)
  became `list-style: middle square` (still refused); same count of
  refusals asserted.
- `make qemu-lot73`: the Lot 67 test server serves `/lot73/anim.html`
  (the 3-frame looping GIF and a 2-frame GIF with loop 1). Console:
  `web-images` reports 2 animated images and 5 frames; `web-anim` lists
  both; after eight ticks the frames shown and the plays of the looping
  image grow and the loop-1 image is stopped. GUI: screenshots of the web
  window see red, green and blue frames (blue is only frame 3 of the
  looping image). `web-render /web/lot73.html`: 2 clamped lines ending
  with "...", the inside marker 16 px left of its text, SMALL CAPS, the
  paragraph cut between two columns, EDGE 40 px after "pre " (inline
  edges), the form controls and a closed details. Round 2: the picture
  page (gif source and 1x candidate fetched, never.gif never, WebP
  shown, 3 images) and the HTTP cache (max-age hit with one GET on the
  server; ETag revalidation answered 200 then 304). CI job `lot73-anim`.
- Mutations: every new branch has a mutant killed by a host test (list
  in the delivery notes).

## 7. Review points
- `kernel/aes_gcm.c` changed (kernel source, no ABI change): S-box table
  replaced by `os_aes_ct.h`, GHASH masked. Same results (kernel GCM known
  answers in `test_aes_ct.c`).
- `osui_web_page_ops_t` gains a last member `anim_tick` (Ring 3 struct
  between the GUI and the shell, not a syscall ABI).
- Inline elements with horizontal margin, border or padding now take
  that room (they were ignored silently): pages with padded links or
  badges lay out a few pixels wider, as in browsers.
- Form controls now paint (they were dropped with their attributes);
  pages with forms grow by the control boxes.
- Pages with `@media` rules now apply the matching blocks (they were all
  skipped); presentational attributes now take effect: old table layouts
  change.
- `WE_MAX_RULES` 48 -> 64.
- `we_node_t` grows by 36 bytes (forms, box-decoration-break);
  `we_doc_t` stays under 1 MiB (static assert).
- Bitsliced AES costs more cycles per block than the table (about 8
  S-box passes per block instead of 16 lookups, each pass 113 gates on
  32-bit words); TLS throughput in QEMU stays well inside the timeouts.

## 8. Partial user stories, first remaining gap
- US-031 partial: animated images and more CSS in the one engine;
  `phase3_complete` stays false. Roadmap statuses unchanged.
- First remaining gap: no JavaScript, so pages that build their content
  with scripts stay empty; form controls paint but take no input inside
  the page (the Lot 71 `web-forms` / `web-input` commands still fill and
  submit forms); WebP lossy (VP8) and animated WebP are refused.
