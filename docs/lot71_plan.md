# Lot 71 plan and result: HTML forms, Latin-1 text, long pages, full-screen window, chat width, search fallbacks, retransmission

Branch `cursor/lot71`, base `origin/main` after PR #227 (Lot 70).

## 1. HTML forms (shared shell and GUI path)
- `userspace/wf_form.c`: forms and fields (text, search, hidden,
  password, textarea, select, checkbox, radio, submit) are kept from the
  reduced page. GET forms build the query string, POST forms send
  `application/x-www-form-urlencoded` through `wf_fetch_req_t`; a 303
  after a POST is followed with GET.
- The form is encoded in the page charset: a Latin-1 page sends
  `caf%E9`, a UTF-8 page sends `caf%C3%A9` (HTML rule).
- Console: `web-open URL`, `web-forms`, `web-input N TEXT`,
  `web-submit [N]`, `web-section N|next|prev`. GUI: a click on a field
  focuses it, keys go to the field, Enter submits, ESC leaves the field.
  Both call `osui_gui_web_command`, one code path; the page state lives in
  `userspace/shell_fetch.c` (`osui_web_page_ops_t`).
- Checkboxes, radios and selects are real controls: `[x] label`,
  `(*) label`, `[ option v ]`. A click toggles the checkbox, checks the
  radio (its group is unchecked), moves the select to the next option.
  Keyboard: Tab reaches them too, Space toggles, LEFT/RIGHT/HOME/END
  and a typed letter move a select (`> ` marks the focused control).
  Console: `web-input N on|off`, `web-input N LABEL|VALUE|#K`; `web-forms`
  lists the options. Unchecked boxes are not sent (HTML rule). Bounded:
  24 options per select, a 4 KB option pool per page.
- google.com: the form is found and submitted correctly (GET, `ie=ISO-8859-1`,
  `q=caf%E9`, status 200). Google then answers with a page that needs
  JavaScript (`enablejs`), so there are no results to show. Mohhdy
  reports `blocked js-required`. This is a Google limit, not a form bug.

## 1b. Cookies
- `userspace/wf_cookie.c`: a bounded jar (24 cookies, RFC 6265 subset):
  Domain and Path matching, host-only cookies, Secure only over https,
  Max-Age=0 or a past Expires deletes. A Domain that does not match the
  host, or a top-level domain, is refused. Stored and sent on every hop,
  so a login answer `302 + Set-Cookie` keeps its session after the
  redirect. `fetch`, the browser, forms and search share one jar.
- The jar is in memory for the shell session only, never written to
  `/persist`. `cookies` lists domain, path, name and value length (never
  the value); `cookies clear [DOMAIN]`, `cookies off|on`.
- Honest limits: SameSite and HttpOnly are stored, not enforced (there
  is no script and every request is a top-level navigation); no public
  suffix list beyond "a dot inside"; a future Expires is a session cookie.
- Tests: `test_wf_cookie.c`, three host checks on `/cookie/login`, and the
  console part of `make qemu-lot71`.

## 2. Charset, entities, Latin-1 glyphs
- Display bytes are CP1252 (`include/os_charset.h`). The fonts have
  glyphs for 0x80..0xFE.
- The Content-Type charset wins over the meta tag (`wf_charset_header`).
  Google sends ISO-8859-1 bytes with a UTF-8 meta tag; this used to give `?`.
- `userspace/webeng.c` keeps display bytes and numeric references to
  Latin-1/CP1252 (`&#233;`, `&#8364;`) instead of `?`. A code point
  without a glyph (emoji) is still `?`. One older expectation changed on
  purpose (`webeng_lot63_text.inc`: `&#xe9;` is now `e acute`).
- Console output is UTF-8 again (`wf_html_text_display`).

## 3. Long pages
- The reduced page (up to 192 KB) is cut into sections (`wf_section.c`)
  that each fit the engine tables (`WE_MAX_ITEMS`, `WE_POOL`). DOWN/UP
  past the end of a section opens the next/previous one. Proof:
  `/long.html` (1500 paragraphs, 26 sections) is read to the end in QEMU.
- `web-find TEXT` searches the whole page (tags skipped, `&amp;` and
  friends decoded, ASCII and Latin-1 case folded, across inline tags) and
  opens the section of the next match; `web-find` alone goes to the next
  one, wrapping. Tests: `test_find_across_sections`, QEMU long page.
- Honest limit: one section is laid out at a time; a CSS box that spans
  two sections is cut.

## 4. Full-screen web/editor window
- `kernel/gfx_desktop.c`: the full window is a WM window
  (`g_wm_full`), maximized by default. The green button restores it to
  3/4 of the desktop, centered; the title bar moves it, the grip resizes
  it, the green button maximizes it again. The page is laid out for the
  new width.
- Proof: unit `test_lot71_full_window_restore_resize`, QEMU tablet clicks in
  `make qemu-lot71`.

## 5. Chat width when a pane is resized
- Root cause: the chat width was computed from the pane's current box,
  so it changed when the pane was dragged bigger. It now comes from the
  pane's default place (`pane_win.base`).
- Proof: unit `test_lot71_chat_width_stable_on_pane_resize`, QEMU pixel
  check (chat body run 212 px before and after the pane grew by 120 px).

## 6. Search despite the bot checks
- `search-config`: `searx URL`, `json on|off`, `key brave KEY`,
  `key google KEY CX`, `wiki LANG`, `clear`. Saved in `/persist/search.cfg`;
  keys are shown masked and never logged.
- Fallback chain: configured engine, DuckDuckGo html, DuckDuckGo lite,
  Google html, Wikipedia API. HTTP 401/403/429 or a bot check moves to the
  next one; the status line says `tried ...`.
- Measured from the build box (2026-11): DuckDuckGo answers 202 (bot
  check), public SearXNG instances answer with a bot check or 429, Mojeek
  shows a captcha, Google needs JavaScript. The Wikipedia API answers, so
  without configuration `search` ends with Wikipedia results. A private
  SearXNG or a Brave/Google API key must be set by the user.
- Offline proof: the test server's `/searx/search` (html and json) in
  `make qemu-lot71` and `tests/scripts/test_lot67_web_host.sh`.

## 7. Partial user stories
- US-068/US-073 partial: lost user data is now resent.
  `userspace/p2p_rtx.c` keeps a bounded copy (8 per peer, 256 bytes) of
  each message or KV put until its DACK. When `p2p_flow` declares it lost,
  it leaves again as an RTX frame under a new sealed counter, at most 3
  times, then it is given up. The receiver delivers an original once (a
  256-counter window), so a lost ack does not duplicate a message.
  `p2p-flow` shows `rtx kept .. resent .. gave_up .. dup_rx ..`. Tests:
  `test_p2p_rtx.c` and `test_lot71_retransmission_and_dedup` in `test_p2p.c`
  (lost datagram, lost ack, peer cut for good). No congestion window
  beyond the Lot 69 pacer; bodies over 256 bytes are not kept.
- US-076 partial: the ARM boot self-tests add `p2p_rtx.c` and
  `os_charset.h`, compiled unchanged (CI `arm-port`).
- US-031 partial: Latin-1 text in the web engine (section 2).
- US-080, US-081: unchanged in this lot. The battery stays simule.
- US-078/US-097: still simule, not advanced.

## 8. Tests and proofs
- `make test-all`, the host web test (`tests/scripts/test_lot67_web_host.sh`,
  35 checks) and `make qemu-lot71` (CI job `qemu-lot71`).
- Screenshots: `/workspace/lot71-shots` (form page, typed field, answer,
  Latin-1 page, full window restored/moved/resized/maximized, chat before
  and after the pane resize).
