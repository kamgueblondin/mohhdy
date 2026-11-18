# Lot 114 plan - coverage job under 15 minutes, generators, async/await and Promise, external scripts, fetch and XMLHttpRequest, events after load and approximate layout values in the page pass (US-031)

Base: origin/main 9caa669 after PR 271 (the Lot 113 branch) was merged.
Branch lot114-js-external. All new guest code is freestanding C in Ring 3
(the shell process): no libc, no FPU, no 64-bit division. Kernel, rights
and ABI unchanged: no new syscall, no new guest program, no change under
kernel/, include/ or fs/. Test programs stay in my_initrd_test.tar (this
lot adds none). US-031 stays partial. phase3_complete stays false. US-039
(the page sanitizer) is unchanged and still runs after the script pass.
US-068/US-073, US-076, US-080 and US-081 stay partial. US-078 and US-097
stay simule.

A row is listed only when its code, tests and docs are in the branch.

## Scope

| Id | Feature | Files | Proof |
|----|---------|-------|-------|
| A | CI: the "Host unit-test coverage (gcov floor)" job took 23m36s on PR 271. Root causes: the host tests ran one after another (657 s locally), gcov ran once per object with every .gcda again, and the job also booted the phase 6 platform image. Fix: run_all_tests.sh runs the tests of a category in parallel (TEST_JOBS, default nproc) and prints them in discovery order (same output, counts and exit code); gcov_host_summary.py runs one gcov per .gcda in a process pool and merges in a fixed order (same 91.77 %, 189426/206409 lines); the QEMU platform boot moved to its own job (platform-groundwork). The floor and the test list are unchanged | tests/scripts/run_all_tests.sh, scripts/gcov_host_summary.py, .github/workflows/ci.yml | local: test-all 141 s (was 657 s), coverage 221 s; CI times in the PR |
| B | generators: function*, yield, yield*, next/return/throw, for-of and spread over generators and iterator objects (an object with next), closed on early exit; each generator runs on its own coroutine stack carved from the arena (i386 stack switch in wf_js_co_switch) | userspace/wf_jsco.c, wf_jsparse.c, wf_jsrun.c, wf_jslib.c, wf_jsi.h | test_lot114_async.c (k_gen, 24 cases equal to node v24) |
| C | async functions and await (function, arrow, method), run synchronously to the first await and resumed by a Promise job | userspace/wf_jsco.c, wf_jsparse.c, wf_jsrun.c | test_lot114_async.c (k_async, 20 cases equal to node v24) |
| D | Promise: constructor, then/catch/finally, resolve/reject, all, allSettled, any (AggregateError), race, thenables, queueMicrotask; a FIFO microtask queue drained after every run; unhandled rejections reported once ("Uncaught (in promise) ...") | userspace/wf_jsco.c, wf_jslib.c (js_run_jobs) | test_lot114_async.c (k_promise, 30 cases equal to node v24; bounds) |
| E | external scripts (src=) through a fetch hook of the page pass: in document order, document.write and currentScript work in them, defer and async ones after parsing (readyState "interactive"), load / error events on the script element, only http(s), status 200, at most WF_DOM_EXT_SCRIPTS (16) of WF_DOM_EXT_BYTES (256 KiB); scripts created with createElement run once inserted (after the current script returns); innerHTML scripts never run | userspace/wf_jsdom.c, wf_jsdom.h, shell_fetch.c (js_fetch_hook on wf_guest_fetch_ex; a web-load page reads them from its directory) | test_lot114_page.c (external_order, external_errors, external_bound_and_no_hook, dynamic_scripts), `make qemu-lot114` |
| F | Promise jobs in the page: drained after every script, callback, event and timer; a timer resolved promise (await new Promise(r => setTimeout(r, 10))) works on the virtual clock; setTimeout passes its extra arguments; the page's queueMicrotask is the real one (it was a 0 ms timer in Lot 113) | userspace/wf_jsdom.c | test_lot114_page.c (microtasks_in_page) |
| G | fetch(), Response (status, ok, statusText, url, headers, text, json, clone), Headers (get, has, set, append, delete, forEach), XMLHttpRequest (open, send, setRequestHeader, readyState, status, responseText, response with responseType json, getResponseHeader, onload/onerror/onreadystatechange/onloadend, addEventListener, synchronous mode): same origin only, GET and POST, WF_DOM_NET_REQS (16) requests per pass, WF_DOM_NET_BYTES (256 KiB) per answer, answers delivered as tasks after the current script | userspace/wf_jsnet.c, wf_jsnet.h | test_lot114_page.c (fetch, xhr, net_bound), `make qemu-lot114` |
| H | on* attributes (onclick, onload, onerror...) compiled into handlers when they first fire (this: the element, event: the argument); a syntax error drops only that handler; setAttribute of an on* attribute replaces it | userspace/wf_jsdom.c | test_lot114_page.c (handler_attrs) |
| I | approximate layout values: offsetWidth/Height/Top/Left, offsetParent, clientWidth/Height, scrollWidth/Height, getBoundingClientRect from a block model (8x16 px characters, h1 16x32, h2 12x24, 1024x768 viewport, body margin 8, list indent 40, inline-block controls with fixed sizes, style width/height in px, display none/block/inline/inline-block), recomputed after a DOM change | userspace/wf_jsdom.c | test_lot114_page.c (layout), `make qemu-lot114` |
| J | events after load: the realm stays alive after the pass; wf_jspage_event dispatches click (MouseEvent fields, checkbox and radio toggling, submit of the button's form), key (keydown, keypress, keyup with key/code/keyCode, the character typed into a focused text field, Backspace, Enter submits), input (value, input, change), focus/blur (activeElement, focusin/focusout), submit and any other type, then runs Promise jobs, timers and inserted scripts, and writes the page again; el.focus()/blur() and document.activeElement | userspace/wf_jsdom.c | test_lot114_page.c (live_click, live_forms, live_timers_and_custom, focus_api) |
| K | one shell/GUI path for J: shell_page_js (shell_fetch.c) takes "web-js click/key/input/focus/blur/submit/event/clicks" from the console and the GUI input line (osui_gui_web_command), the GUI's click on a "?__wfjs=N" link (the serializer wraps every element with a click handler in such a link) and the keys the page view does not use ("web-js gkey", only when the page listens to keys); the page written again goes through the same reader, sanitizer and forms without a new script pass; a new page ends the live realm | userspace/shell_fetch.c, osui_gui.c, osui_gui.h (page_js), shell.c, wf_jsweb.c, wf_jsweb.h | test_lot113_dom.c (commands), `make qemu-lot114` |
| M | the reader's own controls reach the scripts: field-input / check / change / focus / blur / submit with the target "=ORD,NTH,NAME" (control NTH of those named NAME, else control ORD in document order as webeng_finput.inc counts them); the GUI's engine forms (a click, typing, Enter, a box, a select: web_js_fields) and the console forms of Lot 71 (web-input, keys in a focused field, web-submit: "web-js rfield set/submit N") send them through shell_page_js; change fires on blur and before submit after an edit; a prevented submit is not sent, a prevented box click keeps its old state, a value a script gives the field goes back into the control; the page is written again only when a script changed the DOM; select.value / selectedIndex move the selected option and el.form is the owner form | userspace/wf_jsdom.c, wf_jsdom.h, shell_fetch.c, osui_gui.c, wf_jsweb.c | test_lot114_page.c (select_and_form, reader_fields), `make qemu-lot114` (web-fclick, web-ftype, Enter, web-input, web-submit) |
| N | web APIs of the page realm: URL (WHATWG parser for the common cases: special and other schemes, relative references, dot segments, default ports, percent-encoding; href/protocol/username/password/host/hostname/port/pathname/search/hash setters, origin, searchParams, toJSON, URL.canParse), URLSearchParams (string, object and pairs input, append/delete/get/getAll/has/set/sort/forEach/size/toString, live link to its URL), FormData (from a form: the successful controls in document order; append/set/get/getAll/has/delete/forEach), fetch and XMLHttpRequest bodies from URLSearchParams (urlencoded) and FormData (multipart/form-data), localStorage and sessionStorage per origin across pages (QuotaExceededError, SecurityError on file:), AbortController / AbortSignal (abort, reason, aborted, onabort and listeners, throwIfAborted, AbortSignal.abort, AbortSignal.any, a fetch signal), DOMException (name, message, code) | userspace/wf_jsapi.c, wf_jsapi.h, wf_jsnet.c, wf_jsdom.c | test_lot114_api.c (38 cases equal to node v24, formdata_from_form, bodies_and_abort, storage) |
| O | the reader (web-read, wf_article.c) wrote character references raw ("&amp;"), visible once a script set textContent to text with an ampersand: ASCII references are now written as characters, as wf_section.c does for the sections (other characters are already display bytes) | userspace/wf_article.c | test_wf_article.c (references_decoded), `make qemu-lot114` ("api114 ?x=1 true q=a+b&n=1") |
| L | QEMU page /lot114/app.html with lib114.js, late114.js, data114.json and a POST echo; `make qemu-lot114`; CI job lot114-js-page | scripts/lot67_web_server.py, tests/scripts/test_qemu_lot114.py, Makefile, .github/workflows/ci.yml | CI |

## Not covered (honest)

- Async generators and for await (refused at parse time), Symbol and the
  iterator protocol by Symbol.iterator (for-of takes arrays, strings,
  Map, Set, generators and objects with a next method), Proxy, Reflect,
  WeakMap, typed arrays, BigInt, private class members, eval and new
  Function stay refused.
- The network of a page is synchronous inside: fetch() and send() wait
  for the guest fetch, only the delivery of the answer is a task. No
  CORS: a cross-origin fetch or XMLHttpRequest fails (logged); external
  scripts may come from any http(s) origin, as in browsers. No request
  headers other than Content-Type, no cookies option, no streams, Blob,
  ArrayBuffer or File bodies (FormData holds strings only), no WebSocket, no
  Worker, no service worker. A redirected answer keeps redirected=false.
- Async scripts run with the deferred ones after parsing, in document
  order (browsers run them when they arrive). Scripts inserted with
  createElement run after the current script or callback (browsers run an
  inline one at insertion).
- Layout values are an approximation of a block model, not the
  renderer's layout (webeng.c is not asked): no margins or paddings other
  than the body margin and list indent, no borders, floats, flex, grid,
  table layout, line breaking by font metrics, images without width and
  height are 0x0, nothing scrolls (scrollTop 0). The values are stable
  and documented in wf_jsdom.c so scripts that only compare or test for
  non-zero sizes work.
- Events after load come from "web-js" lines, from GUI clicks on the
  marked links and from the reader's controls (row M). A reader field
  sends input / change / focus / blur / submit, not keydown / keyup for
  each key (the page view keys that no control takes do send them). A
  field is found by its name (and rank among that name) or by its rank
  among the controls: a page whose scripts insert unnamed controls before
  it can shift the rank. A value a script writes into another control
  than the one the user changed shows only while the user has not edited
  that control (the reader's state of an edited control wins). The default action of a click on a real link (follow
  the href) is the reader's, not the event's: links with an href other
  than "#" or javascript: are not marked and are followed without running
  their click listeners. A form submit after a click is dispatched to the
  scripts but not sent by the pass (the reader form sends it).
- The live realm has no collector: every event allocates in the same
  arena (6 MiB); when it is full the realm ends (error=arena_full) and
  the page stays as it was last written.
- Web APIs (row N): URL has no IDNA (hosts are lowercased ASCII) and no
  IPv6 (a bracketed host is refused); IPv4 number forms are copied, not
  normalized. Storage items are read with getItem/setItem and the other
  methods only (localStorage.k is not an item); the store lives in the
  shell process and is lost when the shell ends; sessionStorage is the
  same store in its own area (not per tab). entries(), keys() and
  values() return arrays. No AbortSignal.timeout; an abort cannot stop a
  request already made (the network is synchronous inside), it only
  rejects the promise that waits for the answer.
- The pass is still off by default (web-js on).

## Bounds (guest)

| Bound | Value | Where |
|-------|-------|-------|
| REPL arena (user heap) | 3 MiB (was 1.5 MiB: coroutine stacks) | WF_JSWEB_REPL_ARENA |
| page arena (user heap, reused, the live realm) | 6 MiB (was 4 MiB) | WF_JSWEB_PAGE_ARENA |
| coroutine stack / live coroutines | 192 KiB / 8 (RangeError past them) | JS_CO_STACK, JS_CO_MAX |
| microtasks per drain | 100000 (RangeError) | JS_JOBS_MAX |
| external scripts per pass / bytes each | 16 / 256 KiB | WF_DOM_EXT_SCRIPTS, WF_DOM_EXT_BYTES |
| fetch + XMLHttpRequest per pass / bytes per answer | 16 / 256 KiB | WF_DOM_NET_REQS, WF_DOM_NET_BYTES |
| scripts inserted by scripts waiting to run | 16 | WF_DOM_DYN_SCRIPTS |
| localStorage + sessionStorage: bytes / items / origins (the oldest dropped) | 32 KiB / 256 / 8 | WF_JSAPI_STORE_BYTES, _ITEMS, _ORIGINS |
| URL length | 2048 bytes (TypeError past it) | WF_JSAPI_URL_MAX |
| marked clickable elements per output | 128 | WF_DOM_MARKS |
| scripts plus callbacks per pass, and per event | 256 | WF_DOM_PAGE_CALLS |

Static data added to the shell in this lot: the 256 KiB network buffer
and the 96 KiB layout boxes in wf_jsdom.o.

## Security

The sanitizer (US-039) is unchanged and runs after every pass and every
event: the marked "?__wfjs=N" links are plain relative links; scripts,
on* attributes and javascript: URLs are still removed. The network of a
page goes through the guest fetch path with its TLS checks and the
settings "net off" switch; fetch and XMLHttpRequest stay on the page's
origin; every request and answer is bounded. A hostile page ends with a
bound error or a full arena, never a hang. Kernel rights unchanged.

## ETAT_REEL

See the Lot 114 section of docs/ETAT_REEL.md.

## Not done (first gaps for Lot 115)

1. Layout values from the renderer (webeng.c we_geom, the same boxes the
   GUI paints) instead of the block model, so offsetWidth and
   getBoundingClientRect follow the page's CSS. This is the first gap.
2. web-js on by default after a compatibility pass over the Lot 67 to 113
   pages and the script wall verdicts (Lot 87).
3. Symbol and the iterator protocol, async generators and for await.
4. A collector or arena compaction for the live realm and the REPL.
5. Asynchronous network inside the pass (overlap fetches) and CORS.
6. keydown / keyup from reader fields, and script values of controls the
   user edited.
