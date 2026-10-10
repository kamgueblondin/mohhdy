/* userspace/platform.c - Phase 6 multi-platform groundwork. See platform.h. */
#include "platform.h"

static int s_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_cat(char* d, const char* s, int cap) { int n = s_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void s_copy(char* d, const char* s, int cap) { int i = 0; while (s && s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void cat_u(char* d, uint32_t v, int cap) {
    char t[11]; char r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; s_cat(d, r, cap);
}
static void cat_hex(char* d, uint32_t v, int cap) {
    char t[9]; int i;
    for (i = 7; i >= 0; i--) { t[i] = "0123456789abcdef"[v & 15U]; v >>= 4; }
    t[8] = 0; s_cat(d, t, cap);
}
static int iabs(int v) { return v < 0 ? -v : v; }

/* ------------------------------------------------------------------ HAL */
int plat_port_supported(const char* arch, const char** reason) {
    if (arch && s_eq(arch, "i386")) { if (reason) *reason = "built"; return 1; }
    if (reason) *reason = (arch && (s_eq(arch, "arm") || s_eq(arch, "aarch64")))
        ? "not ported: no ARM kernel, boot code or toolchain in this tree"
        : "unknown architecture";
    return 0;
}

int plat_hal_report(const plat_hal_t* h, char* out, int cap) {
    out[0] = 0;
    s_cat(out, "hal arch ", cap); s_cat(out, h->arch, cap);
    s_cat(out, " bits ", cap); cat_u(out, h->word_bits, cap);
    s_cat(out, " endian ", cap); s_cat(out, h->little_endian ? "little" : "big", cap);
    s_cat(out, " page ", cap); cat_u(out, h->page_size, cap);
    s_cat(out, "\nhal cpu vendor ", cap); s_cat(out, h->cpu_vendor[0] ? h->cpu_vendor : "unknown", cap);
    s_cat(out, " family ", cap); cat_u(out, h->cpu_family, cap);
    s_cat(out, " model ", cap); cat_u(out, h->cpu_model, cap);
    s_cat(out, " fpu ", cap); s_cat(out, (h->features_edx & 1U) ? "yes" : "no", cap);
    s_cat(out, " tsc ", cap); s_cat(out, (h->features_edx & (1U << 4)) ? "yes" : "no", cap);
    s_cat(out, " sse2 ", cap); s_cat(out, (h->features_edx & (1U << 26)) ? "yes" : "no", cap);
    s_cat(out, " rdrand ", cap); s_cat(out, (h->features_ecx & (1U << 30)) ? "yes" : "no", cap);
    s_cat(out, "\nhal mem total_kib ", cap); cat_u(out, h->mem_total_kib, cap);
    s_cat(out, " free_kib ", cap); cat_u(out, h->mem_free_kib, cap);
    s_cat(out, "\nhal ports i386=built arm=not-ported\n", cap);
    return s_len(out);
}

/* --------------------------------------------------------------- screen */
const char* plat_form_name(int f) {
    return f == PLAT_FORM_PHONE ? "phone" : f == PLAT_FORM_TABLET ? "tablet" : f == PLAT_FORM_DESKTOP ? "desktop" : "?";
}
int plat_layout(int w, int h, plat_layout_t* o) {
    int shortest;
    if (!o || w < 160 || h < 120 || w > 8192 || h > 8192) return -1;
    o->portrait = h > w;
    shortest = o->portrait ? w : h;
    o->form = (w < 600 || shortest < 400) ? PLAT_FORM_PHONE : (w < 1024) ? PLAT_FORM_TABLET : PLAT_FORM_DESKTOP;
    o->font_px = o->form == PLAT_FORM_PHONE ? 16 : o->form == PLAT_FORM_TABLET ? 16 : 16;
    if (o->form == PLAT_FORM_PHONE && shortest >= 360) o->font_px = 20; /* bigger touch targets */
    o->cols = w / (o->font_px / 2);
    o->rows = h / o->font_px;
    if (o->cols > 160) o->cols = 160;
    if (o->rows > 60) o->rows = 60;
    o->panels = o->form == PLAT_FORM_DESKTOP ? 3 : (o->form == PLAT_FORM_TABLET && !o->portrait) ? 2 : 1;
    o->compact = o->form == PLAT_FORM_PHONE || o->cols < 60;
    return 0;
}

/* ------------------------------------------------------------- gestures */
const char* plat_gesture_name(int g) {
    static const char* const k[] = {"none", "tap", "double-tap", "long-press", "swipe-left", "swipe-right",
                                    "swipe-up", "swipe-down", "drag"};
    return (g >= 0 && g <= PLAT_G_DRAG) ? k[g] : "none";
}
int plat_gesture(const plat_point_t* p, int n) {
    int i, downs = 0, first = -1, last = -1, dx, dy;
    uint32_t dt;
    if (!p || n < 2) return PLAT_G_NONE;
    for (i = 0; i < n; i++) {
        if (p[i].down && (i == 0 || !p[i - 1].down)) downs++;
        if (p[i].down) { if (first < 0) first = i; last = i; }
    }
    if (first < 0) return PLAT_G_NONE;
    /* two short presses within 400 ms */
    if (downs >= 2 && p[last].t_ms - p[first].t_ms <= 400U) return PLAT_G_DOUBLE_TAP;
    if (downs != 1) return PLAT_G_NONE;
    dx = p[last].x - p[first].x; dy = p[last].y - p[first].y;
    dt = p[last].t_ms - p[first].t_ms;
    if (iabs(dx) < 10 && iabs(dy) < 10) return dt >= 500U ? PLAT_G_LONG_PRESS : PLAT_G_TAP;
    if (dt <= 300U && (iabs(dx) >= 50 || iabs(dy) >= 50)) {
        if (iabs(dx) >= iabs(dy)) return dx < 0 ? PLAT_G_SWIPE_LEFT : PLAT_G_SWIPE_RIGHT;
        return dy < 0 ? PLAT_G_SWIPE_UP : PLAT_G_SWIPE_DOWN;
    }
    return PLAT_G_DRAG;
}

/* --------------------------------------------------------------- energy */
const char* plat_power_name(int p) {
    return p == PLAT_PWR_PERFORMANCE ? "performance" : p == PLAT_PWR_BALANCED ? "balanced" : p == PLAT_PWR_SAVER ? "saver" : "?";
}
int plat_power_parse(const char* s) {
    if (s_eq(s, "performance")) return PLAT_PWR_PERFORMANCE;
    if (s_eq(s, "balanced")) return PLAT_PWR_BALANCED;
    if (s_eq(s, "saver")) return PLAT_PWR_SAVER;
    return 0;
}
int plat_power_policy(int p, plat_power_policy_t* o) {
    if (!o) return -1;
    o->profile = p;
    switch (p) {
    case PLAT_PWR_PERFORMANCE: o->idle_yields = 1; o->poll_budget = 8; o->screen_dim_s = 0; o->background_ms = 10; return 0;
    case PLAT_PWR_BALANCED: o->idle_yields = 2; o->poll_budget = 4; o->screen_dim_s = 300; o->background_ms = 50; return 0;
    case PLAT_PWR_SAVER: o->idle_yields = 8; o->poll_budget = 1; o->screen_dim_s = 60; o->background_ms = 250; return 0;
    default: return -1;
    }
}
int plat_power_auto(uint32_t busy, int on_battery) {
    if (on_battery) return busy >= 80U ? PLAT_PWR_BALANCED : PLAT_PWR_SAVER;
    if (busy >= 70U) return PLAT_PWR_PERFORMANCE;
    if (busy <= 10U) return PLAT_PWR_SAVER;
    return PLAT_PWR_BALANCED;
}

/* -------------------------------------------------------------- devices */
int plat_dev_add(plat_devices_t* r, const char* name, const char* cls, int present, const char* detail) {
    plat_device_t* d;
    if (!r || r->n >= PLAT_DEVICES) return -1;
    d = &r->d[r->n++];
    s_copy(d->name, name, (int)sizeof(d->name)); s_copy(d->cls, cls, (int)sizeof(d->cls));
    d->present = present ? 1 : 0; s_copy(d->detail, detail, (int)sizeof(d->detail));
    return 0;
}
int plat_dev_report(const plat_devices_t* r, const char* name, char* out, int cap) {
    int i, shown = 0, present = 0;
    out[0] = 0;
    for (i = 0; i < r->n; i++) {
        const plat_device_t* d = &r->d[i];
        if (d->present) present++;
        if (name && name[0] && !s_eq(name, d->name)) continue;
        shown++;
        s_cat(out, "dev ", cap); s_cat(out, d->name, cap); s_cat(out, " class ", cap); s_cat(out, d->cls, cap);
        s_cat(out, d->present ? " present " : " absent ", cap); s_cat(out, d->detail, cap); s_cat(out, "\n", cap);
    }
    if (name && name[0] && !shown) { s_cat(out, "dev error unknown device\n", cap); return -1; }
    s_cat(out, "dev ok count ", cap); cat_u(out, (uint32_t)r->n, cap);
    s_cat(out, " present ", cap); cat_u(out, (uint32_t)present, cap); s_cat(out, "\n", cap);
    return shown;
}

/* ---------------------------------------------------------------- compat */
static uint32_t le16(const uint8_t* b) { return (uint32_t)b[0] | ((uint32_t)b[1] << 8); }
static uint32_t le32(const uint8_t* b) { return le16(b) | (le16(b + 2) << 16); }
int plat_elf_check(const uint8_t* b, int len, plat_compat_t* o) {
    o->ok = 0; o->entry = 0; o->phnum = 0; o->machine = 0;
    if (!b || len < 52 || b[0] != 0x7f || b[1] != 'E' || b[2] != 'L' || b[3] != 'F') { o->reason = "not an ELF image"; return 0; }
    o->machine = le16(b + 18);
    if (b[4] != 1) { o->reason = "ELF64: only 32-bit i386 binaries run here"; return 0; }
    if (b[5] != 1) { o->reason = "big-endian ELF"; return 0; }
    if (o->machine != 3U) {
        o->reason = o->machine == 40U ? "ARM binary: no ARM port" : o->machine == 62U ? "x86-64 binary" : "foreign machine";
        return 0;
    }
    if (le16(b + 16) != 2U) { o->reason = "not an executable (ET_EXEC)"; return 0; }
    o->entry = le32(b + 24); o->phnum = le16(b + 44);
    if (o->phnum == 0U) { o->reason = "no program header"; return 0; }
    o->ok = 1; o->reason = "i386 ET_EXEC";
    return 1;
}

/* --------------------------------------------------------- notifications */
uint32_t plat_note_push(plat_notes_t* n, int prio, const char* text) {
    int i, slot = -1, worst = -1;
    if (!n || !text || !text[0] || prio < 1 || prio > 3) return 0;
    for (i = 0; i < PLAT_NOTES; i++) /* coalesce duplicates */
        if (n->q[i].id && !n->q[i].acked && s_eq(n->q[i].text, text)) { n->q[i].count++; if (prio > n->q[i].prio) n->q[i].prio = prio; return n->q[i].id; }
    for (i = 0; i < PLAT_NOTES; i++) if (!n->q[i].id || n->q[i].acked) { slot = i; break; }
    if (slot < 0) { /* full: replace the lowest priority, oldest first */
        for (i = 0; i < PLAT_NOTES; i++) if (worst < 0 || n->q[i].prio < n->q[worst].prio) worst = i;
        if (n->q[worst].prio > prio) { n->dropped++; return 0; }
        n->dropped++; slot = worst;
    }
    n->q[slot].id = ++n->next_id; n->q[slot].prio = prio; n->q[slot].acked = 0; n->q[slot].count = 1;
    s_copy(n->q[slot].text, text, (int)sizeof(n->q[slot].text));
    return n->q[slot].id;
}
int plat_note_ack(plat_notes_t* n, uint32_t id) {
    int i;
    for (i = 0; i < PLAT_NOTES; i++) if (n->q[i].id == id && !n->q[i].acked) { n->q[i].acked = 1; return 0; }
    return -1;
}
int plat_note_report(const plat_notes_t* n, char* out, int cap) {
    int p, i, pending = 0;
    out[0] = 0;
    for (p = 3; p >= 1; p--)
        for (i = 0; i < PLAT_NOTES; i++) {
            const plat_note_t* q = &n->q[i];
            if (!q->id || q->acked || q->prio != p) continue;
            pending++;
            s_cat(out, "note ", cap); cat_u(out, q->id, cap); s_cat(out, " prio ", cap); cat_u(out, (uint32_t)q->prio, cap);
            s_cat(out, " x", cap); cat_u(out, q->count, cap); s_cat(out, " ", cap); s_cat(out, q->text, cap); s_cat(out, "\n", cap);
        }
    s_cat(out, "notify ok pending ", cap); cat_u(out, (uint32_t)pending, cap);
    s_cat(out, " dropped ", cap); cat_u(out, n->dropped, cap); s_cat(out, "\n", cap);
    return pending;
}

/* ----------------------------------------------------- migration, deploy */
uint32_t plat_fnv(const uint8_t* b, int len) {
    uint32_t h = 2166136261U; int i;
    for (i = 0; i < len; i++) { h ^= b[i]; h *= 16777619U; }
    return h;
}
static int put_raw(char* out, int pos, int cap, const char* s, int n) {
    int i;
    if (pos < 0 || pos + n >= cap) return -1;
    for (i = 0; i < n; i++) out[pos + i] = s[i];
    out[pos + n] = 0;
    return pos + n;
}
static int put_txt(char* out, int pos, int cap, const char* s) { return put_raw(out, pos, cap, s, s_len(s)); }
static int put_num(char* out, int pos, int cap, uint32_t v, int hex) {
    char t[12]; t[0] = 0;
    if (hex) cat_hex(t, v, 12); else cat_u(t, v, 12);
    return put_txt(out, pos, cap, t);
}
static int name_ok(const char* s) {
    int i, n = s_len(s);
    if (n == 0 || n >= 64) return 0;
    for (i = 0; i < n; i++) if (s[i] <= ' ' || s[i] == '/' || (unsigned char)s[i] > 126) return 0;
    return !(s[0] == '.' && (n == 1 || (n == 2 && s[1] == '.')));
}
int plat_mig_pack(const plat_mig_entry_t* e, int n, char* out, int cap) {
    int i, pos = 0;
    uint32_t all;
    if (!e || n < 0 || n > PLAT_MIG_FILES || !out) return -1;
    pos = put_txt(out, pos, cap, "MMIG1\n");
    for (i = 0; i < n && pos >= 0; i++) {
        if (!name_ok(e[i].name) || e[i].len < 0) return -1;
        pos = put_txt(out, pos, cap, "F ");
        pos = put_txt(out, pos, cap, e[i].name);
        pos = put_txt(out, pos, cap, " ");
        pos = put_num(out, pos, cap, (uint32_t)e[i].len, 0);
        pos = put_txt(out, pos, cap, " ");
        pos = put_num(out, pos, cap, plat_fnv((const uint8_t*)e[i].data, e[i].len), 1);
        pos = put_txt(out, pos, cap, "\n");
        pos = put_raw(out, pos, cap, e[i].data, e[i].len);
        pos = put_txt(out, pos, cap, "\n");
    }
    if (pos < 0) return -1;
    all = plat_fnv((const uint8_t*)out, pos);
    pos = put_txt(out, pos, cap, "END ");
    pos = put_num(out, pos, cap, (uint32_t)n, 0);
    pos = put_txt(out, pos, cap, " ");
    pos = put_num(out, pos, cap, all, 1);
    pos = put_txt(out, pos, cap, "\n");
    return pos;
}
static int read_word(const char* in, int len, int* pos, char* w, int cap) {
    int n = 0;
    while (*pos < len && in[*pos] != ' ' && in[*pos] != '\n') { if (n >= cap - 1) return -1; w[n++] = in[(*pos)++]; }
    w[n] = 0;
    return n;
}
static int parse_u(const char* w, uint32_t* v, int hex) {
    uint32_t r = 0; int i = 0;
    if (!w[0]) return -1;
    for (; w[i]; i++) {
        char c = w[i]; uint32_t d;
        if (c >= '0' && c <= '9') d = (uint32_t)(c - '0');
        else if (hex && c >= 'a' && c <= 'f') d = (uint32_t)(c - 'a' + 10);
        else return -1;
        r = r * (hex ? 16U : 10U) + d;
    }
    *v = r;
    return 0;
}
int plat_mig_unpack(const char* in, int len, plat_mig_entry_t* e, int max) {
    int pos = 6, n = 0;
    char w[72];
    uint32_t size, sum, count;
    if (!in || len < 6 || in[0] != 'M' || in[1] != 'M' || in[2] != 'I' || in[3] != 'G' || in[4] != '1' || in[5] != '\n') return -1;
    for (;;) {
        if (pos + 2 > len) return -1;
        if (in[pos] == 'E') {
            uint32_t all = plat_fnv((const uint8_t*)in, pos);
            pos += 4;
            if (read_word(in, len, &pos, w, 12) <= 0 || parse_u(w, &count, 0) || pos >= len || in[pos++] != ' ') return -1;
            if (read_word(in, len, &pos, w, 12) <= 0 || parse_u(w, &sum, 1)) return -1;
            if (count != (uint32_t)n) return -1;
            return sum == all ? n : -2;
        }
        if (in[pos] != 'F' || in[pos + 1] != ' ') return -1;
        pos += 2;
        if (n >= max || n >= PLAT_MIG_FILES) return -3;
        if (read_word(in, len, &pos, e[n].name, 64) <= 0 || !name_ok(e[n].name) || pos >= len || in[pos++] != ' ') return -1;
        if (read_word(in, len, &pos, w, 12) <= 0 || parse_u(w, &size, 0) || pos >= len || in[pos++] != ' ') return -1;
        if (read_word(in, len, &pos, w, 12) <= 0 || parse_u(w, &sum, 1) || pos >= len || in[pos++] != '\n') return -1;
        if ((int)size < 0 || pos + (int)size + 1 > len || in[pos + (int)size] != '\n') return -1;
        e[n].data = in + pos; e[n].len = (int)size;
        if (plat_fnv((const uint8_t*)e[n].data, e[n].len) != sum) return -2;
        pos += (int)size + 1;
        n++;
    }
}
int plat_deploy_parse(const char* in, int len, plat_deploy_item_t* it, int max) {
    int pos = 0, n = 0;
    char w[72];
    if (!in || len < 8) return -1;
    if (read_word(in, len, &pos, w, 12) <= 0 || !s_eq(w, "DEPLOY1") || pos >= len || in[pos++] != '\n') return -1;
    while (pos < len) {
        if (in[pos] == '\n') { pos++; continue; }
        if (n >= max) return -1;
        if (read_word(in, len, &pos, w, 8) <= 0 || !s_eq(w, "file") || pos >= len || in[pos++] != ' ') return -1;
        if (read_word(in, len, &pos, it[n].src, 64) <= 0 || it[n].src[0] != '/' || pos >= len || in[pos++] != ' ') return -1;
        if (read_word(in, len, &pos, it[n].dst, 64) <= 0 || it[n].dst[0] != '/' || pos >= len || in[pos++] != ' ') return -1;
        if (read_word(in, len, &pos, w, 12) <= 0 || parse_u(w, &it[n].fnv, 1)) return -1;
        {
            int k; /* no traversal */
            for (k = 0; it[n].dst[k + 1]; k++) if (it[n].dst[k] == '.' && it[n].dst[k + 1] == '.') return -1;
        }
        n++;
        if (pos < len && in[pos] == '\n') pos++;
    }
    return n;
}
