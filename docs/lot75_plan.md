# Lot 75 plan and result: reader engine forms, type=file, multipart/form-data,
# form= / formaction, caret-color, user-select, pointer-events, scroll-margin

Base origin/main fd8c954 (lot 74), branch cursor/lot75. Engine only in
`userspace/webeng.c` and its `.inc` files (plus the reader `wf_html` /
`wf_form` / `wf_section` path for fetched pages), freestanding i386 C in
Ring 3, no kernel / rights / ABI change except the userspace page_ops
`request` ctype argument (review point), we_doc_t under 1 MiB, ASCII only.

## 1. Reader-view engine forms
- `wf_html.c` emits real `<form action=ABS method=...>` and field
  placeholders `\001N\002` instead of `mohhdy-form:` links.
- `wf_form_emit_html` expands placeholders to `<input>` / `<textarea>` /
  `<select>` / `<button>` with live values.
- Absolute actions resolve against the page URL so GET/POST hit remotes
  through the existing fetch path (host tests use loopback, no DNS).

## 2. type=file and multipart/form-data
- `WE_FORM_FILE`, `we_form_set_file` (absolute VFS path, refuses `..` and
  URLs), file parts read via the image/VFS reader.
- `we_form_request` builds a real multipart body with a generated
  boundary; `WE_FFILE_MAX` size limit; empty file fields send
  `filename=""`.
- `shell_page_request` / GUI ops take Content-Type.

## 3. form= / formaction / formenctype
- `form=id` associates a control with a named form.
- Submitter `formaction` / `formenctype` / `formmethod` override the form.

## 4. CSS
- `caret-color`, `user-select`, `pointer-events` (none skips hit testing),
  `scroll-margin` / `scroll-padding` (shorthand and longhands).

## Tests
- Unit: `webeng_lot75_forms.inc`, `webeng_lot75_more.inc`,
  `webeng_lot75_ext.inc`, `webeng_lot75_sys.inc`, `test_wf_lot75.c`.
- QEMU: `make qemu-lot75` (reader `/form.html`, local `/web/lot75.html`).
- Mutations: see `/workspace/run/lot75/mutations.txt`.

## Review points
1. type=file kept in wf_form (was refused); urlencoded submit skips file.
2. multipart builds a real body (was sent urlencoded).
3. Reader `mohhdy-form:` links replaced by engine form markup.
4. `osui_web_page_ops_t.request` and `shell_page_request` take ctype.
5. `WE_FREQ_BODY` 1024 -> 8192.
6. `formaction` reuses the list= pool span on submitters (no list= there).

## First gap
Further CSS/HTML still partial (US-031 remains partial, phase3_complete
false): no JS VM, no Chromium/WebKit, no new font engine, no margin
collapsing. Next candidates include richer file UX, `formtarget`, and
more layout.
