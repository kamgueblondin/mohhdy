# Lot 115 plan - layout values from the renderer, and the next JavaScript and web gaps of the page pass (US-031)

Base: origin/main 6d8f814, after PR 272 (the Lot 114 branch) was merged.
Branch lot115-js-gc. All new guest code is freestanding C in Ring 3 (the
shell process): no libc, no FPU, no 64-bit division. Kernel rights and
ABI are unchanged: no new syscall, no new guest program, nothing changed
under fs/. The only kernel change is drawing code of the desktop
renderer (row H: kernel/gfx_desktop.c, kernel/gfx_wm.c). Test programs stay in my_initrd_test.tar; this lot
adds none. US-031 stays partial and phase3_complete stays false. US-039
(the page sanitizer) is unchanged and still runs after the script pass.

A row is listed only when its code, tests and docs are in the branch.

## Scope

| Row | What | Where | Proof |
|-----|------|-------|-------|
| A | Layout values from the renderer (the first gap of Lot 114). The page pass serializes its document without scripts, cut at WF_JSLAY_NODES (150) element and text nodes. The shell's hook (wf_jslayout.c) lays that copy out with webeng.c, the engine the GUI paints with, using the pass's 1024x768 viewport. The engine's border boxes, and its padding boxes for clientWidth/Height, replace the block model's for the elements they match. Elements are matched by tag in document order, with a short look-ahead for elements only one side has (head, title, implied elements). An inline element is the union of its painted text items. Elements past the budget, and every element when the engine refuses the page, keep the Lot 114 block model. offsetWidth/Height/Top/Left, client*, scroll* and getBoundingClientRect read them. A DOM change lays the page out again when the next value is read. "web-js status" reports layout_engine, the number of elements the last layout matched. | userspace/wf_jslayout.c, wf_jslayout.h, wf_jsdom.c, wf_jsdom.h, wf_jsweb.c, shell_fetch.c, webeng_geom.inc (we_client_box), webeng.h | test_lot115_layout.c (box_model, model_without_hook, flex_and_relayout, flex_legacy_is_what_the_gui_paints, inline, budget, refused, hook_direct), `make qemu-lot115` |
| B | Symbol (the third gap of Lot 114). A symbol is a value of tag JT_SYM whose record has the layout of a string (js_str_t) with a kind that is never "interned", so a symbol key is an ordinary uint32 property key compared by identity, without a second property table. Symbol(), Symbol.for and Symbol.keyFor (a registry object in the realm), Symbol.prototype (toString, valueOf, description, [Symbol.toPrimitive]), Object(symbol), Object.getOwnPropertySymbols; key listings (Object.keys, for-in, JSON, getOwnPropertyNames) skip symbols, Object.assign and {...o} copy enumerable ones. The runtime honours Symbol.iterator, Symbol.hasInstance (instanceof), Symbol.toPrimitive (with the "default", "string" and "number" hints) and Symbol.toStringTag (Object.prototype.toString). typeof gives "symbol"; a symbol does not convert to a string or a number implicitly (TypeError) but String(sym) works. Host objects (the DOM) never see a symbol key. | userspace/wf_jssym.c (new), wf_jsi.h, wf_jsrt.c, wf_jslib.c, wf_jslib2.c | test_lot115_lang.c (symbol: 15 cases, the expectations printed by node v24) |
| C | The iterator protocol. for-of over an object with its own or inherited [Symbol.iterator] (also one set on Array.prototype or an array) reads one value at a time and calls the iterator's return() on break, return, continue to an outer label or a throw; spread, Array.from, new Set/Map and Promise.all read user iterables; array destructuring reads only the elements the pattern needs from a user iterator or a generator and then closes it (an endless iterator works). keys(), values() and entries() of arrays, Map and Set, and String.prototype[Symbol.iterator], give iterator objects (next, return, [Symbol.iterator]) over a snapshot; the built-in iterator methods keep their fast paths. | wf_jssym.c, wf_jsrun.c (ex_forof, bind_pattern), wf_jslib.c (js_iter_values, Array.from, keys/values/entries), wf_jslib2.c (Map and Set) | test_lot115_lang.c (iter: 13 cases) |
| D | Class private members and static blocks. The lexer reads #name inside a class body; obj.#x and obj?.#x become an index on an N_PRIVNAME node, so reads, writes, compound assignment, ++, calls and destructuring targets all take the existing index paths. Each evaluation of a class binds a new private symbol per name in the class scope (two classes made by one factory do not share names). Private keys are own-only: a read or a write on an object without the member is a TypeError, #x in o is the brand check, a private method is not writable, a getter without a setter refuses writes, a frozen object keeps writable private fields. Instance private methods and accessors are kept on a holder in the class scope and installed on each instance before its fields. Static private fields and methods, static { } blocks (run in order with the static fields, this = the class) and a super() that returns another object (the derived class's fields go on that object) are covered. Early errors: an undeclared #x, delete of a private member, #constructor, a duplicate name (a getter and a setter may share one), a lone #x outside "#x in o". | wf_jsparse.c, wf_jsrun.c (eval_class, init_fields, ev, call_func), wf_jsrt.c (own-only private get/put/has), wf_jsi.h | test_lot115_lang.c (private: 15 cases, static: 3 cases, test_private_refused, test_private_hidden) |
| E | UTF-16 string semantics. Strings stay UTF-8 in the arena (the DOM, the console and the reader are UTF-8), but length, s[i], charAt, charCodeAt, codePointAt, at, slice, substring, substr, indexOf, lastIndexOf, includes/startsWith/endsWith positions, padStart/padEnd, split(""), Object.keys of a string, the replace callback offset, search, match index, exec index and lastIndex count UTF-16 code units. An astral character is two units; a slice through it keeps a lone surrogate as a 3-byte WTF-8 sequence, + and template literals join a high and a low half back into the 4-byte form, String.fromCharCode makes one code point from a pair, JSON.stringify writes a lone half as \udxxx and the console shows it as U+FFFD. An empty regular expression match steps over a whole character. toUpperCase/toLowerCase also map Latin-1, Latin Extended-A, basic Greek and Cyrillic (the 2-byte letters whose cases have the same length). The unit count is cached in the string record (upper bits of js_str_t.interned); ASCII strings take an identity fast path; other strings keep one conversion cursor per realm, so a loop over s[i] stays linear. | userspace/wf_jsu16.c (new), wf_jsrt.c, wf_jslib2.c, wf_jsre.c, wf_jsrun.c | test_lot115_lang.c (utf16: 11 cases, test_utf16_cache, test_utf16_log_valid) |
| F | Arena reclaim (the second gap of Lot 114). A mark and sweep collector that never moves a block, so every offset the C code holds stays valid. Each block has a 4-byte header (kind, mark, free, size) and a bitmap at the start of the arena records where payloads start, so any 32-bit word can be checked as a reference. Marking starts from the js_vm_t fields (minus its counters and buffers) and the host's roots (the page DOM gives its node objects, listener arrays, timers, runtime prototypes and the statics of wf_jsapi.c, wf_jsnet.c and wf_jshist.c); objects, scopes, property and value arrays and syntax nodes are scanned by their layout, other blocks and the stacks of suspended generators and async functions word by word (also as pointers). The mark stack is fixed (512 entries) and a heap walk takes over when it overflows. Sweep merges free neighbours, lowers the top when the last blocks are free, and fills 16 size-class free lists that js_alloc uses before the top. A collection only runs when no script runs (js_eval, js_call_value, js_run_jobs and coroutines refuse it), so no C local holds a reference: after a REPL line, before a page event, between page scripts and between timers; js_gc_idle collects after 256 KiB (or half the live size) of new allocations or when a quarter of the arena is left. Suspended generators nobody can reach give their stack back (the JS_CO_MAX bound counts live ones). `js` prints `osui js gc runs N live L freed F free B`, `web-js status` prints `gc N freed F free B`; a REPL realm is reset only when a collection does not bring it under the 64 KiB margin. The Promise resolve functions now keep their record offset with the flag in bit 0 (was offset << 1), and bound functions get their length. | userspace/wf_jsgc.c (new), wf_jsi.h, wf_js.h, wf_jsrt.c, wf_jslib.c, wf_jsco.c, wf_jsrun.c, wf_jsparse.c, wf_jsdom.c, wf_jsapi.c, wf_jsnet.c, wf_jshist.c, wf_jsweb.c | test_lot115_gc.c (12 multi-stage scenarios compared with node v24 under a collection after every stage with freed memory poisoned; a 60-line session in a 1 MiB arena and the same session failing without reclaim; reuse of freed blocks; refused inside scripts and coroutines; host roots; unreachable generators; mark stack overflow; bounds), test_lot115_lang.c and test_lot114_async.c (every case again in one realm with a collection after each), test_lot114_page.c (150 clicks leaving 300 KiB each on a live page) |
| G | Real-page compatibility. tests/fixtures/lot115 holds 15 small hand-written pages in the style of real site scripts (a cookie banner, tabs with arrow keys, a form validator, lazy images, a mini jQuery, templating with <template>, an event bus, a cart in localStorage, URL state, JSON config, text utilities, modern ES, analytics queue, nav menu, DOM traversal; 30 KiB in all, each under 4 KiB). Each logs its results; expected.txt is what jsdom 24 on node v24 logs for the same pages (scripts/lot115_record.js, a developer tool: jsdom is not a dependency). Running them first showed failing pages; each gap was fixed in the engine or the DOM: $<name> in replacement strings, Array toSorted/toReversed/with, escape/unescape, raw template text (String.raw, strings.raw) and string literals substituted in tagged templates (they were taken as text parts), eventPhase, the KeyboardEvent, MouseEvent, InputEvent and FocusEvent constructors (init fields with their defaults, instanceof; host events get the same prototypes), el.attributes (length, index, by name), form.elements (index, name or id, a RadioNodeList with value for a radio group), tabIndex (reflected), delete on dataset (a host delete hook, js_host_del_fn), srcset/sizes/download/media reflection, template.content (the parsed children move to a fragment, as in browsers; innerHTML of a template reads and writes the content), and the full character reference table of the web stack (wf_entity.c) plus the 37 HTML 4 names it leaves out for lack of a glyph. All 15 pages now log what jsdom logs. | userspace/wf_jslib.c, wf_jslib2.c, wf_jsre.c, wf_jsparse.c, wf_jsrun.c, wf_jsrt.c, wf_js.h, wf_jsi.h, wf_jsdom.c, scripts/lot67_web_server.py (/lot115/compat/NAME.html) | test_lot115_compat.c (the 15 pages line by line, fixture size, engine, events and DOM details against the reference), `make qemu-lot115` (templating and tabs: the guest's web-js log equals expected.txt) |
| H | Window titles and desktop labels at every text scale. At 1.5x (the default on 1024x768) and 2x the title buttons grow with the bar (Lot 90) but titles kept fixed widths (rw - 80, rw - 110): a long app or page title ran up to or under the minimize button and was cut mid-word without a mark. A title now stops GFX_WM_TITLE_GAP (8) px before the minimize button (gfx_wm_title_room) and a cut title ends with "..." (at a word end in its last third, gfx_desktop_title_fit). At 2x the desktop icon labels, centred under their tiles, left the screen on the right; they are now kept 4 px inside it (gfx_desktop_label_x). The icon hit boxes of the layout stayed on the 1x rows while the tiles were drawn on rows grown by the label glyphs: the layout now uses the drawn rows. | kernel/gfx_desktop.c, gfx_desktop.h, gfx_wm.c, gfx_wm.h | test_lot115_chrome.c (renders the desktop at 1x .. 2x on 1024x768 and 1920x1080: no title ink in the gap or under the buttons, a short title whole, a long one filling its room, labels on screen with ink, hit boxes of the drawn rows; fails on the Lot 114 renderer) |
| I | Weak collections and Reflect (wf_jsweak.c). WeakMap, WeakSet, WeakRef and FinalizationRegistry keep their entries in a JK_WEAK table that the collector does not trace: after marking, a value is marked while its key is alive (to a fixed point, so a value holding another entry's key keeps it) and the entries whose key died are emptied before the sweep. Keys are objects and symbols Symbol.for did not register (as in the language). Up to 64 tables are cleared per collection; more are kept strongly (garbage kept, nothing live lost). Reflect has its 13 functions over the Object internals; Object.defineProperty and Object.setPrototypeOf now refuse a frozen or non-extensible object and an incompatible change of a non-configurable property (they silently applied it), and getOwnPropertyNames lists the built-in length (arrays, arguments, String objects) and length/name (functions). | userspace/wf_jsweak.c, wf_jsgc.c (weak_note, weak_settle), wf_jslib.c (js_define_desc, js_set_proto), wf_jsrt.c, wf_jsrun.c (for-in skips the built-in names), wf_jsi.h, Makefile | test_lot115_weak.c (35 cases against node v24; reclaim through WeakRef.deref, the ephemeron chain, a session of temporary keys that does not grow the arena, the 16384-entry bound, tables past the per-run limit), the lang page of `make qemu-lot115` |
| J | Proxy (wf_jsproxy.c). A proxy object (x0 target, x1 handler) is met by the engine's own paths: [[Get]] (also a proxy on the prototype chain), [[Set]] (with the receiver: a proxy receiver takes the write through its descriptor and defineProperty), [[HasProperty]], [[Delete]], the own key lists (Object.keys/entries, getOwnPropertyNames/Symbols, for-in, JSON.stringify, spread, assign, Reflect.ownKeys; enumerability through getOwnPropertyDescriptor), [[Call]], [[Construct]], getPrototypeOf/setPrototypeOf, defineProperty, getOwnPropertyDescriptor and hasOwnProperty. A missing trap forwards to the target; Proxy.revocable revokes; Array.isArray and JSON see through to an array target; the built-in array iterator over a proxy reads length and indexes through the traps. | userspace/wf_jsproxy.c, wf_jsrt.c, wf_jsrun.c, wf_jslib.c, wf_jslib2.c, wf_jssym.c, wf_jsweak.c, wf_jsi.h, Makefile | test_lot115_proxy.c (24 cases against node v24: every trap, forwarding, a reactive store, a validator, negative indexes, a call recorder, revocation and the errors; the collector keeps target and handler; a chain of 4000 proxies ends in a RangeError), the lang page of `make qemu-lot115` |
| K | Async generators and for await (wf_jsco.c, wf_jsparse.c, wf_jsrun.c). An async generator runs on a Lot 114 coroutine and suspends either at a yield (its value answers the request at the head of the queue) or at an await (the await resume functions continue it). next/throw/return queue { mode, value, promise } requests on the object and return promises; a yield awaits its operand and a return statement awaits its value, as in the language; return(v) and throw(e) work before the start and after the end. for await steps an async iterator ([Symbol.asyncIterator]) awaiting each next(), awaits each value of a plain iterable, and awaits return() on break, return or a thrown body. Async generator methods in object literals and classes. | userspace/wf_jsco.c (js_agen_new, the request queue, agen_drain), wf_jsparse.c, wf_jsrun.c (ex_forawait, return await), wf_jslib.c, wf_jsi.h | test_lot115_agen.c (17 cases against node v24, including the microtask order of queued requests; the refused forms; the 1024-request bound; abandoned suspended generators give their stacks back to the collector), test_lot113/114 and lang pins updated, the lang page of `make qemu-lot115` |
| Z | QEMU pages /lot115/layout.html and /lot115/lang.html (private members, Symbol.iterator, a symbol key, UTF-16 lengths, WeakMap, WeakRef, Reflect, Proxy and an async generator under for await in a page script; the Lot 114 subset refused the page), then eight js REPL lines of 400 KiB of garbage each in the 3 MiB REPL arena (Lot 114 reset the realm), run by `make qemu-lot115` and the CI job lot115-js-layout | scripts/lot67_web_server.py, tests/scripts/test_qemu_lot115.py, Makefile, .github/workflows/ci.yml | CI |

## Not covered (honest)

- The scripts read the boxes the GUI paints, not the boxes a browser
  would compute. webeng.c keeps its legacy flex model for a plain flex
  row (no justify-content, gap or sized item paths): items share the row
  equally, whatever their width. The webeng_lot52 and lot64 tests pin
  that behaviour, and the GUI paints it. A row with justify-content (or
  the other CSS flex triggers) takes the engine's CSS path.
- Only the first WF_JSLAY_NODES nodes are laid out by the engine
  (WE_MAX_NODES is 160). Later elements get block model values, so on a
  long page they are approximate and not aligned with the engine's.
- External style sheets are not fetched for the layout copy. The page's
  <style> elements and style= attributes are read.
- The engine document (under 1 MiB) is taken from the user heap the first
  time a script reads a layout value, and is kept until the shell ends.
- Symbols: no Symbol.species, Symbol.unscopables, Symbol.isConcatSpreadable
  or Symbol.match behaviour (the symbols exist; the runtime ignores them).
  Iterators over Map and Set report the
  toStringTag "Array Iterator".
- keys(), values() and entries() iterate a snapshot taken when they are
  called, not the live collection.
- Private names: no private names in an object literal or outside a
  class body (as in the language); a `return` inside a static block is
  not refused.
- UTF-16: comparison (<, sort) orders UTF-8 bytes, which differs from
  UTF-16 order only between astral characters and U+E000..U+FFFF. The
  sharp s keeps its case (its upper case is "SS"). normalize() returns the
  string unchanged. Regular expressions still match bytes (an astral
  character matches as a whole with the u flag or a literal, not as two
  units).
- Reclaim only happens between runs: a single script, event handler or
  timer callback that allocates more than the arena still aborts with
  "out of memory" (the collector would need the C stack of the running
  interpreter as roots). Interned strings (every property name ever used)
  are never reclaimed. Marking is conservative for the blocks it does not
  know the layout of and for generator stacks: a number that looks like
  an offset keeps a block alive (never the reverse). The free lists are
  first fit in 16 size classes; fragmentation is not compacted.
- Async generators: yield* inside an async generator is refused at parse
  time ("yield* in async generators is not supported by the guest"); node
  accepts it. A suspended async generator holds one of the JS_CO_MAX (8)
  coroutine stacks like any generator; abandoned ones give it back at the
  next collection (between runs), so more than 8 started and dropped in
  one run is a RangeError. At most 1024 requests wait on one generator.
- Proxy: the invariants a trap result must keep against a non-configurable
  or non-extensible target are not checked; isExtensible and
  preventExtensions have no trap (they act on the target); instanceof and
  a plain lookup's prototype walk use the target's prototype at creation
  (the getPrototypeOf trap answers Object.getPrototypeOf and Reflect);
  Object.prototype.toString of a proxy says Object; internal fast paths
  that read a property table directly see an empty object.
- Weak collections: the FinalizationRegistry cleanup callback is never
  called (the specification allows it; the collector does not run
  scripts). A table holds 16384 entries (RangeError past that). A weak
  table is cleared only by a collection, which runs between runs.
  getOwnPropertyNames of a sloppy function has no arguments/caller.
- Compatibility: the fixtures are small hand-written pages, not captured
  sites; jsdom is the reference, not a browser (it has no layout and no
  structuredClone, which the recorder takes from node). Not in the subset:
  the HTML 5 named references beyond HTML 4, a live Attr or NamedNodeMap
  (el.attributes gives snapshots), form.elements for controls outside the
  form (form= attribute), template contents of nested templates cloned
  with their own content, cloneNode of a template element (its content is
  not copied).
- web-read of the templating fixture shows its title alone: the reader
  drops a block with id or class "comments" as page chrome (Lot 85,
  wf_score.c), by design. The page pass itself is right (its console and
  the HTML it writes match the host), so QEMU checks the console.
- Chrome: menu items, status row and dock were not changed here.
- Lot 113 pinned `typeof Symbol` (undefined), Map keys() as an array and
  the refusal of #x as documented limits; test_lot113_js.c now pins the
  new behaviour (a conscious change, as the comment there asks).

## Bounds (guest)

| Bound | Value | Where |
|-------|-------|-------|
| nodes in the layout copy | 150 (the engine holds 160) | WF_JSLAY_NODES, WE_MAX_NODES |
| layout copy | 32 KiB (cut quietly; not a DOM bound) | g_lay_html in wf_jsdom.c |
| engine document | sizeof(we_doc_t) < 1 MiB, user heap, once | wf_jslayout.c |
| a symbol | 16 bytes plus its description, in the realm's arena | wf_jssym.c |
| private names | 96 declared or used names in nested class bodies per parse | PS_MAX_PRIV in wf_jsparse.c |
| cached UTF-16 length | strings up to 4194302 units (longer ones are counted again) | JS_U16_MAXC in wf_jsi.h |
| collector bitmap | vm->limit / 32 bytes of the arena (84 KiB for the 3 MiB REPL arena, 180 KiB for the 6 MiB page arena) | js_gc_init in wf_jsgc.c |
| block header | 4 bytes per block; a block is at most 64 MiB | wf_jsgc.c |
| mark stack | 512 entries in js_vm_t, then heap walks | JS_GC_MSTACK |
| idle trigger | 256 KiB allocated since the last run (or half the live size), or a quarter of the arena left | JS_GC_MIN_STEP, js_gc_idle |

## Security

The collector never moves a block and only frees blocks no root reaches;
a wrong guess keeps garbage, it cannot free a live block. Its marking is
bounded (fixed stack, heap walks). The layout copy has no scripts. It is laid out by the same engine that
paints untrusted pages in the GUI, with the same bounds, and nothing is
painted from it. The sanitizer and the kernel rights are unchanged.

## ETAT_REEL

See the Lot 115 section of docs/ETAT_REEL.md.

## Not done (first gaps for Lot 116)

1. More of the page laid out by the engine than its 160 nodes, for
   example by laying out the section around the element being read.
   This is the first gap.
2. Reclaim inside a run (the running interpreter's C stack as roots),
   so one handler that allocates more than the arena can finish.
3. web-js on by default after a compatibility pass.
4. yield* inside an async generator (refused at parse time), and the
   FinalizationRegistry cleanup callback.
