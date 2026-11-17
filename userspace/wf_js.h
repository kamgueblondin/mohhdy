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
 * What it is not: no generators, async functions, Promise, Proxy,
 * Symbol, typed arrays, getters on arbitrary built-ins, labels on
 * blocks, with, eval, or tail calls. Strings are byte strings (UTF-8
 * bytes as they come; length counts bytes, not UTF-16 units). There is
 * no garbage collector: one realm lives in one caller-given arena and
 * an allocation past its end aborts the run (RangeError-like, not
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
/* limits for the next js_eval / js_call_value */
void js_set_budget(js_vm_t *vm, uint32_t steps);
/* arena use: bytes used, bytes total */
uint32_t js_mem_used(const js_vm_t *vm);
uint32_t js_mem_size(const js_vm_t *vm);
uint32_t js_steps_used(const js_vm_t *vm);

#endif
