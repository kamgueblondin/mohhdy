/* wf_jsparse.c - Lot 113: lexer and recursive-descent parser of the
 * JavaScript subset (see wf_js.h). The tree lives in the realm's arena
 * (js_node_t). Errors stop the parse with "SyntaxError: ... (line N)"
 * in vm->err. Nesting is bounded (PS_MAX_NEST) and the interpreter's
 * stack is checked as well. */
#include "wf_jsi.h"

enum { TK_EOF = 0, TK_NUM, TK_STR, TK_TMPL, TK_IDENT, TK_OP, TK_REGEX, TK_ERR, TK_PRIV /* Lot 115: #name */ };

enum {
    KW_NONE = 0, KW_VAR, KW_LET, KW_CONST, KW_FUNCTION, KW_RETURN, KW_IF, KW_ELSE, KW_FOR, KW_WHILE, KW_DO,
    KW_BREAK, KW_CONTINUE, KW_NEW, KW_DELETE, KW_TYPEOF, KW_INSTANCEOF, KW_IN, KW_OF, KW_THIS, KW_NULL,
    KW_TRUE, KW_FALSE, KW_THROW, KW_TRY, KW_CATCH, KW_FINALLY, KW_SWITCH, KW_CASE, KW_DEFAULT, KW_VOID,
    KW_CLASS, KW_EXTENDS, KW_SUPER, KW_STATIC, KW_GET, KW_SET, KW_ASYNC, KW_AWAIT, KW_YIELD, KW_DEBUGGER,
    KW_WITH, KW_IMPORT, KW_EXPORT, KW_ENUM
};

static const char *const k_kw[] = {
    "", "var", "let", "const", "function", "return", "if", "else", "for", "while", "do", "break", "continue",
    "new", "delete", "typeof", "instanceof", "in", "of", "this", "null", "true", "false", "throw", "try",
    "catch", "finally", "switch", "case", "default", "void", "class", "extends", "super", "static", "get",
    "set", "async", "await", "yield", "debugger", "with", "import", "export", "enum", 0
};

/* keywords that are never identifiers */
static int kw_reserved(int kw) {
    switch (kw) {
    case KW_NONE: case KW_LET: case KW_OF: case KW_STATIC: case KW_GET: case KW_SET: case KW_ASYNC:
    case KW_AWAIT: case KW_YIELD:
        return 0;
    default: return 1;
    }
}

/* punctuators, longest first within a shared prefix */
static const struct { const char *s; int op; } k_punct[] = {
    { ">>>=", OP_SHR_A }, { "...", OP_ELLIPSIS }, { "===", OP_SEQ }, { "!==", OP_SNE }, { "**=", OP_EXP_A },
    { "<<=", OP_SHL_A }, { ">>=", OP_SAR_A }, { ">>>", OP_SHR }, { "&&=", OP_AND_A }, { "||=", OP_OR_A },
    { "?" "?=", OP_NULLISH_A }, { "=>", OP_ARROW }, { "==", OP_EQ }, { "!=", OP_NE }, { "<=", OP_LE },
    { ">=", OP_GE }, { "&&", OP_AND }, { "||", OP_OR }, { "?" "?", OP_NULLISH }, { "?.", OP_QDOT },
    { "++", OP_INC }, { "--", OP_DEC }, { "+=", OP_ADD_A }, { "-=", OP_SUB_A }, { "*=", OP_MUL_A },
    { "/=", OP_DIV_A }, { "%=", OP_MOD_A }, { "&=", OP_BAND_A }, { "|=", OP_BOR_A }, { "^=", OP_BXOR_A },
    { "<<", OP_SHL }, { ">>", OP_SAR }, { "**", OP_EXP }, { "{", OP_LBRACE }, { "}", OP_RBRACE },
    { "(", OP_LPAREN }, { ")", OP_RPAREN }, { "[", OP_LBRACK }, { "]", OP_RBRACK }, { ";", OP_SEMI },
    { ",", OP_COMMA }, { ".", OP_DOT }, { "?", OP_QUESTION }, { ":", OP_COLON }, { "<", OP_LT },
    { ">", OP_GT }, { "+", OP_ADD }, { "-", OP_SUB }, { "*", OP_MUL }, { "/", OP_DIV }, { "%", OP_MOD },
    { "&", OP_BAND }, { "|", OP_BOR }, { "^", OP_BXOR }, { "!", OP_NOT }, { "~", OP_BNOT }, { "=", OP_ASSIGN },
    { 0, 0 }
};

typedef struct {
    int type, start, end, nl, op, kw, tail;
    uint32_t line, str, str2;
    jf64 num;
} tok_t;

typedef struct {
    int pos;
    uint32_t line;
    tok_t t;
    int last_sig;
} lexsave_t;

#define PS_MAX_NEST 120
#define PS_MAX_PRIV 96          /* Lot 115: private names in nested class bodies */

typedef struct {
    js_vm_t *vm;
    const char *s;
    int len, pos;
    uint32_t line;
    tok_t t;
    int last_sig;      /* the previous token allows a regex after it */
    int failed;
    int nest;
    uint32_t vhead, vtail;      /* hoisted var names of the current function */
    int fn_flags;               /* F_ARGS seen in the current function */
    int in_func, in_class;
    int in_gen, in_async;       /* Lot 114: yield / await are operators here */
    /* Lot 115: private names. Each class body declares names (pdecl from
     * its pbase); references (pref) not declared by the class that ends
     * move out to the enclosing class; none may be left at the outermost */
    int cdepth;
    int npdecl, npref;
    uint32_t pdecl[PS_MAX_PRIV];
    uint8_t pkind[PS_MAX_PRIV];     /* 1 getter, 2 setter, 3 field or method; 4 static */
    uint32_t pref[PS_MAX_PRIV];
    uint32_t pref_line[PS_MAX_PRIV];
    char buf[1024];             /* string literal decoding */
} ps_t;

static void ps_error(ps_t *p, const char *msg) {
    js_vm_t *vm = p->vm;
    char b[200];
    int n = 0, i;
    uint32_t ln = p->t.line ? p->t.line : p->line, d = 1;
    const char *pre = "SyntaxError: ";
    if (p->failed) return;
    p->failed = 1;
    for (i = 0; pre[i] && n < 150; i++) b[n++] = pre[i];
    for (i = 0; msg[i] && n < 170; i++) b[n++] = msg[i];
    for (i = 0; " (line " [i] && n < 180; i++) b[n++] = " (line "[i];
    while (d * 10 <= ln) d *= 10;
    while (d && n < 196) { b[n++] = (char)('0' + (ln / d) % 10); d /= 10; }
    b[n++] = ')';
    b[n] = 0;
    js_abort(vm, b);
    vm->err_line = (int)ln;
}

static int stack_low(ps_t *p) {
    char probe;
    js_vm_t *vm = p->vm;
    if (vm->stack_base && (uint32_t)((uint8_t *)&probe - vm->stack_base) < JS_STACK_RESERVE) return 1;
    return 0;
}

/* ---- lexer ---- */

static int is_id_start(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c >= 0x80; }
static int is_id_char(int c) { return is_id_start(c) || (c >= '0' && c <= '9'); }
static int is_digit(int c) { return c >= '0' && c <= '9'; }

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int utf8_put(char *b, int n, int max, uint32_t cp) {
    if (cp < 0x80) { if (n < max) b[n++] = (char)cp; }
    else if (cp < 0x800) {
        if (n + 1 < max) { b[n++] = (char)(0xC0 | (cp >> 6)); b[n++] = (char)(0x80 | (cp & 0x3F)); }
    } else if (cp < 0x10000) {
        if (n + 2 < max) {
            b[n++] = (char)(0xE0 | (cp >> 12)); b[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            b[n++] = (char)(0x80 | (cp & 0x3F));
        }
    } else if (n + 3 < max) {
        b[n++] = (char)(0xF0 | (cp >> 18)); b[n++] = (char)(0x80 | ((cp >> 12) & 0x3F));
        b[n++] = (char)(0x80 | ((cp >> 6) & 0x3F)); b[n++] = (char)(0x80 | (cp & 0x3F));
    }
    return n;
}

/* skips spaces and comments; returns 1 when a line break was crossed */
static int skip_space(ps_t *p) {
    int nl = 0;
    while (p->pos < p->len) {
        int c = (uint8_t)p->s[p->pos];
        if (c == '\n') { nl = 1; p->line++; p->pos++; continue; }
        if (c == ' ' || c == '\t' || c == '\r' || c == 0x0B || c == 0x0C) { p->pos++; continue; }
        if (c == 0xC2 && p->pos + 1 < p->len && (uint8_t)p->s[p->pos + 1] == 0xA0) { p->pos += 2; continue; }
        if (c == 0xEF && p->pos + 2 < p->len && (uint8_t)p->s[p->pos + 1] == 0xBB && (uint8_t)p->s[p->pos + 2] == 0xBF) {
            p->pos += 3; continue;
        }
        if (c == '/' && p->pos + 1 < p->len && p->s[p->pos + 1] == '/') {
            while (p->pos < p->len && p->s[p->pos] != '\n') p->pos++;
            continue;
        }
        if (c == '/' && p->pos + 1 < p->len && p->s[p->pos + 1] == '*') {
            p->pos += 2;
            while (p->pos + 1 < p->len && !(p->s[p->pos] == '*' && p->s[p->pos + 1] == '/')) {
                if (p->s[p->pos] == '\n') { nl = 1; p->line++; }
                p->pos++;
            }
            p->pos += 2;
            if (p->pos > p->len) p->pos = p->len;
            continue;
        }
        /* HTML-like comments a page script may start or end with */
        if (c == '<' && p->pos + 3 < p->len && p->s[p->pos + 1] == '!' && p->s[p->pos + 2] == '-' && p->s[p->pos + 3] == '-') {
            while (p->pos < p->len && p->s[p->pos] != '\n') p->pos++;
            continue;
        }
        if (c == '-' && (nl || p->pos == 0) && p->pos + 2 < p->len && p->s[p->pos + 1] == '-' && p->s[p->pos + 2] == '>') {
            while (p->pos < p->len && p->s[p->pos] != '\n') p->pos++;
            continue;
        }
        break;
    }
    return nl;
}

/* one escape after a backslash at p->pos (already past '\'); appends to buf */
static int read_escape(ps_t *p, char *b, int n, int max) {
    int c;
    if (p->pos >= p->len) return n;
    c = (uint8_t)p->s[p->pos++];
    switch (c) {
    case 'n': if (n < max) b[n++] = '\n'; break;
    case 't': if (n < max) b[n++] = '\t'; break;
    case 'r': if (n < max) b[n++] = '\r'; break;
    case 'b': if (n < max) b[n++] = '\b'; break;
    case 'f': if (n < max) b[n++] = '\f'; break;
    case 'v': if (n < max) b[n++] = '\v'; break;
    case '0':
        if (p->pos < p->len && is_digit(p->s[p->pos])) goto octal;
        if (n < max) b[n++] = 0;
        break;
    case '\r': if (p->pos < p->len && p->s[p->pos] == '\n') p->pos++; p->line++; break;
    case '\n': p->line++; break;
    case 'x': {
        int h1 = p->pos < p->len ? hexval(p->s[p->pos]) : -1, h2 = p->pos + 1 < p->len ? hexval(p->s[p->pos + 1]) : -1;
        if (h1 < 0 || h2 < 0) { ps_error(p, "Invalid hexadecimal escape sequence"); return n; }
        p->pos += 2;
        n = utf8_put(b, n, max, (uint32_t)(h1 * 16 + h2));
        break;
    }
    case 'u': {
        uint32_t cp = 0;
        int k;
        if (p->pos < p->len && p->s[p->pos] == '{') {
            p->pos++;
            while (p->pos < p->len && p->s[p->pos] != '}') {
                int h = hexval(p->s[p->pos++]);
                if (h < 0 || cp > 0x10FFFF) { ps_error(p, "Invalid Unicode escape sequence"); return n; }
                cp = cp * 16 + (uint32_t)h;
            }
            p->pos++;
        } else {
            for (k = 0; k < 4; k++) {
                int h = p->pos < p->len ? hexval(p->s[p->pos]) : -1;
                if (h < 0) { ps_error(p, "Invalid Unicode escape sequence"); return n; }
                cp = cp * 16 + (uint32_t)h;
                p->pos++;
            }
            /* a surrogate pair becomes one code point */
            if (cp >= 0xD800 && cp < 0xDC00 && p->pos + 5 < p->len && p->s[p->pos] == '\\' && p->s[p->pos + 1] == 'u') {
                uint32_t lo = 0;
                int ok = 1;
                for (k = 0; k < 4; k++) {
                    int h = hexval(p->s[p->pos + 2 + k]);
                    if (h < 0) { ok = 0; break; }
                    lo = lo * 16 + (uint32_t)h;
                }
                if (ok && lo >= 0xDC00 && lo < 0xE000) {
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    p->pos += 6;
                }
            }
        }
        n = utf8_put(b, n, max, cp);
        break;
    }
    default:
        if (c >= '1' && c <= '7') {
            int v;
        octal:
            v = c - '0';
            if (p->pos < p->len && p->s[p->pos] >= '0' && p->s[p->pos] <= '7') { v = v * 8 + (p->s[p->pos++] - '0'); }
            if (v < 32 && p->pos < p->len && p->s[p->pos] >= '0' && p->s[p->pos] <= '7') { v = v * 8 + (p->s[p->pos++] - '0'); }
            n = utf8_put(b, n, max, (uint32_t)v);
            break;
        }
        if (n < max) b[n++] = (char)c;
        break;
    }
    return n;
}

/* a growable decode: short strings use p->buf, longer ones the arena */
typedef struct { char *b; int n, cap; uint32_t big; } sbuf_t;

static void sb_init(ps_t *p, sbuf_t *sb) { sb->b = p->buf; sb->n = 0; sb->cap = (int)sizeof(p->buf); sb->big = 0; }

static int sb_room(ps_t *p, sbuf_t *sb, int need) {
    if (sb->n + need < sb->cap) return 1;
    {
        int cap = sb->cap * 2, i;
        uint32_t r;
        char *nb;
        while (cap < sb->n + need + 1) cap *= 2;
        if (cap > (1 << 20)) { ps_error(p, "string literal too long"); return 0; }
        r = js_alloc(p->vm, (uint32_t)cap);
        if (!r) { p->failed = 1; return 0; }
        nb = (char *)JPTR(p->vm, r);
        for (i = 0; i < sb->n; i++) nb[i] = sb->b[i];
        sb->b = nb;
        sb->cap = cap;
        sb->big = r;
        return 1;
    }
}

static void lex_string(ps_t *p, int q) {
    sbuf_t sb;
    sb_init(p, &sb);
    p->pos++;
    while (p->pos < p->len && p->s[p->pos] != q) {
        int c = (uint8_t)p->s[p->pos];
        if (!sb_room(p, &sb, 8)) return;
        if (c == '\n') { ps_error(p, "Invalid or unexpected token (unterminated string)"); return; }
        if (c == '\\') { p->pos++; sb.n = read_escape(p, sb.b, sb.n, sb.cap - 1); continue; }
        sb.b[sb.n++] = (char)c;
        p->pos++;
    }
    if (p->pos >= p->len) { ps_error(p, "Invalid or unexpected token (unterminated string)"); return; }
    p->pos++;
    p->t.type = TK_STR;
    p->t.str = js_str_new(p->vm, sb.b, sb.n);
}

/* template characters from p->pos up to '${' or '`' */
static void lex_tmpl_chars(ps_t *p) {
    sbuf_t sb;
    sb_init(p, &sb);
    p->t.type = TK_TMPL;
    p->t.tail = 0;
    while (p->pos < p->len) {
        int c = (uint8_t)p->s[p->pos];
        if (!sb_room(p, &sb, 8)) return;
        if (c == '`') { p->pos++; p->t.tail = 1; break; }
        if (c == '$' && p->pos + 1 < p->len && p->s[p->pos + 1] == '{') { p->pos += 2; break; }
        if (c == '\\') { p->pos++; sb.n = read_escape(p, sb.b, sb.n, sb.cap - 1); continue; }
        if (c == '\n') p->line++;
        if (c == '\r') { p->pos++; continue; }
        sb.b[sb.n++] = (char)c;
        p->pos++;
        if (p->pos >= p->len) { ps_error(p, "Unterminated template literal"); return; }
    }
    p->t.str = js_str_new(p->vm, sb.b, sb.n);
}

static void lex_number(ps_t *p) {
    int st = p->pos, used;
    char tmp[400];
    int n = 0;
    if (p->s[p->pos] == '0' && p->pos + 1 < p->len) {
        int x = p->s[p->pos + 1], base = 0;
        if (x == 'x' || x == 'X') base = 16;
        else if (x == 'o' || x == 'O') base = 8;
        else if (x == 'b' || x == 'B') base = 2;
        else if (is_digit(x)) {      /* legacy octal 017, or decimal 019 */
            int k = p->pos + 1, oct = 1;
            while (k < p->len && is_digit(p->s[k])) { if (p->s[k] > '7') oct = 0; k++; }
            if (oct) {
                jf64 acc = 0;
                for (k = p->pos + 1; k < p->len && is_digit(p->s[k]); k++)
                    acc = jf_add(jf_mul(acc, jf_from_i32(8)), jf_from_i32(p->s[k] - '0'));
                p->pos = k;
                p->t.type = TK_NUM;
                p->t.num = acc;
                return;
            }
        }
        if (base) {
            jf64 acc = 0, b = jf_from_i32(base);
            int any = 0;
            p->pos += 2;
            while (p->pos < p->len) {
                int c = p->s[p->pos], d;
                if (c == '_') { p->pos++; continue; }
                d = hexval(c);
                if (d < 0 || d >= base) break;
                acc = jf_add(jf_mul(acc, b), jf_from_i32(d));
                any = 1;
                p->pos++;
            }
            if (!any) { ps_error(p, "Invalid or unexpected token (number)"); return; }
            if (p->pos < p->len && p->s[p->pos] == 'n') { ps_error(p, "BigInt literals are not supported by the guest"); return; }
            p->t.type = TK_NUM;
            p->t.num = acc;
            return;
        }
    }
    while (p->pos < p->len && n < (int)sizeof(tmp) - 1) {
        int c = p->s[p->pos];
        if (c == '_') { p->pos++; continue; }
        if (is_digit(c) || c == '.') { tmp[n++] = (char)c; p->pos++; continue; }
        if ((c == 'e' || c == 'E') && p->pos + 1 < p->len &&
            (is_digit(p->s[p->pos + 1]) || ((p->s[p->pos + 1] == '+' || p->s[p->pos + 1] == '-') && p->pos + 2 < p->len && is_digit(p->s[p->pos + 2])))) {
            tmp[n++] = (char)c; p->pos++;
            tmp[n++] = p->s[p->pos++];
            continue;
        }
        break;
    }
    tmp[n] = 0;
    p->t.num = jf_parse(tmp, n, &used);
    if (used != n) {
        /* "1.toString" is not valid, "1..toString" is: give back an unread '.' */
        p->pos = st + used;
    }
    if (p->pos < p->len && p->s[p->pos] == 'n') { ps_error(p, "BigInt literals are not supported by the guest"); return; }
    if (p->pos < p->len && is_id_start((uint8_t)p->s[p->pos])) { ps_error(p, "Invalid or unexpected token (number)"); return; }
    p->t.type = TK_NUM;
}

static void lex_regex(ps_t *p) {
    int st = p->pos + 1, in_class = 0, fs;
    p->pos++;
    while (p->pos < p->len) {
        int c = p->s[p->pos];
        if (c == '\n') { ps_error(p, "Invalid regular expression: missing /"); return; }
        if (c == '\\') { p->pos += 2; continue; }
        if (c == '[') in_class = 1;
        else if (c == ']') in_class = 0;
        else if (c == '/' && !in_class) break;
        p->pos++;
    }
    if (p->pos >= p->len) { ps_error(p, "Invalid regular expression: missing /"); return; }
    p->t.type = TK_REGEX;
    p->t.str = js_str_new(p->vm, p->s + st, p->pos - st);
    p->pos++;
    fs = p->pos;
    while (p->pos < p->len && is_id_char((uint8_t)p->s[p->pos])) p->pos++;
    p->t.str2 = js_str_new(p->vm, p->s + fs, p->pos - fs);
}

static int kw_lookup(const char *s, int n) {
    int k, i;
    if (n < 2 || n > 10) return KW_NONE;
    for (k = 1; k_kw[k]; k++) {
        for (i = 0; i < n && k_kw[k][i] == s[i]; i++) {}
        if (i == n && k_kw[k][n] == 0) return k;
    }
    return KW_NONE;
}

static void next(ps_t *p) {
    int c;
    p->t.nl = skip_space(p);
    p->t.start = p->pos;
    p->t.line = p->line;
    p->t.op = OP_NONE;
    p->t.kw = KW_NONE;
    p->t.str = 0;
    if (p->failed) { p->t.type = TK_EOF; return; }
    if (p->pos >= p->len) { p->t.type = TK_EOF; p->t.end = p->pos; return; }
    c = (uint8_t)p->s[p->pos];
    if (is_id_start(c) || c == '\\') {
        int st = p->pos;
        if (c == '\\') { ps_error(p, "Unicode escapes in identifiers are not supported by the guest"); return; }
        while (p->pos < p->len && is_id_char((uint8_t)p->s[p->pos])) p->pos++;
        p->t.type = TK_IDENT;
        p->t.kw = kw_lookup(p->s + st, p->pos - st);
        p->t.str = js_intern(p->vm, p->s + st, p->pos - st);
        p->last_sig = p->t.kw != KW_NONE && p->t.kw != KW_THIS && p->t.kw != KW_SUPER && p->t.kw != KW_NULL &&
                      p->t.kw != KW_TRUE && p->t.kw != KW_FALSE && kw_reserved(p->t.kw);
    } else if (is_digit(c) || (c == '.' && p->pos + 1 < p->len && is_digit(p->s[p->pos + 1]))) {
        lex_number(p);
        p->last_sig = 0;
    } else if (c == '"' || c == '\'') {
        lex_string(p, c);
        p->last_sig = 0;
    } else if (c == '`') {
        p->pos++;
        lex_tmpl_chars(p);
        p->last_sig = 0;
    } else if (c == '/' && p->last_sig) {
        lex_regex(p);
        p->last_sig = 0;
    } else if (c == '#') {
        /* Lot 115: a private name #x, only inside a class body */
        int st = p->pos;
        if (p->pos + 1 >= p->len || !is_id_start((uint8_t)p->s[p->pos + 1]) || p->s[p->pos + 1] == '\\') {
            ps_error(p, "Invalid or unexpected token '#'");
            return;
        }
        p->pos++;
        while (p->pos < p->len && is_id_char((uint8_t)p->s[p->pos])) p->pos++;
        p->t.type = TK_PRIV;
        p->t.str = js_intern(p->vm, p->s + st, p->pos - st);
        p->last_sig = 0;
    } else {
        int k;
        for (k = 0; k_punct[k].s; k++) {
            const char *ps = k_punct[k].s;
            int i;
            for (i = 0; ps[i] && p->pos + i < p->len && p->s[p->pos + i] == ps[i]; i++) {}
            if (ps[i] == 0) {
                /* "?." followed by a digit is "?" then a number (a ? .5 : b) */
                if (k_punct[k].op == OP_QDOT && p->pos + 2 < p->len && is_digit(p->s[p->pos + 2])) continue;
                p->t.type = TK_OP;
                p->t.op = k_punct[k].op;
                p->pos += i;
                break;
            }
        }
        if (!k_punct[k].s) { ps_error(p, "Invalid or unexpected token"); return; }
        p->last_sig = !(p->t.op == OP_RPAREN || p->t.op == OP_RBRACK || p->t.op == OP_RBRACE ||
                        p->t.op == OP_INC || p->t.op == OP_DEC);
    }
    p->t.end = p->pos;
}

static void lex_save(ps_t *p, lexsave_t *s) { s->pos = p->pos; s->line = p->line; s->t = p->t; s->last_sig = p->last_sig; }
static void lex_restore(ps_t *p, const lexsave_t *s) { p->pos = s->pos; p->line = s->line; p->t = s->t; p->last_sig = s->last_sig; }

static int is_op(ps_t *p, int op) { return p->t.type == TK_OP && p->t.op == op; }
static int is_kw(ps_t *p, int kw) { return p->t.type == TK_IDENT && p->t.kw == kw; }

static int accept(ps_t *p, int op) {
    if (is_op(p, op)) { next(p); return 1; }
    return 0;
}

static void expect(ps_t *p, int op, const char *what) {
    if (!accept(p, op)) {
        char m[80];
        const char *pre = "Unexpected token, expected ";
        int n = 0, i;
        for (i = 0; pre[i]; i++) m[n++] = pre[i];
        for (i = 0; what[i] && n < 78; i++) m[n++] = what[i];
        m[n] = 0;
        ps_error(p, p->t.type == TK_EOF ? "Unexpected end of input" : m);
    }
}

static void semi(ps_t *p) {
    if (accept(p, OP_SEMI)) return;
    if (is_op(p, OP_RBRACE) || p->t.type == TK_EOF || p->t.nl) return;
    ps_error(p, "Unexpected token (missing ;)");
}

/* ---- nodes ---- */

uint32_t js_node_new(js_vm_t *vm, int k, uint32_t line) {
    uint32_t r = js_alloc(vm, sizeof(js_node_t));
    if (!r) return 0;
    JNODE(vm, r)->k = (uint8_t)k;
    JNODE(vm, r)->line = line;
    return r;
}

static uint32_t mk(ps_t *p, int k) {
    uint32_t r = js_node_new(p->vm, k, p->t.line);
    if (!r) p->failed = 1;
    return r;
}

static uint32_t mk2(ps_t *p, int k, int op, uint32_t a, uint32_t b) {
    uint32_t r = mk(p, k);
    if (!r) return 0;
    JNODE(p->vm, r)->op = (uint8_t)op;
    JNODE(p->vm, r)->a = a;
    JNODE(p->vm, r)->b = b;
    return r;
}

#define N(r) JNODE(p->vm, (r))

typedef struct { uint32_t head, tail; } list_t;
/* Lot 115: obj.#x, obj?.#x and #x in obj */
static uint32_t priv_ref(ps_t *p) {
    uint32_t n;
    if (p->cdepth == 0) { ps_error(p, "Private field must be declared in an enclosing class"); return 0; }
    if (p->npref >= PS_MAX_PRIV) { ps_error(p, "too many private name uses in one class for the guest"); return 0; }
    p->pref[p->npref] = p->t.str;
    p->pref_line[p->npref] = p->t.line;
    p->npref++;
    n = mk(p, N_PRIVNAME);
    if (n) N(n)->a = p->t.str;
    next(p);
    return n;
}

static void list_add(ps_t *p, list_t *l, uint32_t n) {
    if (!n) return;
    if (!l->head) l->head = n; else N(l->tail)->next = n;
    l->tail = n;
}

static void hoist_name(ps_t *p, uint32_t name) {
    uint32_t v = mk(p, N_VARNAME);
    if (!v) return;
    N(v)->a = name;
    if (!p->vhead) p->vhead = v; else N(p->vtail)->next = v;
    p->vtail = v;
}

/* every identifier bound by a pattern goes to the function's var list */
static void hoist_pattern(ps_t *p, uint32_t pat) {
    js_node_t *n;
    uint32_t e;
    if (!pat) return;
    n = N(pat);
    switch (n->k) {
    case N_IDENT: hoist_name(p, n->a); break;
    case N_OPAT: case N_APAT:
        for (e = n->a; e; e = N(e)->next) hoist_pattern(p, N(e)->b);
        break;
    default: break;
    }
}

static uint32_t parse_assign(ps_t *p, int no_in);
static uint32_t parse_expr(ps_t *p, int no_in);
static uint32_t parse_statement(ps_t *p);
static uint32_t parse_function(ps_t *p, int is_decl, int is_method, int is_arrow);
static uint32_t parse_function_k(ps_t *p, int is_decl, int is_method, int kind);
static uint32_t parse_class(ps_t *p, int is_decl);
static uint32_t parse_binding(ps_t *p);
static uint32_t parse_unary(ps_t *p);

static int ident_ok(ps_t *p) { return p->t.type == TK_IDENT && !kw_reserved(p->t.kw); }

static uint32_t parse_ident(ps_t *p) {
    uint32_t n;
    if (!ident_ok(p)) { ps_error(p, "Unexpected token (identifier expected)"); return 0; }
    n = mk(p, N_IDENT);
    if (n) N(n)->a = p->t.str;
    if (p->t.str == p->vm->s_arguments) p->fn_flags |= F_ARGS;
    next(p);
    return n;
}

/* a property name: identifier (keywords too), string, number or [expr] */
static uint32_t parse_prop_key(ps_t *p, int *computed) {
    uint32_t k = 0;
    *computed = 0;
    if (p->t.type == TK_IDENT || p->t.type == TK_STR) {
        k = js_intern_str(p->vm, p->t.str);
        next(p);
    } else if (p->t.type == TK_NUM) {
        k = js_intern_str(p->vm, js_num_to_str(p->vm, p->t.num));
        next(p);
    } else if (accept(p, OP_LBRACK)) {
        k = parse_assign(p, 0);
        expect(p, OP_RBRACK, "]");
        *computed = 1;
    } else {
        ps_error(p, "Unexpected token (property name expected)");
    }
    return k;
}

/* an array or object literal reread as a destructuring pattern */
static uint32_t to_pattern(ps_t *p, uint32_t e) {
    js_node_t *n;
    uint32_t it;
    if (!e || p->failed) return e;
    n = N(e);
    switch (n->k) {
    case N_IDENT: case N_MEMBER: case N_INDEX: return e;
    case N_ASSIGN:
        if (n->op != OP_ASSIGN) break;
        {   /* a = default inside a pattern: PELEM with default */
            uint32_t el = mk(p, N_PELEM);
            if (!el) return 0;
            N(el)->b = to_pattern(p, n->a);
            N(el)->c = n->b;
            return el;
        }
    case N_ARRAY: {
        list_t l = { 0, 0 };
        for (it = n->a; it; it = N(it)->next) {
            uint32_t pe = mk(p, N_PELEM), x = it;
            if (!pe) return 0;
            if (N(x)->k == N_ELISION) { list_add(p, &l, pe); continue; }
            if (N(x)->k == N_SPREAD) { N(pe)->fl = F_REST; N(pe)->b = to_pattern(p, N(x)->a); list_add(p, &l, pe); continue; }
            if (N(x)->k == N_ASSIGN && N(x)->op == OP_ASSIGN) { N(pe)->c = N(x)->b; x = N(x)->a; }
            N(pe)->b = to_pattern(p, x);
            list_add(p, &l, pe);
        }
        n = N(e);
        n->k = N_APAT;
        n->a = l.head;
        return e;
    }
    case N_OBJECT:
        n->k = N_OPAT;
        for (it = n->a; it; it = N(it)->next) {
            js_node_t *x = N(it);
            if (x->k == N_SPREAD) {
                x->k = N_PPROP;
                x->b = to_pattern(p, x->a);
                x->a = 0;
                x->fl = F_REST;
                continue;
            }
            x->k = N_PPROP;
            {
                uint32_t v = x->b, def = x->c;
                if (v && N(v)->k == N_ASSIGN && N(v)->op == OP_ASSIGN) { def = N(v)->b; v = N(v)->a; }
                x = N(it);
                x->b = to_pattern(p, v);
                x->c = def;
            }
        }
        return e;
    case N_PELEM: case N_APAT: case N_OPAT: return e;
    default: break;
    }
    ps_error(p, "Invalid destructuring assignment target");
    return e;
}

static uint32_t parse_template(ps_t *p) {
    uint32_t t = mk(p, N_TMPL);
    list_t parts = { 0, 0 };
    if (!t) return 0;
    for (;;) {
        uint32_t s = mk(p, N_STR);
        int tail = p->t.tail;
        if (!s) return 0;
        N(s)->a = p->t.str;
        list_add(p, &parts, s);
        if (tail) { next(p); break; }
        next(p);
        list_add(p, &parts, parse_expr(p, 0));
        if (p->failed) return 0;
        if (!is_op(p, OP_RBRACE)) { ps_error(p, "Unexpected token in template literal (expected })"); return 0; }
        lex_tmpl_chars(p);
        if (p->failed) return 0;
    }
    N(t)->a = parts.head;
    return t;
}

static uint32_t parse_array(ps_t *p) {
    uint32_t a = mk(p, N_ARRAY);
    list_t l = { 0, 0 };
    next(p);
    while (!is_op(p, OP_RBRACK) && !p->failed) {
        if (is_op(p, OP_COMMA)) {
            next(p);
            list_add(p, &l, mk(p, N_ELISION));
            continue;
        }
        if (is_op(p, OP_ELLIPSIS)) {
            uint32_t s = mk(p, N_SPREAD);
            next(p);
            if (s) N(s)->a = parse_assign(p, 0);
            list_add(p, &l, s);
        } else {
            list_add(p, &l, parse_assign(p, 0));
        }
        if (!is_op(p, OP_RBRACK)) expect(p, OP_COMMA, ",");
    }
    expect(p, OP_RBRACK, "]");
    if (a) N(a)->a = l.head;
    return a;
}

static uint32_t parse_object(ps_t *p) {
    uint32_t o = mk(p, N_OBJECT);
    list_t l = { 0, 0 };
    next(p);
    while (!is_op(p, OP_RBRACE) && !p->failed) {
        uint32_t pr;
        int computed = 0;
        if (is_op(p, OP_ELLIPSIS)) {
            pr = mk(p, N_SPREAD);
            next(p);
            if (pr) N(pr)->a = parse_assign(p, 0);
            list_add(p, &l, pr);
            if (!is_op(p, OP_RBRACE)) expect(p, OP_COMMA, ",");
            continue;
        }
        pr = mk(p, N_PROP);
        if (!pr) return 0;
        if (is_op(p, OP_MUL)) {     /* Lot 114: *name() {} */
            next(p);
            N(pr)->a = parse_prop_key(p, &computed);
            if (computed) N(pr)->fl |= F_COMPUTED;
            N(pr)->fl |= F_METHOD;
            N(pr)->b = parse_function_k(p, 0, 1, FK_GEN);
            list_add(p, &l, pr);
            if (!is_op(p, OP_RBRACE)) expect(p, OP_COMMA, ",");
            continue;
        }
        if ((is_kw(p, KW_GET) || is_kw(p, KW_SET) || is_kw(p, KW_ASYNC))) {
            lexsave_t sv;
            int kw = p->t.kw;
            lex_save(p, &sv);
            next(p);
            if (!is_op(p, OP_COLON) && !is_op(p, OP_LPAREN) && !is_op(p, OP_COMMA) && !is_op(p, OP_RBRACE) &&
                !is_op(p, OP_ASSIGN)) {
                if (kw == KW_ASYNC) {   /* Lot 114: async name() {} */
                    if (p->t.nl) { ps_error(p, "Unexpected token in object literal"); return 0; }
                    if (is_op(p, OP_MUL)) { ps_error(p, "async generators are not supported by the guest"); return 0; }
                    N(pr)->a = parse_prop_key(p, &computed);
                    if (computed) N(pr)->fl |= F_COMPUTED;
                    N(pr)->fl |= F_METHOD;
                    N(pr)->b = parse_function_k(p, 0, 1, FK_ASYNC);
                    list_add(p, &l, pr);
                    if (!is_op(p, OP_RBRACE)) expect(p, OP_COMMA, ",");
                    continue;
                }
                N(pr)->a = parse_prop_key(p, &computed);
                if (computed) N(pr)->fl |= F_COMPUTED;
                N(pr)->fl |= kw == KW_GET ? F_GETTER : F_SETTER;
                N(pr)->b = parse_function(p, 0, 1, 0);
                list_add(p, &l, pr);
                if (!is_op(p, OP_RBRACE)) expect(p, OP_COMMA, ",");
                continue;
            }
            lex_restore(p, &sv);
        }
        {
            int was_ident = ident_ok(p) || p->t.type == TK_IDENT;
            uint32_t name_str = p->t.type == TK_IDENT ? p->t.str : 0;
            int was_reserved = p->t.type == TK_IDENT && kw_reserved(p->t.kw);
            N(pr)->a = parse_prop_key(p, &computed);
            if (computed) N(pr)->fl |= F_COMPUTED;
            if (is_op(p, OP_LPAREN)) {
                N(pr)->fl |= F_METHOD;
                N(pr)->b = parse_function(p, 0, 1, 0);
            } else if (accept(p, OP_COLON)) {
                N(pr)->b = parse_assign(p, 0);
            } else if (was_ident && !computed && !was_reserved) {
                uint32_t id = mk(p, N_IDENT);
                if (!id) return 0;
                N(id)->a = name_str;
                if (name_str == p->vm->s_arguments) p->fn_flags |= F_ARGS;
                N(pr)->fl |= F_SHORT;
                if (accept(p, OP_ASSIGN)) {         /* only valid once read as a pattern */
                    uint32_t as = mk2(p, N_ASSIGN, OP_ASSIGN, id, parse_assign(p, 0));
                    N(pr)->b = as;
                } else {
                    N(pr)->b = id;
                }
            } else {
                ps_error(p, "Unexpected token in object literal");
                return 0;
            }
        }
        list_add(p, &l, pr);
        if (!is_op(p, OP_RBRACE)) expect(p, OP_COMMA, ",");
    }
    expect(p, OP_RBRACE, "}");
    if (o) N(o)->a = l.head;
    return o;
}

/* is the '(' just read the start of an arrow function's parameters? */
static int arrow_ahead(ps_t *p) {
    int i = p->t.end, depth = 1;
    while (i < p->len && depth > 0) {
        char c = p->s[i];
        if (c == '(' || c == '[' || c == '{') depth++;
        else if (c == ')' || c == ']' || c == '}') depth--;
        else if (c == '"' || c == '\'' || c == '`') {
            char q = c;
            i++;
            while (i < p->len && p->s[i] != q) { if (p->s[i] == '\\') i++; i++; }
        } else if (c == '/' && i + 1 < p->len && p->s[i + 1] == '/') {
            while (i < p->len && p->s[i] != '\n') i++;
        } else if (c == '/' && i + 1 < p->len && p->s[i + 1] == '*') {
            i += 2;
            while (i + 1 < p->len && !(p->s[i] == '*' && p->s[i + 1] == '/')) i++;
            i++;
        }
        i++;
    }
    if (depth) return 0;
    while (i < p->len && (p->s[i] == ' ' || p->s[i] == '\t' || p->s[i] == '\r')) i++;
    return i + 1 < p->len && p->s[i] == '=' && p->s[i + 1] == '>';
}

static uint32_t parse_primary(ps_t *p) {
    uint32_t n = 0;
    js_vm_t *vm = p->vm;
    switch (p->t.type) {
    case TK_NUM:
        n = mk(p, N_NUM);
        if (n) { N(n)->a = (uint32_t)p->t.num; N(n)->b = (uint32_t)(p->t.num >> 32); }
        next(p);
        return n;
    case TK_STR:
        n = mk(p, N_STR);
        if (n) N(n)->a = p->t.str;
        next(p);
        return n;
    case TK_TMPL:
        return parse_template(p);
    case TK_PRIV: {                     /* Lot 115: #x in obj (the brand check) */
        lexsave_t sv;
        lex_save(p, &sv);
        next(p);
        if (!is_kw(p, KW_IN)) { ps_error(p, "Unexpected private name (only #x in obj stands alone)"); return 0; }
        lex_restore(p, &sv);
        return priv_ref(p);
    }
    case TK_REGEX:
        n = mk(p, N_REGEX);
        if (n) { N(n)->a = p->t.str; N(n)->b = p->t.str2; }
        next(p);
        return n;
    case TK_IDENT:
        switch (p->t.kw) {
        case KW_THIS: n = mk(p, N_THIS); next(p); return n;
        case KW_NULL: n = mk(p, N_NULL); next(p); return n;
        case KW_TRUE: n = mk(p, N_TRUE); next(p); return n;
        case KW_FALSE: n = mk(p, N_FALSE); next(p); return n;
        case KW_FUNCTION: return parse_function(p, 0, 0, 0);
        case KW_CLASS: return parse_class(p, 0);
        case KW_SUPER:
            n = mk(p, N_SUPER);
            next(p);
            if (!is_op(p, OP_LPAREN) && !is_op(p, OP_DOT) && !is_op(p, OP_LBRACK)) { ps_error(p, "'super' keyword unexpected here"); return 0; }
            return n;
        case KW_ASYNC: {
            lexsave_t sv;
            lex_save(p, &sv);
            next(p);
            if (is_kw(p, KW_FUNCTION) && !p->t.nl) return parse_function_k(p, 0, 0, FK_ASYNC);
            lex_restore(p, &sv);
            return parse_ident(p);
        }
        default:
            if (kw_reserved(p->t.kw)) { ps_error(p, "Unexpected token (reserved word)"); return 0; }
            if (p->t.str == vm->s_undefined) {
                n = mk(p, N_IDENT);
                if (n) N(n)->a = p->t.str;
                next(p);
                return n;
            }
            return parse_ident(p);
        }
    case TK_OP:
        if (p->t.op == OP_LPAREN) {
            next(p);
            n = parse_expr(p, 0);
            expect(p, OP_RPAREN, ")");
            return n;
        }
        if (p->t.op == OP_LBRACK) return parse_array(p);
        if (p->t.op == OP_LBRACE) return parse_object(p);
        break;
    default: break;
    }
    ps_error(p, p->t.type == TK_EOF ? "Unexpected end of input" : "Unexpected token");
    return 0;
}

static uint32_t parse_args(ps_t *p) {
    list_t l = { 0, 0 };
    next(p);       /* ( */
    while (!is_op(p, OP_RPAREN) && !p->failed) {
        if (is_op(p, OP_ELLIPSIS)) {
            uint32_t s = mk(p, N_SPREAD);
            next(p);
            if (s) N(s)->a = parse_assign(p, 0);
            list_add(p, &l, s);
        } else {
            list_add(p, &l, parse_assign(p, 0));
        }
        if (!is_op(p, OP_RPAREN)) expect(p, OP_COMMA, ",");
    }
    expect(p, OP_RPAREN, ")");
    return l.head;
}

/* member and call chains; no_call for the callee of `new` */
static uint32_t parse_chain(ps_t *p, uint32_t e, int no_call) {
    int chain = 0;
    for (;;) {
        if (p->failed) return 0;
        if (is_op(p, OP_DOT)) {
            uint32_t m;
            next(p);
            if (p->t.type == TK_PRIV) {         /* Lot 115: obj.#x */
                uint32_t pn = priv_ref(p);
                if (!pn) return 0;
                e = mk2(p, N_INDEX, 0, e, pn);
                continue;
            }
            if (p->t.type != TK_IDENT) { ps_error(p, "Unexpected token after . (name expected)"); return 0; }
            m = mk2(p, N_MEMBER, 0, e, js_intern_str(p->vm, p->t.str));
            next(p);
            e = m;
        } else if (is_op(p, OP_QDOT)) {
            if (no_call) { ps_error(p, "Invalid optional chain from new expression"); return 0; }
            next(p);
            chain = 1;
            if (is_op(p, OP_LPAREN)) {
                uint32_t c = mk(p, N_CALL);
                if (!c) return 0;
                N(c)->a = e;
                N(c)->b = parse_args(p);
                N(c)->fl |= F_OPT;
                e = c;
            } else if (accept(p, OP_LBRACK)) {
                uint32_t ix = parse_expr(p, 0);
                expect(p, OP_RBRACK, "]");
                e = mk2(p, N_INDEX, 0, e, ix);
                if (e) N(e)->fl |= F_OPT;
            } else if (p->t.type == TK_IDENT) {
                e = mk2(p, N_MEMBER, 0, e, js_intern_str(p->vm, p->t.str));
                if (e) N(e)->fl |= F_OPT;
                next(p);
            } else if (p->t.type == TK_PRIV) {   /* Lot 115: obj?.#x */
                uint32_t pn = priv_ref(p);
                if (!pn) return 0;
                e = mk2(p, N_INDEX, 0, e, pn);
                if (e) N(e)->fl |= F_OPT;
            } else { ps_error(p, "Unexpected token after ?."); return 0; }
        } else if (is_op(p, OP_LBRACK)) {
            uint32_t ix;
            next(p);
            ix = parse_expr(p, 0);
            expect(p, OP_RBRACK, "]");
            e = mk2(p, N_INDEX, 0, e, ix);
        } else if (is_op(p, OP_LPAREN) && !no_call) {
            uint32_t c = mk(p, N_CALL);
            if (!c) return 0;
            N(c)->a = e;
            N(c)->b = parse_args(p);
            e = c;
        } else if (p->t.type == TK_TMPL) {
            uint32_t tg = mk(p, N_TTAG);
            if (chain) { ps_error(p, "Invalid tagged template on optional chain"); return 0; }
            if (!tg) return 0;
            N(tg)->a = e;
            N(tg)->b = parse_template(p);
            e = tg;
        } else {
            break;
        }
    }
    if (chain && e) N(e)->fl |= F_CHAIN;
    return e;
}

static uint32_t parse_new(ps_t *p) {
    uint32_t n = mk(p, N_NEW), callee;
    next(p);      /* new */
    if (is_op(p, OP_DOT)) {
        next(p);
        if (p->t.type != TK_IDENT || p->t.str != js_intern(p->vm, "target", 6)) { ps_error(p, "new. must be followed by target"); return 0; }
        next(p);
        if (n) N(n)->k = N_NEWTARGET;
        return n;
    }
    if (is_kw(p, KW_NEW)) callee = parse_new(p);
    else callee = parse_primary(p);
    callee = parse_chain(p, callee, 1);
    if (!n) return 0;
    N(n)->a = callee;
    if (is_op(p, OP_LPAREN)) N(n)->b = parse_args(p);
    return n;
}

static uint32_t parse_lhs(ps_t *p) {
    uint32_t e;
    if (is_kw(p, KW_NEW)) e = parse_new(p);
    else e = parse_primary(p);
    return parse_chain(p, e, 0);
}

static uint32_t parse_postfix(ps_t *p) {
    uint32_t e = parse_lhs(p);
    if ((is_op(p, OP_INC) || is_op(p, OP_DEC)) && !p->t.nl) {
        int op = p->t.op;
        if (!e || (N(e)->k != N_IDENT && N(e)->k != N_MEMBER && N(e)->k != N_INDEX)) {
            ps_error(p, "Invalid left-hand side expression in postfix operation");
            return 0;
        }
        next(p);
        e = mk2(p, N_UPDATE, op, e, 0);
    }
    return e;
}

static uint32_t parse_unary(ps_t *p) {
    int op = OP_NONE;
    if (p->t.type == TK_OP) {
        switch (p->t.op) {
        case OP_NOT: case OP_BNOT: op = p->t.op; break;
        case OP_SUB: op = OP_NEG; break;
        case OP_ADD: op = OP_PLUS; break;
        case OP_INC: case OP_DEC: {
            uint32_t e;
            op = p->t.op;
            next(p);
            e = parse_unary(p);
            if (!e || (N(e)->k != N_IDENT && N(e)->k != N_MEMBER && N(e)->k != N_INDEX)) {
                ps_error(p, "Invalid left-hand side expression in prefix operation");
                return 0;
            }
            e = mk2(p, N_UPDATE, op, e, 0);
            if (e) N(e)->fl |= F_PREFIX;
            return e;
        }
        default: break;
        }
    } else if (p->t.type == TK_IDENT) {
        if (p->t.kw == KW_TYPEOF) op = OP_TYPEOF;
        else if (p->t.kw == KW_VOID) op = OP_VOID;
        else if (p->t.kw == KW_DELETE) op = OP_DELETE;
        else if (p->t.kw == KW_AWAIT && p->in_async) {
            uint32_t e;
            if (++p->nest > PS_MAX_NEST || stack_low(p)) { ps_error(p, "expression nested too deeply for the guest"); return 0; }
            next(p);
            e = parse_unary(p);
            p->nest--;
            return mk2(p, N_AWAIT, 0, e, 0);
        }
        else if (p->t.kw == KW_AWAIT && p->in_func) { ps_error(p, "await is only valid in async functions"); return 0; }
    }
    if (op != OP_NONE) {
        uint32_t e;
        if (++p->nest > PS_MAX_NEST || stack_low(p)) { ps_error(p, "expression nested too deeply for the guest"); return 0; }
        next(p);
        e = parse_unary(p);
        p->nest--;
        if (op == OP_DELETE && e && N(e)->k == N_INDEX && N(e)->b && N(N(e)->b)->k == N_PRIVNAME) {
            ps_error(p, "Private fields can not be deleted");
            return 0;
        }
        if (op != OP_TYPEOF && op != OP_DELETE && op != OP_VOID && is_op(p, OP_EXP)) {
            ps_error(p, "Unary operator used immediately before exponentiation expression");
            return 0;
        }
        return mk2(p, N_UNARY, op, e, 0);
    }
    return parse_postfix(p);
}

static int bin_prec(ps_t *p, int no_in, int *op) {
    if (p->t.type == TK_IDENT) {
        if (p->t.kw == KW_INSTANCEOF) { *op = OP_INSTANCEOF; return 8; }
        if (p->t.kw == KW_IN && !no_in) { *op = OP_IN; return 8; }
        return 0;
    }
    if (p->t.type != TK_OP) return 0;
    *op = p->t.op;
    switch (p->t.op) {
    case OP_NULLISH: return 1;
    case OP_OR: return 2;
    case OP_AND: return 3;
    case OP_BOR: return 4;
    case OP_BXOR: return 5;
    case OP_BAND: return 6;
    case OP_EQ: case OP_NE: case OP_SEQ: case OP_SNE: return 7;
    case OP_LT: case OP_GT: case OP_LE: case OP_GE: return 8;
    case OP_SHL: case OP_SAR: case OP_SHR: return 9;
    case OP_ADD: case OP_SUB: return 10;
    case OP_MUL: case OP_DIV: case OP_MOD: return 11;
    case OP_EXP: return 12;
    default: return 0;
    }
}

static uint32_t parse_binary(ps_t *p, int min_prec, int no_in) {
    uint32_t left = parse_unary(p);
    for (;;) {
        int op = 0, prec = bin_prec(p, no_in, &op), k;
        uint32_t right;
        if (!prec || prec < min_prec || p->failed) break;
        next(p);
        if (++p->nest > PS_MAX_NEST || stack_low(p)) { ps_error(p, "expression nested too deeply for the guest"); return 0; }
        right = parse_binary(p, op == OP_EXP ? prec : prec + 1, no_in);   /* ** is right-associative */
        p->nest--;
        k = (op == OP_AND || op == OP_OR || op == OP_NULLISH) ? N_LOGICAL : N_BINARY;
        left = mk2(p, k, op, left, right);
    }
    return left;
}

static uint32_t parse_cond(ps_t *p, int no_in) {
    uint32_t c = parse_binary(p, 1, no_in);
    if (is_op(p, OP_QUESTION)) {
        uint32_t n = mk(p, N_COND);
        next(p);
        if (!n) return 0;
        N(n)->a = c;
        N(n)->b = parse_assign(p, 0);
        expect(p, OP_COLON, ":");
        N(n)->c = parse_assign(p, no_in);
        return n;
    }
    return c;
}

static int is_assign_op(int op) { return op >= OP_ASSIGN && op <= OP_NULLISH_A; }

static uint32_t parse_arrow_body(ps_t *p, uint32_t fn) {
    uint32_t saved_vh = p->vhead, saved_vt = p->vtail;
    int saved_flags = p->fn_flags, saved_in = p->in_func, saved_gen = p->in_gen, saved_async = p->in_async;
    /* an arrow has no own `arguments`: F_ARGS goes to the enclosing function */
    p->vhead = p->vtail = 0;
    p->in_func = 1;
    p->in_gen = 0;
    p->in_async = (N(fn)->op & FK_ASYNC) != 0;
    if (is_op(p, OP_LBRACE)) {
        list_t body = { 0, 0 };
        next(p);
        while (!is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->failed) list_add(p, &body, parse_statement(p));
        expect(p, OP_RBRACE, "}");
        N(fn)->c = body.head;
    } else {
        N(fn)->c = parse_assign(p, 0);
        N(fn)->fl |= F_EXPRBODY;
    }
    N(fn)->d = p->vhead;
    p->vhead = saved_vh;
    p->vtail = saved_vt;
    p->fn_flags |= saved_flags;
    p->in_func = saved_in;
    p->in_gen = saved_gen;
    p->in_async = saved_async;
    return fn;
}

static uint32_t parse_params(ps_t *p) {
    list_t l = { 0, 0 };
    expect(p, OP_LPAREN, "(");
    while (!is_op(p, OP_RPAREN) && !p->failed) {
        uint32_t pr = mk(p, N_PARAM);
        if (!pr) return 0;
        if (accept(p, OP_ELLIPSIS)) N(pr)->fl |= F_REST;
        N(pr)->a = parse_binding(p);
        if (accept(p, OP_ASSIGN)) N(pr)->b = parse_assign(p, 0);
        list_add(p, &l, pr);
        if (N(pr)->fl & F_REST) break;
        if (!is_op(p, OP_RPAREN)) expect(p, OP_COMMA, ",");
    }
    expect(p, OP_RPAREN, ")");
    return l.head;
}

static uint32_t parse_assign(ps_t *p, int no_in) {
    uint32_t left;
    if (p->failed) return 0;
    if (++p->nest > PS_MAX_NEST || stack_low(p)) { ps_error(p, "expression nested too deeply for the guest"); return 0; }
    /* Lot 114: yield [*] [expr] inside a generator */
    if (p->in_gen && is_kw(p, KW_YIELD)) {
        uint32_t y = mk(p, N_YIELD);
        if (!y) return 0;
        next(p);
        if (accept(p, OP_MUL)) {
            N(y)->op = 1;
            N(y)->a = parse_assign(p, no_in);
        } else if (!p->t.nl && p->t.type != TK_EOF &&
                   !(p->t.type == TK_OP && (p->t.op == OP_RPAREN || p->t.op == OP_RBRACK || p->t.op == OP_RBRACE ||
                                            p->t.op == OP_COMMA || p->t.op == OP_SEMI || p->t.op == OP_COLON)) &&
                   !(p->t.type == TK_IDENT && p->t.kw == KW_IN)) {
            N(y)->a = parse_assign(p, no_in);
        }
        p->nest--;
        return y;
    }
    /* Lot 114: async x => ..., async (a, b) => ... */
    if (is_kw(p, KW_ASYNC)) {
        lexsave_t sv;
        lex_save(p, &sv);
        next(p);
        if (!p->t.nl && ident_ok(p)) {
            uint32_t pname = p->t.str;
            next(p);
            if (is_op(p, OP_ARROW) && !p->t.nl) {
                uint32_t fn = mk(p, N_FUNC), pr = mk(p, N_PARAM), id = mk(p, N_IDENT);
                if (!fn || !pr || !id) return 0;
                N(id)->a = pname;
                N(pr)->a = id;
                N(fn)->b = pr;
                N(fn)->fl |= F_ARROW;
                N(fn)->op = FK_ASYNC;
                next(p);
                parse_arrow_body(p, fn);
                p->nest--;
                return fn;
            }
        } else if (!p->t.nl && is_op(p, OP_LPAREN) && arrow_ahead(p)) {
            uint32_t fn = mk(p, N_FUNC);
            int sv_async = p->in_async;
            if (!fn) return 0;
            N(fn)->fl |= F_ARROW;
            N(fn)->op = FK_ASYNC;
            {
                uint32_t sv_vh = p->vhead, sv_vt = p->vtail;
                p->in_async = 1;
                N(fn)->b = parse_params(p);
                p->in_async = sv_async;
                p->vhead = sv_vh; p->vtail = sv_vt;
            }
            if (!is_op(p, OP_ARROW) || p->t.nl) { ps_error(p, "Unexpected token (expected =>)"); return 0; }
            next(p);
            parse_arrow_body(p, fn);
            p->nest--;
            return fn;
        }
        lex_restore(p, &sv);
    }
    /* arrow functions */
    if (ident_ok(p) && p->t.kw != KW_ASYNC && !(p->in_gen && p->t.kw == KW_YIELD)) {
        lexsave_t sv;
        lex_save(p, &sv);
        next(p);
        if (is_op(p, OP_ARROW) && !p->t.nl) {
            uint32_t fn = mk(p, N_FUNC), pr = mk(p, N_PARAM), id = mk(p, N_IDENT);
            if (!fn || !pr || !id) return 0;
            N(id)->a = sv.t.str;
            N(pr)->a = id;
            N(fn)->b = pr;
            N(fn)->fl |= F_ARROW;
            next(p);
            parse_arrow_body(p, fn);
            p->nest--;
            return fn;
        }
        lex_restore(p, &sv);
    } else if (is_op(p, OP_LPAREN) && arrow_ahead(p)) {
        uint32_t fn = mk(p, N_FUNC);
        if (!fn) return 0;
        N(fn)->fl |= F_ARROW;
        {
            uint32_t sv_vh = p->vhead, sv_vt = p->vtail;
            N(fn)->b = parse_params(p);
            p->vhead = sv_vh; p->vtail = sv_vt;
        }
        if (!is_op(p, OP_ARROW) || p->t.nl) { ps_error(p, "Unexpected token (expected =>)"); return 0; }
        next(p);
        parse_arrow_body(p, fn);
        p->nest--;
        return fn;
    }
    left = parse_cond(p, no_in);
    if (p->t.type == TK_OP && is_assign_op(p->t.op)) {
        int op = p->t.op;
        uint32_t right;
        if (!left) return 0;
        if (op == OP_ASSIGN && (N(left)->k == N_ARRAY || N(left)->k == N_OBJECT)) left = to_pattern(p, left);
        else if (N(left)->k != N_IDENT && N(left)->k != N_MEMBER && N(left)->k != N_INDEX) {
            ps_error(p, "Invalid left-hand side in assignment");
            return 0;
        }
        next(p);
        right = parse_assign(p, no_in);
        left = mk2(p, N_ASSIGN, op, left, right);
    }
    p->nest--;
    return left;
}

static uint32_t parse_expr(ps_t *p, int no_in) {
    uint32_t e = parse_assign(p, no_in);
    while (is_op(p, OP_COMMA) && !p->failed) {
        next(p);
        e = mk2(p, N_SEQ, 0, e, parse_assign(p, no_in));
    }
    return e;
}

/* a binding target: identifier, [pattern] or {pattern} */
static uint32_t parse_binding(ps_t *p) {
    if (is_op(p, OP_LBRACK) || is_op(p, OP_LBRACE)) {
        uint32_t e = is_op(p, OP_LBRACK) ? parse_array(p) : parse_object(p);
        return to_pattern(p, e);
    }
    return parse_ident(p);
}

static uint32_t parse_function(ps_t *p, int is_decl, int is_method, int is_arrow) {
    (void)is_arrow;
    return parse_function_k(p, is_decl, is_method, 0);
}

/* kind: FK_GEN (function*), FK_ASYNC (async function), 0 */
static uint32_t parse_function_k(ps_t *p, int is_decl, int is_method, int kind) {
    uint32_t fn = mk(p, N_FUNC), saved_vh = p->vhead, saved_vt = p->vtail;
    int saved_flags = p->fn_flags, saved_in = p->in_func, saved_gen = p->in_gen, saved_async = p->in_async;
    list_t body = { 0, 0 };
    if (!fn) return 0;
    if (!is_method) {
        next(p);      /* function */
        if (accept(p, OP_MUL)) kind |= FK_GEN;
        if (ident_ok(p) || (p->t.type == TK_IDENT && (p->t.kw == KW_YIELD || p->t.kw == KW_AWAIT))) {
            N(fn)->a = p->t.str;
            next(p);
        } else if (is_decl) {
            ps_error(p, "Function statements require a function name");
            return 0;
        }
    } else {
        N(fn)->fl |= F_METHOD;
    }
    if ((kind & FK_GEN) && (kind & FK_ASYNC)) { ps_error(p, "async generators are not supported by the guest"); return 0; }
    N(fn)->op = (uint8_t)kind;
    p->vhead = p->vtail = 0;
    p->fn_flags = 0;
    p->in_func = 1;
    p->in_gen = (kind & FK_GEN) != 0;
    p->in_async = (kind & FK_ASYNC) != 0;
    N(fn)->b = parse_params(p);
    expect(p, OP_LBRACE, "{");
    while (!is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->failed) list_add(p, &body, parse_statement(p));
    expect(p, OP_RBRACE, "}");
    N(fn)->c = body.head;
    N(fn)->d = p->vhead;
    N(fn)->fl |= (uint16_t)(p->fn_flags & F_ARGS);
    p->vhead = saved_vh;
    p->vtail = saved_vt;
    p->fn_flags = saved_flags;
    p->in_func = saved_in;
    p->in_gen = saved_gen;
    p->in_async = saved_async;
    if (is_decl) {
        uint32_t d = mk(p, N_FUNCDECL);
        if (!d) return 0;
        N(d)->a = fn;
        hoist_name(p, N(fn)->a);
        return d;
    }
    return fn;
}

/* Lot 115: a private name declared by the class body being parsed; a
 * getter and a setter may share a name, nothing else may repeat */
static void priv_declare(ps_t *p, uint32_t name, int kind, int base) {
    int i;
    if (name == js_intern(p->vm, "#constructor", 12)) { ps_error(p, "Classes may not have a private field named '#constructor'"); return; }
    for (i = base; i < p->npdecl; i++) {
        if (p->pdecl[i] != name) continue;
        if ((p->pkind[i] & 3) != 3 && (kind & 3) != 3 && (p->pkind[i] & 3) != (kind & 3) &&
            (p->pkind[i] & 4) == (kind & 4)) {
            p->pkind[i] = (uint8_t)(p->pkind[i] | (kind & 3));
            return;
        }
        ps_error(p, "Identifier has already been declared (private name)");
        return;
    }
    if (p->npdecl >= PS_MAX_PRIV) { ps_error(p, "too many private names for the guest"); return; }
    p->pdecl[p->npdecl] = name;
    p->pkind[p->npdecl] = (uint8_t)kind;
    p->npdecl++;
}

/* at the end of a class body: uses it declares are settled, the others
 * go to the enclosing class (an error at the outermost one) */
static void priv_settle(ps_t *p, int dbase, int rbase) {
    int i, j, keep = rbase;
    for (i = rbase; i < p->npref; i++) {
        int found = 0;
        for (j = dbase; j < p->npdecl; j++) if (p->pdecl[j] == p->pref[i]) { found = 1; break; }
        if (found) continue;
        if (p->cdepth == 0) {
            p->t.line = p->pref_line[i];
            ps_error(p, "Private field must be declared in an enclosing class");
            return;
        }
        p->pref[keep] = p->pref[i];
        p->pref_line[keep] = p->pref_line[i];
        keep++;
    }
    p->npref = keep;
    p->npdecl = dbase;
}

/* static { ... }: a method with no parameters the class calls once */
static uint32_t parse_static_block(ps_t *p) {
    uint32_t fn = mk(p, N_FUNC), saved_vh = p->vhead, saved_vt = p->vtail;
    int saved_flags = p->fn_flags, saved_in = p->in_func, saved_gen = p->in_gen, saved_async = p->in_async;
    list_t body = { 0, 0 };
    if (!fn) return 0;
    N(fn)->fl |= F_METHOD;
    p->vhead = p->vtail = 0;
    p->fn_flags = 0;
    p->in_func = 1;
    p->in_gen = 0;
    p->in_async = 0;
    expect(p, OP_LBRACE, "{");
    while (!is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->failed) list_add(p, &body, parse_statement(p));
    expect(p, OP_RBRACE, "}");
    N(fn)->c = body.head;
    N(fn)->d = p->vhead;
    N(fn)->fl |= (uint16_t)(p->fn_flags & F_ARGS);
    p->vhead = saved_vh;
    p->vtail = saved_vt;
    p->fn_flags = saved_flags;
    p->in_func = saved_in;
    p->in_gen = saved_gen;
    p->in_async = saved_async;
    return fn;
}

static uint32_t parse_class(ps_t *p, int is_decl) {
    uint32_t c = mk(p, N_CLASS);
    list_t members = { 0, 0 };
    int saved_class = p->in_class;
    if (!c) return 0;
    next(p);      /* class */
    if (ident_ok(p)) { N(c)->a = p->t.str; next(p); }
    else if (is_decl) { ps_error(p, "A class declaration requires a name"); return 0; }
    if (is_kw(p, KW_EXTENDS)) {
        next(p);
        N(c)->b = parse_lhs(p);
    }
    expect(p, OP_LBRACE, "{");
    p->in_class = 1;
    {
    int dbase = p->npdecl, rbase = p->npref;
    p->cdepth++;
    while (!is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->failed) {
        uint32_t m;
        int is_static = 0, flags = 0, computed = 0, kind = 0, priv = 0;
        uint32_t key;
        if (accept(p, OP_SEMI)) continue;
        if (is_kw(p, KW_STATIC)) {
            lexsave_t sv;
            lex_save(p, &sv);
            next(p);
            if (is_op(p, OP_LPAREN) || is_op(p, OP_ASSIGN)) lex_restore(p, &sv);
            else is_static = 1;
            if (is_static && is_op(p, OP_LBRACE)) {    /* Lot 115: static { } */
                uint32_t fn = parse_static_block(p);
                if (!fn) return 0;
                m = mk(p, N_FIELD);
                if (!m) return 0;
                N(m)->op = MK_SBLOCK;
                N(m)->b = fn;
                N(m)->fl = F_STATIC;
                list_add(p, &members, m);
                continue;
            }
        }
        if (is_op(p, OP_MUL)) {             /* Lot 114: *name() {} */
            next(p);
            kind = FK_GEN;
        } else if (is_kw(p, KW_ASYNC)) {    /* Lot 114: async name() {} */
            lexsave_t sv;
            lex_save(p, &sv);
            next(p);
            if (!is_op(p, OP_LPAREN) && !is_op(p, OP_ASSIGN) && !is_op(p, OP_SEMI) && !is_op(p, OP_RBRACE) && !p->t.nl) {
                if (is_op(p, OP_MUL)) { ps_error(p, "async generators are not supported by the guest"); return 0; }
                kind = FK_ASYNC;
            } else {
                lex_restore(p, &sv);
            }
        }
        if (is_kw(p, KW_GET) || is_kw(p, KW_SET)) {
            lexsave_t sv;
            int kw = p->t.kw;
            lex_save(p, &sv);
            next(p);
            if (!is_op(p, OP_LPAREN) && !is_op(p, OP_ASSIGN) && !is_op(p, OP_SEMI) && !is_op(p, OP_RBRACE))
                flags = kw == KW_GET ? F_GETTER : F_SETTER;
            else lex_restore(p, &sv);
        }
        if (p->t.type == TK_PRIV) {         /* Lot 115: #x */
            key = p->t.str;
            priv = 1;
            next(p);
            priv_declare(p, key, (flags == F_GETTER ? 1 : flags == F_SETTER ? 2 : 3) | (is_static ? 4 : 0), dbase);
        } else {
            key = parse_prop_key(p, &computed);
        }
        if (p->failed) return 0;
        if (kind && (flags || !is_op(p, OP_LPAREN))) { ps_error(p, "Unexpected token in class body"); return 0; }
        if (is_op(p, OP_LPAREN)) {
            uint32_t fn = parse_function_k(p, 0, 1, kind);
            if (!fn) return 0;
            if (!priv && !computed && !is_static && !flags && key == js_intern(p->vm, "constructor", 11)) {
                if (kind) { ps_error(p, "Class constructor may not be a generator or async method"); return 0; }
                N(fn)->fl |= F_CTOR;
                if (N(c)->b) N(fn)->fl |= F_DERIVED;
                N(c)->d = fn;
                continue;
            }
            m = mk(p, N_METHOD);
            if (!m) return 0;
            N(m)->a = key;
            N(m)->b = fn;
            N(m)->op = (uint8_t)(priv ? MK_PRIVATE : MK_PUBLIC);
            N(m)->fl = (uint16_t)(flags | (is_static ? F_STATIC : 0) | (computed ? F_COMPUTED : 0));
        } else {
            /* a field: name [= initializer]; */
            if (flags) { ps_error(p, "Unexpected token in class body (accessor without a body)"); return 0; }
            m = mk(p, N_FIELD);
            if (!m) return 0;
            N(m)->a = key;
            N(m)->op = (uint8_t)(priv ? MK_PRIVATE : MK_PUBLIC);
            N(m)->fl = (uint16_t)((is_static ? F_STATIC : 0) | (computed ? F_COMPUTED : 0));
            if (accept(p, OP_ASSIGN)) {
                /* the initializer runs as a method body (this = the instance) */
                uint32_t sv_vh = p->vhead, sv_vt = p->vtail;
                int sv_in = p->in_func, sv_gen = p->in_gen, sv_async = p->in_async;
                p->vhead = p->vtail = 0;
                p->in_func = 1;
                p->in_gen = 0;
                p->in_async = 0;
                N(m)->b = parse_assign(p, 0);
                p->vhead = sv_vh; p->vtail = sv_vt;
                p->in_func = sv_in;
                p->in_gen = sv_gen;
                p->in_async = sv_async;
            }
            semi(p);
        }
        list_add(p, &members, m);
    }
    p->cdepth--;
    if (!p->failed) priv_settle(p, dbase, rbase);
    }
    expect(p, OP_RBRACE, "}");
    p->in_class = saved_class;
    N(c)->c = members.head;
    if (is_decl) {
        uint32_t d = mk(p, N_CLASSDECL);
        if (!d) return 0;
        N(d)->a = c;
        return d;
    }
    return c;
}

/* var/let/const declarations; kind is the keyword */
static uint32_t parse_var(ps_t *p, int kw, int no_in, int in_for) {
    uint32_t v = mk(p, N_VAR);
    list_t l = { 0, 0 };
    if (!v) return 0;
    N(v)->op = (uint8_t)(kw == KW_VAR ? DK_VAR : kw == KW_LET ? DK_LET : DK_CONST);
    next(p);
    for (;;) {
        uint32_t d = mk(p, N_DECL);
        if (!d) return 0;
        N(d)->a = parse_binding(p);
        if (kw == KW_VAR) hoist_pattern(p, N(d)->a);
        if (accept(p, OP_ASSIGN)) N(d)->b = parse_assign(p, no_in);
        else if (!in_for && kw == KW_CONST) { ps_error(p, "Missing initializer in const declaration"); return 0; }
        else if (!in_for && N(N(d)->a)->k != N_IDENT) { ps_error(p, "Missing initializer in destructuring declaration"); return 0; }
        list_add(p, &l, d);
        if (!accept(p, OP_COMMA)) break;
    }
    N(v)->a = l.head;
    return v;
}

static int let_is_decl(ps_t *p) {
    lexsave_t sv;
    int r;
    if (!is_kw(p, KW_LET)) return 0;
    lex_save(p, &sv);
    next(p);
    r = (p->t.type == TK_IDENT && !kw_reserved(p->t.kw)) || is_op(p, OP_LBRACK) || is_op(p, OP_LBRACE);
    lex_restore(p, &sv);
    return r;
}

static uint32_t parse_block_body(ps_t *p, uint32_t blk) {
    list_t l = { 0, 0 };
    while (!is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->failed) {
        uint32_t s = parse_statement(p);
        if (s) {
            int k = N(s)->k;
            if (k == N_FUNCDECL || k == N_CLASSDECL || (k == N_VAR && N(s)->op != DK_VAR)) N(blk)->fl |= F_LEX;
        }
        list_add(p, &l, s);
    }
    expect(p, OP_RBRACE, "}");
    N(blk)->a = l.head;
    return blk;
}

static uint32_t parse_for(ps_t *p) {
    uint32_t n = mk(p, N_FOR), init = 0;
    int decl_kw = 0;
    if (!n) return 0;
    next(p);
    if (is_kw(p, KW_AWAIT)) { ps_error(p, "for await is not supported by the guest"); return 0; }
    expect(p, OP_LPAREN, "(");
    if (is_op(p, OP_SEMI)) {
        init = 0;
    } else if (is_kw(p, KW_VAR) || is_kw(p, KW_CONST) || let_is_decl(p)) {
        decl_kw = p->t.kw;
        init = parse_var(p, decl_kw, 1, 1);
    } else {
        init = parse_expr(p, 1);
    }
    if (p->failed) return 0;
    if (is_kw(p, KW_OF) || is_kw(p, KW_IN)) {
        int of = is_kw(p, KW_OF);
        N(n)->k = (uint8_t)(of ? N_FOROF : N_FORIN);
        next(p);
        if (!decl_kw && init) {
            if (N(init)->k == N_ARRAY || N(init)->k == N_OBJECT) init = to_pattern(p, init);
            else if (N(init)->k != N_IDENT && N(init)->k != N_MEMBER && N(init)->k != N_INDEX) {
                ps_error(p, "Invalid left-hand side in for-loop");
                return 0;
            }
        }
        N(n)->a = init;
        N(n)->b = of ? parse_assign(p, 0) : parse_expr(p, 0);
        expect(p, OP_RPAREN, ")");
        N(n)->c = parse_statement(p);
        return n;
    }
    N(n)->a = init;
    expect(p, OP_SEMI, ";");
    if (!is_op(p, OP_SEMI)) N(n)->b = parse_expr(p, 0);
    expect(p, OP_SEMI, ";");
    if (!is_op(p, OP_RPAREN)) N(n)->c = parse_expr(p, 0);
    expect(p, OP_RPAREN, ")");
    N(n)->d = parse_statement(p);
    return n;
}

static uint32_t parse_statement(ps_t *p) {
    uint32_t n = 0;
    if (p->failed) return 0;
    if (++p->nest > PS_MAX_NEST || stack_low(p)) { ps_error(p, "statements nested too deeply for the guest"); return 0; }
    if (p->t.type == TK_OP) {
        switch (p->t.op) {
        case OP_LBRACE:
            n = mk(p, N_BLOCK);
            next(p);
            if (n) parse_block_body(p, n);
            p->nest--;
            return n;
        case OP_SEMI:
            n = mk(p, N_EMPTY);
            next(p);
            p->nest--;
            return n;
        default: break;
        }
    } else if (p->t.type == TK_IDENT) {
        switch (p->t.kw) {
        case KW_VAR: case KW_CONST:
            n = parse_var(p, p->t.kw, 0, 0);
            semi(p);
            p->nest--;
            return n;
        case KW_LET:
            if (let_is_decl(p)) {
                n = parse_var(p, KW_LET, 0, 0);
                semi(p);
                p->nest--;
                return n;
            }
            break;
        case KW_FUNCTION:
            n = parse_function(p, 1, 0, 0);
            p->nest--;
            return n;
        case KW_ASYNC: {
            lexsave_t sv;
            lex_save(p, &sv);
            next(p);
            if (is_kw(p, KW_FUNCTION) && !p->t.nl) {   /* Lot 114 */
                n = parse_function_k(p, 1, 0, FK_ASYNC);
                p->nest--;
                return n;
            }
            lex_restore(p, &sv);
            break;
        }
        case KW_CLASS:
            n = parse_class(p, 1);
            p->nest--;
            return n;
        case KW_IF:
            n = mk(p, N_IF);
            next(p);
            expect(p, OP_LPAREN, "(");
            if (n) N(n)->a = parse_expr(p, 0);
            expect(p, OP_RPAREN, ")");
            if (n) N(n)->b = parse_statement(p);
            if (is_kw(p, KW_ELSE)) { next(p); if (n) N(n)->c = parse_statement(p); }
            p->nest--;
            return n;
        case KW_FOR:
            n = parse_for(p);
            p->nest--;
            return n;
        case KW_WHILE:
            n = mk(p, N_WHILE);
            next(p);
            expect(p, OP_LPAREN, "(");
            if (n) N(n)->a = parse_expr(p, 0);
            expect(p, OP_RPAREN, ")");
            if (n) N(n)->b = parse_statement(p);
            p->nest--;
            return n;
        case KW_DO:
            n = mk(p, N_DOWHILE);
            next(p);
            if (n) N(n)->b = parse_statement(p);
            if (!is_kw(p, KW_WHILE)) { ps_error(p, "Unexpected token (expected while)"); return 0; }
            next(p);
            expect(p, OP_LPAREN, "(");
            if (n) N(n)->a = parse_expr(p, 0);
            expect(p, OP_RPAREN, ")");
            accept(p, OP_SEMI);
            p->nest--;
            return n;
        case KW_RETURN:
            if (!p->in_func) { ps_error(p, "Illegal return statement"); return 0; }
            n = mk(p, N_RETURN);
            next(p);
            if (!is_op(p, OP_SEMI) && !is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->t.nl && n) N(n)->a = parse_expr(p, 0);
            semi(p);
            p->nest--;
            return n;
        case KW_BREAK: case KW_CONTINUE:
            n = mk(p, p->t.kw == KW_BREAK ? N_BREAK : N_CONTINUE);
            next(p);
            if (ident_ok(p) && !p->t.nl && n) { N(n)->a = p->t.str; next(p); }
            semi(p);
            p->nest--;
            return n;
        case KW_THROW:
            n = mk(p, N_THROW);
            next(p);
            if (p->t.nl) { ps_error(p, "Illegal newline after throw"); return 0; }
            if (n) N(n)->a = parse_expr(p, 0);
            semi(p);
            p->nest--;
            return n;
        case KW_TRY: {
            uint32_t b;
            n = mk(p, N_TRY);
            next(p);
            if (!is_op(p, OP_LBRACE)) { ps_error(p, "Unexpected token (expected { after try)"); return 0; }
            b = mk(p, N_BLOCK); next(p);
            if (b) parse_block_body(p, b);
            if (n) N(n)->a = b;
            if (is_kw(p, KW_CATCH)) {
                next(p);
                if (accept(p, OP_LPAREN)) {
                    if (n) N(n)->b = parse_binding(p);
                    expect(p, OP_RPAREN, ")");
                }
                if (!is_op(p, OP_LBRACE)) { ps_error(p, "Unexpected token (expected { after catch)"); return 0; }
                b = mk(p, N_BLOCK); next(p);
                if (b) parse_block_body(p, b);
                if (n) N(n)->c = b;
            }
            if (is_kw(p, KW_FINALLY)) {
                next(p);
                if (!is_op(p, OP_LBRACE)) { ps_error(p, "Unexpected token (expected { after finally)"); return 0; }
                b = mk(p, N_BLOCK); next(p);
                if (b) parse_block_body(p, b);
                if (n) N(n)->d = b;
            }
            if (n && !N(n)->c && !N(n)->d) { ps_error(p, "Missing catch or finally after try"); return 0; }
            p->nest--;
            return n;
        }
        case KW_SWITCH: {
            list_t cases = { 0, 0 };
            n = mk(p, N_SWITCH);
            next(p);
            expect(p, OP_LPAREN, "(");
            if (n) N(n)->a = parse_expr(p, 0);
            expect(p, OP_RPAREN, ")");
            expect(p, OP_LBRACE, "{");
            while (!is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->failed) {
                uint32_t c = mk(p, N_CASE);
                list_t body = { 0, 0 };
                if (!c) return 0;
                if (is_kw(p, KW_CASE)) { next(p); N(c)->a = parse_expr(p, 0); }
                else if (is_kw(p, KW_DEFAULT)) { next(p); N(c)->fl |= F_STATIC; }
                else { ps_error(p, "Unexpected token in switch (expected case)"); return 0; }
                expect(p, OP_COLON, ":");
                while (!is_kw(p, KW_CASE) && !is_kw(p, KW_DEFAULT) && !is_op(p, OP_RBRACE) && p->t.type != TK_EOF && !p->failed) {
                    uint32_t s = parse_statement(p);
                    if (s && n && (N(s)->k == N_FUNCDECL || N(s)->k == N_CLASSDECL || (N(s)->k == N_VAR && N(s)->op != DK_VAR)))
                        N(n)->fl |= F_LEX;
                    list_add(p, &body, s);
                }
                N(c)->b = body.head;
                list_add(p, &cases, c);
            }
            expect(p, OP_RBRACE, "}");
            if (n) N(n)->b = cases.head;
            p->nest--;
            return n;
        }
        case KW_DEBUGGER:
            n = mk(p, N_EMPTY);
            next(p);
            semi(p);
            p->nest--;
            return n;
        case KW_IMPORT: case KW_EXPORT:
            ps_error(p, "modules (import/export) are not supported by the guest");
            return 0;
        case KW_WITH:
            ps_error(p, "with statements are not supported by the guest");
            return 0;
        default:
            if (ident_ok(p)) {   /* label: */
                lexsave_t sv;
                uint32_t name = p->t.str;
                lex_save(p, &sv);
                next(p);
                if (is_op(p, OP_COLON)) {
                    next(p);
                    n = mk(p, N_LABEL);
                    if (n) { N(n)->a = name; N(n)->b = parse_statement(p); }
                    p->nest--;
                    return n;
                }
                lex_restore(p, &sv);
            }
            break;
        }
    }
    n = mk(p, N_EXPR);
    if (n) N(n)->a = parse_expr(p, 0);
    semi(p);
    p->nest--;
    return n;
}

uint32_t js_parse(js_vm_t *vm, const char *src, int len, int as_function_body) {
    static ps_t ps;
    ps_t *p = &ps;
    uint32_t prog;
    list_t l = { 0, 0 };
    int i;
    for (i = 0; i < (int)sizeof(ps); i++) ((char *)p)[i] = 0;
    p->vm = vm;
    p->s = src;
    p->len = len;
    p->line = 1;
    p->last_sig = 1;
    p->in_func = as_function_body;
    prog = js_node_new(vm, N_PROG, 1);
    if (!prog) return 0;
    next(p);
    while (p->t.type != TK_EOF && !p->failed) list_add(p, &l, parse_statement(p));
    if (p->failed || vm->ctl == CTL_ABORT) return 0;
    JNODE(vm, prog)->a = l.head;
    JNODE(vm, prog)->d = p->vhead;
    JNODE(vm, prog)->fl = (uint16_t)(p->fn_flags & F_ARGS);
    return prog;
}
