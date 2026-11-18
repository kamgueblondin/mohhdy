/* wf_js.h - Lot 113 (US-031): a bounded JavaScript subset in Ring 3.
 *
 * What it is: a tree-walking interpreter for the core of ECMAScript 5
 * with the ES2015+ forms pages use most (let/const with block scope,
 * arrow functions, template literals, classes with extends and super,
 * destructuring, default and rest parameters, spread, for-of on arrays
 * and strings, optional chaining, ??, **), the usual built-ins (Object,
 * Function, Array, String, Number, Boolean, Math, JSON, RegExp, Error
 * and its subtypes, a fixed-clock Date) and a console that writes to a
 * bounded log. Numbers are binary64 in software (wf_jsnum.h).
 *
 * Lot 114 adds generators (function*, yield, yield*), async functions
 * and arrows (await), Promise (then/catch/finally, resolve, reject,
 * all, allSettled, any, race) and queueMicrotask; microtasks run when
 * the host calls js_run_jobs.
 *
 * Lot 115 adds Symbol (Symbol.for, the well-known symbols iterator,
 * hasInstance, toPrimitive and toStringTag honoured by the runtime), the
 * iterator protocol for user objects (lazy for-of with return(), spread,
 * destructuring that reads only what it needs), class private members
 * (#x fields, methods, accessors, static ones, #x in o) and static
 * blocks, and UTF-16 string semantics: strings stay UTF-8 bytes in the
 * arena but length and every index count UTF-16 code units
 * (wf_jsu16.c).
 *
 * What it is not: no async generators, for await, Proxy, typed arrays, getters on arbitrary built-ins, labels on
 * blocks, with, eval, or tail calls. String comparison (<, sort) orders
 * UTF-8 bytes, which differs from UTF-16 order only between astral
 * characters and U+E000..U+FFFF; case mapping covers ASCII and the
 * 2-byte letters whose two cases have the same length. One realm
 * lives in one caller-given arena; wf_jsgc.c reclaims unreachable
 * blocks between runs (mark and sweep, blocks never move) and an
 * allocation past the end during a run aborts it (RangeError-like, not
 * catchable). A step budget and a call-depth limit stop runaway code
 * the same way. The interpreter runs on its own stack (js_vm_t.stack)
 * so the 64 KiB Ring 3 stack is not used by JavaScript recursion.
 *
 * Freestanding C: no libc, no FPU, no 64-bit division. */
#ifndef WF_JS_H
#define WF_JS_H

#include <stdint.h>

typedef struct js_vm js_vm_t;

/* a value: 12 bytes, see wf_jsi.h for the tags */
typedef struct { uint8_t t; uint8_t f; uint16_t pad; uint32_t a; uint32_t b; } jv;

typedef jv (*js_native_fn)(js_vm_t *vm, jv thisv, jv *argv, int argc, uint32_t magic);

/* host objects (the page DOM): a get/set hook on objects of class
 * JS_CLASS_HOST; return 1 when the hook handled the key */
typedef int (*js_host_get_fn)(js_vm_t *vm, jv obj, uint32_t key, jv *out);
typedef int (*js_host_set_fn)(js_vm_t *vm, jv obj, uint32_t key, jv val);
/* Lot 115: delete obj[key] on a host object; 1 when handled (*ok gets
 * the result of the delete operator) */
typedef int (*js_host_del_fn)(js_vm_t *vm, jv obj, uint32_t key, int *ok);

#define JS_OK        0
#define JS_THROWN   -1      /* an exception nobody caught; vm->err has it */
#define JS_ABORTED  -2      /* budget, depth, memory or syntax: vm->err */

/* one realm in mem[0..size); stack_size bytes of mem's end are the
 * interpreter's stack. Returns 0, or -1 when the arena is too small. */
int  js_init(js_vm_t *vm, void *mem, uint32_t size);
/* run a script (global code); *out (may be 0) gets its completion value */
int  js_eval(js_vm_t *vm, const char *src, int len, jv *out);
/* call a function value with no this and argc arguments */
int  js_call_value(js_vm_t *vm, jv fn, jv thisv, jv *argv, int argc, jv *out);
/* the console/REPL display of a value (strings quoted when quote != 0) */
int  js_display(js_vm_t *vm, jv v, char *out, int max, int quote);
/* String(v) into out */
int  js_to_cstr(js_vm_t *vm, jv v, char *out, int max);
/* the console log since the last js_log_clear */
const char *js_log_text(js_vm_t *vm, int *len);
void js_log_clear(js_vm_t *vm);
/* the error text of the last run that did not return JS_OK */
const char *js_last_error(const js_vm_t *vm);
/* sizeof(js_vm_t), for callers that allocate it */
int js_vm_size(void);
/* limits for the next js_eval / js_call_value */
void js_set_budget(js_vm_t *vm, uint32_t steps);
/* arena use: bytes used, bytes total */
uint32_t js_mem_used(const js_vm_t *vm);
uint32_t js_mem_size(const js_vm_t *vm);
uint32_t js_steps_used(const js_vm_t *vm);
/* Lot 114: run the queued microtasks (promise reactions, await
 * resumptions, queueMicrotask callbacks) until the queue is empty.
 * JS_OK, JS_THROWN when a callback threw or a rejection was not handled
 * (vm->err has the first, the log has every one), JS_ABORTED on a bound. */
/* Lot 115: arena reclaim (wf_jsgc.c). js_gc collects now (0 inside a
 * running script); js_gc_idle collects when enough was allocated since
 * the last run. Host roots: the callback calls js_gc_scan on every
 * area outside the arena that holds references. */
typedef void (*js_gc_roots_fn)(js_vm_t *vm, void *ctx);
typedef struct { uint32_t runs, live, freed_last, freed_total, free_bytes, top, bitmap; } js_gc_stats_t;
uint32_t js_gc(js_vm_t *vm);
int js_gc_idle(js_vm_t *vm);
void js_gc_scan(js_vm_t *vm, const void *p, uint32_t bytes);
void js_gc_set_roots(js_vm_t *vm, js_gc_roots_fn fn, void *ctx);
void js_gc_config(js_vm_t *vm, int stress, int poison, int off);
void js_gc_stats(const js_vm_t *vm, js_gc_stats_t *st);
uint32_t js_mem_free(const js_vm_t *vm);
int  js_run_jobs(js_vm_t *vm);
int  js_jobs_pending(const js_vm_t *vm);

#endif
