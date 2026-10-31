/* plat_form.c - Lot 68 (US-080): see plat_form.h. */
#include "plat_form.h"
#include "../include/os_ui_scale.h"

static int f_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static int f_eq(const char* a, const char* b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static void f_cat(char* d, int cap, const char* s) {
    int n = f_len(d), i = 0;
    while (s && s[i] && n < cap - 1) d[n++] = s[i++];
    if (cap > 0) d[n < cap ? n : cap - 1] = 0;
}
static void f_cat_u(char* d, int cap, uint32_t v) {
    char t[12], o[12];
    int k = 0, j;
    do { t[k++] = (char)('0' + v % 10U); v /= 10U; } while (v && k < 11);
    for (j = 0; j < k; j++) o[j] = t[k - 1 - j];
    o[k] = 0;
    f_cat(d, cap, o);
}

uint32_t plat_form_isqrt(uint32_t v) {
    uint32_t r = 0, bit = 1U << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return r;
}

const char* plat_form_kind_name(int form) {
    switch (form) {
    case PF_PHONE: return "phone";
    case PF_TABLET: return "tablet";
    case PF_DESKTOP: return "desktop";
    case PF_TV: return "tv";
    default: return "unknown";
    }
}

int plat_form_kind_parse(const char* s) {
    if (f_eq(s, "phone") || f_eq(s, "mobile")) return PF_PHONE;
    if (f_eq(s, "tablet")) return PF_TABLET;
    if (f_eq(s, "desktop") || f_eq(s, "pc")) return PF_DESKTOP;
    if (f_eq(s, "tv")) return PF_TV;
    if (f_eq(s, "auto")) return 0;
    return -1;
}

const char* plat_form_chat_name(int mode) {
    return mode == PF_CHAT_FULL ? "full" : mode == PF_CHAT_FLOAT ? "float" : "center";
}

/* snap a scale to the steps the renderer smooths best (1x 1.5x 2x 3x 4x) */
static int snap_q(int q) {
    static const int steps[] = {4, 6, 8, 12, 16};
    int i, best = 4, bd = 1000;
    for (i = 0; i < 5; i++) {
        int d = q > steps[i] ? q - steps[i] : steps[i] - q;
        if (d < bd || (d == bd && steps[i] > best)) { bd = d; best = steps[i]; }
    }
    return best;
}

int plat_form_plan(int w, int h, int diag_tenths, int form, plat_form_plan_t* o) {
    int shortest, q;
    if (!o || w < 160 || h < 120 || w > 8192 || h > 8192) return -1;
    if (diag_tenths < 0 || diag_tenths > 1500) return -1;
    if (form < 0 || form > PF_TV) return -1;
    o->w = w; o->h = h;
    o->diag_tenths = diag_tenths;
    o->portrait = h > w;
    shortest = o->portrait ? w : h;
    o->ppi = 0;
    if (diag_tenths > 0) {
        uint32_t dpx = plat_form_isqrt((uint32_t)w * (uint32_t)w + (uint32_t)h * (uint32_t)h);
        o->ppi = (int)((dpx * 10U + (uint32_t)diag_tenths / 2U) / (uint32_t)diag_tenths);
    }
    o->forced = form != 0;
    if (!form) {
        if (diag_tenths > 0) {
            /* physical size known: inches decide */
            form = diag_tenths < 70 ? PF_PHONE : diag_tenths < 130 ? PF_TABLET : diag_tenths < 400 ? PF_DESKTOP : PF_TV;
        } else {
            form = (w < 600 || shortest < 400) ? PF_PHONE : (shortest < 700 || w < 1024) ? PF_TABLET : PF_DESKTOP;
        }
    }
    o->form = form;
    /* text scale: with a ppi, aim at an 8x8 glyph of about 2.4 mm (phone,
     * held close), 2.8 mm (tablet, desktop) or 6 mm (tv, far away);
     * without, the framebuffer rule of the renderer. */
    if (o->ppi > 0) {
        int mm10 = form == PF_TV ? 60 : form == PF_PHONE ? 24 : 28;
        /* glyph px = mm * ppi / 25.4 ; q = px / 8 * 4 */
        int px = (mm10 * o->ppi + 127) / 254;
        q = (px * 4 + 4) / 8;
    } else {
        q = os_ui_auto_q(w, h);
        if (form == PF_PHONE && q < 6 && shortest >= 360) q = 6;   /* touch: bigger labels */
        if (form == PF_TV && q < 8) q = 8;
    }
    if (q < OS_UI_Q_MIN) q = OS_UI_Q_MIN;
    if (q > OS_UI_Q_MAX) q = OS_UI_Q_MAX;
    o->ui_q = snap_q(q);
    o->glyph_px = os_ui_px(8, o->ui_q);
    o->glyph_mm10 = o->ppi > 0 ? (o->glyph_px * 254 + o->ppi / 2) / o->ppi : 0;
    /* 44 pt touch target (Apple HIG / 48 dp Android): 44/72 inch */
    o->touch_px = o->ppi > 0 ? (44 * o->ppi + 36) / 72 : (form == PF_PHONE || form == PF_TABLET ? 44 * o->ui_q / 4 : 24 * o->ui_q / 4);
    if (o->touch_px < 16) o->touch_px = 16;
    o->chat_mode = form == PF_PHONE ? PF_CHAT_FULL : form == PF_DESKTOP && w >= 1280 ? PF_CHAT_FLOAT : PF_CHAT_CENTER;
    o->dock = (form == PF_TABLET && !o->portrait) || (form == PF_DESKTOP && w * 10 > h * 21) ? PF_DOCK_LEFT : PF_DOCK_BOTTOM;
    o->columns = w / os_ui_char_w(o->ui_q);
    o->rows = h / os_ui_line_h(o->ui_q);
    o->compact_menu = form == PF_PHONE || o->columns < 90;
    o->web_css_w = (w - 32) * 4 / o->ui_q;
    if (o->web_css_w > 640) o->web_css_w = 640;
    if (o->web_css_w < 240) o->web_css_w = 240;
    return 0;
}

int plat_form_report(const plat_form_plan_t* p, char* out, int cap) {
    char qb[16];
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    if (!p) return -1;
    f_cat(out, cap, "form-factor ok ");
    f_cat_u(out, cap, (uint32_t)p->w); f_cat(out, cap, "x"); f_cat_u(out, cap, (uint32_t)p->h);
    f_cat(out, cap, " form "); f_cat(out, cap, plat_form_kind_name(p->form));
    f_cat(out, cap, p->forced ? " (forced)" : " (auto)");
    f_cat(out, cap, p->portrait ? " portrait" : " landscape");
    f_cat(out, cap, " ppi ");
    if (p->ppi) f_cat_u(out, cap, (uint32_t)p->ppi); else f_cat(out, cap, "unknown");
    os_ui_q_format(p->ui_q, qb);
    f_cat(out, cap, " ui-scale "); f_cat(out, cap, qb);
    f_cat(out, cap, " glyph_px "); f_cat_u(out, cap, (uint32_t)p->glyph_px);
    if (p->glyph_mm10) {
        f_cat(out, cap, " glyph_mm "); f_cat_u(out, cap, (uint32_t)(p->glyph_mm10 / 10));
        f_cat(out, cap, "."); f_cat_u(out, cap, (uint32_t)(p->glyph_mm10 % 10));
    }
    f_cat(out, cap, " touch_px "); f_cat_u(out, cap, (uint32_t)p->touch_px);
    f_cat(out, cap, " chat "); f_cat(out, cap, plat_form_chat_name(p->chat_mode));
    f_cat(out, cap, p->dock == PF_DOCK_LEFT ? " dock left" : " dock bottom");
    f_cat(out, cap, p->compact_menu ? " menu compact" : " menu full");
    f_cat(out, cap, " grid "); f_cat_u(out, cap, (uint32_t)p->columns); f_cat(out, cap, "x"); f_cat_u(out, cap, (uint32_t)p->rows);
    f_cat(out, cap, " web_css_w "); f_cat_u(out, cap, (uint32_t)p->web_css_w);
    f_cat(out, cap, "\n");
    return f_len(out);
}
