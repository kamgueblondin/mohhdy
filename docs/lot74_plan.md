# Lot 74 plan and result: interactive forms, GET/POST submission, @layer, rowspan in collapsed tables, anchored :has(), fragments and :target, number/range inputs, position:sticky

Branch `cursor/lot74`, base `origin/main` 8c329ac. One engine
(`userspace/webeng.c` and its `.inc` files), freestanding i386 C in Ring 3,
no kernel change, no new syscall, no ABI change.

## 1. Interactive forms (engine)
- `webeng_forms.inc`, `webeng_finput.inc`: focus (click, Tab, Shift-Tab,
  tabindex, autofocus), typing with a caret, Backspace, checkbox and radio
  groups, select cycling, reset, live `:focus`, `:checked`, `:valid`,
  `:invalid`, `:placeholder-shown` states (`webeng_cssq.inc`).
- Validation: required, minlength/maxlength, pattern-free type checks
  (email, url, number with min/max/step, step=any).
- Submission: GET builds `action?name=value&...` (urlencoded, form order);
  POST builds an `application/x-www-form-urlencoded` body. A refused
  (invalid) submit focuses the first invalid control.
- number and range: arrows step and snap to the step base; range draws a
  track with a `::fill` child and a click sets the value.

## 2. GUI and shell glue
- `osui_gui.c`: click and key routing to the focused control, console
  commands `web-fclick X Y`, `web-fkey KEY`, `web-ftype TEXT`, `web-fstate`.
- `shell_fetch.c`: `shell_page_request` sends the POST through the existing
  fetch path (`page_ops.request`). Command registry regenerated (481).

## 3. @layer
- Named layers (`@layer a { }`, `@layer a, b;`, nested `a.b`), up to 16,
  post-order ranking; unlayered rules win over layered ones, and the order
  is reversed for `!important`. A full table gives the diag
  `css layer table full`.

## 4. rowspan with border-collapse
- `rowspan=0` spans to the end of its row group; a span never leaves its
  `thead`/`tbody`/`tfoot` (diag `table rowspan clamped to its group`).
  Row backgrounds are painted per run, skipping cells spanned from above.

## 5. :has()
- Relative selectors anchored at the subject: `:has(> a)`, `:has(+ a)`,
  `:has(~ a)` and descendants, no longer matched from the root.

## 6. Fragments and :target
- `we_set_target` (percent-decoded id), `:target`, `:target-within`; the
  GUI scrolls to the target, keeps `#frag` on local links and does not
  reload the same page.

## 7. position:sticky
- Top and bottom sticky boxes kept inside their containing block while
  scrolling (`sticky_apply` from `we_scroll` and at the end of layout).

## 8. Later Lot 74 additions
- `@font-face` refused with diag `font face refused`.
- `table-layout:fixed` (first row / col widths).
- `pattern=` validation (subset regex with `*+?{}` classes and escapes).
- `dialog` with `open`, click toggle, `method=dialog` / `formmethod=dialog`.
- `overflow:auto/scroll` vertical scrollbar + `we_box_scroll_click`.
- `iframe srcdoc` nested fragment (no network `src`).
- CSS nesting (`&` and descendant), `accent-color`, `appearance:none`.

## 9. Proof
- Unit tests: `tests/unit/userspace/webeng_lot74_forms.inc`,
  `webeng_lot74_forms2.inc`, `webeng_lot74_css.inc`, GUI tests in
  `test_osui_gui.c`.
- QEMU: `make qemu-lot74` (`tests/scripts/test_qemu_lot74.py`,
  `initrd_content/web/lot74.html`): click, typing, Tab, checkbox, number
  step, GET seen by a host echo server, POST refused while required is
  empty then sent urlencoded, @layer, :has, rowspan, fragment scroll.
  CI job `lot74-forms`.

## 10. Review points
- The Lot 73 assertion on the diag `css layer order ignored` now expects 0
  (layers are honoured).
- `"0"` left the Lot 56 list of bad rowspan values (rowspan=0 is valid).
- `:target` now matches; `@supports (position: sticky)` is now true.
- Engine POST works on local pages (also with an absolute http action);
  fetched http pages stay in the Lot 71 reader view, whose forms are links.

## 11. First gap
- Forms on fetched (reader-view) http pages do not use the engine forms;
  no file input, no multipart/form-data.
