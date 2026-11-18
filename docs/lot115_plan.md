# Lot 115 plan - layout values from the renderer, and the next JavaScript and web gaps of the page pass (US-031)

Base: origin/main 6d8f814, after PR 272 (the Lot 114 branch) was merged.
Branch lot115-js-gc. All new guest code is freestanding C in Ring 3 (the
shell process): no libc, no FPU, no 64-bit division. Kernel, rights and
ABI are unchanged: no new syscall, no new guest program, nothing changed
under kernel/ or fs/. Test programs stay in my_initrd_test.tar; this lot
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
| Z | QEMU pages /lot115/layout.html and /lot115/lang.html (private members, Symbol.iterator, a symbol key and UTF-16 lengths in a page script; the Lot 114 subset refused the page), run by `make qemu-lot115` and the CI job lot115-js-layout | scripts/lot67_web_server.py, tests/scripts/test_qemu_lot115.py, Makefile, .github/workflows/ci.yml | CI |

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
  No Reflect, Proxy or WeakMap. Iterators over Map and Set report the
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

## Security

The layout copy has no scripts. It is laid out by the same engine that
paints untrusted pages in the GUI, with the same bounds, and nothing is
painted from it. The sanitizer and the kernel rights are unchanged.

## ETAT_REEL

See the Lot 115 section of docs/ETAT_REEL.md.

## Not done (first gaps for Lot 116)

1. More of the page laid out by the engine than its 160 nodes, for
   example by laying out the section around the element being read.
   This is the first gap.
2. A collector or arena reclaim for the live realm and the REPL.
3. Async generators and for await (they need the iterator protocol this
   lot adds and the coroutines of Lot 114).
4. web-js on by default after a compatibility pass.
