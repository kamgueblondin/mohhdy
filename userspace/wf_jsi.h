/* wf_jsi.h - Lot 113: internals shared by wf_jsrt.c (values, strings,
 * objects), wf_jsparse.c (lexer, parser), wf_jsrun.c (interpreter),
 * wf_jslib.c (built-ins), wf_jsre.c (regular expressions) and
 * wf_jsdom.c (the page DOM). Every reference is an offset into the
 * realm's arena (0 is "none"). */
#ifndef WF_JSI_H
#define WF_JSI_H

#include <stdint.h>
#include "wf_js.h"
#include "wf_jsnum.h"

/* value tags */
enum { JT_UNDEF = 0, JT_NULL, JT_BOOL, JT_INT, JT_NUM, JT_STR, JT_OBJ, JT_EMPTY };

/* object classes */
enum {
    JS_CLASS_OBJECT = 0, JS_CLASS_ARRAY, JS_CLASS_FUNC, JS_CLASS_NATIVE, JS_CLASS_BOUND, JS_CLASS_ERROR,
    JS_CLASS_BOOLEAN, JS_CLASS_NUMBER, JS_CLASS_STRING, JS_CLASS_REGEXP, JS_CLASS_ARGS, JS_CLASS_DATE,
    JS_CLASS_HOST, JS_CLASS_ITER, JS_CLASS_MAP, JS_CLASS_SET,
    JS_CLASS_PROMISE, JS_CLASS_GEN           /* Lot 114 */
};

typedef struct { uint32_t len; uint32_t hash; uint32_t interned; char s[4]; } js_str_t;

#define P_ENUM     1u
#define P_WRITE    2u
#define P_CONF     4u
#define P_ACCESSOR 8u
#define P_DEFAULT  (P_ENUM | P_WRITE | P_CONF)
#define P_HIDDEN   (P_WRITE | P_CONF)

typedef struct { uint32_t key; uint32_t flags; jv v; } js_prop_t;

#define O_EXT     1u      /* extensible */
#define O_FROZEN  2u

typedef struct {
    uint8_t cls, oflags;
    uint16_t nprop, pcap, pad;
    uint32_t proto, props;
    uint32_t arr, alen, acap;
    uint32_t x0, x1, x2;
} js_obj_t;

/* environments (scopes); vars reuse the property layout */
#define V_CONST 1u
#define V_LET   2u
#define V_TDZ   4u
enum { E_GLOBAL = 0, E_FUNC, E_ARROW, E_BLOCK, E_CATCH };
#define E_KIND(k) ((k) & 0xFFu)
#define E_CAPTURED 0x100u     /* a closure holds it: never recycled */

typedef struct {
    uint32_t parent;
    uint16_t n, cap;
    uint32_t vars;
    uint32_t kind;
    jv thisv;
    uint32_t fobj;       /* the called function (super, new.target) */
    uint32_t newtarget;
} js_env_t;

/* AST */
typedef struct {
    uint8_t k, op;
    uint16_t fl;
    uint32_t line;
    uint32_t a, b, c, d, next;
} js_node_t;

enum {
    N_NONE = 0,
    /* expressions */
    N_NUM, N_STR, N_TMPL, N_TTAG, N_IDENT, N_THIS, N_SUPER, N_ARRAY, N_OBJECT, N_PROP, N_FUNC, N_CLASS, N_UNARY,
    N_UPDATE, N_BINARY, N_LOGICAL, N_ASSIGN, N_COND, N_CALL, N_NEW, N_MEMBER, N_INDEX, N_SEQ, N_SPREAD,
    N_REGEX, N_NULL, N_TRUE, N_FALSE, N_UNDEF_LIT, N_NEWTARGET, N_ELISION,
    /* patterns */
    N_OPAT, N_APAT, N_PPROP, N_PELEM, N_PARAM,
    /* statements */
    N_PROG, N_VAR, N_DECL, N_EXPR, N_BLOCK, N_IF, N_FOR, N_FORIN, N_FOROF, N_WHILE, N_DOWHILE, N_RETURN,
    N_BREAK, N_CONTINUE, N_THROW, N_TRY, N_SWITCH, N_CASE, N_LABEL, N_EMPTY, N_FUNCDECL, N_CLASSDECL,
    N_VARNAME, N_METHOD, N_FIELD, N_DEBUGGER,
    /* Lot 114 */
    N_YIELD,          /* op 1: yield* (delegate) */
    N_AWAIT
};

/* Lot 114: N_FUNC op bits (the 16 fl bits are all taken) */
#define FK_GEN   1u       /* function* */
#define FK_ASYNC 2u       /* async function */

/* N_VAR op */
#define DK_VAR   0
#define DK_LET   1
#define DK_CONST 2

/* node flags */
#define F_ARROW     0x0001u
#define F_EXPRBODY  0x0002u
#define F_ARGS      0x0004u   /* body reads `arguments` */
#define F_METHOD    0x0008u
#define F_CTOR      0x0010u   /* class constructor */
#define F_DERIVED   0x0020u   /* class with extends */
#define F_STATIC    0x0040u
#define F_COMPUTED  0x0080u
#define F_REST      0x0100u
#define F_OPT       0x0200u   /* ?. on this link */
#define F_CHAIN     0x0400u   /* outermost link of an optional chain */
#define F_PREFIX    0x0800u
#define F_LEX       0x1000u   /* block declares let/const/class/function */
#define F_SHORT     0x2000u   /* shorthand property */
#define F_GETTER    0x4000u
#define F_SETTER    0x8000u

/* operators (punctuators and keyword operators) */
enum {
    OP_NONE = 0,
    OP_LBRACE, OP_RBRACE, OP_LPAREN, OP_RPAREN, OP_LBRACK, OP_RBRACK, OP_SEMI, OP_COMMA, OP_DOT, OP_ELLIPSIS,
    OP_QDOT, OP_QUESTION, OP_COLON, OP_ARROW,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE, OP_SEQ, OP_SNE,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_EXP, OP_INC, OP_DEC,
    OP_SHL, OP_SAR, OP_SHR, OP_BAND, OP_BOR, OP_BXOR, OP_NOT, OP_BNOT, OP_AND, OP_OR, OP_NULLISH,
    OP_ASSIGN, OP_ADD_A, OP_SUB_A, OP_MUL_A, OP_DIV_A, OP_MOD_A, OP_EXP_A, OP_SHL_A, OP_SAR_A, OP_SHR_A,
    OP_BAND_A, OP_BOR_A, OP_BXOR_A, OP_AND_A, OP_OR_A, OP_NULLISH_A,
    OP_TYPEOF, OP_VOID, OP_DELETE, OP_IN, OP_INSTANCEOF, OP_NEG, OP_PLUS,
    OP_COUNT
};

/* control state after a statement */
enum { CTL_NONE = 0, CTL_RETURN, CTL_BREAK, CTL_CONTINUE, CTL_THROW, CTL_ABORT };

#define JS_MAX_NATIVES 384
#define JS_ITAB 8192          /* interned string slots (power of two) */
#define JS_STACK_SIZE (384u * 1024u)
#define JS_STACK_RESERVE (48u * 1024u)   /* abort below this much free stack */
#define JS_LOG_MAX 8192
#define JS_MAX_DEPTH 64
#define JS_DEFAULT_BUDGET 4000000u
/* Lot 114: coroutines (generators, async functions) and microtasks */
#define JS_CO_STACK (192u * 1024u)       /* per started coroutine, in the arena */
#define JS_CALL_MARGIN (16u * 1024u)     /* calls stop this far above the abort threshold */
#define JS_CO_MAX 8                      /* coroutines holding a stack at once */
#define JS_JOBS_MAX 100000u              /* microtasks in one drain */
#define JS_UNHANDLED_MAX 16

/* JS_CLASS_FUNC objects use arr/alen/acap for: */
#define FN_HOME(o)  ((o)->arr)        /* home object (super lookups) */
#define FN_CLASS(o) ((o)->alen)       /* class node (instance fields) */
#define FN_FLAGS(o) ((o)->acap)
#define FN_CLASSCTOR 1u
#define FN_DERIVED   2u
#define FN_DEFAULT   4u               /* synthesized default constructor */

/* error kinds */
enum { JE_ERROR = 0, JE_TYPE, JE_RANGE, JE_SYNTAX, JE_REFERENCE, JE_URI, JE_EVAL, JE_COUNT };

struct js_vm {
    uint8_t *mem;
    uint32_t size, top, limit;            /* limit: start of the stack area */
    uint8_t *stack_top, *stack_base;
    uint32_t itab, icount;
    uint32_t global, genv;
    uint32_t p_object, p_function, p_array, p_string, p_number, p_boolean, p_regexp, p_date, p_iter, p_map, p_set;
    uint32_t p_err[JE_COUNT];
    uint32_t c_object, c_array, c_function;
    int ctl;
    jv exc, ret;
    uint32_t label;                       /* break/continue label (str) or 0 */
    uint32_t steps, budget;
    int depth;
    int opt_short;
    int in_new;                           /* a native called through new */
    uint32_t super_this;                  /* a native called through super() */
    uint32_t free_env;                    /* recycled environments */
    uint32_t free_vars[8];                /* recycled var arrays, cap 4 << k */
    jv completion;                        /* value of the last expression statement */
    char err[200];
    int err_line;
    char log[JS_LOG_MAX];
    int log_len;
    js_native_fn nat[JS_MAX_NATIVES];
    uint32_t nat_magic[JS_MAX_NATIVES];
    int nnat;
    js_host_get_fn host_get;
    js_host_set_fn host_set;
    void *host_ctx;
    uint32_t rng;
    uint32_t date_now_lo, date_now_hi;    /* fixed clock (ms since 1970) */
    /* common interned names */
    uint32_t s_length, s_prototype, s_constructor, s_message, s_name, s_toString, s_valueOf, s_callee,
        s_arguments, s_get, s_set, s_value, s_writable, s_enumerable, s_configurable, s_index, s_input,
        s_lastIndex, s_join, s_stack, s_cause, s_default, s_empty, s_undefined, s_toJSON, s_next, s_done,
        s_raw, s_source, s_flags, s_global;
    /* parser output kept for the run */
    const char *src;
    int src_len;
    /* entry for js_eval on the private stack */
    int run_rc;
    uint32_t run_prog;
    jv run_out;
    jv run_fn, run_this;
    jv *run_argv;
    int run_argc;
    /* Lot 114: promises, generators, async functions (wf_jsco.c) */
    uint32_t p_promise, p_gen, c_promise;
    uint32_t co_cur;                      /* the running coroutine, 0 on the main stack */
    uint32_t co_free_stack;               /* recycled coroutine stacks */
    int co_live;
    uint32_t job_head, job_tail;          /* the microtask queue */
    uint32_t rec_free;                    /* recycled job and reaction records */
    uint32_t unhandled[JS_UNHANDLED_MAX];
    int nunhandled;
    uint32_t jobs_run, rejections_reported, rejections_lost;
};

#define JPTR(vm, r) ((void *)((vm)->mem + (r)))
#define JOBJ(vm, r) ((js_obj_t *)JPTR(vm, r))
#define JSTR(vm, r) ((js_str_t *)JPTR(vm, r))
#define JNODE(vm, r) ((js_node_t *)JPTR(vm, r))
#define JENV(vm, r) ((js_env_t *)JPTR(vm, r))

/* ---- wf_jsrt.c ---- */
jv jv_undef(void);
jv jv_null(void);
jv jv_bool(int b);
jv jv_int(int32_t i);
jv jv_num(jf64 n);                 /* an int32 value is stored as JT_INT */
jv jv_str(uint32_t r);
jv jv_obj(uint32_t r);
jv jv_empty(void);
jf64 jv_f64(jv v);                 /* JT_INT or JT_NUM */
int jv_is_num(jv v);
int jv_is_obj(jv v);
int jv_is_nullish(jv v);

uint32_t js_alloc(js_vm_t *vm, uint32_t n);          /* 0 on exhaustion (vm aborted) */
void js_abort(js_vm_t *vm, const char *msg);
uint32_t js_str_new(js_vm_t *vm, const char *s, int len);
uint32_t js_str_cat(js_vm_t *vm, uint32_t a, uint32_t b);
uint32_t js_str_c(js_vm_t *vm, const char *s);       /* NUL-terminated */
uint32_t js_intern(js_vm_t *vm, const char *s, int len);
uint32_t js_intern_str(js_vm_t *vm, uint32_t r);     /* the interned twin of r */
uint32_t js_intern_find(js_vm_t *vm, const char *s, int len);
int js_str_eq(js_vm_t *vm, uint32_t a, uint32_t b);
int js_str_cmp(js_vm_t *vm, uint32_t a, uint32_t b);
uint32_t js_str_sub(js_vm_t *vm, uint32_t r, int from, int to);
int js_str_index_key(js_vm_t *vm, uint32_t key, uint32_t *idx);   /* "123" -> 123 */
uint32_t js_key_of_index(js_vm_t *vm, uint32_t idx);

uint32_t js_obj_new(js_vm_t *vm, int cls, uint32_t proto);
uint32_t js_array_new(js_vm_t *vm, uint32_t cap);
int js_array_push(js_vm_t *vm, uint32_t arr, jv v);
int js_array_set(js_vm_t *vm, uint32_t arr, uint32_t idx, jv v);
int js_array_set_len(js_vm_t *vm, uint32_t arr, uint32_t len);
jv js_array_get(js_vm_t *vm, uint32_t arr, uint32_t idx);
js_prop_t *js_own_prop(js_vm_t *vm, uint32_t obj, uint32_t key);
int js_define(js_vm_t *vm, uint32_t obj, uint32_t key, jv v, uint32_t flags);
int js_define_c(js_vm_t *vm, uint32_t obj, const char *name, jv v, uint32_t flags);
int js_define_accessor(js_vm_t *vm, uint32_t obj, uint32_t key, uint32_t getter, uint32_t setter, uint32_t flags);
int js_delete_own(js_vm_t *vm, uint32_t obj, uint32_t key);
uint32_t js_native_new(js_vm_t *vm, const char *name, js_native_fn fn, int nargs, uint32_t magic);
uint32_t js_def_native(js_vm_t *vm, uint32_t obj, const char *name, js_native_fn fn, int nargs, uint32_t magic);
int js_is_callable(js_vm_t *vm, jv v);
int js_is_array(js_vm_t *vm, jv v);

/* exceptions */
jv js_throw(js_vm_t *vm, jv v);
jv js_throw_err(js_vm_t *vm, int kind, const char *msg);
jv js_throw_err2(js_vm_t *vm, int kind, const char *msg, uint32_t s);
uint32_t js_error_new(js_vm_t *vm, int kind, uint32_t msg);

/* conversions (may call JavaScript: check vm->ctl after) */
int js_truthy(js_vm_t *vm, jv v);
jf64 js_to_number(js_vm_t *vm, jv v);
jv js_to_numeric(js_vm_t *vm, jv v);
int32_t js_to_int32(js_vm_t *vm, jv v);
uint32_t js_to_uint32(js_vm_t *vm, jv v);
jf64 js_to_integer(js_vm_t *vm, jv v);   /* ToIntegerOrInfinity */
uint32_t js_to_string(js_vm_t *vm, jv v);    /* a string ref */
uint32_t js_to_key(js_vm_t *vm, jv v);       /* interned string ref */
jv js_to_primitive(js_vm_t *vm, jv v, int hint_string);
uint32_t js_to_object(js_vm_t *vm, jv v);
uint32_t js_num_to_str(js_vm_t *vm, jf64 n);
jf64 js_str_to_num(js_vm_t *vm, uint32_t s);
int js_strict_eq(js_vm_t *vm, jv a, jv b);
int js_loose_eq(js_vm_t *vm, jv a, jv b);
int js_same_value_zero(js_vm_t *vm, jv a, jv b);
uint32_t js_typeof(js_vm_t *vm, jv v);

/* properties through the prototype chain and host hooks */
jv js_get(js_vm_t *vm, jv obj, uint32_t key);
jv js_get_recv(js_vm_t *vm, jv obj, uint32_t key, jv recv);
jv js_get_v(js_vm_t *vm, jv obj, jv key);
jv js_get_c(js_vm_t *vm, jv obj, const char *name);
int js_put(js_vm_t *vm, jv obj, uint32_t key, jv v);
int js_put_v(js_vm_t *vm, jv obj, jv key, jv v);
int js_has(js_vm_t *vm, uint32_t obj, uint32_t key);
int js_delete(js_vm_t *vm, jv obj, jv key);
uint32_t js_own_keys(js_vm_t *vm, uint32_t obj, int only_enum);   /* array of strings */
int js_instanceof(js_vm_t *vm, jv v, jv ctor);
jv js_length_of(js_vm_t *vm, jv obj, uint32_t *len);

/* console log */
void js_log_put(js_vm_t *vm, const char *s, int n);

/* ---- wf_jsparse.c ---- */
uint32_t js_parse(js_vm_t *vm, const char *src, int len, int as_function_body);
uint32_t js_node_new(js_vm_t *vm, int k, uint32_t line);

/* ---- wf_jsrun.c ---- */
jv js_call(js_vm_t *vm, jv fn, jv thisv, jv *argv, int argc);
jv js_construct(js_vm_t *vm, jv ctor, jv *argv, int argc, jv newtarget);
jv js_run_program(js_vm_t *vm, uint32_t prog);
void js_step(js_vm_t *vm);
uint32_t js_env_new(js_vm_t *vm, uint32_t parent, int kind);
int js_env_declare(js_vm_t *vm, uint32_t env, uint32_t name, jv v, uint32_t flags);
uint32_t js_closure(js_vm_t *vm, uint32_t fnode, uint32_t env);

/* ---- wf_jslib.c ---- */
int js_lib_init(js_vm_t *vm);
int js_lib2_init(js_vm_t *vm);      /* wf_jslib2.c: String, Number, Math, JSON, Date, Map, Set */
uint32_t js_ctor_new(js_vm_t *vm, const char *name, js_native_fn fn, int nargs, uint32_t proto);
jv js_this_number(js_vm_t *vm, jv thisv, int *ok);
uint32_t js_this_string(js_vm_t *vm, jv thisv);
uint32_t js_json_stringify(js_vm_t *vm, jv v, jv replacer, jv space, int *undef);
jv js_iter_values(js_vm_t *vm, jv v, uint32_t *arr_out);   /* spread/for-of source as an array */
uint32_t js_join(js_vm_t *vm, uint32_t arr, uint32_t sep);

/* a growing byte buffer in the arena (wf_jslib2.c); the old block is
 * left behind on growth, so a build costs at most twice its size */
typedef struct { uint32_t ref, len, cap; } js_sb_t;
void js_sb_init(js_vm_t *vm, js_sb_t *b);
void js_sb_put(js_vm_t *vm, js_sb_t *b, const char *s, int n);
void js_sb_str(js_vm_t *vm, js_sb_t *b, uint32_t str);
void js_sb_ch(js_vm_t *vm, js_sb_t *b, char c);
uint32_t js_sb_finish(js_vm_t *vm, js_sb_t *b);

/* ---- wf_jsco.c (Lot 114) ---- */
int js_co_init(js_vm_t *vm);
jv js_gen_new(js_vm_t *vm, uint32_t f, uint32_t env);
jv js_gen_step(js_vm_t *vm, uint32_t g, int mode, jv v, int *done);   /* mode 0 next, 1 throw, 2 return */
void js_gen_close(js_vm_t *vm, uint32_t g);
int js_is_gen(js_vm_t *vm, jv v);
jv js_co_yield(js_vm_t *vm, jv v, int delegate);
jv js_co_await(js_vm_t *vm, jv v);
jv js_async_call(js_vm_t *vm, uint32_t f, uint32_t env);
jv js_async_rejected(js_vm_t *vm);
int js_is_promise(js_vm_t *vm, jv v);
int js_promise_state(js_vm_t *vm, jv v, jv *value);    /* -1 not a promise, 0 pending, 1 fulfilled, 2 rejected */
uint32_t js_promise_new(js_vm_t *vm);
void js_promise_resolve(js_vm_t *vm, uint32_t p, jv x);
void js_promise_reject(js_vm_t *vm, uint32_t p, jv reason);
int js_jobs_drain(js_vm_t *vm);
/* wf_jsrun.c: the body of a function whose environment is ready */
jv js_fn_body(js_vm_t *vm, uint32_t f, uint32_t env);

/* ---- wf_jsre.c ---- */
int js_regexp_init(js_vm_t *vm);
uint32_t js_regexp_new(js_vm_t *vm, uint32_t source, uint32_t flags);
/* a literal: the compiled program is kept in *cache for the next evaluation */
uint32_t js_regexp_lit(js_vm_t *vm, uint32_t source, uint32_t flags, uint32_t *cache);
/* exec at lastIndex rules; returns the match array or null */
jv js_regexp_exec(js_vm_t *vm, uint32_t re, uint32_t str);
int js_is_regexp(js_vm_t *vm, jv v);
#define JS_RE_MAXCAP 32
/* find the first match at or after byte start; caps gets 2 * ncap
 * offsets (start, end; -1 when a group did not take part). 1 on a
 * match, 0 on none, -1 when the run aborted (budget) */
int js_regexp_search(js_vm_t *vm, uint32_t re, uint32_t str, int start, int *caps);
int js_regexp_ncap(js_vm_t *vm, uint32_t re);      /* groups + 1 */
int js_regexp_global(js_vm_t *vm, uint32_t re);
/* the exec() result array for a match */
jv js_regexp_result(js_vm_t *vm, uint32_t re, uint32_t str, const int *caps);

#define JARG(i) ((i) < argc ? argv[(i)] : jv_undef())
#define JCK(vm) do { if ((vm)->ctl) return jv_undef(); } while (0)

#endif
