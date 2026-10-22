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

/* ---- Agent Pipeline Streaming ---- */
void pm_stream_init(pm_stream_t *st, const char *agent_id) {
    if (!st) return;
    st->write_pos = 0;
    st->read_pos = 0;
    st->closed = 0;
    st->buffer[0] = 0;
    st->agent_id[0] = 0;
    if (agent_id) s_copy(st->agent_id, agent_id, sizeof(st->agent_id));
}

int pm_stream_append(pm_stream_t *st, const char *chunk, int len) {
    if (!st || !chunk || st->closed) return -1;
    if (len < 0) len = s_len(chunk);
    int space = PM_STREAM_BUF_MAX - 1 - st->write_pos;
    if (space <= 0) return 0;
    int to_copy = (len < space) ? len : space;
    for (int i = 0; i < to_copy; i++) {
        st->buffer[st->write_pos++] = chunk[i];
    }
    st->buffer[st->write_pos] = 0;
    return to_copy;
}

int pm_stream_read(pm_stream_t *st, char *out, int cap) {
    if (!st || !out || cap < 2) return -1;
    out[0] = 0;
    int avail = st->write_pos - st->read_pos;
    if (avail <= 0) return 0;
    int to_read = (avail < cap - 1) ? avail : cap - 1;
    for (int i = 0; i < to_read; i++) {
        out[i] = st->buffer[st->read_pos++];
    }
    out[to_read] = 0;
    if (st->read_pos == st->write_pos) {
        st->read_pos = 0;
        st->write_pos = 0;
        st->buffer[0] = 0;
    }
    return to_read;
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
        if (i >= n || (!tok_is(&t[i], "to") && !tok_is(&t[i], "="))) return cc_fail(c, col_of(t, i, n, ls), "expected 'to'");
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
        char name[8];
        int head, jz;
        i++;
        name[0] = '#';
        name[1] = (char)('a' + (c->nblocks & 7));
        name[2] = (char)('a' + (c->depth & 7));
        name[3] = (char)('a' + (c->prog->code_len & 15));
        name[4] = 0;
        slot = var_slot(c, name, 4, 1);
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
        if (i < n && tok_is(&t[i], "user")) {
            i++;
            if (i < n && (tok_is(&t[i], "says") || tok_is(&t[i], "dit") || tok_is(&t[i], "triggers"))) i++;
        }
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

/* ------------------------------------------------- US-055 cross compiler */
/* Lot 5: PMC1 bytecode -> one portable C99 translation unit. Every
 * instruction becomes a labelled statement, jumps become goto, the value
 * stack and the semantics are those of vm_exec (same step budget, same path
 * rule, same "pm> " output). system.memory reads MOHHDY_MEM on the target.
 * One target only (C99 source); no native code generator. */
typedef struct { char* o; int n, cap, over; } pm_xo_t;
static void xo_s(pm_xo_t* x, const char* s) {
    while (*s) {
        if (x->n >= x->cap - 1) { x->over = 1; return; }
        x->o[x->n++] = *s++;
    }
    x->o[x->n] = 0;
}
static void xo_i(pm_xo_t* x, int v) { char n[12]; s_int(n, v, 12); xo_s(x, n); }

static const char k_pm_c_prelude[] =
    "/* PromptMessage -> C99 (pm-cross, MOHHDY). Build: cc -std=c99 -o app app.c\n"
    "   Run: ./app (main program) or ./app \"phrase\" (trigger). system.memory\n"
    "   reads the MOHHDY_MEM environment variable (default 0). */\n"
    "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n"
    "typedef struct { int s, n; char t[192]; } V;\n"
    "static V S[12]; static int sp, xf, trig; static unsigned long steps;\n"
    "static int die(const char* m) { fflush(stdout); fprintf(stderr, \"pm error: %s\\n\", m); exit(3); return 3; }\n"
    "static void txt(const V* v, char* o) { if (v->s) { strncpy(o, v->t, 191); o[191] = 0; } else sprintf(o, \"%d\", v->n); }\n"
    "static int tru(const V* v) { return v->s ? v->t[0] != 0 : v->n != 0; }\n"
    "static int low(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }\n"
    "static int ieq(const char* a, const char* b) { while (*a && low(*a) == low(*b)) { a++; b++; } return *a == *b; }\n"
    "static V* top(void) { if (sp < 1) die(\"stack underflow\"); return &S[sp - 1]; }\n"
    "static V* push(void) { if (sp >= 12) die(\"stack overflow\"); return &S[sp++]; }\n"
    "static void pi(int n) { V* v = push(); v->s = 0; v->n = n; }\n"
    "static void sysmem(void) { const char* e = getenv(\"MOHHDY_MEM\"); pi(e ? atoi(e) : 0); }\n"
    "static void add(void) { V *l, *r; char a[192], b[192]; if (sp < 2) die(\"stack underflow\"); l = &S[sp - 2]; r = &S[sp - 1];\n"
    "  if (!l->s && !r->s) l->n = l->n + r->n; else { txt(l, a); txt(r, b); strncat(a, b, 191 - strlen(a)); l->s = 1; strcpy(l->t, a); } sp--; }\n"
    "static void cmp(int c) { V *l, *r; char a[192], b[192]; int x; if (sp < 2) die(\"stack underflow\"); l = &S[sp - 2]; r = &S[sp - 1];\n"
    "  if (c == 6) { txt(l, a); txt(r, b); x = strstr(a, b) != 0; }\n"
    "  else if (!l->s && !r->s) { int p = l->n, q = r->n; x = c == 0 ? p == q : c == 1 ? p != q : c == 2 ? p < q : c == 3 ? p > q : c == 4 ? p <= q : p >= q; }\n"
    "  else { txt(l, a); txt(r, b); if (c == 0) x = strcmp(a, b) == 0; else if (c == 1) x = strcmp(a, b) != 0; else x = die(\"order compare on text\"); }\n"
    "  l->s = 0; l->n = x; sp--; }\n"
    "static void logic(int both) { V *l, *r; int x, y; if (sp < 2) die(\"stack underflow\"); l = &S[sp - 2]; r = &S[sp - 1];\n"
    "  x = tru(l); y = tru(r); l->s = 0; l->n = both ? (x && y) : (x || y); sp--; }\n"
    "static void not_(void) { V* v = top(); int a = !tru(v); v->s = 0; v->n = a; }\n"
    "static void fexists(void) { V* v = top(); char p[192]; FILE* f; txt(v, p); f = fopen(p, \"rb\"); v->s = 0; v->n = f != 0; if (f) fclose(f); }\n"
    "static int jz(void) { V* v = top(); sp--; return !tru(v); }\n"
    "static void print_(void) { char t[192]; txt(top(), t); printf(\"pm> %s\\n\", t); sp--; }\n"
    "static int allowed(const char* p) { return p[0] && !strstr(p, \"..\") && strncmp(p, \"/bin/\", 5) && strncmp(p, \"/models/\", 8)\n"
    "  && strncmp(p, \"bin/\", 4) && strncmp(p, \"models/\", 7); }\n"
    "static void write_(int app) { char p[192], c[193]; FILE* f; size_t n; if (sp < 2) die(\"stack underflow\"); txt(&S[sp - 2], p); txt(&S[sp - 1], c); sp -= 2;\n"
    "  if (!allowed(p)) die(\"path denied\"); n = strlen(c); if (n < 191) { c[n++] = '\\n'; c[n] = 0; }\n"
    "  f = fopen(p, app ? \"ab\" : \"wb\"); if (!f || fwrite(c, 1, n, f) != n) die(\"write failed\"); fclose(f); }\n"
    "static void show(void) { char p[192], b[192]; FILE* f; int n, i, s = 0; txt(top(), p); sp--; f = fopen(p, \"rb\"); if (!f) die(\"file not found\");\n"
    "  n = (int)fread(b, 1, 191, f); fclose(f); b[n] = 0;\n"
    "  for (i = 0; i <= n; i++) if (b[i] == '\\n' || b[i] == 0) { char c0 = b[i]; b[i] = 0; if (i > s || c0 == '\\n') printf(\"pm> %s\\n\", b + s); s = i + 1; } }\n"
    "static void expect(int line) { V* v = top(); if (!tru(v)) { xf++; printf(\"pm-expect failed line %d\\n\", line); } sp--; }\n"
    "static int fin(void) { fflush(stdout); return xf ? 2 : 0; }\n"
    "#define T if (++steps > 20000UL) die(\"step budget exhausted\");\n";

int pm_cross_c(const pm_program_t* prog, char* out, int cap, int* instructions) {
    pm_xo_t x;
    int pc = 0, i, count = 0;
    if (instructions) *instructions = 0;
    if (!prog || !out || cap < 64 || prog->code_len == 0) return PM_ERR_LIMIT;
    x.o = out; x.n = 0; x.cap = cap; x.over = 0; out[0] = 0;
    xo_s(&x, k_pm_c_prelude);
    xo_s(&x, "static V R["); xo_i(&x, prog->nvars ? prog->nvars : 1); xo_s(&x, "];\n");
    xo_s(&x, "static const char P[] = \"");
    for (i = 0; i < prog->pool_len; i++) {
        unsigned char c = (unsigned char)prog->pool[i];
        if (c >= 32 && c < 127 && c != '"' && c != '\\' && c != '?') {
            char one[2]; one[0] = (char)c; one[1] = 0; xo_s(&x, one);
        } else {
            char esc[5];
            esc[0] = '\\'; esc[1] = (char)('0' + ((c >> 6) & 7)); esc[2] = (char)('0' + ((c >> 3) & 7));
            esc[3] = (char)('0' + (c & 7)); esc[4] = 0;
            xo_s(&x, esc);
        }
        if ((i & 63) == 63) xo_s(&x, "\"\n  \"");
    }
    xo_s(&x, "\";\n");
    xo_s(&x, "static void ps(int a) { V* v = push(); v->s = 1; strncpy(v->t, P + a, 191); v->t[191] = 0; }\n");
    xo_s(&x, "static void ld(int a) { *push() = R[a]; }\n");
    xo_s(&x, "static void st(int a) { R[a] = *top(); sp--; }\n");
    /* `./app PHRASE` mirrors `pm-run` followed by `pm-say PHRASE`: the
     * main program runs first, so the trigger sees its variables. */
    xo_s(&x, "static const char* phrase;\n");
    xo_s(&x, "int main(int argc, char** argv) {\n  if (argc > 1) phrase = argv[1];\n  goto L0;\n");
    while (pc < prog->code_len) {
        int op = prog->code[pc], at = pc, a = 0;
        if (op >= OP_COUNT) return PM_ERR_IMAGE;
        pc++;
        if (op == OP_PUSHI) { if (pc + 4 > prog->code_len) return PM_ERR_IMAGE; a = rd32(prog->code + pc); pc += 4; }
        else if (op == OP_LINE || op == OP_PUSHS || op == OP_JMP || op == OP_JZ || op == OP_EXPECT) {
            if (pc + 2 > prog->code_len) return PM_ERR_IMAGE;
            a = rd16(prog->code + pc); pc += 2;
        } else if (op == OP_LOAD || op == OP_STORE || op == OP_CMP) {
            if (pc + 1 > prog->code_len) return PM_ERR_IMAGE;
            a = prog->code[pc++];
        }
        if ((op == OP_JMP || op == OP_JZ) && a >= prog->code_len) return PM_ERR_IMAGE;
        if ((op == OP_LOAD || op == OP_STORE) && a >= prog->nvars) return PM_ERR_IMAGE;
        if (op == OP_PUSHS && a >= prog->pool_len) return PM_ERR_IMAGE;
        count++;
        xo_s(&x, "L"); xo_i(&x, at); xo_s(&x, ": T ");
        switch (op) {
        case OP_HALT: xo_s(&x, "if (phrase && !trig) goto DISPATCH; return fin();"); break;
        case OP_RET: xo_s(&x, "if (!trig) die(\"ret outside trigger\"); return fin();"); break;
        case OP_LINE: xo_s(&x, "; /* line "); xo_i(&x, a); xo_s(&x, " */"); break;
        case OP_PUSHI: xo_s(&x, "pi("); xo_i(&x, a); xo_s(&x, ");"); break;
        case OP_PUSHS: xo_s(&x, "ps("); xo_i(&x, a); xo_s(&x, ");"); break;
        case OP_LOAD: xo_s(&x, "ld("); xo_i(&x, a); xo_s(&x, ");"); break;
        case OP_STORE: xo_s(&x, "st("); xo_i(&x, a); xo_s(&x, ");"); break;
        case OP_SYSMEM: xo_s(&x, "sysmem();"); break;
        case OP_ADD: xo_s(&x, "add();"); break;
        case OP_CMP: xo_s(&x, "cmp("); xo_i(&x, a); xo_s(&x, ");"); break;
        case OP_AND: xo_s(&x, "logic(1);"); break;
        case OP_OR: xo_s(&x, "logic(0);"); break;
        case OP_NOT: xo_s(&x, "not_();"); break;
        case OP_FEXISTS: xo_s(&x, "fexists();"); break;
        case OP_JMP: xo_s(&x, "goto L"); xo_i(&x, a); xo_s(&x, ";"); break;
        case OP_JZ: xo_s(&x, "if (jz()) goto L"); xo_i(&x, a); xo_s(&x, ";"); break;
        case OP_PRINT: xo_s(&x, "print_();"); break;
        case OP_WRITE: xo_s(&x, "write_(0);"); break;
        case OP_APPEND: xo_s(&x, "write_(1);"); break;
        case OP_SHOW: xo_s(&x, "show();"); break;
        case OP_EXPECT: xo_s(&x, "expect("); xo_i(&x, a); xo_s(&x, ");"); break;
        default: return PM_ERR_IMAGE;
        }
        xo_s(&x, "\n");
        if (x.over) return PM_ERR_LIMIT;
    }
    xo_s(&x, "  if (phrase && !trig) goto DISPATCH;\n  return fin();\nDISPATCH:\n  trig = 1;\n");
    for (i = 0; i < prog->ntrig; i++) {
        xo_s(&x, "  if (ieq(phrase, P + "); xo_i(&x, prog->triggers[i].phrase);
        xo_s(&x, ")) goto L"); xo_i(&x, prog->triggers[i].addr); xo_s(&x, ";\n");
    }
    xo_s(&x, "  return die(\"no trigger\");\n}\n");
    if (x.over) return PM_ERR_LIMIT;
    if (instructions) *instructions = count;
    return x.n;
}

/* ------------------------------------------- US-058 development assistant */
/* Lot 5: local, rule based (no language model). On a compile error it
 * names the nearest keyword (edit distance <= 2) to the first word of the
 * failing line, and points out a missing `then`, `to` or `times`; on a
 * clean program it lints: variables set but never read, `if` without
 * `else` writing files, triggers without output. */
static const char* const k_pm_keywords[] = {
    "print", "say", "set", "to", "create", "file", "with", "content", "append", "show",
    "expect", "stop", "use", "if", "then", "else", "end", "repeat", "times", "when",
    "user", "says", 0
};

static int pm_edit_distance(const char* a, int al, const char* b) {
    int bl = s_len(b), i, j, prev[24], cur[24];
    if (al > 22 || bl > 22) return 99;
    for (j = 0; j <= bl; j++) prev[j] = j;
    for (i = 1; i <= al; i++) {
        cur[0] = i;
        for (j = 1; j <= bl; j++) {
            int c = prev[j - 1] + (lower(a[i - 1]) == b[j - 1] ? 0 : 1);
            if (prev[j] + 1 < c) c = prev[j] + 1;
            if (cur[j - 1] + 1 < c) c = cur[j - 1] + 1;
            cur[j] = c;
        }
        for (j = 0; j <= bl; j++) prev[j] = cur[j];
    }
    return prev[bl];
}

/* line `want` (1-based) of src into buf */
static void pm_src_line(const char* src, int len, int want, char* buf, int cap) {
    int i = 0, line = 1, k = 0;
    buf[0] = 0;
    while (i < len && line < want) { if (src[i] == '\n') line++; i++; }
    while (i < len && src[i] != '\n' && k < cap - 1) buf[k++] = src[i++];
    buf[k] = 0;
}

static int pm_word_in(const char* line, const char* w) {
    int i = 0, n = s_len(w);
    while (line[i]) {
        int j;
        while (line[i] == ' ' || line[i] == '\t') i++;
        for (j = 0; j < n && lower(line[i + j]) == w[j]; j++) {}
        if (j == n && (line[i + j] == 0 || line[i + j] == ' ' || line[i + j] == '\t')) return 1;
        while (line[i] && line[i] != ' ' && line[i] != '\t') {
            if (line[i] == '"') { i++; while (line[i] && line[i] != '"') i++; if (line[i]) i++; }
            else i++;
        }
    }
    return 0;
}

int pm_assist(const char* src, int len, const pm_host_t* host, char* out, int cap, int* hints) {
    static pm_program_t prog;
    char line[PM_VAL_MAX], n[12];
    int st, count = 0;
    if (!out || cap <= 0) return PM_ERR_LIMIT;
    out[0] = 0;
    st = pm_compile(src, len, &prog, host);
    if (st != PM_OK) {
        int s = 0, e, k, best = -1, bestd = 3;
        s_cat(out, "pm-assist error line ", cap); s_int(n, prog.err_line, 12); s_cat(out, n, cap);
        s_cat(out, ": ", cap); s_cat(out, prog.err, cap); s_cat(out, "\n", cap);
        pm_src_line(src, len, prog.err_line, line, (int)sizeof(line));
        while (line[s] == ' ' || line[s] == '\t') s++;
        e = s;
        while (line[e] && line[e] != ' ' && line[e] != '\t') e++;
        for (k = 0; k_pm_keywords[k]; k++) {
            int d = pm_edit_distance(line + s, e - s, k_pm_keywords[k]);
            if (d < bestd) { bestd = d; best = k; }
        }
        if (best >= 0 && bestd > 0) {
            s_cat(out, "pm-assist hint: did you mean '", cap); s_cat(out, k_pm_keywords[best], cap);
            s_cat(out, "' instead of '", cap);
            { char w[24]; int m = e - s < 23 ? e - s : 23; for (k = 0; k < m; k++) w[k] = line[s + k]; w[m] = 0; s_cat(out, w, cap); }
            s_cat(out, "'\n", cap);
            count++;
        }
        if ((pm_word_in(line, "if") || pm_word_in(line, "when")) && !pm_word_in(line, "then")) {
            s_cat(out, "pm-assist hint: add 'then' after the condition\n", cap); count++;
        }
        if (pm_word_in(line, "set") && !pm_word_in(line, "to") && !s_has(line, "=")) {
            s_cat(out, "pm-assist hint: write 'set NAME to VALUE'\n", cap); count++;
        }
        if (pm_word_in(line, "repeat") && !pm_word_in(line, "times")) {
            s_cat(out, "pm-assist hint: write 'repeat N times'\n", cap); count++;
        }
        if (count == 0) { s_cat(out, "pm-assist hint: check quotes and the block 'end' lines\n", cap); count++; }
        if (hints) *hints = count;
        return PM_ERR_SYNTAX;
    }
    {   /* lint: variables set but never read (LOAD) */
        int used[PM_VARS], v, pc = 0;
        for (v = 0; v < PM_VARS; v++) used[v] = 0;
        while (pc < prog.code_len) {
            int op = prog.code[pc++];
            if (op == OP_PUSHI) pc += 4;
            else if (op == OP_LINE || op == OP_PUSHS || op == OP_JMP || op == OP_JZ || op == OP_EXPECT) pc += 2;
            else if (op == OP_LOAD) { if (prog.code[pc] < PM_VARS) used[prog.code[pc]] = 1; pc++; }
            else if (op == OP_STORE || op == OP_CMP) pc++;
        }
        for (v = 0; v < prog.nvars; v++) {
            if (prog.vars[v][0] == '#' || used[v]) continue;
            s_cat(out, "pm-assist lint: variable ", cap); s_cat(out, prog.vars[v], cap);
            s_cat(out, " is set but never read\n", cap);
            count++;
        }
    }
    {   /* lint: triggers whose code never prints or writes before RET */
        int t;
        for (t = 0; t < prog.ntrig; t++) {
            int pc = prog.triggers[t].addr, effect = 0;
            while (pc < prog.code_len) {
                int op = prog.code[pc++];
                if (op == OP_PRINT || op == OP_WRITE || op == OP_APPEND || op == OP_SHOW) effect = 1;
                if (op == OP_RET || op == OP_HALT) break;
                if (op == OP_PUSHI) pc += 4;
                else if (op == OP_LINE || op == OP_PUSHS || op == OP_JMP || op == OP_JZ || op == OP_EXPECT) pc += 2;
                else if (op == OP_LOAD || op == OP_STORE || op == OP_CMP) pc++;
            }
            if (!effect) {
                s_cat(out, "pm-assist lint: trigger \"", cap); s_cat(out, prog.pool + prog.triggers[t].phrase, cap);
                s_cat(out, "\" has no visible effect\n", cap);
                count++;
            }
        }
    }
    s_cat(out, "pm-assist ok statements ", cap); s_int(n, prog.statements, 12); s_cat(out, n, cap);
    s_cat(out, " hints ", cap); s_int(n, count, 12); s_cat(out, n, cap); s_cat(out, " engine=rules\n", cap);
    if (hints) *hints = count;
    return PM_OK;
}

/* ------------------------------------------- US-060 revocation list (CRL) */
/* Lot 5: local revocation list for the local PromptMessage CA. One line
 * per revoked source, "pmcrl1 <32 hex>\n", the first 16 bytes of the
 * pm_sig_digest of the revoked source. Local file, no distribution. */
static int crl_hex(const unsigned char* d, char* out) {
    int i;
    for (i = 0; i < PM_CRL_DIGEST; i++) {
        out[2 * i] = "0123456789abcdef"[d[i] >> 4];
        out[2 * i + 1] = "0123456789abcdef"[d[i] & 15];
    }
    return 2 * PM_CRL_DIGEST;
}

int pm_crl_has(const char* crl, int len, const unsigned char* digest) {
    char want[2 * PM_CRL_DIGEST];
    int i = 0, k;
    if (!crl || len <= 0 || !digest) return 0;
    crl_hex(digest, want);
    while (i + 7 + 2 * PM_CRL_DIGEST <= len) {
        int match = 1;
        const char* tag = "pmcrl1 ";
        for (k = 0; k < 7; k++) if (crl[i + k] != tag[k]) { match = 0; break; }
        if (match) {
            for (k = 0; k < 2 * PM_CRL_DIGEST; k++) if (crl[i + 7 + k] != want[k]) { match = 0; break; }
            if (match) return 1;
        }
        while (i < len && crl[i] != '\n') i++;
        i++;
    }
    return 0;
}

int pm_crl_add(char* crl, int len, int cap, const unsigned char* digest) {
    int need = 7 + 2 * PM_CRL_DIGEST + 1;
    if (!crl || len < 0 || !digest) return PM_ERR_LIMIT;
    if (pm_crl_has(crl, len, digest)) return len;
    if (len + need > cap) return PM_ERR_LIMIT;
    { const char* tag = "pmcrl1 "; int k; for (k = 0; k < 7; k++) crl[len + k] = tag[k]; }
    crl_hex(digest, crl + len + 7);
    crl[len + need - 1] = '\n';
    return len + need;
}

int pm_ca_rotate_status(int rotation_count, char* out, int cap) {
    int p = 0;
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    {
        const char *head = "pm-ca-rotate status=ok rotations=";
        int i = 0;
        while (head[i] && p < cap - 1) out[p++] = head[i++];
    }
    {
        char num[16];
        int k = 0, v = rotation_count < 0 ? 0 : rotation_count;
        if (v == 0) { num[k++] = '0'; }
        else {
            char rev[16]; int r = 0;
            while (v && r < 15) { rev[r++] = (char)('0' + (v % 10)); v /= 10; }
            while (r) num[k++] = rev[--r];
        }
        num[k] = 0;
        for (int i = 0; num[i] && p < cap - 1; i++) out[p++] = num[i];
    }
    if (p < cap - 1) out[p++] = '\n';
    out[p] = 0;
    return p;
}

/* ---- US-061 Multi-Agent Bus & Session Context ---- */
static int pm_streq(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

static void pm_strcpy(char* d, int cap, const char* s) {
    int i = 0;
    if (!d || cap <= 0) return;
    if (!s) s = "";
    while (s[i] && i < cap - 1) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

void pm_context_init(pm_context_t* ctx, const char* agent_id, uint32_t session_id) {
    if (!ctx) return;
    pm_strcpy(ctx->agent_id, sizeof(ctx->agent_id), agent_id ? agent_id : "agent0");
    ctx->session_id = session_id;
    ctx->var_count = 0;
    for (int i = 0; i < PM_VARS; i++) {
        ctx->keys[i][0] = 0;
        ctx->vals[i][0] = 0;
    }
}

int pm_context_set_var(pm_context_t* ctx, const char* key, const char* val) {
    if (!ctx || !key) return -1;
    for (int i = 0; i < ctx->var_count; i++) {
        if (pm_streq(ctx->keys[i], key)) {
            pm_strcpy(ctx->vals[i], sizeof(ctx->vals[i]), val);
            return 0;
        }
    }
    if (ctx->var_count >= PM_VARS) return -1;
    pm_strcpy(ctx->keys[ctx->var_count], sizeof(ctx->keys[ctx->var_count]), key);
    pm_strcpy(ctx->vals[ctx->var_count], sizeof(ctx->vals[ctx->var_count]), val);
    ctx->var_count++;
    return 0;
}

const char* pm_context_get_var(const pm_context_t* ctx, const char* key) {
    if (!ctx || !key) return 0;
    for (int i = 0; i < ctx->var_count; i++) {
        if (pm_streq(ctx->keys[i], key)) {
            return ctx->vals[i];
        }
    }
    return 0;
}

int pm_context_serialize(const pm_context_t* ctx, char* out, int cap) {
    int p = 0;
    if (!ctx || !out || cap < 16) return -1;
    pm_strcpy(out, cap, ctx->agent_id);
    p = 0; while (out[p]) p++;
    if (p < cap - 1) out[p++] = ':';
    {
        char num[16];
        int k = 0; uint32_t v = ctx->session_id;
        if (v == 0) num[k++] = '0';
        else {
            char rev[16]; int r = 0;
            while (v && r < 15) { rev[r++] = (char)('0' + (v % 10)); v /= 10; }
            while (r) num[k++] = rev[--r];
        }
        num[k] = 0;
        for (int i = 0; num[i] && p < cap - 1; i++) out[p++] = num[i];
    }
    if (p < cap - 1) out[p++] = ':';
    for (int i = 0; i < ctx->var_count && p < cap - 2; i++) {
        const char* k = ctx->keys[i];
        const char* v = ctx->vals[i];
        while (*k && p < cap - 2) out[p++] = *k++;
        out[p++] = '=';
        while (*v && p < cap - 2) out[p++] = *v++;
        out[p++] = ';';
    }
    out[p] = 0;
    return p;
}

int pm_context_deserialize(pm_context_t* ctx, const char* in, int len) {
    if (!ctx || !in || len <= 0) return -1;
    pm_context_init(ctx, "deserialized", 0);
    int pos = 0;
    int i = 0;
    while (pos < len && in[pos] != ':' && i < PM_AGENT_NAME_MAX - 1) {
        ctx->agent_id[i++] = in[pos++];
    }
    ctx->agent_id[i] = 0;
    if (pos < len && in[pos] == ':') pos++;
    uint32_t sid = 0;
    while (pos < len && in[pos] >= '0' && in[pos] <= '9') {
        sid = sid * 10 + (uint32_t)(in[pos++] - '0');
    }
    ctx->session_id = sid;
    if (pos < len && in[pos] == ':') pos++;
    while (pos < len && ctx->var_count < PM_VARS) {
        char key[PM_NAME_MAX];
        char val[PM_VAL_MAX];
        int kp = 0, vp = 0;
        while (pos < len && in[pos] != '=' && in[pos] != ';' && kp < PM_NAME_MAX - 1) {
            key[kp++] = in[pos++];
        }
        key[kp] = 0;
        if (pos < len && in[pos] == '=') pos++;
        while (pos < len && in[pos] != ';' && vp < PM_VAL_MAX - 1) {
            val[vp++] = in[pos++];
        }
        val[vp] = 0;
        if (pos < len && in[pos] == ';') pos++;
        if (kp > 0) {
            pm_context_set_var(ctx, key, val);
        }
    }
    return 0;
}

int pm_pipe_exec(const pm_program_t* prog1, const pm_program_t* prog2,
                 pm_vm_t* vm1, pm_vm_t* vm2,
                 pm_context_t* ctx, const pm_host_t* host) {
    if (!prog1 || !prog2 || !vm1 || !vm2 || !ctx) return PM_ERR_RUNTIME;
    pm_vm_reset(vm1);
    pm_vm_reset(vm2);
    int st1 = pm_run(prog1, vm1, host);
    if (st1 != PM_OK) return st1;

    for (int i = 0; i < prog1->nvars; i++) {
        if (vm1->vars[i].is_str) {
            pm_context_set_var(ctx, prog1->vars[i], vm1->vars[i].str);
        }
    }

    int st2 = pm_run(prog2, vm2, host);
    return st2;
}

/* ---- US-062 Multi-Agent Async Message Queue & Bus ---- */
typedef struct {
    char channel[32];
    char agent_id[24];
} pm_bus_sub_t;

typedef struct {
    char channel[32];
    char sender[24];
    char message[128];
    int active;
} pm_bus_msg_slot_t;

static pm_bus_sub_t g_pm_bus_subs[PM_BUS_CHANNELS * PM_BUS_SUBS_PER_CHAN];
static int g_pm_bus_sub_count = 0;

static pm_bus_msg_slot_t g_pm_bus_msg_queue[PM_BUS_MSG_QUEUE];
static int g_pm_bus_msg_head = 0;
static int g_pm_bus_msg_tail = 0;

void pm_bus_init(void) {
    g_pm_bus_sub_count = 0;
    for (int i = 0; i < PM_BUS_CHANNELS * PM_BUS_SUBS_PER_CHAN; i++) {
        g_pm_bus_subs[i].channel[0] = 0;
        g_pm_bus_subs[i].agent_id[0] = 0;
    }
    g_pm_bus_msg_head = 0;
    g_pm_bus_msg_tail = 0;
    for (int i = 0; i < PM_BUS_MSG_QUEUE; i++) {
        g_pm_bus_msg_queue[i].active = 0;
    }
}

int pm_bus_subscribe(const char *channel, const char *agent_id) {
    if (!channel || !agent_id) return -1;
    for (int i = 0; i < g_pm_bus_sub_count; i++) {
        if (pm_streq(g_pm_bus_subs[i].channel, channel) &&
            pm_streq(g_pm_bus_subs[i].agent_id, agent_id)) {
            return 0;
        }
    }
    if (g_pm_bus_sub_count >= PM_BUS_CHANNELS * PM_BUS_SUBS_PER_CHAN) return -1;
    pm_strcpy(g_pm_bus_subs[g_pm_bus_sub_count].channel, sizeof(g_pm_bus_subs[0].channel), channel);
    pm_strcpy(g_pm_bus_subs[g_pm_bus_sub_count].agent_id, sizeof(g_pm_bus_subs[0].agent_id), agent_id);
    g_pm_bus_sub_count++;
    return 0;
}

int pm_bus_publish(const char *channel, const char *sender, const char *message) {
    if (!channel || !sender || !message) return -1;
    int next_tail = (g_pm_bus_msg_tail + 1) % PM_BUS_MSG_QUEUE;
    if (next_tail == g_pm_bus_msg_head && g_pm_bus_msg_queue[g_pm_bus_msg_head].active) {
        return -1;
    }
    pm_bus_msg_slot_t *slot = &g_pm_bus_msg_queue[g_pm_bus_msg_tail];
    pm_strcpy(slot->channel, sizeof(slot->channel), channel);
    pm_strcpy(slot->sender, sizeof(slot->sender), sender);
    pm_strcpy(slot->message, sizeof(slot->message), message);
    slot->active = 1;
    g_pm_bus_msg_tail = next_tail;
    return 0;
}

int pm_bus_dequeue(const char *agent_id, char *out_channel, char *out_sender, char *out_message, int cap) {
    if (!agent_id) return 0;
    int cur = g_pm_bus_msg_head;
    while (cur != g_pm_bus_msg_tail || g_pm_bus_msg_queue[cur].active) {
        if (g_pm_bus_msg_queue[cur].active) {
            int matched = 0;
            for (int s = 0; s < g_pm_bus_sub_count; s++) {
                if (pm_streq(g_pm_bus_subs[s].agent_id, agent_id) &&
                    (pm_streq(g_pm_bus_subs[s].channel, g_pm_bus_msg_queue[cur].channel) || pm_streq(g_pm_bus_subs[s].channel, "*"))) {
                    matched = 1;
                    break;
                }
            }
            if (matched) {
                if (out_channel) pm_strcpy(out_channel, 32, g_pm_bus_msg_queue[cur].channel);
                if (out_sender) pm_strcpy(out_sender, 24, g_pm_bus_msg_queue[cur].sender);
                if (out_message) pm_strcpy(out_message, cap, g_pm_bus_msg_queue[cur].message);
                g_pm_bus_msg_queue[cur].active = 0;
                if (cur == g_pm_bus_msg_head) {
                    g_pm_bus_msg_head = (g_pm_bus_msg_head + 1) % PM_BUS_MSG_QUEUE;
                }
                return 1;
            }
        }
        cur = (cur + 1) % PM_BUS_MSG_QUEUE;
        if (cur == g_pm_bus_msg_tail) break;
    }
    return 0;
}
