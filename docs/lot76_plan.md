# Lot 76 plan

Base: origin/main b848573 (PR #232 lot 75). Branch: cursor/lot76.

## Chosen items (from lot 75 first gap + refused lists)

### A. Forms still partial / refused
1. input type=color and type=date as real WE_FORM_* types (today both
   emit "input type taken as text"). Paint a color swatch / date field,
   validate (#rrggbb and YYYY-MM-DD), convert the lot 73 refusal count.
2. autocomplete= on controls (off/on/new-password/...). Stored; off is
   honored as a flag for future autofill (no JS).
3. formtarget= on form and submitters (_self / _blank accepted; others
   refused). No new windows: _blank is recorded on the request only.
4. <output> as a form-associated readonly control (name, for=, form=,
   value text) that participates in form= association.

### B. CSS / HTML still thin for real pages
5. content: attr() accepts title, value, placeholder, name (today only
   id/class/href/src/alt). Convert or extend content tests.
6. white-space / text-overflow edge cases already largely present; add
   covered tests only where a gap shows up while doing A/B.

### Out of scope (unchanged)
No JS VM, Chromium/WebKit, new font engine, margin collapsing.
US-031 stays partial; phase3_complete stays false.

## Delivery shape (like lots 74-75)
- Code in userspace/webeng*.inc / webeng.h / webeng.c / wf_* if reader
  emit needs color/date.
- Tests: webeng_lot76_*.inc + test_wf_lot76 if reader emit; host no DNS.
- QEMU: initrd_content/web/lot76.html + test_qemu_lot76.py + make
  qemu-lot76 + CI job.
- Mutations in /workspace/run/lot76/mutations.txt (every new branch).
- Target +8000..10000 useful lines.
