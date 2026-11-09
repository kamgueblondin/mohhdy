/* desk_cfg.c - Lot 86: see desk_cfg.h. */
#include "desk_cfg.h"
#include "../include/os_ui_scale.h"

static const char *const k_engines[DESK_ENGINES] = {
    "wikipedia", "ddg", "ddg-html", "ddg-lite", "google", "local"
};

static int dlen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static char dlow(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static int deq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && dlow(a[i]) == dlow(b[i])) i++;
    return dlow(a[i]) == dlow(b[i]);
}
static int deq_prefix(const char *s, const char *p) {
    int i = 0;
    if (!s) return 0;
    while (p[i]) { if (dlow(s[i]) != dlow(p[i])) return 0; i++; }
    return 1;
}
static void dcat(char *d, int cap, const char *s) {
    int n = dlen(d), i = 0;
    while (s && s[i] && n < cap - 1) d[n++] = s[i++];
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
}
static void dcat_u(char *d, int cap, unsigned v) {
    char t[12];
    int k = 0, j;
    do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v && k < 11);
    for (j = 0; j < k; j++) {
        char one[2];
        one[0] = t[k - 1 - j];
        one[1] = 0;
        dcat(d, cap, one);
    }
}

void desk_cfg_init(desk_cfg_t *c) {
    if (!c) return;
    c->scale_q = 6;             /* 1.5x, the desktop default from 1024 px */
    c->net_on = 1;
    c->engine = 0;              /* wikipedia, the first fallback since lot 85 */
    c->changes = 0;
}

const char *desk_cfg_engine_name(int index) {
    if (index < 0 || index >= DESK_ENGINES) return "";
    return k_engines[index];
}

int desk_cfg_engine_count(void) { return DESK_ENGINES; }

int desk_cfg_set_scale(desk_cfg_t *c, int q) {
    int n;
    if (!c) return -1;
    if (q < OS_UI_Q_MIN || q > OS_UI_Q_MAX) return -1;
    n = os_ui_q_clamp(q);
    /* only the steps the keyboard zoom uses, so 9 and 11 and 13 and 15
     * are not a scale the rest of the desktop can land on */
    {
        static const int steps[] = {4, 5, 6, 7, 8, 10, 12, 14, 16};
        int i, ok = 0;
        for (i = 0; i < 9; i++) if (steps[i] == n) ok = 1;
        if (!ok) return -1;
    }
    if (c->scale_q != n) { c->scale_q = n; c->changes++; }
    return 0;
}

int desk_cfg_set_scale_word(desk_cfg_t *c, const char *word) {
    if (!word || !word[0]) return -1;
    if (deq(word, "1") || deq(word, "1x") || deq(word, "1.0")) return desk_cfg_set_scale(c, 4);
    if (deq(word, "1.5") || deq(word, "1.5x")) return desk_cfg_set_scale(c, 6);
    if (deq(word, "2") || deq(word, "2x")) return desk_cfg_set_scale(c, 8);
    if (deq(word, "3") || deq(word, "3x")) return desk_cfg_set_scale(c, 12);
    if (deq(word, "4") || deq(word, "4x")) return desk_cfg_set_scale(c, 16);
    if (word[0] >= '0' && word[0] <= '9') {
        int v = 0, i = 0;
        while (word[i] >= '0' && word[i] <= '9' && i < 3) { v = v * 10 + word[i] - '0'; i++; }
        if (word[i]) return -1;
        return desk_cfg_set_scale(c, v);
    }
    return -1;
}

int desk_cfg_step_scale(desk_cfg_t *c, int dir) {
    int n;
    if (!c) return -1;
    n = os_ui_q_step(c->scale_q, dir);
    if (n == c->scale_q) return 0;
    c->scale_q = n;
    c->changes++;
    return 0;
}

int desk_cfg_set_net(desk_cfg_t *c, int on) {
    int v;
    if (!c) return -1;
    v = on ? 1 : 0;
    if (c->net_on != v) { c->net_on = v; c->changes++; }
    return 0;
}

int desk_cfg_set_engine(desk_cfg_t *c, const char *name) {
    int i;
    if (!c || !name || !name[0]) return -1;
    for (i = 0; i < DESK_ENGINES; i++) if (deq(name, k_engines[i])) {
        if (c->engine != i) { c->engine = i; c->changes++; }
        return 0;
    }
    return -1;
}

int desk_cfg_cycle_engine(desk_cfg_t *c) {
    if (!c) return -1;
    c->engine = (c->engine + 1) % DESK_ENGINES;
    c->changes++;
    return 0;
}

int desk_cfg_button(desk_cfg_t *c, int button) {
    if (!c) return 0;
    if (button == 1) { (void)desk_cfg_step_scale(c, 1); return 1; }
    if (button == 2) { (void)desk_cfg_step_scale(c, -1); return 1; }
    if (button == 3) { (void)desk_cfg_set_net(c, !c->net_on); return 1; }
    if (button == 4) { (void)desk_cfg_cycle_engine(c); return 1; }
    return 0;
}

int desk_cfg_save(const desk_cfg_t *c, char *out, int cap) {
    if (!c || !out || cap < 16) return -1;
    out[0] = 0;
    dcat(out, cap, "settings scale ");
    dcat_u(out, cap, (unsigned)c->scale_q);
    dcat(out, cap, "\nsettings net ");
    dcat(out, cap, c->net_on ? "on\n" : "off\n");
    dcat(out, cap, "settings engine ");
    dcat(out, cap, desk_cfg_engine_name(c->engine));
    dcat(out, cap, "\n");
    return dlen(out);
}

int desk_cfg_load(desk_cfg_t *c, const char *text, int len) {
    desk_cfg_t tmp;
    int i = 0;
    if (!c || !text || len < 0) return -1;
    desk_cfg_init(&tmp);
    tmp.changes = 0;
    while (i < len) {
        char line[64];
        int k = 0;
        const char *arg;
        while (i < len && text[i] != '\n' && k < (int)sizeof(line) - 1) line[k++] = text[i++];
        if (i < len && text[i] == '\n') i++;
        line[k] = 0;
        if (k == 0 || line[0] == '#') continue;
        if (k < 9 || !deq_prefix(line, "settings ")) return -1;
        arg = line + 9;
        if (deq_prefix(arg, "scale ")) {
            if (desk_cfg_set_scale_word(&tmp, arg + 6) != 0) return -1;
        } else if (deq_prefix(arg, "net ")) {
            if (deq(arg + 4, "on")) { if (desk_cfg_set_net(&tmp, 1) != 0) return -1; }
            else if (deq(arg + 4, "off")) { if (desk_cfg_set_net(&tmp, 0) != 0) return -1; }
            else return -1;
        } else if (deq_prefix(arg, "engine ")) {
            if (desk_cfg_set_engine(&tmp, arg + 7) != 0) return -1;
        } else return -1;
    }
    *c = tmp;
    return 0;
}

int desk_cfg_report(const desk_cfg_t *c, char *out, int cap) {
    if (!c || !out || cap < 16) return -1;
    out[0] = 0;
    dcat(out, cap, "Parametres (console et bureau, un seul reglage)\n");
    dcat(out, cap, "  echelle ");
    dcat_u(out, cap, (unsigned)c->scale_q);
    dcat(out, cap, "/4  (");
    dcat_u(out, cap, (unsigned)os_ui_char_w(c->scale_q));
    dcat(out, cap, " px par glyphe, chrome plafonne a ");
    dcat_u(out, cap, (unsigned)os_ui_char_w(desk_cfg_chrome_q(c->scale_q)));
    dcat(out, cap, " px)\n");
    dcat(out, cap, "  reseau ");
    dcat(out, cap, c->net_on ? "on (fetch autorise)\n" : "off (fetch refuse avant le socket)\n");
    dcat(out, cap, "  moteur ");
    dcat(out, cap, desk_cfg_engine_name(c->engine));
    dcat(out, cap, "\n");
    dcat(out, cap, "  boutons: echelle +/-, reseau, moteur. Batterie simulee, pas de DDC.\n");
    return dlen(out);
}

int desk_cfg_chrome_q(int slot_q) {
    int q = os_ui_q_clamp(slot_q <= 0 ? OS_UI_Q1 : slot_q);
    if (q > OS_UI_Q_CHROME_MAX) q = OS_UI_Q_CHROME_MAX;
    return q;
}

int desk_cfg_chrome_audit(char *out, int cap) {
    static const int slots[] = {
        OS_UI_Z_WEB, OS_UI_Z_CHAT, OS_UI_Z_SHELL, OS_UI_Z_PANE, OS_UI_Z_IDE, OS_UI_Z_APP0
    };
    static const char *const names[] = { "web", "chat", "shell", "pane", "ide", "app0" };
    static const int scales[] = {4, 5, 6, 7, 8, 10, 12, 14, 16};
    int s, q, bad = 0;
    if (out && cap > 0) out[0] = 0;
    for (s = 0; s < 6; s++) {
        if (slots[s] < 0 || slots[s] >= OS_UI_Z_COUNT) bad++;
        for (q = 0; q < 9; q++) {
            int chrome = desk_cfg_chrome_q(scales[q]);
            int body = os_ui_q_clamp(scales[q]);
            if (chrome > OS_UI_Q_CHROME_MAX) bad++;
            if (chrome > body) bad++;
            if (os_ui_char_w(chrome) > os_ui_char_w(OS_UI_Q_CHROME_MAX)) bad++;
            if (body < OS_UI_Q_MIN || body > OS_UI_Q_MAX) bad++;
        }
        if (out && cap > 8) {
            dcat(out, cap, names[s]);
            dcat(out, cap, " chrome ");
            dcat_u(out, cap, (unsigned)desk_cfg_chrome_q(16));
            dcat(out, cap, " ");
        }
    }
    if (out && cap > 4) dcat(out, cap, bad ? "bad\n" : "ok\n");
    return bad;
}
