/* osui_scale.c - Lot 68: see osui_scale.h. Freestanding, no libc. */
#include "osui_scale.h"

static const char *const k_slot_names[OS_UI_Z_COUNT] = {
    "web", "chat", "shell", "pane", "ide", "app0", "app1", "app2", "app3", "app4", "app5", "app6"
};

static int s_len(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_cat(char *d, int cap, const char *s) {
    int n = s_len(d), i = 0;
    while (s && s[i] && n < cap - 1) d[n++] = s[i++];
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
}
static void s_cat_u(char *d, int cap, unsigned v) {
    char t[12]; int k = 0, j; char o[12];
    if (!v) t[k++] = '0';
    while (v && k < 11) { t[k++] = (char)('0' + v % 10U); v /= 10U; }
    for (j = 0; j < k; j++) o[j] = t[k - 1 - j];
    o[k] = 0;
    s_cat(d, cap, o);
}
static void s_cat_q(char *d, int cap, int q) { char b[8]; os_ui_q_format(q, b); s_cat(d, cap, b); }

void osui_scale_init(osui_scale_t *s) {
    int i;
    uint8_t *p = (uint8_t *)s;
    if (!s) return;
    for (i = 0; i < (int)sizeof(*s); i++) p[i] = 0;
    s->ui_q = OS_UI_Q_AUTO;
    for (i = 0; i < OS_UI_Z_COUNT; i++) s->zoom[i] = OS_UI_Q1;
}

static int screen_w(const osui_scale_t *s) { return s->fb_w > 0 ? s->fb_w : 1024; }
static int screen_h(const osui_scale_t *s) { return s->fb_h > 0 ? s->fb_h : 768; }

int osui_scale_set_ui(osui_scale_t *s, int q) {
    if (!s) return -1;
    if (q != OS_UI_Q_AUTO && (q < OS_UI_Q_MIN || q > OS_UI_Q_MAX)) return -1;
    if (s->ui_q != (uint8_t)q) { s->ui_q = (uint8_t)q; s->changes++; }
    return 0;
}

int osui_scale_zoom(osui_scale_t *s, int slot, int dir) {
    int z;
    if (!s || slot < 0 || slot >= OS_UI_Z_COUNT) return -1;
    z = s->zoom[slot] ? s->zoom[slot] : OS_UI_Q1;
    if (dir == 0) z = OS_UI_Q1;
    else {
        int now = os_ui_effective_q(s->ui_q, z, screen_w(s), screen_h(s)), want = os_ui_q_step(now, dir);
        int g = s->ui_q == OS_UI_Q_AUTO ? os_ui_auto_q(screen_w(s), screen_h(s)) : s->ui_q;
        /* zoom so that global * zoom / 4 reaches the next step */
        z = (want * 4 + g / 2) / g;
        if (z < 1) z = 1;
        while (os_ui_effective_q(s->ui_q, z, screen_w(s), screen_h(s)) <= now && dir > 0 && z < 64) z++;
        while (os_ui_effective_q(s->ui_q, z, screen_w(s), screen_h(s)) >= now && dir < 0 && z > 1) z--;
        if (z > 64) z = 64;
    }
    if (s->zoom[slot] != (uint8_t)z) { s->zoom[slot] = (uint8_t)z; s->changes++; }
    return z;
}

int osui_scale_effective(const osui_scale_t *s, int slot) {
    if (!s) return OS_UI_Q1;
    return os_ui_effective_q(s->ui_q, slot >= 0 && slot < OS_UI_Z_COUNT ? s->zoom[slot] : 0, screen_w(s), screen_h(s));
}

void osui_scale_web_view(osui_scale_t *s, int pw, int ph, int q, int fbw, int fbh) {
    if (!s) return;
    if (fbw > 0 && fbh > 0) { s->fb_w = fbw; s->fb_h = fbh; }
    s->web_pw = pw; s->web_ph = ph; s->web_q = q;
}

int osui_scale_web_viewport(const osui_scale_t *s, int fallback) {
    int q, v;
    if (!s || s->web_pw <= 0) return fallback;
    q = s->web_q >= OS_UI_Q_MIN ? s->web_q : osui_scale_effective(s, OS_UI_Z_WEB);
    v = s->web_pw * 4 / q;
    if (v < 240) v = 240;
    if (v > 4000) v = 4000;
    return v;
}

int osui_scale_slot(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < OS_UI_Z_COUNT; i++) if (s_eq(name, k_slot_names[i])) return i;
    if (s_eq(name, "browser")) return OS_UI_Z_WEB;
    if (s_eq(name, "terminal")) return OS_UI_Z_SHELL;
    if (s_eq(name, "editor")) return OS_UI_Z_IDE;
    return -1;
}

const char *osui_scale_slot_name(int slot) {
    return slot >= 0 && slot < OS_UI_Z_COUNT ? k_slot_names[slot] : "?";
}

int osui_scale_save(const osui_scale_t *s, char *buf, int cap) {
    int i;
    if (!s || !buf || cap < 64) return -1;
    buf[0] = 0;
    s_cat(buf, cap, "ui_scale=");
    s_cat_u(buf, cap, s->ui_q);
    s_cat(buf, cap, " zoom=");
    for (i = 0; i < OS_UI_Z_COUNT; i++) {
        if (i) s_cat(buf, cap, ",");
        s_cat_u(buf, cap, s->zoom[i]);
    }
    if (s->font_mode) s_cat(buf, cap, " font=1");
    s_cat(buf, cap, "\n");
    return s_len(buf);
}

static int read_u(const char *b, int len, int *pos, int *out) {
    int v = 0, d = 0;
    while (*pos < len && b[*pos] >= '0' && b[*pos] <= '9') { v = v * 10 + (b[*pos] - '0'); (*pos)++; if (++d > 3) return -1; }
    if (!d) return -1;
    *out = v;
    return 0;
}

int osui_scale_load(osui_scale_t *s, const char *b, int len) {
    osui_scale_t t;
    int pos = 0, v, i;
    static const char k1[] = "ui_scale=", k2[] = " zoom=";
    if (!s || !b || len <= 0) return -1;
    t = *s;
    for (i = 0; k1[i]; i++, pos++) if (pos >= len || b[pos] != k1[i]) return -1;
    if (read_u(b, len, &pos, &v) != 0) return -1;
    if (v != OS_UI_Q_AUTO && (v < OS_UI_Q_MIN || v > OS_UI_Q_MAX)) return -1;
    t.ui_q = (uint8_t)v;
    for (i = 0; k2[i]; i++, pos++) if (pos >= len || b[pos] != k2[i]) return -1;
    for (i = 0; i < OS_UI_Z_COUNT; i++) {
        if (i) { if (pos >= len || b[pos] != ',') return -1; pos++; }
        if (read_u(b, len, &pos, &v) != 0 || v < 1 || v > 64) return -1;
        t.zoom[i] = (uint8_t)v;
    }
    t.font_mode = 0;
    if (pos + 6 <= len && b[pos] == ' ' && b[pos + 1] == 'f' && b[pos + 2] == 'o' && b[pos + 3] == 'n' &&
        b[pos + 4] == 't' && b[pos + 5] == '=') {
        pos += 6;
        if (read_u(b, len, &pos, &v) != 0 || v > 1) return -1;
        t.font_mode = (uint8_t)v;
    }
    t.changes = s->changes;
    t.saved_at = s->changes;
    *s = t;
    return 0;
}

static const char *word(const char *p, char *w, int cap) {
    int n = 0;
    while (*p == ' ') p++;
    while (*p && *p != ' ' && n < cap - 1) w[n++] = *p++;
    w[n] = 0;
    while (*p == ' ') p++;
    return p;
}

static void report(const osui_scale_t *s, char *out, int cap) {
    int i;
    out[0] = 0;
    s_cat(out, cap, "ui-scale ok global ");
    if (s->ui_q == OS_UI_Q_AUTO) {
        s_cat(out, cap, "auto (");
        s_cat_q(out, cap, os_ui_auto_q(screen_w(s), screen_h(s)));
        s_cat(out, cap, " for ");
        s_cat_u(out, cap, (unsigned)screen_w(s));
        s_cat(out, cap, "x");
        s_cat_u(out, cap, (unsigned)screen_h(s));
        s_cat(out, cap, ")");
    } else s_cat_q(out, cap, s->ui_q);
    s_cat(out, cap, " char ");
    s_cat_u(out, cap, (unsigned)os_ui_char_w(osui_scale_effective(s, OS_UI_Z_CHAT)));
    s_cat(out, cap, "px");
    for (i = 0; i < OS_UI_Z_COUNT; i++) {
        if (s->zoom[i] == OS_UI_Q1) continue;
        s_cat(out, cap, " ");
        s_cat(out, cap, k_slot_names[i]);
        s_cat(out, cap, "=");
        s_cat_q(out, cap, osui_scale_effective(s, i));
    }
    s_cat(out, cap, "\n");
}

int osui_scale_command(osui_scale_t *s, const char *line, char *out, int cap) {
    char cmd[16], a[16], b[16];
    const char *p;
    int q, slot;
    if (!s || !line || !out || cap < 1) return -1;
    p = word(line, cmd, (int)sizeof(cmd));
    if (!s_eq(cmd, "ui-scale") && !s_eq(cmd, "text-size") && !s_eq(cmd, "ui-font")) return -1;
    p = word(p, a, (int)sizeof(a));
    p = word(p, b, (int)sizeof(b));
    if (s_eq(cmd, "ui-font")) {
        /* Lot 69: native square fonts (default) or the smoothed 8x8 */
        if (b[0] || (a[0] && !s_eq(a, "native") && !s_eq(a, "smooth") && !s_eq(a, "show"))) {
            out[0] = 0;
            s_cat(out, cap, "ui-font error usage: ui-font [native|smooth]\n");
            return 1;
        }
        if (s_eq(a, "native") || s_eq(a, "smooth")) {
            uint8_t m = s_eq(a, "smooth") ? 1 : 0;
            if (s->font_mode != m) { s->font_mode = m; s->changes++; }
        }
        out[0] = 0;
        s_cat(out, cap, "ui-font ok ");
        s_cat(out, cap, s->font_mode ? "smooth (8x8 upscaled, EPX)" : "native (IBM Plex Mono raster 12/16/24/32 px)");
        s_cat(out, cap, "\n");
        return 0;
    }
    if (s_eq(cmd, "ui-scale")) {
        if (!a[0] || s_eq(a, "show")) { report(s, out, cap); return 0; }
        if (b[0]) goto usage;
        if (s_eq(a, "+") || s_eq(a, "-")) {
            int now = s->ui_q == OS_UI_Q_AUTO ? os_ui_auto_q(screen_w(s), screen_h(s)) : s->ui_q;
            q = os_ui_q_step(now, a[0] == '+' ? 1 : -1);
        } else if (s_eq(a, "0") || s_eq(a, "reset")) q = OS_UI_Q1;
        else q = os_ui_q_parse(a);
        if (q < 0 || osui_scale_set_ui(s, q) != 0) goto usage;
        report(s, out, cap);
        return 0;
    }
    /* text-size [WINDOW] N|+|-|0 : zoom of one window (chat by default) */
    slot = OS_UI_Z_CHAT;
    if (a[0] && osui_scale_slot(a) >= 0) { slot = osui_scale_slot(a); for (q = 0; q < 16; q++) a[q] = b[q]; b[0] = 0; }
    if (!a[0] || b[0] || *p) goto usage;
    if (s_eq(a, "+")) (void)osui_scale_zoom(s, slot, 1);
    else if (s_eq(a, "-")) (void)osui_scale_zoom(s, slot, -1);
    else if (s_eq(a, "0") || s_eq(a, "reset")) (void)osui_scale_zoom(s, slot, 0);
    else {
        q = os_ui_q_parse(a);
        if (q <= 0) goto usage;
        if (s->zoom[slot] != (uint8_t)q) { s->zoom[slot] = (uint8_t)q; s->changes++; }
    }
    out[0] = 0;
    s_cat(out, cap, "text-size ok ");
    s_cat(out, cap, k_slot_names[slot]);
    s_cat(out, cap, " zoom ");
    s_cat_q(out, cap, s->zoom[slot]);
    s_cat(out, cap, " effective ");
    s_cat_q(out, cap, osui_scale_effective(s, slot));
    s_cat(out, cap, " char ");
    s_cat_u(out, cap, (unsigned)os_ui_char_w(osui_scale_effective(s, slot)));
    s_cat(out, cap, "px\n");
    return 0;
usage:
    out[0] = 0;
    s_cat(out, cap, s_eq(cmd, "ui-scale") ? "ui-scale error usage: ui-scale [auto|1|1.5|2|3|150%|+|-|0]\n"
                                           : "text-size error usage: text-size [web|chat|shell|pane|ide|app0..app5] N|+|-|0\n");
    return 1;
}
