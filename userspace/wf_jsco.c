/* wf_jsco.c - Lot 114 (US-031): generators, async functions, Promise and
 * the microtask queue of the JavaScript subset (see wf_js.h).
 *
 * The interpreter walks the tree with C recursion, so a generator or an
 * async function that suspends in the middle of an expression keeps its
 * C frames. Each one that has started runs on its own stack (a block of
 * the realm's arena, JS_CO_STACK bytes) and wf_js_co_switch swaps the
 * stack pointer and the callee-saved registers (i386 only). At most
 * JS_CO_MAX coroutines hold a stack at the same time; a stack is reused
 * when its coroutine finishes. Lot 115: a suspended coroutine nobody can
 * reach any more is reclaimed with its stack by wf_jsgc.c between runs
 * (the stack is a JK_COSTACK block, scanned word by word).
 *
 * Promise follows the ECMAScript job model: reactions run as microtasks
 * in FIFO order when the host calls js_run_jobs (after every script,
 * callback and REPL line). `await v` is PromiseResolve(v) then
 * PerformPromiseThen with the coroutine's resume functions, so the
 * ordering matches current engines. A rejection nobody handled by the
 * end of a drain is reported once as "Uncaught (in promise) ...".
 *
 * Not done: async generators, for await, Promise subclassing (species).
 * Lot 115: the resolve and await functions keep their record offset with
 * a flag in bit 0 as magic (offsets are 4-aligned), so the collector sees
 * an offset inside the record. */
#include "wf_jsi.h"

static int cslen(const char *s) { int n = 0; while (s[n]) n++; return n; }
#define KEY(vm, s) js_intern((vm), (s), cslen(s))

/* ---- the stack switch ---- */

#if defined(__i386__)
void wf_js_co_switch(uint32_t *save_sp, uint32_t new_sp);
__asm__(
    ".text\n"
    ".globl wf_js_co_switch\n"
    ".hidden wf_js_co_switch\n"
    ".type wf_js_co_switch, @function\n"
    "wf_js_co_switch:\n"
    "    movl 4(%esp), %eax\n"
    "    movl 8(%esp), %edx\n"
    "    pushl %ebp\n"
    "    pushl %ebx\n"
    "    pushl %esi\n"
    "    pushl %edi\n"
    "    movl %esp, (%eax)\n"
    "    movl %edx, %esp\n"
    "    popl %edi\n"
    "    popl %esi\n"
    "    popl %ebx\n"
    "    popl %ebp\n"
    "    ret\n"
    ".size wf_js_co_switch, .-wf_js_co_switch\n");
#define CO_CAN 1
#else
#define CO_CAN 0
#endif

enum { CO_NEW = 0, CO_SUSP, CO_RUN, CO_DONE };
enum { CO_GEN = 1, CO_ASYNC = 2 };
enum { RM_NEXT = 0, RM_THROW, RM_RETURN };

typedef struct {
    uint32_t sp, caller_sp;
    uint32_t stack;            /* arena ref of the stack block, 0 when none */
    uint32_t f, env;
    uint32_t promise;          /* async: the result promise */
    uint32_t aful, arej;       /* async: the resume functions of await */
    uint8_t state, kind, mode, pad;
    int depth_in, depth_base;
    jv val;
} js_co_t;

#define CO(vm, r) ((js_co_t *)JPTR(vm, r))

static js_vm_t *g_co_vm;

static void co_entry(void) {
    js_vm_t *vm = g_co_vm;
    uint32_t c = vm->co_cur;
    jv r = js_fn_body(vm, CO(vm, c)->f, CO(vm, c)->env);
    CO(vm, c)->val = r;
    CO(vm, c)->state = CO_DONE;
#if CO_CAN
    wf_js_co_switch(&CO(vm, c)->sp, CO(vm, c)->caller_sp);
#endif
    for (;;) { }
}

/* a stack for coroutine c: recycled or new */
static int co_stack(js_vm_t *vm, uint32_t c) {
    uint32_t s = vm->co_free_stack, top;
    uint32_t *fr;
    if (!CO_CAN) { js_throw_err(vm, JE_SYNTAX, "generators and async functions need the i386 guest"); return 0; }
    if (s) {
        vm->co_free_stack = *(uint32_t *)JPTR(vm, s);
    } else {
        if (vm->co_live >= JS_CO_MAX) {
            js_throw_err(vm, JE_RANGE, "too many suspended generators or async functions (guest bound)");
            return 0;
        }
        s = js_alloc_k(vm, JS_CO_STACK + 16u, JK_COSTACK);
        if (!s) return 0;
        s = (s + 15u) & ~15u;
    }
    vm->co_live++;
    CO(vm, c)->stack = s;
    /* the first switch pops four registers and returns into co_entry;
     * co_entry then sees a 16-byte aligned frame like any call */
    top = (uint32_t)(unsigned long)(vm->mem + s + JS_CO_STACK);
    top &= ~15u;
    fr = (uint32_t *)(unsigned long)(top - 24u);
    fr[0] = 0; fr[1] = 0; fr[2] = 0; fr[3] = 0;           /* edi esi ebx ebp */
    fr[4] = (uint32_t)(unsigned long)co_entry;
    fr[5] = 0;                                            /* co_entry never returns */
    CO(vm, c)->sp = top - 24u;
    return 1;
}

/* Lot 115 (wf_jsgc.c): the references of a coroutine record. Its stack
 * block is marked through co->stack, and only the part above the saved
 * stack pointer is scanned (the frames of the suspended coroutine); the
 * dead frames below it would keep garbage alive. */
void js_co_gc_scan(js_vm_t *vm, uint32_t c) {
    js_co_t *co = CO(vm, c);
    uint32_t base = (uint32_t)(unsigned long)vm->mem;
    js_gc_mark_ref(vm, co->stack);
    js_gc_mark_ref(vm, co->f);
    js_gc_mark_ref(vm, co->env);
    js_gc_mark_ref(vm, co->promise);
    js_gc_mark_ref(vm, co->aful);
    js_gc_mark_ref(vm, co->arej);
    js_gc_scan(vm, &co->val, (uint32_t)sizeof(co->val));
    if (co->stack && co->state == CO_SUSP) {
        uint32_t lo = co->sp - base, hi = co->stack + JS_CO_STACK;
        if (lo >= co->stack && lo < hi) js_gc_scan(vm, vm->mem + lo, hi - lo);
    }
}

static void co_release(js_vm_t *vm, uint32_t c) {
    uint32_t s = CO(vm, c)->stack;
    if (!s) return;
    *(uint32_t *)JPTR(vm, s) = vm->co_free_stack;
    vm->co_free_stack = s;
    CO(vm, c)->stack = 0;
    vm->co_live--;
}

static uint32_t co_new(js_vm_t *vm, int kind, uint32_t f, uint32_t env) {
    uint32_t c = js_alloc_k(vm, sizeof(js_co_t), JK_CO);
    int i;
    if (!c) return 0;
    for (i = 0; i < (int)sizeof(js_co_t); i++) vm->mem[c + (uint32_t)i] = 0;
    CO(vm, c)->kind = (uint8_t)kind;
    CO(vm, c)->f = f;
    CO(vm, c)->env = env;
    CO(vm, c)->depth_in = 1;
    CO(vm, c)->val = jv_undef();
    return c;
}

/* run c until it suspends or finishes; 0, or -1 when it could not start */
static int co_resume(js_vm_t *vm, uint32_t c, int mode, jv v) {
    uint8_t *sb = vm->stack_base, *st = vm->stack_top;
    int sd = vm->depth, so = vm->opt_short;
    uint32_t scur = vm->co_cur;
    if (CO(vm, c)->state == CO_NEW && !co_stack(vm, c)) return -1;
    if (vm->depth + CO(vm, c)->depth_in >= JS_MAX_DEPTH) {
        js_throw_err(vm, JE_RANGE, "Maximum call stack size exceeded (guest bound)");
        return -1;
    }
    CO(vm, c)->mode = (uint8_t)mode;
    CO(vm, c)->val = v;
    CO(vm, c)->depth_base = sd;
    vm->depth = sd + CO(vm, c)->depth_in;
    vm->stack_base = vm->mem + CO(vm, c)->stack;
    vm->stack_top = vm->mem + CO(vm, c)->stack + JS_CO_STACK;
    vm->co_cur = c;
    CO(vm, c)->state = CO_RUN;
    g_co_vm = vm;
#if CO_CAN
    wf_js_co_switch(&CO(vm, c)->caller_sp, CO(vm, c)->sp);
#endif
    CO(vm, c)->depth_in = vm->depth - CO(vm, c)->depth_base;
    vm->depth = sd;
    vm->opt_short = so;
    vm->stack_base = sb;
    vm->stack_top = st;
    vm->co_cur = scur;
    if (CO(vm, c)->state == CO_DONE) co_release(vm, c);
    return 0;
}

/* inside a coroutine: give v to the resumer; returns how it resumed */
static jv co_suspend_raw(js_vm_t *vm, jv v, int *mode) {
    uint32_t c = vm->co_cur;
    CO(vm, c)->val = v;
    CO(vm, c)->state = CO_SUSP;
#if CO_CAN
    wf_js_co_switch(&CO(vm, c)->sp, CO(vm, c)->caller_sp);
#endif
    *mode = CO(vm, c)->mode;
    return CO(vm, c)->val;
}

static jv co_suspend(js_vm_t *vm, jv v) {
    int mode;
    jv r = co_suspend_raw(vm, v, &mode);
    if (mode == RM_THROW) return js_throw(vm, r);
    if (mode == RM_RETURN) { vm->ret = r; vm->ctl = CTL_RETURN; return jv_undef(); }
    return r;
}

/* ---- small records (jobs, reactions, capabilities) ---- */

typedef struct {
    uint32_t next;
    uint32_t kind;
    uint32_t a, b;
    jv v, h, h2;
} js_rec_t;

#define REC(vm, r) ((js_rec_t *)JPTR(vm, r))

static uint32_t rec_new(js_vm_t *vm) {
    uint32_t r = vm->rec_free;
    int i;
    if (r) vm->rec_free = REC(vm, r)->next;
    else {
        r = js_alloc(vm, sizeof(js_rec_t));
        if (!r) return 0;
    }
    for (i = 0; i < (int)sizeof(js_rec_t); i++) vm->mem[r + (uint32_t)i] = 0;
    REC(vm, r)->v = jv_undef();
    REC(vm, r)->h = jv_undef();
    REC(vm, r)->h2 = jv_undef();
    return r;
}

static void rec_free(js_vm_t *vm, uint32_t r) {
    REC(vm, r)->next = vm->rec_free;
    vm->rec_free = r;
}

/* ---- the microtask queue ---- */

enum { JOB_REACTION = 1, JOB_THENABLE, JOB_CALLBACK };
enum { RX_FULFILL = 0, RX_REJECT = 1 };

static void job_push(js_vm_t *vm, uint32_t j) {
    REC(vm, j)->next = 0;
    if (vm->job_tail) REC(vm, vm->job_tail)->next = j;
    else vm->job_head = j;
    vm->job_tail = j;
}

int js_jobs_pending(const js_vm_t *vm) { return vm->job_head != 0; }

/* ---- promises ---- */

#define PS_PENDING 0u
#define PS_FULFILLED 1u
#define PS_REJECTED 2u
#define PF_HANDLED 1u
#define PF_TRACKED 2u

static jv p_value(js_vm_t *vm, uint32_t p) {
    js_obj_t *o = JOBJ(vm, p);
    jv v;
    v.t = (uint8_t)(o->arr & 0xFFu);
    v.f = (uint8_t)((o->arr >> 8) & 0xFFu);
    v.pad = 0;
    v.a = o->alen;
    v.b = o->acap;
    return v;
}

static void p_set_value(js_vm_t *vm, uint32_t p, jv v) {
    js_obj_t *o = JOBJ(vm, p);
    o->arr = (uint32_t)v.t | ((uint32_t)v.f << 8);
    o->alen = v.a;
    o->acap = v.b;
}

int js_is_promise(js_vm_t *vm, jv v) { return v.t == JT_OBJ && JOBJ(vm, v.a)->cls == JS_CLASS_PROMISE; }

int js_promise_state(js_vm_t *vm, jv v, jv *value) {
    if (!js_is_promise(vm, v)) return -1;
    if (value) *value = p_value(vm, v.a);
    return (int)JOBJ(vm, v.a)->x0;
}

uint32_t js_promise_new(js_vm_t *vm) {
    uint32_t p = js_obj_new(vm, JS_CLASS_PROMISE, vm->p_promise);
    if (!p) return 0;
    JOBJ(vm, p)->x0 = PS_PENDING;
    JOBJ(vm, p)->x1 = 0;
    JOBJ(vm, p)->x2 = 0;
    p_set_value(vm, p, jv_undef());
    return p;
}

static void untrack(js_vm_t *vm, uint32_t p) {
    int i;
    if (!(JOBJ(vm, p)->x2 & PF_TRACKED)) return;
    JOBJ(vm, p)->x2 &= ~PF_TRACKED;
    for (i = 0; i < vm->nunhandled; i++) {
        if (vm->unhandled[i] == p) {
            vm->unhandled[i] = vm->unhandled[--vm->nunhandled];
            return;
        }
    }
}

static void settle(js_vm_t *vm, uint32_t p, uint32_t state, jv v) {
    uint32_t r, nx;
    if (JOBJ(vm, p)->x0 != PS_PENDING) return;
    JOBJ(vm, p)->x0 = state;
    p_set_value(vm, p, v);
    r = JOBJ(vm, p)->x1;
    JOBJ(vm, p)->x1 = 0;
    if (state == PS_REJECTED && !r && !(JOBJ(vm, p)->x2 & PF_HANDLED)) {
        if (vm->nunhandled < JS_UNHANDLED_MAX) {
            vm->unhandled[vm->nunhandled++] = p;
            JOBJ(vm, p)->x2 |= PF_TRACKED;
        } else {
            vm->rejections_lost++;
        }
    }
    for (; r; r = nx) {
        nx = REC(vm, r)->next;
        /* the reaction record becomes the job */
        REC(vm, r)->kind = JOB_REACTION;
        REC(vm, r)->b = state == PS_FULFILLED ? RX_FULFILL : RX_REJECT;
        REC(vm, r)->v = v;
        job_push(vm, r);
    }
}

void js_promise_reject(js_vm_t *vm, uint32_t p, jv reason) { settle(vm, p, PS_REJECTED, reason); }

/* the resolve steps: a thenable is followed in a job, anything else fulfills */
void js_promise_resolve(js_vm_t *vm, uint32_t p, jv x) {
    jv then;
    if (JOBJ(vm, p)->x0 != PS_PENDING || vm->ctl == CTL_ABORT) return;
    if (x.t == JT_OBJ && x.a == p) {
        uint32_t e = js_error_new(vm, JE_TYPE, js_str_c(vm, "Chaining cycle detected for promise #<Promise>"));
        if (e) settle(vm, p, PS_REJECTED, jv_obj(e));
        return;
    }
    if (x.t != JT_OBJ) { settle(vm, p, PS_FULFILLED, x); return; }
    then = js_get(vm, x, KEY(vm, "then"));
    if (vm->ctl == CTL_THROW) {
        jv e = vm->exc;
        vm->ctl = CTL_NONE;
        vm->exc = jv_undef();
        settle(vm, p, PS_REJECTED, e);
        return;
    }
    if (vm->ctl) return;
    if (!js_is_callable(vm, then)) { settle(vm, p, PS_FULFILLED, x); return; }
    {
        uint32_t j = rec_new(vm);
        if (!j) return;
        REC(vm, j)->kind = JOB_THENABLE;
        REC(vm, j)->a = p;
        REC(vm, j)->v = x;
        REC(vm, j)->h = then;
        job_push(vm, j);
    }
}

/* PerformPromiseThen; derived may be 0 (await) */
static void perform_then(js_vm_t *vm, uint32_t p, jv onf, jv onr, uint32_t derived) {
    uint32_t r = rec_new(vm);
    if (!r) return;
    REC(vm, r)->h = js_is_callable(vm, onf) ? onf : jv_undef();
    REC(vm, r)->h2 = js_is_callable(vm, onr) ? onr : jv_undef();
    REC(vm, r)->a = derived;
    if (JOBJ(vm, p)->x0 == PS_PENDING) {
        /* append: reactions run in the order they were added */
        uint32_t t = JOBJ(vm, p)->x1;
        if (!t) JOBJ(vm, p)->x1 = r;
        else {
            while (REC(vm, t)->next) t = REC(vm, t)->next;
            REC(vm, t)->next = r;
        }
    } else {
        untrack(vm, p);
        REC(vm, r)->kind = JOB_REACTION;
        REC(vm, r)->b = JOBJ(vm, p)->x0 == PS_FULFILLED ? RX_FULFILL : RX_REJECT;
        REC(vm, r)->v = p_value(vm, p);
        job_push(vm, r);
    }
    JOBJ(vm, p)->x2 |= PF_HANDLED;
}

/* PromiseResolve(%Promise%, v) */
static uint32_t promise_of(js_vm_t *vm, jv v) {
    uint32_t p;
    if (js_is_promise(vm, v)) {
        jv c = js_get(vm, v, vm->s_constructor);
        if (vm->ctl) return 0;
        if (c.t == JT_OBJ && c.a == vm->c_promise) return v.a;
    }
    p = js_promise_new(vm);
    if (!p) return 0;
    js_promise_resolve(vm, p, v);
    return p;
}

/* resolving functions: magic is a record {a: promise, b: already resolved} */
static jv resolve_fn(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t cap = m & ~3u;
    (void)thisv;
    if (REC(vm, cap)->b) return jv_undef();
    REC(vm, cap)->b = 1;
    if (m & 1u) js_promise_reject(vm, REC(vm, cap)->a, JARG(0));
    else js_promise_resolve(vm, REC(vm, cap)->a, JARG(0));
    return jv_undef();
}

/* records are 4-byte aligned arena offsets below 2 GiB: the low bit of
 * magic says resolve (0) or reject (1) */
static int resolving_pair(js_vm_t *vm, uint32_t p, jv *res, jv *rej, uint32_t *cap_out) {
    uint32_t cap = rec_new(vm), f1, f2;
    if (!cap) return 0;
    REC(vm, cap)->a = p;
    f1 = js_native_new(vm, "", resolve_fn, 1, cap);
    f2 = js_native_new(vm, "", resolve_fn, 1, cap | 1u);
    if (!f1 || !f2) return 0;
    *res = jv_obj(f1);
    *rej = jv_obj(f2);
    if (cap_out) *cap_out = cap;
    return 1;
}

static void run_job(js_vm_t *vm, uint32_t j) {
    js_rec_t r = *REC(vm, j);
    rec_free(vm, j);
    if (r.kind == JOB_REACTION) {
        jv h = r.b == RX_FULFILL ? r.h : r.h2, out;
        if (h.t == JT_UNDEF) {
            if (r.a) {
                if (r.b == RX_FULFILL) js_promise_resolve(vm, r.a, r.v);
                else js_promise_reject(vm, r.a, r.v);
            }
            return;
        }
        out = js_call(vm, h, jv_undef(), &r.v, 1);
        if (vm->ctl == CTL_THROW) {
            jv e = vm->exc;
            vm->ctl = CTL_NONE;
            vm->exc = jv_undef();
            if (r.a) js_promise_reject(vm, r.a, e);
            return;
        }
        if (vm->ctl) return;
        if (r.a) js_promise_resolve(vm, r.a, out);
    } else if (r.kind == JOB_THENABLE) {
        jv args[2];
        uint32_t cap;
        if (!resolving_pair(vm, r.a, &args[0], &args[1], &cap)) return;
        (void)js_call(vm, r.h, r.v, args, 2);
        if (vm->ctl == CTL_THROW) {
            jv e = vm->exc;
            vm->ctl = CTL_NONE;
            vm->exc = jv_undef();
            if (!REC(vm, cap)->b) {
                REC(vm, cap)->b = 1;
                js_promise_reject(vm, r.a, e);
            }
        }
    } else if (r.kind == JOB_CALLBACK) {
        (void)js_call(vm, r.h, jv_undef(), 0, 0);
    }
}

static void note_uncaught(js_vm_t *vm, const char *pre, jv e) {
    char b[sizeof(vm->err)];
    int n = 0, i;
    for (i = 0; pre[i] && n < 60; i++) b[n++] = pre[i];
    n += js_display(vm, e, b + n, (int)sizeof(b) - n - 1, 1);
    if (vm->ctl == CTL_THROW) vm->ctl = CTL_NONE;
    b[n] = 0;
    if (!vm->err[0]) for (i = 0; i <= n; i++) vm->err[i] = b[i];
    js_log_put(vm, b, n);
    js_log_put(vm, "\n", 1);
}

/* run every queued job (and the jobs they queue); 1 when an error was
 * not caught (vm->err has the first), 0 otherwise. Runs on the private
 * stack (js_run_jobs in wf_jslib.c). */
int js_jobs_drain(js_vm_t *vm) {
    int errors = 0, i;
    uint32_t n = 0;
    while (vm->job_head && vm->ctl != CTL_ABORT) {
        uint32_t j = vm->job_head;
        vm->job_head = REC(vm, j)->next;
        if (!vm->job_head) vm->job_tail = 0;
        if (++n > JS_JOBS_MAX) { js_abort(vm, "RangeError: too many microtasks in one drain (guest bound)"); break; }
        vm->jobs_run++;
        run_job(vm, j);
        if (vm->ctl == CTL_THROW) {
            jv e = vm->exc;
            vm->ctl = CTL_NONE;
            vm->exc = jv_undef();
            note_uncaught(vm, "Uncaught ", e);
            errors++;
        }
        vm->depth = 0;
    }
    if (vm->ctl == CTL_ABORT) return errors;
    for (i = 0; i < vm->nunhandled; i++) {
        uint32_t p = vm->unhandled[i];
        JOBJ(vm, p)->x2 &= ~PF_TRACKED;
        JOBJ(vm, p)->x2 |= PF_HANDLED;      /* reported once */
        note_uncaught(vm, "Uncaught (in promise) ", p_value(vm, p));
        vm->rejections_reported++;
        errors++;
    }
    vm->nunhandled = 0;
    return errors;
}

/* ---- generators ---- */

jv js_gen_new(js_vm_t *vm, uint32_t f, uint32_t env) {
    uint32_t g = js_obj_new(vm, JS_CLASS_GEN, vm->p_gen), c = co_new(vm, CO_GEN, f, env);
    if (!g || !c) return jv_undef();
    JOBJ(vm, g)->x0 = c;
    return jv_obj(g);
}

static jv iter_result(js_vm_t *vm, jv v, int done) {
    uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!o) return jv_undef();
    js_define(vm, o, vm->s_value, v, P_DEFAULT);
    js_define(vm, o, vm->s_done, jv_bool(done), P_DEFAULT);
    return jv_obj(o);
}

/* one step of generator g: the yielded or returned value, *done set;
 * an exception of the body is left in vm->ctl */
jv js_gen_step(js_vm_t *vm, uint32_t g, int mode, jv v, int *done) {
    uint32_t c = JOBJ(vm, g)->x0;
    *done = 1;
    if (CO(vm, c)->state == CO_RUN) return js_throw_err(vm, JE_TYPE, "Generator is already running");
    if (CO(vm, c)->state == CO_NEW && mode != RM_NEXT) CO(vm, c)->state = CO_DONE;
    if (CO(vm, c)->state == CO_DONE) {
        if (mode == RM_THROW) return js_throw(vm, v);
        return mode == RM_RETURN ? v : jv_undef();
    }
    if (co_resume(vm, c, mode, v) != 0) return jv_undef();
    if (vm->ctl) {
        CO(vm, c)->state = CO_DONE;
        co_release(vm, c);
        return jv_undef();
    }
    *done = CO(vm, c)->state == CO_DONE;
    return CO(vm, c)->val;
}

int js_is_gen(js_vm_t *vm, jv v) { return v.t == JT_OBJ && JOBJ(vm, v.a)->cls == JS_CLASS_GEN; }

/* close g after a loop left early; keeps the loop's own completion */
void js_gen_close(js_vm_t *vm, uint32_t g) {
    int sctl = vm->ctl, done;
    jv sret = vm->ret, sexc = vm->exc;
    uint32_t slabel = vm->label;
    uint32_t c = JOBJ(vm, g)->x0;
    if (sctl == CTL_ABORT || CO(vm, c)->state == CO_DONE || CO(vm, c)->state == CO_RUN) return;
    vm->ctl = CTL_NONE;
    (void)js_gen_step(vm, g, RM_RETURN, jv_undef(), &done);
    if (vm->ctl == CTL_NONE || sctl == CTL_THROW) {
        if (vm->ctl != CTL_ABORT) { vm->ctl = sctl; vm->ret = sret; vm->exc = sexc; vm->label = slabel; }
    }
}

static jv gen_method(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    int done;
    jv r;
    if (!js_is_gen(vm, thisv)) return js_throw_err(vm, JE_TYPE, "next method called on a value that is not a generator");
    r = js_gen_step(vm, thisv.a, (int)m, JARG(0), &done);
    if (vm->ctl) return jv_undef();
    return iter_result(vm, r, done);
}

/* yield (delegate = 0) and yield* (delegate = 1) inside a generator body */
jv js_co_yield(js_vm_t *vm, jv v, int delegate) {
    uint32_t c = vm->co_cur;
    if (!c || CO(vm, c)->kind != CO_GEN) return js_throw_err(vm, JE_SYNTAX, "yield outside a generator");
    if (!delegate) return co_suspend(vm, v);
    if (js_is_gen(vm, v) || (v.t == JT_OBJ && JOBJ(vm, v.a)->cls != JS_CLASS_ARRAY && JOBJ(vm, v.a)->cls != JS_CLASS_STRING &&
                             js_is_callable(vm, js_get(vm, v, vm->s_next)))) {
        /* an iterator: forward next / throw / return */
        jv received = jv_undef(), next;
        int mode = RM_NEXT;
        if (vm->ctl) return jv_undef();
        next = js_get(vm, v, vm->s_next);
        if (vm->ctl) return jv_undef();
        for (;;) {
            jv r, d, fn;
            if (mode == RM_NEXT) {
                r = js_call(vm, next, v, &received, 1);
            } else {
                fn = js_get(vm, v, KEY(vm, mode == RM_THROW ? "throw" : "return"));
                if (vm->ctl) return jv_undef();
                if (!js_is_callable(vm, fn)) {
                    if (mode == RM_RETURN) { vm->ret = received; vm->ctl = CTL_RETURN; return jv_undef(); }
                    return js_throw_err(vm, JE_TYPE, "The iterator does not provide a 'throw' method");
                }
                r = js_call(vm, fn, v, &received, 1);
            }
            if (vm->ctl) return jv_undef();
            if (r.t != JT_OBJ) return js_throw_err(vm, JE_TYPE, "Iterator result is not an object");
            d = js_get(vm, r, vm->s_done);
            if (vm->ctl) return jv_undef();
            if (js_truthy(vm, d)) {
                jv val = js_get(vm, r, vm->s_value);
                if (vm->ctl) return jv_undef();
                if (mode == RM_RETURN) { vm->ret = val; vm->ctl = CTL_RETURN; return jv_undef(); }
                return val;
            }
            {
                jv yv = js_get(vm, r, vm->s_value);
                if (vm->ctl) return jv_undef();
                received = co_suspend_raw(vm, yv, &mode);
            }
        }
    }
    {
        /* arrays, strings, Map, Set: one yield per element */
        uint32_t arr = 0, i;
        js_iter_values(vm, v, &arr);
        if (vm->ctl) return jv_undef();
        for (i = 0; i < JOBJ(vm, arr)->alen && !vm->ctl; i++) (void)co_suspend(vm, js_array_get(vm, arr, i));
        return jv_undef();
    }
}

/* ---- async functions ---- */

static void async_after(js_vm_t *vm, uint32_t c) {
    uint32_t p = CO(vm, c)->promise;
    if (CO(vm, c)->state != CO_DONE) return;
    if (vm->ctl == CTL_THROW) {
        jv e = vm->exc;
        vm->ctl = CTL_NONE;
        vm->exc = jv_undef();
        js_promise_reject(vm, p, e);
    } else if (vm->ctl == CTL_NONE) {
        js_promise_resolve(vm, p, CO(vm, c)->val);
    }
}

static jv await_resume(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t c = m & ~3u;
    (void)thisv;
    if (CO(vm, c)->state != CO_SUSP) return jv_undef();
    if (co_resume(vm, c, (m & 1u) ? RM_THROW : RM_NEXT, JARG(0)) != 0) return jv_undef();
    async_after(vm, c);
    return jv_undef();
}

jv js_async_call(js_vm_t *vm, uint32_t f, uint32_t env) {
    uint32_t p = js_promise_new(vm), c;
    if (!p) return jv_undef();
    c = co_new(vm, CO_ASYNC, f, env);
    if (!c) return jv_undef();
    CO(vm, c)->promise = p;
    if (co_resume(vm, c, RM_NEXT, jv_undef()) != 0) {
        if (vm->ctl == CTL_THROW) {
            jv e = vm->exc;
            vm->ctl = CTL_NONE;
            vm->exc = jv_undef();
            js_promise_reject(vm, p, e);
        }
        return vm->ctl ? jv_undef() : jv_obj(p);
    }
    async_after(vm, c);
    return vm->ctl ? jv_undef() : jv_obj(p);
}

/* an async function whose parameters threw: a rejected promise */
jv js_async_rejected(js_vm_t *vm) {
    uint32_t p;
    jv e;
    if (vm->ctl != CTL_THROW) return jv_undef();
    e = vm->exc;
    vm->ctl = CTL_NONE;
    vm->exc = jv_undef();
    p = js_promise_new(vm);
    if (!p) return jv_undef();
    js_promise_reject(vm, p, e);
    return jv_obj(p);
}

jv js_co_await(js_vm_t *vm, jv v) {
    uint32_t c = vm->co_cur, p;
    if (!c || CO(vm, c)->kind != CO_ASYNC) return js_throw_err(vm, JE_SYNTAX, "await is only valid in async functions");
    p = promise_of(vm, v);
    if (!p || vm->ctl) return jv_undef();
    if (!CO(vm, c)->aful) {
        uint32_t f1 = js_native_new(vm, "", await_resume, 1, c);
        uint32_t f2 = js_native_new(vm, "", await_resume, 1, c | 1u);
        if (!f1 || !f2) return jv_undef();
        CO(vm, c)->aful = f1;
        CO(vm, c)->arej = f2;
    }
    perform_then(vm, p, jv_obj(CO(vm, c)->aful), jv_obj(CO(vm, c)->arej), 0);
    if (vm->ctl) return jv_undef();
    return co_suspend(vm, jv_undef());
}

/* ---- Promise built-ins ---- */

static jv promise_ctor(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t p, cap;
    jv args[2], ex = JARG(0);
    (void)thisv; (void)m;
    if (!vm->in_new) return js_throw_err(vm, JE_TYPE, "Promise constructor cannot be invoked without 'new'");
    vm->in_new = 0;
    if (!js_is_callable(vm, ex)) {
        char b[96];
        int n = js_display(vm, ex, b, (int)sizeof(b), 0);
        if (vm->ctl) return jv_undef();
        b[n] = 0;
        return js_throw_err2(vm, JE_TYPE, "Promise resolver ",
                             js_str_cat(vm, js_str_c(vm, b), js_str_c(vm, " is not a function")));
    }
    p = js_promise_new(vm);
    if (!p || !resolving_pair(vm, p, &args[0], &args[1], &cap)) return jv_undef();
    (void)js_call(vm, ex, jv_undef(), args, 2);
    if (vm->ctl == CTL_THROW) {
        jv e = vm->exc;
        vm->ctl = CTL_NONE;
        vm->exc = jv_undef();
        if (!REC(vm, cap)->b) {
            REC(vm, cap)->b = 1;
            js_promise_reject(vm, p, e);
        }
    }
    return vm->ctl ? jv_undef() : jv_obj(p);
}

static jv promise_then(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t d;
    if (!js_is_promise(vm, thisv)) return js_throw_err(vm, JE_TYPE, "Promise.prototype.then called on a value that is not a promise");
    d = js_promise_new(vm);
    if (!d) return jv_undef();
    if (m == 1) perform_then(vm, thisv.a, jv_undef(), JARG(0), d);     /* catch */
    else perform_then(vm, thisv.a, JARG(0), JARG(1), d);
    return jv_obj(d);
}

/* finally: magic is a record {h: onFinally, v: value, b: rejected} */
static jv finally_value(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    (void)thisv; (void)argv; (void)argc;
    if (REC(vm, m)->b) return js_throw(vm, REC(vm, m)->v);
    return REC(vm, m)->v;
}

static jv finally_step(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t rec = m & ~3u, keep, p, f, d;
    jv r;
    (void)thisv;
    r = js_call(vm, REC(vm, rec)->h, jv_undef(), 0, 0);
    if (vm->ctl) return jv_undef();
    p = promise_of(vm, r);
    if (!p) return jv_undef();
    keep = rec_new(vm);
    if (!keep) return jv_undef();
    REC(vm, keep)->v = JARG(0);
    REC(vm, keep)->b = m & 1u;
    f = js_native_new(vm, "", finally_value, 0, keep);
    d = js_promise_new(vm);
    if (!f || !d) return jv_undef();
    perform_then(vm, p, jv_obj(f), jv_undef(), d);
    return jv_obj(d);
}

static jv promise_finally(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    jv h = JARG(0);
    uint32_t d, rec, f1, f2;
    (void)m;
    if (!js_is_promise(vm, thisv)) return js_throw_err(vm, JE_TYPE, "Promise.prototype.finally called on a value that is not a promise");
    d = js_promise_new(vm);
    if (!d) return jv_undef();
    if (!js_is_callable(vm, h)) { perform_then(vm, thisv.a, h, h, d); return jv_obj(d); }
    rec = rec_new(vm);
    if (!rec) return jv_undef();
    REC(vm, rec)->h = h;
    f1 = js_native_new(vm, "", finally_step, 1, rec);
    f2 = js_native_new(vm, "", finally_step, 1, rec | 1u);
    if (!f1 || !f2) return jv_undef();
    perform_then(vm, thisv.a, jv_obj(f1), jv_obj(f2), d);
    return jv_obj(d);
}

static jv promise_resolve_s(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t p;
    (void)thisv;
    if (m == 1) {
        p = js_promise_new(vm);
        if (!p) return jv_undef();
        js_promise_reject(vm, p, JARG(0));
        return jv_obj(p);
    }
    p = promise_of(vm, JARG(0));
    return p && !vm->ctl ? jv_obj(p) : jv_undef();
}

/* Promise.all / allSettled / any: one element function per entry.
 * shared record: a = result promise, b = remaining, v = array, kind = op
 * element record: a = shared, b = index, kind = already called */
enum { COMB_ALL = 0, COMB_SETTLED, COMB_ANY, COMB_RACE };

static jv settled_entry(js_vm_t *vm, int ok, jv v) {
    uint32_t o = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!o) return jv_undef();
    js_define(vm, o, KEY(vm, "status"), jv_str(js_str_c(vm, ok ? "fulfilled" : "rejected")), P_DEFAULT);
    js_define(vm, o, ok ? vm->s_value : KEY(vm, "reason"), v, P_DEFAULT);
    return jv_obj(o);
}

static void comb_done(js_vm_t *vm, uint32_t sh) {
    uint32_t p = REC(vm, sh)->a;
    if (REC(vm, sh)->kind == COMB_ANY) {
        uint32_t e = js_error_new(vm, JE_ERROR, js_str_c(vm, "All promises were rejected"));
        if (!e) return;
        js_define(vm, e, vm->s_name, jv_str(js_str_c(vm, "AggregateError")), P_HIDDEN);
        js_define(vm, e, KEY(vm, "errors"), REC(vm, sh)->v, P_HIDDEN);
        js_promise_reject(vm, p, jv_obj(e));
    } else {
        js_promise_resolve(vm, p, REC(vm, sh)->v);
    }
}

static jv comb_element(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t el = m & ~3u, sh = REC(vm, el)->a;
    int rejected = (int)(m & 1u);
    jv v = JARG(0);
    (void)thisv;
    if (REC(vm, el)->kind) return jv_undef();
    REC(vm, el)->kind = 1;
    if (REC(vm, sh)->kind == COMB_SETTLED) v = settled_entry(vm, !rejected, v);
    if (vm->ctl) return jv_undef();
    js_array_set(vm, REC(vm, sh)->v.a, REC(vm, el)->b, v);
    if (--REC(vm, sh)->b == 0) comb_done(vm, sh);
    return jv_undef();
}

/* settle the result directly (race, the reject of all, the resolve of any) */
static jv comb_direct(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t p = m & ~3u;
    (void)thisv;
    if (m & 1u) js_promise_reject(vm, p, JARG(0));
    else js_promise_resolve(vm, p, JARG(0));
    return jv_undef();
}

static jv promise_comb(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t p = js_promise_new(vm), arr = 0, sh, i, n, out;
    jv direct_res, direct_rej;
    (void)thisv;
    if (!p) return jv_undef();
    js_iter_values(vm, JARG(0), &arr);
    if (vm->ctl == CTL_THROW) {
        jv e = vm->exc;
        vm->ctl = CTL_NONE;
        vm->exc = jv_undef();
        js_promise_reject(vm, p, e);
        return jv_obj(p);
    }
    if (vm->ctl) return jv_undef();
    n = JOBJ(vm, arr)->alen;
    sh = rec_new(vm);
    out = js_array_new(vm, n ? n : 1);
    if (!sh || !out) return jv_undef();
    js_array_set_len(vm, out, n);
    REC(vm, sh)->a = p;
    REC(vm, sh)->b = n;
    REC(vm, sh)->v = jv_obj(out);
    REC(vm, sh)->kind = m;
    {
        uint32_t f1 = js_native_new(vm, "", comb_direct, 1, p);
        uint32_t f2 = js_native_new(vm, "", comb_direct, 1, p | 1u);
        if (!f1 || !f2) return jv_undef();
        direct_res = jv_obj(f1);
        direct_rej = jv_obj(f2);
    }
    if (n == 0 && m != COMB_RACE) { comb_done(vm, sh); return jv_obj(p); }
    for (i = 0; i < n && !vm->ctl; i++) {
        uint32_t ip = promise_of(vm, js_array_get(vm, arr, i)), el, fe;
        jv onf, onr;
        if (!ip || vm->ctl) break;
        if (m == COMB_RACE) { onf = direct_res; onr = direct_rej; }
        else {
            el = rec_new(vm);
            if (!el) break;
            REC(vm, el)->a = sh;
            REC(vm, el)->b = i;
            if (m == COMB_ALL) {
                fe = js_native_new(vm, "", comb_element, 1, el);
                onf = jv_obj(fe);
                onr = direct_rej;
            } else if (m == COMB_ANY) {
                fe = js_native_new(vm, "", comb_element, 1, el | 1u);
                onf = direct_res;
                onr = jv_obj(fe);
            } else {
                uint32_t f1 = js_native_new(vm, "", comb_element, 1, el);
                fe = js_native_new(vm, "", comb_element, 1, el | 1u);
                onf = jv_obj(f1);
                onr = jv_obj(fe);
            }
            if (!fe) break;
        }
        perform_then(vm, ip, onf, onr, js_promise_new(vm));
    }
    if (vm->ctl == CTL_THROW) {
        jv e = vm->exc;
        vm->ctl = CTL_NONE;
        vm->exc = jv_undef();
        js_promise_reject(vm, p, e);
    }
    return vm->ctl ? jv_undef() : jv_obj(p);
}

static jv queue_microtask(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t m) {
    uint32_t j;
    (void)thisv; (void)m;
    if (!js_is_callable(vm, JARG(0))) return js_throw_err(vm, JE_TYPE, "queueMicrotask: callback is not a function");
    j = rec_new(vm);
    if (!j) return jv_undef();
    REC(vm, j)->kind = JOB_CALLBACK;
    REC(vm, j)->h = JARG(0);
    job_push(vm, j);
    return jv_undef();
}

int js_co_init(js_vm_t *vm) {
    uint32_t pp = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object), c, gp;
    if (!pp) return -1;
    vm->p_promise = pp;
    c = js_ctor_new(vm, "Promise", promise_ctor, 1, pp);
    if (!c) return -1;
    vm->c_promise = c;
    js_def_native(vm, pp, "then", promise_then, 2, 0);
    js_def_native(vm, pp, "catch", promise_then, 1, 1);
    js_def_native(vm, pp, "finally", promise_finally, 1, 0);
    js_def_native(vm, c, "resolve", promise_resolve_s, 1, 0);
    js_def_native(vm, c, "reject", promise_resolve_s, 1, 1);
    js_def_native(vm, c, "all", promise_comb, 1, COMB_ALL);
    js_def_native(vm, c, "allSettled", promise_comb, 1, COMB_SETTLED);
    js_def_native(vm, c, "any", promise_comb, 1, COMB_ANY);
    js_def_native(vm, c, "race", promise_comb, 1, COMB_RACE);
    js_def_native(vm, vm->global, "queueMicrotask", queue_microtask, 1, 0);
    gp = js_obj_new(vm, JS_CLASS_OBJECT, vm->p_object);
    if (!gp) return -1;
    vm->p_gen = gp;
    js_def_native(vm, gp, "next", gen_method, 1, RM_NEXT);
    js_def_native(vm, gp, "return", gen_method, 1, RM_RETURN);
    js_def_native(vm, gp, "throw", gen_method, 1, RM_THROW);
    return vm->ctl == CTL_ABORT ? -1 : 0;
}
