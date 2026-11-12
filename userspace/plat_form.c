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

/* Lot 69 (US-080): physical size from the kernel display facts. */
int plat_form_physical(const os_display_info_t* d, int* diag_tenths, char* out, int cap) {
    static const char* const why[] = { "edid ok", "no Bochs/QEMU std VGA", "no EDID BAR", "bad EDID header", "bad EDID checksum" };
    if (out && cap > 0) out[0] = 0;
    if (diag_tenths) *diag_tenths = 0;
    if (!d) return -1;
    if (d->source == OS_DISPLAY_SRC_NONE || d->diag_tenths == 0 || d->diag_tenths > 1500) {
        f_cat(out, cap, "display physical unknown (");
        f_cat(out, cap, d->edid_status < 5 ? why[d->edid_status] : "edid error");
        if (d->edid_status == 0) f_cat(out, cap, ", no size");
        f_cat(out, cap, ") plan from pixels only\n");
        return 1;
    }
    if (diag_tenths) *diag_tenths = d->diag_tenths;
    f_cat(out, cap, "display physical ");
    f_cat_u(out, cap, d->mm_w); f_cat(out, cap, "x"); f_cat_u(out, cap, d->mm_h);
    f_cat(out, cap, "mm diag ");
    f_cat_u(out, cap, (uint32_t)(d->diag_tenths / 10)); f_cat(out, cap, ".");
    f_cat_u(out, cap, (uint32_t)(d->diag_tenths % 10));
    f_cat(out, cap, "in source ");
    f_cat(out, cap, d->source == OS_DISPLAY_SRC_EDID_DTD ? "edid-dtd" : "edid-cm");
    f_cat(out, cap, " mfg ");
    { char m[4]; int i; for (i = 0; i < 3; i++) m[i] = (d->mfg[i] >= 'A' && d->mfg[i] <= 'Z') ? d->mfg[i] : '?'; m[3] = 0; f_cat(out, cap, m); }
    f_cat(out, cap, " native ");
    f_cat_u(out, cap, d->native_w); f_cat(out, cap, "x"); f_cat_u(out, cap, d->native_h);
    f_cat(out, cap, "\n");
    return 0;
}

/* Lot 85 (US-080): extras from the same EDID block. Absent fields are
 * left out, so the line only names what the block actually said. */
int plat_form_edid_detail(const os_display_info_t* d, char* out, int cap) {
    int any = 0;
    if (out && cap > 0) out[0] = 0;
    if (!d) return -1;
    if (d->digital || d->refresh_hz || d->gamma_100 || d->std_n || d->established_n ||
        d->range_vmin || d->range_vmax || d->name[0]) any = 1;
    if (!any) return 0;
    f_cat(out, cap, "display edid");
    if (d->digital) f_cat(out, cap, " digital 1");
    if (d->gamma_100) {
        f_cat(out, cap, " gamma ");
        f_cat_u(out, cap, (uint32_t)(d->gamma_100 / 100));
        f_cat(out, cap, ".");
        f_cat_u(out, cap, (uint32_t)((d->gamma_100 / 10) % 10));
        f_cat_u(out, cap, (uint32_t)(d->gamma_100 % 10));
    }
    if (d->refresh_hz) { f_cat(out, cap, " refresh "); f_cat_u(out, cap, d->refresh_hz); }
    if (d->name[0]) {
        int i;
        f_cat(out, cap, " name ");
        for (i = 0; i < 13 && d->name[i]; i++) {
            char one[2];
            char c = d->name[i];
            if (c < 32 || c > 126) c = '?';
            one[0] = c; one[1] = 0;
            f_cat(out, cap, one);
        }
    }
    if (d->std_n && d->std_w && d->std_h) {
        f_cat(out, cap, " standard ");
        f_cat_u(out, cap, d->std_w); f_cat(out, cap, "x"); f_cat_u(out, cap, d->std_h);
        if (d->std_hz) { f_cat(out, cap, "@"); f_cat_u(out, cap, d->std_hz); }
        f_cat(out, cap, " n "); f_cat_u(out, cap, d->std_n);
    }
    if (d->established_n) { f_cat(out, cap, " established "); f_cat_u(out, cap, d->established_n); }
    if (d->range_vmin || d->range_vmax) {
        f_cat(out, cap, " range ");
        f_cat_u(out, cap, d->range_vmin); f_cat(out, cap, "-"); f_cat_u(out, cap, d->range_vmax);
        f_cat(out, cap, "Hz");
        if (d->range_hmin || d->range_hmax) {
            f_cat(out, cap, " "); f_cat_u(out, cap, d->range_hmin); f_cat(out, cap, "-");
            f_cat_u(out, cap, d->range_hmax); f_cat(out, cap, "kHz");
        }
        if (d->range_clock_mhz) { f_cat(out, cap, " clock "); f_cat_u(out, cap, d->range_clock_mhz); }
    }
    if (d->week) { f_cat(out, cap, " week "); f_cat_u(out, cap, d->week); }
    if (d->serial_lo || d->serial_hi) {
        f_cat(out, cap, " serial ");
        f_cat_u(out, cap, ((uint32_t)d->serial_hi << 16) | d->serial_lo);
    }
    if (d->serial_text[0]) {
        int i;
        f_cat(out, cap, " serial_text ");
        for (i = 0; i < 13 && d->serial_text[i]; i++) {
            char one[2];
            char c = d->serial_text[i];
            if (c < 32 || c > 126) c = '?';
            one[0] = c; one[1] = 0;
            f_cat(out, cap, one);
        }
    }
    if (d->chroma_wx || d->chroma_wy) {
        f_cat(out, cap, " white ");
        f_cat_u(out, cap, d->chroma_wx);
        f_cat(out, cap, ",");
        f_cat_u(out, cap, d->chroma_wy);
    }
    if (d->dtd_n > 1) { f_cat(out, cap, " dtd "); f_cat_u(out, cap, d->dtd_n); }
    f_cat(out, cap, "\n");
    return f_len(out);
}

/* ---- Lot 87: CEA-861 facts (US-080) --------------------------------- */

static void f_cat_hex2(char* d, int cap, uint32_t v) {
    static const char hx[] = "0123456789abcdef";
    char t[3];
    t[0] = hx[(v >> 4) & 15]; t[1] = hx[v & 15]; t[2] = 0;
    f_cat(d, cap, t);
}

int plat_form_cea_line(const os_display_info_t* d, char* out, int cap) {
    int i, n;
    if (!out || cap <= 0) return 0;
    out[0] = 0;
    if (!d) return 0;
    f_cat(out, cap, "display cea");
    if (d->cea_status == 1) { f_cat(out, cap, " none\n"); return (int)f_len(out); }
    if (d->cea_status == 2) {
        f_cat(out, cap, " not-cea tag 0x");
        f_cat_hex2(out, cap, d->cea_ext_tag);
        f_cat(out, cap, "\n");
        return (int)f_len(out);
    }
    if (d->cea_status != 0) { f_cat(out, cap, " refused\n"); return (int)f_len(out); }
    f_cat(out, cap, " rev "); f_cat_u(out, cap, d->cea_rev);
    f_cat(out, cap, " svd "); f_cat_u(out, cap, d->cea_svd_n);
    n = d->cea_svd_n < 8 ? d->cea_svd_n : 8;
    if (n > 0) {
        f_cat(out, cap, " vics ");
        for (i = 0; i < n; i++) {
            if (i) f_cat(out, cap, ",");
            f_cat_u(out, cap, d->cea_vic[i]);
        }
        if (d->cea_svd_n > 8) f_cat(out, cap, ",...");
    }
    if (d->cea_best_w && d->cea_best_h) {
        f_cat(out, cap, " best ");
        f_cat_u(out, cap, d->cea_best_w); f_cat(out, cap, "x"); f_cat_u(out, cap, d->cea_best_h);
        f_cat(out, cap, "@"); f_cat_u(out, cap, d->cea_best_hz);
        f_cat(out, cap, d->cea_best_from == 1 ? " (native)" : d->cea_best_from == 2 ? " (dtd)" : " (largest vic)");
    }
    if (d->cea_sad_n) {
        f_cat(out, cap, " audio "); f_cat_u(out, cap, d->cea_sad_max_ch); f_cat(out, cap, "ch");
    } else if (d->cea_flags & OS_DISPLAY_CEA_AUDIO) f_cat(out, cap, " audio basic");
    else f_cat(out, cap, " audio 0");
    if (d->cea_flags & OS_DISPLAY_CEA_HDMI) {
        uint32_t p = d->cea_phys_addr;
        f_cat(out, cap, " hdmi ");
        f_cat_u(out, cap, (p >> 12) & 15); f_cat(out, cap, ".");
        f_cat_u(out, cap, (p >> 8) & 15); f_cat(out, cap, ".");
        f_cat_u(out, cap, (p >> 4) & 15); f_cat(out, cap, ".");
        f_cat_u(out, cap, p & 15);
    }
    if (d->cea_flags & OS_DISPLAY_CEA_HF) f_cat(out, cap, " hf");
    if (d->cea_tmds_mhz) { f_cat(out, cap, " tmds "); f_cat_u(out, cap, d->cea_tmds_mhz); }
    if (d->cea_flags & OS_DISPLAY_CEA_HDR) f_cat(out, cap, " hdr");
    if (d->cea_flags & OS_DISPLAY_CEA_YCC444) f_cat(out, cap, " ycc444");
    if (d->cea_flags & OS_DISPLAY_CEA_YCC422) f_cat(out, cap, " ycc422");
    if (d->cea_y420_n) { f_cat(out, cap, " y420 "); f_cat_u(out, cap, d->cea_y420_n); }
    if (d->cea_flags & OS_DISPLAY_CEA_UNDERSCAN) f_cat(out, cap, " underscan");
    if (d->cea_dtd_n) { f_cat(out, cap, " dtd "); f_cat_u(out, cap, d->cea_dtd_n); }
    f_cat(out, cap, d->cea_tv ? " sink tv\n" : " sink monitor\n");
    return (int)f_len(out);
}

int plat_form_tv_safe_q(const os_display_info_t* d, const char **why) {
    static const int steps[3] = { 12, 8, 6 };
    int want, k, fw, fh;
    const char *r;
    if (!why) why = &r;
    if (!d) { *why = "no display"; return 0; }
    if (d->ui_hint_q) { *why = "pinned"; return 0; }
    if (d->cea_status != 0) { *why = "no cea"; return 0; }
    if (!d->cea_tv) { *why = "not a tv"; return 0; }
    /* no framebuffer yet (console boot): the desktop's mode */
    fw = d->fb_w ? (int)d->fb_w : PLAT_FORM_DESK_W;
    fh = d->fb_h ? (int)d->fb_h : PLAT_FORM_DESK_H;
    want = plat_form_tv_scale_q(d);
    for (k = 0; k < 3; k++) {
        int q = steps[k];
        if (q > want) continue;
        /* the desktop needs a usable logical screen */
        if (fw * 4 / q < 640 || fh * 4 / q < 360) continue;
        *why = q == want ? (want >= 12 ? "tv 4k" : want >= 8 ? "tv 1080p" : "tv 720p") : "stepped down for the framebuffer";
        return q;
    }
    *why = "screen too small";
    return 0;
}

int plat_form_tv_line(const os_display_info_t* d, int applied_q, char* out, int cap) {
    const char *why = "";
    int want, safe;
    if (!out || cap < 64) return 0;
    out[0] = 0;
    want = plat_form_tv_scale_q(d);
    if (want <= 0) return 0;
    safe = plat_form_tv_safe_q(d, &why);
    f_cat(out, cap, "display tv-hint scale_q "); f_cat_u(out, cap, (uint32_t)want);
    f_cat(out, cap, " safe_q "); f_cat_u(out, cap, (uint32_t)safe);
    f_cat(out, cap, " ("); f_cat(out, cap, why);
    if (applied_q > 0) { f_cat(out, cap, ", applied "); f_cat_u(out, cap, (uint32_t)applied_q); }
    else f_cat(out, cap, safe ? ", not applied: ui.cfg or user scale" : ", not applied");
    f_cat(out, cap, ")\n");
    if (!(d->cea_flags & OS_DISPLAY_CEA_UNDERSCAN))
        f_cat(out, cap, "display tv-hint overscan likely (no underscan bit): the desktop keeps a 2.5% margin per side clear\n");
    return (int)f_len(out);
}

int plat_form_tv_scale_q(const os_display_info_t* d) {
    uint32_t w;
    if (!d || d->cea_status != 0 || !d->cea_tv) return 0;
    /* the panel the TV says it has, else the current mode */
    w = d->cea_best_from == 1 ? d->cea_best_w : (d->native_w ? d->native_w : d->fb_w);
    if (w >= 3200) return 12;   /* 3x: a 4K TV seen from the sofa */
    if (w >= 1800) return 8;    /* 2x for 1080p */
    return 6;                   /* 1.5x for 720p and below */
}
