# Lot 113 plan - a bounded JavaScript subset in Ring 3 (US-031): interpreter, built-ins, regular expressions, a page DOM with the script pass of fetched pages, and the js / web-js commands

Base: origin/main after PR 270 (the Lot 112 branch) was merged. Branch
lot113-js-subset. All new code is freestanding guest C in Ring 3 (the
shell process): no libc, no FPU, no 64-bit division, built at -O0 with
the userspace flags. Kernel, rights and ABI unchanged: no new syscall,
no new guest program, no change under kernel/, include/ or fs/. US-031
stays partial. phase3_complete stays false. US-039 (the page sanitizer)
is unchanged and still runs after the script pass. US-068/US-073,
US-076, US-080 and US-081 stay partial. US-078 and US-097 stay simule.

Until this lot the guest browser never ran a script: pages that build
their content in JavaScript showed their static HTML (or a "script wall"
verdict, Lot 87). This lot adds a small interpreter and runs the inline
scripts of a fetched HTML page against a bounded DOM before the reader
and the renderer see it. It is off by default (`web-js on` turns it on).

A row is listed only when its code, tests and docs are in the branch.

## Scope

| Id | Feature | Files | Proof |
|----|---------|-------|-------|
| A | binary64 numbers in software: + - * / %, compare, int/uint conversions, parse (correctly rounded) and shortest round-trip formatting, toFixed/toPrecision/toExponential/radix, floor/ceil/round/trunc, sqrt, and series for exp, log, pow, sin, cos, tan, asin, acos, atan, atan2, cbrt, hypot | userspace/wf_jsnum.c, wf_jsnum.h | test_lot113_num.c (bit for bit against the host's SSE2 doubles, ECMA-262 Number::toString examples, strtod) |
| B | values, strings (interned), objects with property flags and accessors, arrays, a bump arena (references are offsets), the step budget and the call-depth limit | userspace/wf_jsrt.c, wf_jsi.h, wf_js.h | test_lot113_js.c |
| C | lexer and parser to an AST: ES5 statements and expressions plus let/const with block scope and TDZ, arrow functions, template literals, classes (extends, super, static members, getters/setters, public fields), destructuring with defaults and rest, default and rest parameters, spread, for-of, optional chaining, ??, ??=, ||=, &&=, **, labels, regular expression literals | userspace/wf_jsparse.c | test_lot113_js.c (k_lang) |
| D | a tree-walking interpreter on a private stack carved from the arena end (i386 trampoline), so JavaScript recursion never uses the 64 KiB Ring 3 stack; completion values; exceptions; closures | userspace/wf_jsrun.c, wf_jslib.c (js_eval, js_call_value) | test_lot113_js.c (realm, errors, bounds) |
| E | built-ins: Object (keys, values, entries, assign, create, freeze, defineProperty, getPrototypeOf...), Function (call, apply, bind), Array (the ES2019 set: map, filter, reduce, sort, flat, splice, includes, at, from, of...), Error and its subtypes, console (log, warn, error, info, debug), parseInt, parseFloat, isNaN, isFinite, encodeURI(Component) and decodeURI(Component) | userspace/wf_jslib.c | test_lot113_js.c (k_lib, console, display) |
| F | String (split, replace and replaceAll with $-patterns and functions, match, matchAll, search, pad, trim, slice...), Number, Boolean, Math (deterministic random), JSON (stringify with replacer, indent, cycle check; parse with reviver), Date (fixed clock, UTC zone), Map and Set | userspace/wf_jslib2.c | test_lot113_js.c (k_lib) |
| G | regular expressions: a backtracking matcher over compiled bytecode with an explicit stack, flags g i m s u y, classes, quantifiers including {n,m} and lazy ones, groups, named groups, backreferences, lookahead and lookbehind; one interpreter step per 16 matcher steps so a catastrophic pattern ends with the step budget | userspace/wf_jsre.c | test_lot113_js.c (k_lib, bounds) |
| H | the page DOM: a tolerant HTML parser (implied end tags for p, li, dt, dd, option, td, th, tr; raw text elements; entities) into fixed pools, a serializer, element and document host objects (nodeType, tagName, id, className, textContent, innerHTML, outerHTML, parent/child/sibling links, live children and childNodes, attributes, style with camelCase properties, classList, dataset, value/checked/disabled/hidden, document.title, cookie, body, head), the tree methods (appendChild, insertBefore, removeChild, replaceChild, remove, append, prepend, before, after, replaceWith, cloneNode, insertAdjacentHTML, createElement, createTextNode, createDocumentFragment...), selectors for querySelector(All), matches and closest (tag, *, #id, .class, [attr], [attr=v], ^= $= *= ~= |=, :first-child, :last-child, :only-child, :root, :empty, :checked, :disabled, :enabled, :not(), descendant, >, +, ~, comma lists) | userspace/wf_jsdom.c, wf_jsdom.h | test_lot113_dom.c (roundtrip, dom_edit, selectors, tree_ops) |
| I | the page script pass: inline scripts in document order, document.write (the written HTML goes after the script and its scripts run), DOMContentLoaded, load (and body onload), addEventListener / removeEventListener / dispatchEvent with bubbling, once, stopPropagation, preventDefault, on* handler properties, Event and CustomEvent, el.click(); timers on a virtual clock after load (setTimeout, setInterval, requestAnimationFrame, queueMicrotask), window extras (location from the page URL, navigator, screen, localStorage/sessionStorage in memory, alert/confirm/prompt logged, matchMedia, getComputedStyle of the inline style, atob/btoa); the page is written back as HTML without its script and noscript elements | userspace/wf_jsdom.c | test_lot113_dom.c (events, load_and_timers, write, window, errors_and_bounds), `make qemu-lot113` |
| J | the shell and GUI commands, one code path (osui_gui_web_command calls wf_jsweb_command): `js CODE` (a persistent REPL realm, no DOM), `web-js on/off/status/log/reset`; the fetch path runs the page pass on every HTML body when web-js is on, and the fetch status line ends with " js scripts=N ran=N failed=N" | userspace/wf_jsweb.c, wf_jsweb.h, shell_fetch.c, osui_gui.c, shell.c | test_lot113_dom.c (commands), `make qemu-lot113` |

## What the subset covers and what it does not (honest)

Covered (each with a host test): the language rows C and D, the
built-in rows E, F and G, the DOM and page rows H and I. 112 language and
library cases of test_lot113_js.c give the same String() as node v24
for the same source (checked when the test was written); the DOM tests
were checked against jsdom 24 for the cases jsdom also runs (selectors,
events, load order, live children, cookie order, window parts).

Not covered (a script that uses them throws, or the value is missing):

- Language: generators and yield, async/await, Promise, Symbol (and so
  iterator protocols: for-of and spread take arrays, strings, arguments,
  Map, Set and array-like host lists), Proxy, Reflect, WeakMap/WeakSet,
  typed arrays and ArrayBuffer, BigInt, private class members (#x),
  getters on built-ins beyond the listed ones, with, eval and new
  Function (EvalError), tail calls, modules (import/export: a script of
  type module is not run).
- Strings are UTF-8 byte strings: length and indexes count bytes, not
  UTF-16 units; toUpperCase/toLowerCase and the regex i flag fold ASCII
  only; Map/Set keys(), values() and entries() return arrays.
- Numbers: Math.pow, Math.exp, Math.atan2 and the other series can be one
  ulp away from node on some inputs (exact for the cases tested).
- Date has a fixed clock (2026-01-01T00:00:00Z) and only the UTC zone;
  Math.random is a deterministic xorshift.
- No garbage collector: one realm lives in one arena. An allocation past
  its end aborts the run (not catchable). The REPL realm is replaced when
  it is full; the page realm is new for every page.
- Page: external scripts (src=) are not fetched and not run; no fetch,
  XMLHttpRequest, WebSocket or Worker; no layout values (offsetWidth,
  getBoundingClientRect and the like read 0); getComputedStyle is the
  inline style only; no events after load (clicks in the GUI do not reach
  scripts); timers run once after load in due order on a virtual clock
  and setInterval at most WF_DOM_INTERVAL_RUNS (3) times; location
  changes and window.open do nothing; document.write after load is
  ignored; scripts set by innerHTML or created with createElement are not
  run (parser- and document.write-inserted ones are); getElementsBy*()
  return arrays (not live), children and childNodes are live;
  document.cookie is local to the page pass (not the wf_cookie jar of the
  fetches) and keeps name=value pairs only; localStorage is not kept
  between pages.
- The pass is off by default: pages look as before until `web-js on`.

## Bounds (guest)

| Bound | Value | Where |
|-------|-------|-------|
| REPL arena (user heap, first `js`) | 1.5 MiB | WF_JSWEB_REPL_ARENA |
| page arena (user heap, first page with web-js on) | 4 MiB, reused | WF_JSWEB_PAGE_ARENA |
| page output buffer (user heap) | the fetch body size (384 KiB) | wf_jsweb_page |
| interpreter stack (end of each arena) | 384 KiB, abort below 48 KiB free | JS_STACK_SIZE, JS_STACK_RESERVE |
| steps per script, callback or REPL line | 3,000,000 (page) / 2,000,000 (REPL) | WF_DOM_SCRIPT_BUDGET, WF_JSWEB_REPL_BUDGET |
| call depth | 64 (a RangeError, catchable) | JS_MAX_DEPTH |
| console log per run | 8 KiB | JS_LOG_MAX |
| DOM nodes / attributes / text bytes | 6000 / 9000 / 384 KiB (RangeError past them) | WF_DOM_NODES, WF_DOM_ATTRS, WF_DOM_BYTES |
| timers pending / scripts plus callbacks per page | 64 / 256 | WF_DOM_TIMERS, WF_DOM_PAGE_CALLS |
| regex backtrack stack / captures | 16384 entries / 32 | RE_BT_CAP, JS_RE_MAXCAP |

Static data added to the shell: about 1.27 MiB of bss (the DOM pools
900 KiB in wf_jsdom.o, the regex backtrack stack and compile scratch
362 KiB in wf_jsre.o, two js_vm_t of 11912 bytes); the arenas are taken
from the user heap only when used.

## Security

The page pass runs before the sanitizer (US-039, unchanged): its output
still goes through webrt_sanitize_page in the GUI and the reducer in the
console, which drop every script, iframe, object, embed, on* attribute
and javascript: URL. A script cannot reach the network (no fetch, no XHR,
no external script), the file system or the kernel: the interpreter has
no host function other than the DOM, the console log and the timers.
Every run is bounded by steps, depth, arena and DOM pools; a hostile page
ends with a bound error, never a hang. The kernel rights model is
unchanged.

## Review points

1. Files outside the new wf_js* files: shell_fetch.c (page_present runs
   wf_jsweb_page on the HTML body when web-js is on, and the status line
   gets wf_jsweb_note), osui_gui.c (osui_gui_web_command calls
   wf_jsweb_command first), shell.c (is_builtin names js and web-js, the
   dispatch, and handle_line passes the js line as typed so quotes and
   blanks stay code), userspace/Makefile (the nine objects), the
   regenerated shared/multiboot_shell_commands.json and
   userspace/mohhdy_osui_bridge.h (499 names), scripts/lot67_web_server.py
   (/lot113/js.html), tests/scripts/run_all_tests.sh (the three Lot 113
   tests; an extra_libs variable for -lm), Makefile (qemu-lot113),
   .github/workflows/ci.yml (lot113-js-subset).
2. Bug found while writing the tests: super.x on a getter ran the getter
   with the prototype as this (js_get_recv, wf_jsrt.c and wf_jsrun.c).
3. Bug found: test_lot113_num.c was in the tree since the WIP commit but
   not in run_all_tests.sh (it needs wf_jsnum.c, SSE2 flags and libm).
4. The private stack: on_stack (wf_jslib.c) switches esp to the arena's
   end with esi holding the caller's stack pointer; on a non-i386 host
   build the interpreter runs on the caller's stack.
5. The page realm is created for every page (js_init) and its arena
   reused; the REPL realm persists until `web-js reset` or until full.
6. The default is off so the Lot 67 to 112 QEMU pages (noscript links,
   script wall verdicts) keep their behaviour.

## Not done (first gaps for lot 114)

1. External scripts: fetch the src= scripts of a page (same origin first,
   bounded size and count) through the existing fetch path and run them in
   order; most real pages need them. This is the first gap.
2. web-js on by default after a compatibility pass over the Lot 67 to 112
   pages and the script wall verdicts (Lot 87).
3. Events after load: GUI clicks and form input dispatched to the page's
   listeners, then a re-render.
4. Promise and async/await (a microtask queue on the virtual clock), then
   Symbol and the iterator protocol.
5. UTF-16 string semantics (length, charCodeAt, case mapping beyond
   ASCII) and a fetch() bound to the page's origin.
6. A collector or arena compaction so a long REPL session keeps its realm.
