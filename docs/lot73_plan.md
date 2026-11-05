# Lot 73 plan and result: animated GIF, constant-time AES, multi-column fragmentation, line clamp, inside list markers, small-caps, underline offset

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

## 6. Tests and proofs
- Unit: `test_aes_ct.c` (8), `test_imgdec_anim.c` (8),
  `test_wf_webimg_anim.c` (8), `test_osui_gui.c` (+1),
  `webeng_lot73.inc` (15, in `test_webeng.c`).
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
  paragraph cut between two columns. CI job `lot73-anim`.
- Mutations: every new branch has a mutant killed by a host test (list
  in the delivery notes).

## 7. Review points
- `kernel/aes_gcm.c` changed (kernel source, no ABI change): S-box table
  replaced by `os_aes_ct.h`, GHASH masked. Same results (kernel GCM known
  answers in `test_aes_ct.c`).
- `osui_web_page_ops_t` gains a last member `anim_tick` (Ring 3 struct
  between the GUI and the shell, not a syscall ABI).
- Bitsliced AES costs more cycles per block than the table (about 8
  S-box passes per block instead of 16 lookups, each pass 113 gates on
  32-bit words); TLS throughput in QEMU stays well inside the timeouts.

## 8. Partial user stories, first remaining gap
- US-031 partial: animated images and more CSS in the one engine;
  `phase3_complete` stays false. Roadmap statuses unchanged.
- First remaining gap: no JavaScript, so pages that build their content
  with scripts stay empty; next in the engine: `box-decoration-break`
  for fragmented painted boxes, `srcset` / `<picture>` selection, an
  HTTP cache (ETag, max-age), animation in the image viewer.
