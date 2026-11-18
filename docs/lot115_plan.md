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
| Z | QEMU page /lot115/layout.html, run by `make qemu-lot115` and the CI job lot115-js-layout | scripts/lot67_web_server.py, tests/scripts/test_qemu_lot115.py, Makefile, .github/workflows/ci.yml | CI |

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

## Bounds (guest)

| Bound | Value | Where |
|-------|-------|-------|
| nodes in the layout copy | 150 (the engine holds 160) | WF_JSLAY_NODES, WE_MAX_NODES |
| layout copy | 32 KiB (cut quietly; not a DOM bound) | g_lay_html in wf_jsdom.c |
| engine document | sizeof(we_doc_t) < 1 MiB, user heap, once | wf_jslayout.c |

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
3. Symbol, the iterator protocol, private class members.
4. web-js on by default after a compatibility pass.
