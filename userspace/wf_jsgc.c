/* wf_jsgc.c - Lot 115 (US-031): arena reclaim for the JavaScript subset.
 *
 * Until Lot 114 the realm's arena was a bump allocator: every string,
 * object, scope and syntax tree stayed until the realm was dropped, so a
 * page that kept running (events, timers) or a REPL session filled its
 * arena and had to be reset. This file adds a mark and sweep collector
 * that never moves a block, so every offset the C code holds stays valid.
 *
 * Layout. Every block allocated after js_init is preceded by a 4-byte
 * header: bits 0-3 the kind (JK_*), bit 4 the mark, bit 5 "free", bits
 * 8-31 the payload size in 4-byte words. A bitmap carved at the start of
 * the arena (one bit per 4 bytes below vm->limit) records where payloads
 * start, so a 32-bit word can be checked as a reference without trusting
 * its value: it is one when it falls inside the payload of a block that
 * is not free. Offsets inside a block count: the engine keeps a few (an
 * aligned coroutine stack; the Promise code gives its resolve functions
 * a record offset with a flag in bit 0 as their magic number).
 *
 * Marking is conservative and needs no type information: the roots are
 * the js_vm_t structure itself (every realm-wide reference lives there),
 * the host's roots (a callback; the page DOM gives its node objects,
 * listeners, timers and the prototypes its modules keep in statics),
 * and every reached block: objects, scopes, property and value arrays and
 * syntax nodes by their layout, leaves (strings, symbols) not at all,
 * other blocks word by word. A coroutine record (wf_jsco.c) gives its
 * stack and the live part of it, above the saved stack pointer: the C
 * frames of the suspended generator or async function, scanned word by
 * word and as pointers (mem + offset). A stack block is only reached
 * through its record's aligned start, so a stray number cannot keep a
 * 192 KiB stack. A false candidate can only keep garbage alive; it can
 * never free a live block. The mark stack is bounded
 * (JS_GC_MSTACK entries); when it overflows the heap is walked again
 * from the marked blocks until nothing new is marked.
 *
 * When. A collection runs only when no script is running (js_gc refuses
 * inside js_eval, js_call_value or js_run_jobs and inside a coroutine):
 * then no C local of the engine holds a reference. Hosts call
 * js_gc_idle after a REPL line, before a page event and between timers;
 * it collects when enough was allocated since the last run or when the
 * arena is three quarters full. The recycled scope, var array, job
 * record and coroutine stack lists are dropped first (their blocks join
 * the free lists) and the UTF-16 cursor cache is cleared.
 *
 * Sweep. Unmarked blocks become free; neighbours merge; a free run at
 * the top of the heap lowers vm->top. Free blocks are kept on
 * JS_GC_CLASSES segregated lists by size; js_alloc takes a block from
 * them (first fit, split when the rest can hold a header and a word)
 * before it bumps vm->top. Interned strings (property keys) are kept by
 * the intern table and are never reclaimed.
 *
 * Bounds: the bitmap is vm->limit / 32 bytes of the arena; the header is
 * 4 bytes per block; marking uses the fixed stack in js_vm_t and no
 * other memory. Freestanding C, no libc. */
#include "wf_jsi.h"

#define H_KIND(h)   ((h) & 0xFu)
#define H_MARK      0x10u
#define H_FREE      0x20u
#define H_WORDS(h)  ((h) >> 8)
#define HDR(vm, r)  (*(uint32_t *)JPTR(vm, (r) - 4u))
#define MAX_WORDS   0xFFFFFFu

/* size classes of the free lists: payload bytes up to this bound */
static const uint32_t k_class_max[JS_GC_CLASSES] = {
    8u, 16u, 24u, 32u, 48u, 64u, 96u, 128u, 192u, 256u, 512u, 1024u, 4096u, 16384u, 65536u, 0xFFFFFFFFu
};

static int size_class(uint32_t n) {
    int c = 0;
    while (c < JS_GC_CLASSES - 1 && n > k_class_max[c]) c++;
    return c;
}

/* ---- the start bitmap ---- */

static uint32_t *bits(js_vm_t *vm) { return (uint32_t *)JPTR(vm, vm->gc_bits); }

static void bit_set(js_vm_t *vm, uint32_t r) { bits(vm)[r >> 7] |= 1u << ((r >> 2) & 31u); }
static void bit_clr(js_vm_t *vm, uint32_t r) { bits(vm)[r >> 7] &= ~(1u << ((r >> 2) & 31u)); }

static int top_bit(uint32_t w) {
    int b = 31;
    while (b > 0 && !(w & (1u << b))) b--;
    return b;
}

/* the payload start at or below r (r inside the heap), 0 when none */
static uint32_t start_at_or_below(js_vm_t *vm, uint32_t r) {
    uint32_t *bm = bits(vm);
    uint32_t wi = r >> 7, lo = vm->gc_heap_lo >> 7;
    uint32_t w = bm[wi] & (0xFFFFFFFFu >> (31u - ((r >> 2) & 31u)));
    for (;;) {
        if (w) return (wi << 7) + ((uint32_t)top_bit(w) << 2);
        if (wi <= lo) return 0;
        wi--;
        w = bm[wi];
    }
}

int js_gc_init(js_vm_t *vm) {
    uint32_t nbytes = ((vm->limit >> 2) + 31u) / 32u * 4u, i;
    if (vm->top + nbytes + 4096u > vm->limit) return -1;
    vm->gc_bits = vm->top;
    for (i = 0; i < nbytes; i++) vm->mem[vm->gc_bits + i] = 0;
    vm->top += nbytes;
    vm->top = (vm->top + 7u) & ~7u;
    vm->gc_heap_lo = vm->top;
    vm->gc_bitmap_bytes = nbytes;
    return 0;
}

/* ---- allocation ---- */

static void free_push(js_vm_t *vm, uint32_t r, uint32_t bytes) {
    int c = size_class(bytes);
    HDR(vm, r) = ((bytes >> 2) << 8) | H_FREE;
    *(uint32_t *)JPTR(vm, r) = vm->gc_free[c];
    vm->gc_free[c] = r;
    vm->gc_free_bytes += bytes;
}

static uint32_t from_free(js_vm_t *vm, uint32_t n) {
    int c;
    for (c = size_class(n); c < JS_GC_CLASSES; c++) {
        uint32_t *link = &vm->gc_free[c];
        int walked = 0;
        while (*link && walked < 64) {
            uint32_t r = *link, have = H_WORDS(HDR(vm, r)) << 2;
            if (have >= n) {
                *link = *(uint32_t *)JPTR(vm, r);
                vm->gc_free_bytes -= have;
                if (have - n >= 8u) {
                    uint32_t rest = r + n + 4u;
                    bit_set(vm, rest);
                    free_push(vm, rest, have - n - 4u);
                    have = n;
                }
                HDR(vm, r) = (have >> 2) << 8;
                return r;
            }
            link = (uint32_t *)JPTR(vm, r);
            walked++;
        }
    }
    return 0;
}

uint32_t js_alloc_k(js_vm_t *vm, uint32_t n, int kind) {
    uint32_t r = 0, i;
    n = (n + 3u) & ~3u;
    if (!n) n = 4u;
    if (!vm->gc_bits) {
        /* before js_gc_init (never after js_init): a plain bump */
        if (n > vm->limit || vm->top > vm->limit - n) goto full;
        r = vm->top;
        vm->top += n;
        for (i = 0; i < n; i++) vm->mem[r + i] = 0;
        return r;
    }
    if ((n >> 2) > MAX_WORDS) goto full;
    if (vm->gc_free_bytes >= n) r = from_free(vm, n);
    if (!r) {
        if (n + 4u > vm->limit || vm->top > vm->limit - n - 4u) goto full;
        r = vm->top + 4u;
        vm->top += n + 4u;
        HDR(vm, r) = (n >> 2) << 8;
        bit_set(vm, r);
    }
    HDR(vm, r) |= (uint32_t)kind & 0xFu;
    n = H_WORDS(HDR(vm, r)) << 2;
    for (i = 0; i < n; i++) vm->mem[r + i] = 0;
    vm->gc_since += n + 4u;
    return r;
full:
    js_abort(vm, "RangeError: out of memory (the realm's arena is full)");
    return 0;
}

uint32_t js_alloc(js_vm_t *vm, uint32_t n) { return js_alloc_k(vm, n, JK_SCAN); }

/* ---- marking ---- */

static void push(js_vm_t *vm, uint32_t r) {
    if (vm->gc_msp < JS_GC_MSTACK) vm->gc_mstack[vm->gc_msp++] = r;
    else vm->gc_overflow = 1;
}

/* w may be a reference: mark the block holding it */
static void cand(js_vm_t *vm, uint32_t w) {
    uint32_t b, h;
    if (w < vm->gc_heap_lo + 4u || w >= vm->top) return;
    if (w >= vm->gc_last && w < vm->gc_last_end) return;      /* the block being scanned or just found */
    b = start_at_or_below(vm, w);
    if (!b) return;
    h = HDR(vm, b);
    if (w >= b + (H_WORDS(h) << 2) && w != b) return;          /* a header, or past the end */
    if (H_KIND(h) == JK_COSTACK && w - b >= 16u) return;       /* only its record's aligned start */
    vm->gc_last = b;
    vm->gc_last_end = b + (H_WORDS(h) << 2);
    if (h & (H_FREE | H_MARK)) return;
    HDR(vm, b) = h | H_MARK;
    vm->gc_marked_bytes += (H_WORDS(h) << 2) + 4u;
    /* a stack is scanned through its record (js_co_gc_scan), leaves never */
    if (H_KIND(h) != JK_LEAF && H_KIND(h) != JK_COSTACK) push(vm, b);
}

void js_gc_mark_ref(js_vm_t *vm, uint32_t r) { if (r) cand(vm, r); }

void js_gc_scan(js_vm_t *vm, const void *p, uint32_t bytes) {
    const uint8_t *q = (const uint8_t *)p;
    uint32_t base = (uint32_t)(unsigned long)vm->mem, i;
    uint32_t keep = vm->gc_last, keep_end = vm->gc_last_end;
    while (((unsigned long)q & 3u) && bytes) { q++; bytes--; }
    for (i = 0; i + 4u <= bytes; i += 4u) {
        uint32_t w = *(const uint32_t *)(const void *)(q + i);
        vm->gc_last = keep;
        vm->gc_last_end = keep_end;
        cand(vm, w);
        vm->gc_last = keep;
        vm->gc_last_end = keep_end;
        cand(vm, w - base);
    }
}

/* a value: its payload words are references for the reference tags
 * (an accessor property keeps its getter in a and its setter in b) */
static void scan_jv(js_vm_t *vm, const jv *v) {
    if (v->t == JT_INT || v->t == JT_NUM || v->t == JT_BOOL) return;
    cand(vm, v->a);
    cand(vm, v->b);
}

static void scan_block(js_vm_t *vm, uint32_t b) {
    uint32_t h = HDR(vm, b), n = H_WORDS(h) << 2, i;
    int kind = (int)H_KIND(h);
    vm->gc_last = b;
    vm->gc_last_end = b + n;
    if (kind == JK_OBJ && n >= (uint32_t)sizeof(js_obj_t)) {
        /* cls, flags and the property counts are not references; every
         * other field may be one (arrays keep counts in alen/acap, a
         * promise keeps its value there, a function its class node) */
        js_obj_t *o = JOBJ(vm, b);
        cand(vm, o->proto);
        cand(vm, o->props);
        cand(vm, o->arr);
        cand(vm, o->alen);
        cand(vm, o->acap);
        cand(vm, o->x0);
        cand(vm, o->x1);
        cand(vm, o->x2);
    } else if (kind == JK_ENV && n >= (uint32_t)sizeof(js_env_t)) {
        js_env_t *e = JENV(vm, b);
        cand(vm, e->parent);
        cand(vm, e->vars);
        scan_jv(vm, &e->thisv);
        cand(vm, e->fobj);
        cand(vm, e->newtarget);
    } else if (kind == JK_PROPS) {
        js_prop_t *p = (js_prop_t *)JPTR(vm, b);
        for (i = 0; i + (uint32_t)sizeof(js_prop_t) <= n; i += (uint32_t)sizeof(js_prop_t), p++) {
            cand(vm, p->key);
            scan_jv(vm, &p->v);
        }
    } else if (kind == JK_JVS) {
        const jv *v = (const jv *)JPTR(vm, b);
        for (i = 0; i + (uint32_t)sizeof(jv) <= n; i += (uint32_t)sizeof(jv), v++) scan_jv(vm, v);
    } else if (kind == JK_CO) {
        js_co_gc_scan(vm, b);
    } else if (kind == JK_NODE && n >= (uint32_t)sizeof(js_node_t)) {
        js_node_t *d = JNODE(vm, b);
        cand(vm, d->a);
        cand(vm, d->b);
        cand(vm, d->c);
        cand(vm, d->d);
        cand(vm, d->next);
    } else {
        js_gc_scan(vm, JPTR(vm, b), n);
    }
}

static void drain(js_vm_t *vm) {
    while (vm->gc_msp) scan_block(vm, vm->gc_mstack[--vm->gc_msp]);
}

/* after an overflow: scan every marked block again until no overflow */
static void rescan(js_vm_t *vm) {
    while (vm->gc_overflow) {
        uint32_t p = vm->gc_heap_lo;
        vm->gc_overflow = 0;
        while (p < vm->top) {
            uint32_t h = *(uint32_t *)JPTR(vm, p), r = p + 4u;
            if ((h & H_MARK) && H_KIND(h) != JK_LEAF && H_KIND(h) != JK_COSTACK) { scan_block(vm, r); drain(vm); }
            p = r + (H_WORDS(h) << 2);
        }
    }
}

/* ---- sweep ---- */

static void sweep(js_vm_t *vm) {
    uint32_t p = vm->gc_heap_lo, run = 0, live = 0, freed = 0, stacks = 0;
    int c;
    for (c = 0; c < JS_GC_CLASSES; c++) vm->gc_free[c] = 0;
    vm->gc_free_bytes = 0;
    while (p < vm->top) {
        uint32_t h = *(uint32_t *)JPTR(vm, p), r = p + 4u, n = H_WORDS(h) << 2;
        uint32_t next = r + n;
        if (h & H_MARK) {
            *(uint32_t *)JPTR(vm, p) = h & ~H_MARK;
            live += n + 4u;
            if (H_KIND(h) == JK_COSTACK) stacks++;
            if (run) { free_push(vm, run + 4u, p - run - 4u); run = 0; }
        } else {
            if (!(h & H_FREE)) freed += n + 4u;
            if (vm->gc_poison) {
                uint32_t i;
                for (i = 0; i < n; i++) vm->mem[r + i] = 0xDB;
            }
            if (!run) run = p;
            else bit_clr(vm, r);        /* merged into the run */
        }
        p = next;
    }
    if (run) {
        bit_clr(vm, run + 4u);
        vm->top = run;
    }
    vm->co_live = (int)stacks;
    vm->gc_live = live;
    vm->gc_freed_last = freed;
    vm->gc_freed_total += freed;
}

/* the realm structure: every field except the counters, buffers and
 * native tables (numbers there would only keep garbage alive) */
#define OFS(f) ((uint32_t)__builtin_offsetof(js_vm_t, f))
#define SPAN(f) OFS(f), (uint32_t)sizeof(((js_vm_t *)0)->f)
static void scan_vm(js_vm_t *vm) {
    static const uint32_t skip[][2] = {
        { SPAN(size) }, { SPAN(top) }, { SPAN(limit) }, { SPAN(icount) }, { SPAN(steps) }, { SPAN(budget) },
        { SPAN(depth) }, { SPAN(err) }, { SPAN(err_line) }, { SPAN(log) }, { SPAN(log_len) }, { SPAN(nat) },
        { SPAN(nat_magic) }, { SPAN(nnat) }, { SPAN(rng) }, { SPAN(date_now_lo) }, { SPAN(date_now_hi) },
        { SPAN(src_len) }, { SPAN(run_argc) }, { SPAN(co_live) }, { SPAN(nunhandled) }, { SPAN(jobs_run) },
        { SPAN(rejections_reported) }, { SPAN(rejections_lost) }, { SPAN(sym_serial) }
    };
    const uint8_t *base = (const uint8_t *)vm;
    uint32_t at = 0, end = OFS(gc_bits), k;      /* the collector's own fields come last */
    while (at < end) {
        uint32_t stop = end, next = end;
        for (k = 0; k < (uint32_t)(sizeof(skip) / sizeof(skip[0])); k++) {
            if (skip[k][0] >= at && skip[k][0] < stop) { stop = skip[k][0]; next = skip[k][0] + skip[k][1]; }
        }
        if (stop > at) {
            vm->gc_last = 0;
            vm->gc_last_end = 0;
            js_gc_scan(vm, base + at, stop - at);
        }
        at = next > at ? next : at + 4u;
    }
}

int js_gc_busy(const js_vm_t *vm) { return vm->gc_hold > 0 || vm->co_cur != 0; }

uint32_t js_gc(js_vm_t *vm) {
    int k;
    if (!vm->gc_bits || js_gc_busy(vm)) return 0;
    /* recycled lists: their blocks are garbage, let the sweep take them */
    vm->free_env = 0;
    for (k = 0; k < 8; k++) vm->free_vars[k] = 0;
    vm->rec_free = 0;
    vm->co_free_stack = 0;
    vm->u16_str = 0;
    vm->u16_unit = 0;
    vm->u16_byte = 0;
    vm->gc_msp = 0;
    vm->gc_overflow = 0;
    vm->gc_marked_bytes = 0;
    vm->gc_last = 0;
    vm->gc_last_end = 0;
    scan_vm(vm);
    drain(vm);
    if (vm->gc_roots) vm->gc_roots(vm, vm->gc_roots_ctx);
    drain(vm);
    rescan(vm);
    sweep(vm);
    vm->gc_runs++;
    vm->gc_since = 0;
    return vm->gc_freed_last;
}

uint32_t js_mem_free(const js_vm_t *vm) { return vm->limit - vm->top + vm->gc_free_bytes; }

int js_gc_idle(js_vm_t *vm) {
    uint32_t heap, room, step;
    if (!vm->gc_bits || js_gc_busy(vm) || vm->gc_off) return 0;
    heap = vm->limit - vm->gc_heap_lo;
    room = js_mem_free(vm);
    step = vm->gc_live / 2u;
    if (step < JS_GC_MIN_STEP) step = JS_GC_MIN_STEP;
    if (!vm->gc_stress && vm->gc_since < step && room > heap / 4u) return 0;
    if (!vm->gc_stress && vm->gc_since < 4096u) return 0;     /* nothing new to find */
    js_gc(vm);
    return 1;
}

void js_gc_set_roots(js_vm_t *vm, js_gc_roots_fn fn, void *ctx) {
    vm->gc_roots = fn;
    vm->gc_roots_ctx = ctx;
}

void js_gc_config(js_vm_t *vm, int stress, int poison, int off) {
    vm->gc_stress = stress;
    vm->gc_poison = poison;
    vm->gc_off = off;
}

void js_gc_stats(const js_vm_t *vm, js_gc_stats_t *st) {
    st->runs = vm->gc_runs;
    st->live = vm->gc_live;
    st->freed_last = vm->gc_freed_last;
    st->freed_total = vm->gc_freed_total;
    st->free_bytes = js_mem_free(vm);
    st->top = vm->top;
    st->bitmap = vm->gc_bitmap_bytes;
}

/* a heap walk for the tests: -1 when a header or the bitmap is wrong */
int js_gc_check(js_vm_t *vm) {
    uint32_t p = vm->gc_heap_lo, nfree = 0, r;
    int c;
    while (p < vm->top) {
        uint32_t h = *(uint32_t *)JPTR(vm, p), n = H_WORDS(h) << 2;
        r = p + 4u;
        if (h & H_MARK) return -1;
        if (start_at_or_below(vm, r) != r) return -2;
        if (n && start_at_or_below(vm, r + n - 1u) != r) return -3;
        if (h & H_FREE) nfree += n;
        p = r + n;
    }
    if (p != vm->top) return -4;
    if (nfree != vm->gc_free_bytes) return -5;
    for (c = 0; c < JS_GC_CLASSES; c++) {
        for (r = vm->gc_free[c]; r; r = *(uint32_t *)JPTR(vm, r)) {
            if (!(HDR(vm, r) & H_FREE)) return -6;
            if (size_class(H_WORDS(HDR(vm, r)) << 2) != c) return -7;
        }
    }
    return 0;
}
