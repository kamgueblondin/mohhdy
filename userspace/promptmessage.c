/* userspace/promptmessage.c - Phase 4 PromptMessage compiler, image and VM.
 * See promptmessage.h and docs/promptmessage.md. Freestanding C. */
#include "promptmessage.h"

enum {
    OP_HALT = 0, OP_LINE, OP_PUSHI, OP_PUSHS, OP_LOAD, OP_STORE, OP_SYSMEM, OP_ADD,
    OP_CMP, OP_NOT, OP_FEXISTS, OP_JMP, OP_JZ, OP_PRINT, OP_WRITE, OP_APPEND, OP_SHOW,
    OP_EXPECT, OP_RET, OP_AND, OP_OR, OP_COUNT
};
enum { CMP_EQ = 0, CMP_NE, CMP_LT, CMP_GT, CMP_LE, CMP_GE, CMP_CONTAINS };
enum { T_WORD = 1, T_STR, T_NUM, T_OP };
enum { B_IF = 1, B_ELSE, B_REPEAT, B_WHEN };

static const char* const k_op_names[OP_COUNT] = {
    "HALT", "LINE", "PUSHI", "PUSHS", "LOAD", "STORE", "SYSMEM", "ADD", "CMP", "NOT",
    "FEXISTS", "JMP", "JZ", "PRINT", "WRITE", "APPEND", "SHOW", "EXPECT", "RET", "AND", "OR"
};

/* ---------------------------------------------------------------- helpers */
static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static void s_copy(char* d, const char* s, int cap) {
    int i = 0;
    if (cap <= 0) return;
    while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; }
    d[i] = 0;
}
static int s_eq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int s_ieq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && lower(a[i]) == lower(b[i])) i++;
    return a[i] == b[i];
}
static int s_has(const char* hay, const char* needle) {
    int i, j, n = s_len(needle);
    if (n == 0) return 1;
    for (i = 0; hay[i]; i++) {
        for (j = 0; j < n && hay[i + j] == needle[j]; j++) {}
        if (j == n) return 1;
    }
    return 0;
}
static void s_cat(char* d, const char* s, int cap) {
    int n = s_len(d), i = 0;
    while (s[i] && n < cap - 1) d[n++] = s[i++];
    d[n] = 0;
}
static void s_int(char* d, int v, int cap) {
    char t[12]; int n = 0, i = 0; unsigned int u;
    if (v < 0) { if (cap > 1) d[i++] = '-'; u = (unsigned int)(-(v + 1)) + 1U; } else u = (unsigned int)v;
    do { t[n++] = (char)('0' + (u % 10U)); u /= 10U; } while (u && n < 11);
    while (n && i < cap - 1) d[i++] = t[--n];
    d[i] = 0;
}
static void out_line(const pm_host_t* h, const char* a, int num, const char* b) {
    char buf[PM_VAL_MAX + 64]; char n[12];
    buf[0] = 0;
    if (!h || !h->out) return;
    s_cat(buf, a, (int)sizeof(buf));
    if (num != -0x7fffffff) { s_int(n, num, (int)sizeof(n)); s_cat(buf, n, (int)sizeof(buf)); }
    if (b) s_cat(buf, b, (int)sizeof(buf));
    h->out(h->ctx, buf);
}

/* --------------------------------------------------------------- compiler */
typedef struct { int type; const char* p; int len; int num; } pm_tok_t;
#define PM_TOKS 48

typedef struct { int kind; int jz_patch; int end_patch; int head; int slot; } pm_block_t;

typedef struct {
    pm_program_t* prog;
    const pm_host_t* host;
    pm_block_t blocks[PM_BLOCKS];
    int nblocks;
    int line;
    int depth;
    int last_push;  /* code offset of the latest PUSHI, -1 if none */
    int prev_push;
    char strbuf[PM_VAL_MAX];
} pm_cc_t;

static char g_include_src[PM_INCLUDES > 0 ? PM_SRC_MAX : 1];

static int cc_fail(pm_cc_t* c, int col, const char* msg) {
    if (c->prog->err_line == 0) {
        c->prog->err_line = c->line;
        c->prog->err_col = col;
        s_copy(c->prog->err, msg, (int)sizeof(c->prog->err));
    }
    return PM_ERR_SYNTAX;
}
static int cc_limit(pm_cc_t* c, const char* msg) {
    cc_fail(c, 1, msg);
    return PM_ERR_LIMIT;
}
static int emit8(pm_cc_t* c, int v) {
    if (c->prog->code_len >= PM_CODE_MAX) return cc_limit(c, "code too large");
    c->prog->code[c->prog->code_len++] = (unsigned char)v;
    return PM_OK;
}
static int emit16(pm_cc_t* c, int v) {
    if (emit8(c, v & 0xff) || emit8(c, (v >> 8) & 0xff)) return PM_ERR_LIMIT;
    return PM_OK;
}
static int emit32(pm_cc_t* c, int v) {
    if (emit16(c, v & 0xffff) || emit16(c, (v >> 16) & 0xffff)) return PM_ERR_LIMIT;
    return PM_OK;
}
static void patch16(pm_cc_t* c, int at, int v) {
    c->prog->code[at] = (unsigned char)(v & 0xff);
    c->prog->code[at + 1] = (unsigned char)((v >> 8) & 0xff);
}
static int rd16(const unsigned char* p) { return (int)p[0] | ((int)p[1] << 8); }
static int rd32(const unsigned char* p) {
    return (int)((unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24));
}
static int emit_op(pm_cc_t* c, int op) {
    if (op != OP_PUSHI) { c->last_push = -1; c->prev_push = -1; }
    return emit8(c, op);
}
static int emit_pushi(pm_cc_t* c, int v) {
    int at = c->prog->code_len;
    if (emit8(c, OP_PUSHI) || emit32(c, v)) return PM_ERR_LIMIT;
    c->prev_push = c->last_push;
    c->last_push = at;
    return PM_OK;
}
static int pool_add(pm_cc_t* c, const char* s, int n) {
    int at, i;
    /* Reuse an identical string. */
    for (at = 0; at < c->prog->pool_len; at += s_len(c->prog->pool + at) + 1) {
        for (i = 0; i < n && c->prog->pool[at + i] == s[i]; i++) {}
        if (i == n && c->prog->pool[at + n] == 0) return at;
    }
    if (c->prog->pool_len + n + 1 > PM_POOL_MAX) return -1;
    at = c->prog->pool_len;
    for (i = 0; i < n; i++) c->prog->pool[at + i] = s[i];
    c->prog->pool[at + n] = 0;
    c->prog->pool_len = (unsigned short)(at + n + 1);
    return at;
}
static int var_slot(pm_cc_t* c, const char* name, int n, int create) {
    int i, j;
    if (n <= 0 || n >= PM_NAME_MAX) return -1;
    for (i = 0; i < c->prog->nvars; i++) {
        for (j = 0; j < n && c->prog->vars[i][j] == name[j]; j++) {}
        if (j == n && c->prog->vars[i][n] == 0) return i;
    }
    if (!create || c->prog->nvars >= PM_VARS) return -1;
    i = c->prog->nvars++;
    for (j = 0; j < n; j++) c->prog->vars[i][j] = name[j];
    c->prog->vars[i][n] = 0;
    return i;
}
static int tok_is(const pm_tok_t* t, const char* w) {
    int i;
    if (t->type != T_WORD) return 0;
    for (i = 0; i < t->len; i++) if (!w[i] || lower(t->p[i]) != w[i]) return 0;
    return w[t->len] == 0;
}
static int is_word_char(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
           ch == '_' || ch == '.' || ch == '$' || ch == '/' || ch == '-' || (unsigned char)ch >= 0x80;
}

static int tokenize(pm_cc_t* c, const char* s, int n, pm_tok_t* t, int* count) {
    int i = 0, k = 0;
    while (i < n) {
        char ch = s[i];
        if (ch == ' ' || ch == '\t' || ch == '\r') { i++; continue; }
        if (k >= PM_TOKS) return cc_limit(c, "too many tokens");
        t[k].p = s + i; t[k].num = 0;
        if (ch == '"') {
            int j = i + 1;
            while (j < n && s[j] != '"') j++;
            if (j >= n) return cc_fail(c, i + 1, "unterminated string");
            t[k].type = T_STR; t[k].p = s + i + 1; t[k].len = j - i - 1;
            i = j + 1;
        } else if (ch >= '0' && ch <= '9') {
            int v = 0;
            while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); if (v > 100000000) return cc_fail(c, i + 1, "number too large"); i++; }
            if (i < n && s[i] == '%') i++;
            t[k].type = T_NUM; t[k].num = v; t[k].len = (int)(s + i - t[k].p);
        } else if (ch == '=' || ch == '!' || ch == '<' || ch == '>' || ch == '+') {
            t[k].type = T_OP; t[k].len = 1;
            if (i + 1 < n && s[i + 1] == '=' && ch != '+') t[k].len = 2;
            if ((ch == '=' || ch == '!') && t[k].len != 2) return cc_fail(c, i + 1, "expected == or !=");
            i += t[k].len;
        } else if (is_word_char(ch)) {
            int j = i;
            while (j < n && is_word_char(s[j])) j++;
            t[k].type = T_WORD; t[k].len = j - i;
            i = j;
        } else {
            return cc_fail(c, i + 1, "unexpected character");
        }
        t[k].num += 0;
        k++;
    }
    *count = k;
    return PM_OK;
}

static int col_of(const pm_tok_t* t, int i, int n, const char* line_start) {
    if (i < n) return (int)(t[i].p - line_start) + 1;
    return n > 0 ? (int)(t[n - 1].p - line_start) + t[n - 1].len + 1 : 1;
}

/* term := STRING | NUMBER | $var | system.memory */
static int cc_term(pm_cc_t* c, pm_tok_t* t, int* i, int n, const char* ls) {
    int at;
    if (*i >= n) return cc_fail(c, col_of(t, *i, n, ls), "value expected");
    if (t[*i].type == T_STR) {
        at = pool_add(c, t[*i].p, t[*i].len);
        if (at < 0) return cc_limit(c, "string pool full");
        (*i)++;
        if (emit_op(c, OP_PUSHS) || emit16(c, at)) return PM_ERR_LIMIT;
        return PM_OK;
    }
    if (t[*i].type == T_NUM) { (*i)++; return emit_pushi(c, t[*i - 1].num); }
    if (t[*i].type == T_WORD && t[*i].p[0] == '$') {
        int slot = var_slot(c, t[*i].p + 1, t[*i].len - 1, 0);
        if (slot < 0) return cc_fail(c, col_of(t, *i, n, ls), "unknown variable");
        (*i)++;
        if (emit_op(c, OP_LOAD) || emit8(c, slot)) return PM_ERR_LIMIT;
        return PM_OK;
    }
    if (tok_is(&t[*i], "system.memory")) { (*i)++; return emit_op(c, OP_SYSMEM); }
    return cc_fail(c, col_of(t, *i, n, ls), "value expected");
}
/* Constant folding (US-056): PUSHI a, PUSHI b, op  ->  PUSHI result. */
static int fold_pair(pm_cc_t* c, int* a, int* b) {
    int len = c->prog->code_len;
    if (c->last_push < 0 || c->prev_push < 0) return 0;
    if (c->last_push != len - 5 || c->prev_push != len - 10) return 0;
    *a = rd32(c->prog->code + c->prev_push + 1);
    *b = rd32(c->prog->code + c->last_push + 1);
    c->prog->code_len = (unsigned short)(len - 10);
    c->last_push = c->prev_push = -1;
    c->prog->folded++;
    return 1;
}
static int cc_expr(pm_cc_t* c, pm_tok_t* t, int* i, int n, const char* ls) {
    int st = cc_term(c, t, i, n, ls), a, b;
    if (st) return st;
    while (*i < n && t[*i].type == T_OP && t[*i].p[0] == '+') {
        (*i)++;
        if ((st = cc_term(c, t, i, n, ls)) != 0) return st;
        if (fold_pair(c, &a, &b)) { if (emit_pushi(c, a + b)) return PM_ERR_LIMIT; }
        else if (emit_op(c, OP_ADD)) return PM_ERR_LIMIT;
    }
    return PM_OK;
}
static int cmp_code(const pm_tok_t* t) {
    if (t->type == T_OP) {
        if (t->len == 2 && t->p[0] == '=') return CMP_EQ;
        if (t->len == 2 && t->p[0] == '!') return CMP_NE;
        if (t->p[0] == '<') return t->len == 2 ? CMP_LE : CMP_LT;
        if (t->p[0] == '>') return t->len == 2 ? CMP_GE : CMP_GT;
    }
    if (tok_is(t, "contains")) return CMP_CONTAINS;
    if (tok_is(t, "is")) return CMP_EQ;
    return -1;
}
static int fold_cmp(int op, int a, int b) {
    switch (op) {
    case CMP_EQ: return a == b;
    case CMP_NE: return a != b;
    case CMP_LT: return a < b;
    case CMP_GT: return a > b;
    case CMP_LE: return a <= b;
    default: return a >= b;
    }
}
static int cc_simple(pm_cc_t* c, pm_tok_t* t, int* i, int n, const char* ls) {
    int st, op, a, b;
    if (*i < n && tok_is(&t[*i], "not")) {
        (*i)++;
        if ((st = cc_simple(c, t, i, n, ls)) != 0) return st;
        return emit_op(c, OP_NOT);
    }
    if (*i < n && tok_is(&t[*i], "file")) {
        (*i)++;
        if ((st = cc_expr(c, t, i, n, ls)) != 0) return st;
        if (*i >= n || !tok_is(&t[*i], "exists")) return cc_fail(c, col_of(t, *i, n, ls), "expected 'exists'");
        (*i)++;
        return emit_op(c, OP_FEXISTS);
    }
    if ((st = cc_expr(c, t, i, n, ls)) != 0) return st;
    if (*i < n && (op = cmp_code(&t[*i])) >= 0) {
        (*i)++;
        if ((st = cc_expr(c, t, i, n, ls)) != 0) return st;
        if (op != CMP_CONTAINS && fold_pair(c, &a, &b)) return emit_pushi(c, fold_cmp(op, a, b));
        if (emit_op(c, OP_CMP) || emit8(c, op)) return PM_ERR_LIMIT;
    }
    return PM_OK;
}
static int cc_cond(pm_cc_t* c, pm_tok_t* t, int* i, int n, const char* ls) {
    int st = cc_simple(c, t, i, n, ls);
    while (!st && *i < n && (tok_is(&t[*i], "and") || tok_is(&t[*i], "or"))) {
        int op = tok_is(&t[*i], "and") ? OP_AND : OP_OR;
        (*i)++;
        if ((st = cc_simple(c, t, i, n, ls)) != 0) return st;
        st = emit_op(c, op);
    }
    return st;
}
/* JZ with constant folding: a constant true condition emits nothing (patch
 * slot -1), a constant false one becomes an unconditional JMP. */
static int emit_jz(pm_cc_t* c, int* patch) {
    int len = c->prog->code_len;
    if (c->last_push >= 0 && c->last_push == len - 5) {
        int v = rd32(c->prog->code + c->last_push + 1);
        c->prog->code_len = (unsigned short)(len - 5);
        c->last_push = c->prev_push = -1;
        c->prog->folded++;
        if (v) { *patch = -1; return PM_OK; }
        if (emit_op(c, OP_JMP)) return PM_ERR_LIMIT;
    } else if (emit_op(c, OP_JZ)) {
        return PM_ERR_LIMIT;
    }
    *patch = c->prog->code_len;
    return emit16(c, 0);
}
static void patch_here(pm_cc_t* c, int at) { if (at >= 0) patch16(c, at, c->prog->code_len); }

static int cc_block_push(pm_cc_t* c, int kind, int jz, int head, int slot) {
    if (c->nblocks >= PM_BLOCKS) return cc_limit(c, "blocks nested too deep");
    c->blocks[c->nblocks].kind = kind;
    c->blocks[c->nblocks].jz_patch = jz;
    c->blocks[c->nblocks].end_patch = -1;
    c->blocks[c->nblocks].head = head;
    c->blocks[c->nblocks].slot = slot;
    c->nblocks++;
    return PM_OK;
}
static int cc_block_close(pm_cc_t* c, pm_block_t* b) {
    if (b->kind == B_REPEAT) {
        if (emit_op(c, OP_LOAD) || emit8(c, b->slot) || emit_pushi(c, -1) || emit_op(c, OP_ADD) ||
            emit_op(c, OP_STORE) || emit8(c, b->slot) || emit_op(c, OP_JMP) || emit16(c, b->head)) return PM_ERR_LIMIT;
        patch_here(c, b->jz_patch);
    } else if (b->kind == B_WHEN) {
        if (emit_op(c, OP_RET)) return PM_ERR_LIMIT;
        patch_here(c, b->jz_patch);
    } else if (b->kind == B_ELSE) {
        patch_here(c, b->end_patch);
    } else {
        patch_here(c, b->jz_patch);
    }
    return PM_OK;
}

static int cc_compile_text(pm_cc_t* c, const char* src, int len);

/* One statement in t[i..n). Block openers leave an entry on the block stack
 * when nothing follows `then` / `times`; otherwise the statement is inline. */
static int cc_stmt(pm_cc_t* c, pm_tok_t* t, int i, int n, const char* ls) {
    int st, at, slot;
    if (i >= n) return cc_fail(c, col_of(t, i, n, ls), "statement expected");
    c->prog->statements++;
    if (tok_is(&t[i], "print") || tok_is(&t[i], "say")) {
        i++;
        if ((st = cc_expr(c, t, &i, n, ls)) != 0) return st;
        if (emit_op(c, OP_PRINT)) return PM_ERR_LIMIT;
    } else if (tok_is(&t[i], "set")) {
        i++;
        if (i >= n || t[i].type != T_WORD || t[i].p[0] == '$') return cc_fail(c, col_of(t, i, n, ls), "variable name expected");
        slot = var_slot(c, t[i].p, t[i].len, 1);
        if (slot < 0) return cc_fail(c, col_of(t, i, n, ls), "bad or too many variables");
        i++;
        if (i >= n || !tok_is(&t[i], "to")) return cc_fail(c, col_of(t, i, n, ls), "expected 'to'");
        i++;
        if ((st = cc_expr(c, t, &i, n, ls)) != 0) return st;
        if (emit_op(c, OP_STORE) || emit8(c, slot)) return PM_ERR_LIMIT;
    } else if (tok_is(&t[i], "create")) {
        i++;
        if (i >= n || !tok_is(&t[i], "file")) return cc_fail(c, col_of(t, i, n, ls), "expected 'file'");
        i++;
        if ((st = cc_expr(c, t, &i, n, ls)) != 0) return st;
        if (i < n && tok_is(&t[i], "with")) {
            i++;
            if (i >= n || !tok_is(&t[i], "content")) return cc_fail(c, col_of(t, i, n, ls), "expected 'content'");
            i++;
            if ((st = cc_expr(c, t, &i, n, ls)) != 0) return st;
        } else {
            at = pool_add(c, "", 0);
            if (at < 0 || emit_op(c, OP_PUSHS) || emit16(c, at)) return cc_limit(c, "string pool full");
        }
        if (emit_op(c, OP_WRITE)) return PM_ERR_LIMIT;
    } else if (tok_is(&t[i], "append")) {
        /* append EXPR to file EXPR : the VM wants path then content. Compile
         * the content into a scratch region by parsing the path first. */
        int j = i + 1, depth_save, path_start, path_end;
        while (j < n && !tok_is(&t[j], "to")) j++;
        if (j >= n || j + 1 >= n || !tok_is(&t[j + 1], "file")) return cc_fail(c, col_of(t, j, n, ls), "expected 'to file'");
        path_start = j + 2; path_end = n; depth_save = path_start;
        if ((st = cc_expr(c, t, &depth_save, path_end, ls)) != 0) return st;
        if (depth_save != path_end) return cc_fail(c, col_of(t, depth_save, n, ls), "unexpected words");
        at = i + 1;
        if ((st = cc_expr(c, t, &at, j, ls)) != 0) return st;
        if (at != j) return cc_fail(c, col_of(t, at, n, ls), "unexpected words");
        if (emit_op(c, OP_APPEND)) return PM_ERR_LIMIT;
        i = n;
    } else if (tok_is(&t[i], "show")) {
        i++;
        if (i >= n || !tok_is(&t[i], "file")) return cc_fail(c, col_of(t, i, n, ls), "expected 'file'");
        i++;
        if ((st = cc_expr(c, t, &i, n, ls)) != 0) return st;
        if (emit_op(c, OP_SHOW)) return PM_ERR_LIMIT;
    } else if (tok_is(&t[i], "expect")) {
        i++;
        if ((st = cc_cond(c, t, &i, n, ls)) != 0) return st;
        if (emit_op(c, OP_EXPECT) || emit16(c, c->line)) return PM_ERR_LIMIT;
    } else if (tok_is(&t[i], "stop")) {
        i++;
        if (emit_op(c, OP_HALT)) return PM_ERR_LIMIT;
    } else if (tok_is(&t[i], "use")) {
        int got;
        pm_cc_t saved;
        i++;
        if (i >= n || t[i].type != T_STR || i + 1 != n) return cc_fail(c, col_of(t, i, n, ls), "use \"file.pm\" expected");
        if (c->depth > 0) return cc_fail(c, 1, "use is not allowed inside a library");
        if (c->nblocks) return cc_fail(c, 1, "use must be at top level");
        if (!c->host || !c->host->read_file) return cc_fail(c, col_of(t, i, n, ls), "no library loader");
        if (c->prog->includes >= PM_INCLUDES) return cc_limit(c, "too many libraries");
        if (t[i].len >= PM_VAL_MAX) return cc_fail(c, col_of(t, i, n, ls), "path too long");
        for (at = 0; at < t[i].len; at++) c->strbuf[at] = t[i].p[at];
        c->strbuf[t[i].len] = 0;
        got = c->host->read_file(c->host->ctx, c->strbuf, g_include_src, PM_SRC_MAX - 1);
        if (got < 0) return cc_fail(c, col_of(t, i, n, ls), "library not found");
        c->prog->includes++;
        c->prog->statements--;
        saved = *c;
        c->depth = 1;
        st = cc_compile_text(c, g_include_src, got);
        saved.prog = c->prog;
        c->depth = saved.depth;
        c->line = saved.line;
        if (st) return st;
        if (c->nblocks) return cc_fail(c, 1, "library leaves a block open");
        i = n;
    } else if (tok_is(&t[i], "if")) {
        int jz;
        i++;
        if ((st = cc_cond(c, t, &i, n, ls)) != 0) return st;
        if (i >= n || !tok_is(&t[i], "then")) return cc_fail(c, col_of(t, i, n, ls), "expected 'then'");
        i++;
        if ((st = emit_jz(c, &jz)) != 0) return st;
        if (i >= n) return cc_block_push(c, B_IF, jz, 0, 0);
        slot = c->nblocks;
        if ((st = cc_stmt(c, t, i, n, ls)) != 0) return st;
        if (c->nblocks != slot) return cc_fail(c, col_of(t, i, n, ls), "inline statement cannot open a block");
        patch_here(c, jz);
        return PM_OK;
    } else if (tok_is(&t[i], "repeat")) {
        char name[4];
        int head, jz;
        i++;
        name[0] = '#'; name[1] = (char)('0' + c->nblocks); name[2] = (char)('0' + c->depth); name[3] = 0;
        slot = var_slot(c, name, 3, 1);
        if (slot < 0) return cc_limit(c, "too many variables");
        if ((st = cc_expr(c, t, &i, n, ls)) != 0) return st;
        if (i >= n || !tok_is(&t[i], "times")) return cc_fail(c, col_of(t, i, n, ls), "expected 'times'");
        i++;
        if (emit_op(c, OP_STORE) || emit8(c, slot)) return PM_ERR_LIMIT;
        head = c->prog->code_len;
        if (emit_op(c, OP_LOAD) || emit8(c, slot) || emit_pushi(c, 0)) return PM_ERR_LIMIT;
        c->last_push = c->prev_push = -1; /* never fold the loop guard */
        if (emit_op(c, OP_CMP) || emit8(c, CMP_GT) || emit_op(c, OP_JZ)) return PM_ERR_LIMIT;
        jz = c->prog->code_len;
        if (emit16(c, 0)) return PM_ERR_LIMIT;
        if (cc_block_push(c, B_REPEAT, jz, head, slot)) return PM_ERR_LIMIT;
        if (i >= n) return PM_OK;
        slot = c->nblocks;
        if ((st = cc_stmt(c, t, i, n, ls)) != 0) return st;
        if (c->nblocks != slot) return cc_fail(c, col_of(t, i, n, ls), "inline statement cannot open a block");
        c->nblocks--;
        return cc_block_close(c, &c->blocks[c->nblocks]);
    } else if (tok_is(&t[i], "when")) {
        int jmp;
        if (c->nblocks) return cc_fail(c, col_of(t, i, n, ls), "when must be at top level");
        i++;
        if (i + 1 >= n || !tok_is(&t[i], "user") || !(tok_is(&t[i + 1], "says") || tok_is(&t[i + 1], "dit")))
            return cc_fail(c, col_of(t, i, n, ls), "expected 'user says'");
        i += 2;
        if (i >= n || t[i].type != T_STR) return cc_fail(c, col_of(t, i, n, ls), "phrase string expected");
        if (c->prog->ntrig >= PM_TRIGGERS) return cc_limit(c, "too many triggers");
        at = pool_add(c, t[i].p, t[i].len);
        if (at < 0) return cc_limit(c, "string pool full");
        i++;
        if (i >= n || !tok_is(&t[i], "then")) return cc_fail(c, col_of(t, i, n, ls), "expected 'then'");
        i++;
        if (emit_op(c, OP_JMP)) return PM_ERR_LIMIT;
        jmp = c->prog->code_len;
        if (emit16(c, 0)) return PM_ERR_LIMIT;
        c->prog->triggers[c->prog->ntrig].phrase = (unsigned short)at;
        c->prog->triggers[c->prog->ntrig].addr = c->prog->code_len;
        c->prog->ntrig++;
        if (i >= n) return cc_block_push(c, B_WHEN, jmp, 0, 0);
        if ((st = cc_stmt(c, t, i, n, ls)) != 0) return st;
        if (c->nblocks) return cc_fail(c, col_of(t, i, n, ls), "inline statement cannot open a block");
        if (emit_op(c, OP_RET)) return PM_ERR_LIMIT;
        patch_here(c, jmp);
        return PM_OK;
    } else {
        return cc_fail(c, col_of(t, i, n, ls), "unknown statement");
    }
    if (i < n) return cc_fail(c, col_of(t, i, n, ls), "unexpected words");
    return PM_OK;
}

static int cc_line(pm_cc_t* c, const char* ls, int len) {
    pm_tok_t t[PM_TOKS];
    int n = 0, st, k = 0;
    while (k < len && (ls[k] == ' ' || ls[k] == '\t')) k++;
    if (k >= len || ls[k] == '\r') return PM_OK;
    if (ls[k] == '#' || (ls[k] == '/' && k + 1 < len && ls[k + 1] == '/')) return PM_OK;
    if ((st = tokenize(c, ls, len, t, &n)) != 0) return st;
    if (n == 0) return PM_OK;
    if (n == 1 && tok_is(&t[0], "end")) {
        if (!c->nblocks) return cc_fail(c, 1, "end without block");
        c->nblocks--;
        return cc_block_close(c, &c->blocks[c->nblocks]);
    }
    if (n == 1 && tok_is(&t[0], "else")) {
        pm_block_t* b;
        if (!c->nblocks || c->blocks[c->nblocks - 1].kind != B_IF) return cc_fail(c, 1, "else without if");
        b = &c->blocks[c->nblocks - 1];
        if (emit_op(c, OP_JMP)) return PM_ERR_LIMIT;
        b->end_patch = c->prog->code_len;
        if (emit16(c, 0)) return PM_ERR_LIMIT;
        patch_here(c, b->jz_patch);
        b->kind = B_ELSE;
        return PM_OK;
    }
    if (emit_op(c, OP_LINE) || emit16(c, c->line)) return PM_ERR_LIMIT;
    return cc_stmt(c, t, 0, n, ls);
}

static int cc_compile_text(pm_cc_t* c, const char* src, int len) {
    int i = 0, st, base_line = c->line, line = 0;
    while (i < len) {
        int j = i;
        while (j < len && src[j] != '\n') j++;
        line++;
        c->line = c->depth ? base_line : line;
        if ((st = cc_line(c, src + i, j - i)) != 0) return st;
        i = j + 1;
    }
    return PM_OK;
}

int pm_compile(const char* src, int len, pm_program_t* prog, const pm_host_t* host) {
    pm_cc_t c;
    int i, st;
    if (!prog) return PM_ERR_SYNTAX;
    for (i = 0; i < (int)sizeof(*prog); i++) ((char*)prog)[i] = 0;
    if (!src || len < 0 || len > PM_SRC_MAX) { s_copy(prog->err, "source too large", 64); prog->err_line = 1; return PM_ERR_LIMIT; }
    for (i = 0; i < (int)sizeof(c); i++) ((char*)&c)[i] = 0;
    c.prog = prog; c.host = host; c.last_push = c.prev_push = -1;
    st = cc_compile_text(&c, src, len);
    if (!st && c.nblocks) { c.line = 0; for (i = 0; i < len; i++) if (src[i] == '\n') c.line++; c.line++; st = cc_fail(&c, 1, "missing end"); }
    if (!st) st = emit_op(&c, OP_HALT);
    return st;
}

/* ------------------------------------------------------------------ image */
unsigned int pm_checksum(const unsigned char* data, int len) {
    unsigned int h = 2166136261U; int i;
    for (i = 0; i < len; i++) { h ^= data[i]; h *= 16777619U; }
    return h;
}
static void put16(unsigned char* p, int v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }

int pm_image_write(const pm_program_t* prog, unsigned char* out, int cap) {
    int n = 0, i, j; unsigned int h;
    int need = 12 + prog->ntrig * 4 + prog->nvars * PM_NAME_MAX + prog->pool_len + prog->code_len + 4;
    if (!prog || !out || cap < need) return PM_ERR_LIMIT;
    out[n++] = 'P'; out[n++] = 'M'; out[n++] = 'C'; out[n++] = '1';
    put16(out + n, prog->code_len); n += 2;
    put16(out + n, prog->pool_len); n += 2;
    out[n++] = prog->ntrig; out[n++] = prog->nvars; out[n++] = 0; out[n++] = 0;
    for (i = 0; i < prog->ntrig; i++) { put16(out + n, prog->triggers[i].phrase); put16(out + n + 2, prog->triggers[i].addr); n += 4; }
    for (i = 0; i < prog->nvars; i++) for (j = 0; j < PM_NAME_MAX; j++) out[n++] = (unsigned char)prog->vars[i][j];
    for (i = 0; i < prog->pool_len; i++) out[n++] = (unsigned char)prog->pool[i];
    for (i = 0; i < prog->code_len; i++) out[n++] = prog->code[i];
    h = pm_checksum(out, n);
    out[n++] = (unsigned char)h; out[n++] = (unsigned char)(h >> 8); out[n++] = (unsigned char)(h >> 16); out[n++] = (unsigned char)(h >> 24);
    return n;
}

int pm_image_read(const unsigned char* in, int len, pm_program_t* prog) {
    int n = 12, i, j, code_len, pool_len, ntrig, nvars; unsigned int h;
    if (!in || !prog) return PM_ERR_IMAGE;
    for (i = 0; i < (int)sizeof(*prog); i++) ((char*)prog)[i] = 0;
    if (len < 16 || in[0] != 'P' || in[1] != 'M' || in[2] != 'C' || in[3] != '1') return PM_ERR_IMAGE;
    code_len = rd16(in + 4); pool_len = rd16(in + 6); ntrig = in[8]; nvars = in[9];
    if (code_len < 1 || code_len > PM_CODE_MAX || pool_len > PM_POOL_MAX || ntrig > PM_TRIGGERS || nvars > PM_VARS) return PM_ERR_IMAGE;
    if (len != 12 + ntrig * 4 + nvars * PM_NAME_MAX + pool_len + code_len + 4) return PM_ERR_IMAGE;
    h = pm_checksum(in, len - 4);
    if ((unsigned int)rd32(in + len - 4) != h) return PM_ERR_IMAGE;
    for (i = 0; i < ntrig; i++) {
        prog->triggers[i].phrase = (unsigned short)rd16(in + n);
        prog->triggers[i].addr = (unsigned short)rd16(in + n + 2);
        if (prog->triggers[i].phrase >= pool_len || prog->triggers[i].addr >= code_len) return PM_ERR_IMAGE;
        n += 4;
    }
    for (i = 0; i < nvars; i++) { for (j = 0; j < PM_NAME_MAX; j++) prog->vars[i][j] = (char)in[n++]; prog->vars[i][PM_NAME_MAX - 1] = 0; }
    for (i = 0; i < pool_len; i++) prog->pool[i] = (char)in[n++];
    if (pool_len && prog->pool[pool_len - 1] != 0) return PM_ERR_IMAGE;
    for (i = 0; i < code_len; i++) prog->code[i] = in[n++];
    prog->code_len = (unsigned short)code_len; prog->pool_len = (unsigned short)pool_len;
    prog->ntrig = (unsigned char)ntrig; prog->nvars = (unsigned char)nvars;
    return PM_OK;
}

/* --------------------------------------------------------------------- VM */
void pm_vm_reset(pm_vm_t* vm) {
    int i;
    for (i = 0; i < (int)sizeof(*vm); i++) ((char*)vm)[i] = 0;
}
static int vm_fail(pm_vm_t* vm, const char* msg) { s_copy(vm->err, msg, (int)sizeof(vm->err)); return PM_ERR_RUNTIME; }
static int truthy(const pm_value_t* v) { return v->is_str ? v->str[0] != 0 : v->num != 0; }
static void to_text(const pm_value_t* v, char* out, int cap) { if (v->is_str) s_copy(out, v->str, cap); else s_int(out, v->num, cap); }
static int starts(const char* s, const char* p) { int i = 0; while (p[i] && s[i] == p[i]) i++; return p[i] == 0; }
/* Programs may not escape with "..", nor touch binaries or model weights. */
static int path_allowed(const char* p) {
    if (!p[0] || s_has(p, "..")) return 0;
    if (starts(p, "/bin/") || starts(p, "/models/") || starts(p, "bin/") || starts(p, "models/")) return 0;
    return 1;
}

static int vm_exec(const pm_program_t* prog, pm_vm_t* vm, const pm_host_t* host, int pc, int is_trigger) {
    char text[PM_VAL_MAX], text2[PM_VAL_MAX];
    while (1) {
        int op, a;
        pm_value_t* top;
        if (pc < 0 || pc >= prog->code_len) return vm_fail(vm, "pc out of range");
        if (++vm->steps > PM_STEPS) return vm_fail(vm, "step budget exhausted");
        op = prog->code[pc++];
        top = vm->sp > 0 ? &vm->stack[vm->sp - 1] : 0;
        switch (op) {
        case OP_HALT:
            return PM_OK;
        case OP_RET:
            if (is_trigger) return PM_OK;
            return vm_fail(vm, "ret outside trigger");
        case OP_LINE:
            if (pc + 2 > prog->code_len) return vm_fail(vm, "truncated");
            vm->line = rd16(prog->code + pc); pc += 2;
            if (vm->break_line > 0 && vm->line == vm->break_line) {
                int i;
                char buf[PM_VAL_MAX + 48];
                out_line(host, "pm-break line ", vm->line, 0);
                for (i = 0; i < prog->nvars; i++) {
                    if (prog->vars[i][0] == '#') continue;
                    buf[0] = 0;
                    s_cat(buf, "pm-var ", (int)sizeof(buf));
                    s_cat(buf, prog->vars[i], (int)sizeof(buf));
                    s_cat(buf, "=", (int)sizeof(buf));
                    to_text(&vm->vars[i], text, (int)sizeof(text));
                    s_cat(buf, text, (int)sizeof(buf));
                    out_line(host, buf, -0x7fffffff, 0);
                }
                return PM_BREAK;
            }
            if (vm->trace) out_line(host, "pm-trace line ", vm->line, 0);
            break;
        case OP_PUSHI:
        case OP_PUSHS:
        case OP_LOAD:
        case OP_SYSMEM: {
            pm_value_t* v;
            if (vm->sp >= PM_STACK) return vm_fail(vm, "stack overflow");
            v = &vm->stack[vm->sp];
            if (op == OP_PUSHI) {
                if (pc + 4 > prog->code_len) return vm_fail(vm, "truncated");
                v->is_str = 0; v->num = rd32(prog->code + pc); pc += 4;
            } else if (op == OP_PUSHS) {
                if (pc + 2 > prog->code_len) return vm_fail(vm, "truncated");
                a = rd16(prog->code + pc); pc += 2;
                if (a >= prog->pool_len) return vm_fail(vm, "bad string");
                v->is_str = 1; s_copy(v->str, prog->pool + a, PM_VAL_MAX);
            } else if (op == OP_LOAD) {
                if (pc + 1 > prog->code_len) return vm_fail(vm, "truncated");
                a = prog->code[pc++];
                if (a >= prog->nvars) return vm_fail(vm, "bad variable");
                *v = vm->vars[a];
            } else {
                v->is_str = 0;
                v->num = (host && host->mem_used_pct) ? host->mem_used_pct(host->ctx) : 0;
            }
            vm->sp++;
            break;
        }
        case OP_STORE:
            if (pc + 1 > prog->code_len) return vm_fail(vm, "truncated");
            a = prog->code[pc++];
            if (a >= prog->nvars || !top) return vm_fail(vm, "bad store");
            vm->vars[a] = *top; vm->sp--;
            break;
        case OP_ADD:
        case OP_CMP:
        case OP_AND:
        case OP_OR: {
            pm_value_t *l, *r;
            int cmp = 0;
            if (op == OP_CMP) { if (pc + 1 > prog->code_len) return vm_fail(vm, "truncated"); cmp = prog->code[pc++]; }
            if (vm->sp < 2) return vm_fail(vm, "stack underflow");
            l = &vm->stack[vm->sp - 2]; r = &vm->stack[vm->sp - 1];
            if (op == OP_ADD) {
                if (!l->is_str && !r->is_str) l->num = l->num + r->num;
                else {
                    to_text(l, text, (int)sizeof(text)); to_text(r, text2, (int)sizeof(text2));
                    s_cat(text, text2, (int)sizeof(text));
                    l->is_str = 1; s_copy(l->str, text, PM_VAL_MAX);
                }
            } else if (op == OP_AND || op == OP_OR) {
                int x = truthy(l), y = truthy(r);
                l->is_str = 0; l->num = op == OP_AND ? (x && y) : (x || y);
            } else {
                int res;
                if (cmp == CMP_CONTAINS) {
                    to_text(l, text, (int)sizeof(text)); to_text(r, text2, (int)sizeof(text2));
                    res = s_has(text, text2);
                } else if (!l->is_str && !r->is_str) {
                    res = fold_cmp(cmp, l->num, r->num);
                } else {
                    to_text(l, text, (int)sizeof(text)); to_text(r, text2, (int)sizeof(text2));
                    if (cmp == CMP_EQ) res = s_eq(text, text2);
                    else if (cmp == CMP_NE) res = !s_eq(text, text2);
                    else return vm_fail(vm, "order compare on text");
                }
                l->is_str = 0; l->num = res;
            }
            vm->sp--;
            break;
        }
        case OP_NOT:
            if (!top) return vm_fail(vm, "stack underflow");
            a = !truthy(top); top->is_str = 0; top->num = a;
            break;
        case OP_FEXISTS: {
            char probe[4];
            if (!top) return vm_fail(vm, "stack underflow");
            to_text(top, text, (int)sizeof(text));
            a = (host && host->read_file) ? host->read_file(host->ctx, text, probe, (int)sizeof(probe)) >= 0 : 0;
            top->is_str = 0; top->num = a;
            break;
        }
        case OP_JMP:
        case OP_JZ:
            if (pc + 2 > prog->code_len) return vm_fail(vm, "truncated");
            a = rd16(prog->code + pc); pc += 2;
            if (op == OP_JZ) {
                if (!top) return vm_fail(vm, "stack underflow");
                vm->sp--;
                if (truthy(top)) break;
            }
            if (a >= prog->code_len) return vm_fail(vm, "bad jump");
            pc = a;
            break;
        case OP_PRINT:
            if (!top) return vm_fail(vm, "stack underflow");
            to_text(top, text, (int)sizeof(text));
            out_line(host, "pm> ", -0x7fffffff, text);
            vm->prints++; vm->sp--;
            break;
        case OP_WRITE:
        case OP_APPEND: {
            int n;
            if (vm->sp < 2) return vm_fail(vm, "stack underflow");
            to_text(&vm->stack[vm->sp - 2], text, (int)sizeof(text));   /* path */
            to_text(&vm->stack[vm->sp - 1], text2, (int)sizeof(text2)); /* content */
            vm->sp -= 2;
            if (!path_allowed(text)) { vm_fail(vm, "path denied"); return PM_ERR_DENIED; }
            n = s_len(text2);
            if (n < (int)sizeof(text2) - 1) { text2[n++] = '\n'; text2[n] = 0; }
            if (!host || !host->write_file || host->write_file(host->ctx, text, text2, n, op == OP_APPEND) < 0)
                return vm_fail(vm, "write failed");
            vm->writes++;
            break;
        }
        case OP_SHOW: {
            char buf[PM_VAL_MAX];
            int n, i, s = 0;
            if (!top) return vm_fail(vm, "stack underflow");
            to_text(top, text, (int)sizeof(text)); vm->sp--;
            n = (host && host->read_file) ? host->read_file(host->ctx, text, buf, (int)sizeof(buf) - 1) : -1;
            if (n < 0) return vm_fail(vm, "file not found");
            buf[n] = 0;
            for (i = 0; i <= n; i++) {
                if (buf[i] == '\n' || buf[i] == 0) {
                    char c0 = buf[i];
                    buf[i] = 0;
                    if (i > s || c0 == '\n') out_line(host, "pm> ", -0x7fffffff, buf + s);
                    s = i + 1;
                }
            }
            vm->prints++;
            break;
        }
        case OP_EXPECT:
            if (pc + 2 > prog->code_len) return vm_fail(vm, "truncated");
            a = rd16(prog->code + pc); pc += 2;
            if (!top) return vm_fail(vm, "stack underflow");
            vm->expects++;
            if (!truthy(top)) { vm->expect_failed++; out_line(host, "pm-expect failed line ", a, 0); }
            vm->sp--;
            break;
        default:
            return vm_fail(vm, "bad opcode");
        }
    }
}

int pm_run(const pm_program_t* prog, pm_vm_t* vm, const pm_host_t* host) {
    int st;
    if (!prog || !vm || prog->code_len == 0) return PM_ERR_RUNTIME;
    vm->sp = 0; vm->err[0] = 0;
    st = vm_exec(prog, vm, host, 0, 0);
    if (st == PM_OK && vm->expect_failed) return PM_ERR_EXPECT;
    return st;
}

int pm_say(const pm_program_t* prog, pm_vm_t* vm, const pm_host_t* host, const char* phrase) {
    int i;
    if (!prog || !vm || !phrase) return PM_ERR_RUNTIME;
    vm->sp = 0; vm->err[0] = 0;
    for (i = 0; i < prog->ntrig; i++)
        if (s_ieq(prog->pool + prog->triggers[i].phrase, phrase))
            return vm_exec(prog, vm, host, prog->triggers[i].addr, 1);
    return vm_fail(vm, "no trigger");
}

/* ------------------------------------------------------------ doc, disasm */
int pm_doc(const char* src, int len, const pm_program_t* prog, char* out, int cap) {
    int i = 0, k;
    char n[12];
    if (!out || cap <= 0) return PM_ERR_LIMIT;
    out[0] = 0;
    while (src && i < len) {
        int j = i, s = i;
        while (j < len && src[j] != '\n') j++;
        while (s < j && (src[s] == ' ' || src[s] == '\t')) s++;
        if (s + 1 < j && src[s] == '#' && src[s + 1] == '#') {
            int e = s + 2;
            char line[PM_VAL_MAX];
            int m = 0;
            while (e < j && src[e] == ' ') e++;
            while (e < j && src[e] != '\r' && m < (int)sizeof(line) - 1) line[m++] = src[e++];
            line[m] = 0;
            s_cat(out, "pm-doc about ", cap); s_cat(out, line, cap); s_cat(out, "\n", cap);
        }
        i = j + 1;
    }
    if (prog) {
        for (k = 0; k < prog->ntrig; k++) {
            s_cat(out, "pm-doc trigger \"", cap); s_cat(out, prog->pool + prog->triggers[k].phrase, cap); s_cat(out, "\"\n", cap);
        }
        for (k = 0; k < prog->nvars; k++) {
            if (prog->vars[k][0] == '#') continue;
            s_cat(out, "pm-doc variable ", cap); s_cat(out, prog->vars[k], cap); s_cat(out, "\n", cap);
        }
        s_cat(out, "pm-doc summary statements ", cap); s_int(n, prog->statements, 12); s_cat(out, n, cap);
        s_cat(out, " triggers ", cap); s_int(n, prog->ntrig, 12); s_cat(out, n, cap);
        s_cat(out, " libraries ", cap); s_int(n, prog->includes, 12); s_cat(out, n, cap);
        s_cat(out, " bytecode ", cap); s_int(n, prog->code_len, 12); s_cat(out, n, cap);
        s_cat(out, "\n", cap);
    }
    return s_len(out);
}

int pm_disasm(const pm_program_t* prog, char* out, int cap) {
    int pc = 0;
    char n[12];
    if (!prog || !out || cap <= 0) return PM_ERR_LIMIT;
    out[0] = 0;
    while (pc < prog->code_len) {
        int op = prog->code[pc], arg = -0x7fffffff, at = pc;
        if (op >= OP_COUNT) return PM_ERR_IMAGE;
        pc++;
        if (op == OP_PUSHI) { arg = rd32(prog->code + pc); pc += 4; }
        else if (op == OP_LINE || op == OP_PUSHS || op == OP_JMP || op == OP_JZ || op == OP_EXPECT) { arg = rd16(prog->code + pc); pc += 2; }
        else if (op == OP_LOAD || op == OP_STORE || op == OP_CMP) { arg = prog->code[pc++]; }
        s_int(n, at, 12); s_cat(out, n, cap); s_cat(out, " ", cap); s_cat(out, k_op_names[op], cap);
        if (arg != -0x7fffffff) { s_cat(out, " ", cap); s_int(n, arg, 12); s_cat(out, n, cap); }
        s_cat(out, "\n", cap);
        if (s_len(out) >= cap - 1) return PM_ERR_LIMIT;
    }
    return s_len(out);
}
