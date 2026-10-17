#include "mobile_ui.h"

static const char* k_apps[MUI_APPS] = { "chat", "shell", "browser", "files", "p2p", "collab", "settings", "status" };
static const uint32_t k_colors[MUI_APPS] = { 0x2E7D32U, 0x263238U, 0x1565C0U, 0xEF6C00U,
                                             0x6A1B9AU, 0x00838FU, 0x546E7AU, 0xC62828U };

static int slen_mui(const char* s) { int n = 0; while (s[n]) n++; return n; }

const char* mui_app_name(int id) { return (id >= 0 && id < MUI_APPS) ? k_apps[id] : "?"; }

const char* mui_action_name(int a) {
    switch (a) {
    case MUI_ACT_OPEN: return "open";
    case MUI_ACT_BACK: return "back";
    case MUI_ACT_HOME: return "home";
    case MUI_ACT_RECENTS: return "recents";
    case MUI_ACT_SHADE_OPEN: return "shade-open";
    case MUI_ACT_SHADE_CLOSE: return "shade-close";
    default: return "none";
    }
}

static void add(mui_layout_t* l, int id, int x, int y, int w, int h) {
    mui_target_t* t;
    if (l->ntargets >= MUI_MAX_TARGETS) return;
    t = &l->t[l->ntargets++];
    t->id = id; t->x = x; t->y = y; t->w = w; t->h = h;
    if (w < l->min_target) l->min_target = w;
    if (h < l->min_target) l->min_target = h;
}

int mui_layout(int w, int h, mui_layout_t* l) {
    plat_layout_t pl;
    int i, gy, gh, nav_w;
    if (!l || plat_layout(w, h, &pl) != 0) return -1;
    l->w = w; l->h = h; l->form = pl.form; l->portrait = pl.portrait;
    l->ntargets = 0;
    l->min_target = 1 << 20;
    l->status_h = 24;
    l->nav_h = pl.form == PLAT_FORM_PHONE ? 56 : 48;
    if (l->nav_h < MUI_MIN_TARGET) l->nav_h = MUI_MIN_TARGET;
    /* columns: 4 on a portrait phone, 6 in landscape, 5/6 on a tablet;
     * reduced until every cell keeps a MUI_MIN_TARGET icon */
    l->cols = pl.form == PLAT_FORM_PHONE ? (pl.portrait ? 4 : 6) : (pl.portrait ? 5 : 6);
    if (pl.form == PLAT_FORM_DESKTOP) l->cols = 8;
    while (l->cols > 1 && w / l->cols < MUI_MIN_TARGET + 16) l->cols--;
    gy = l->status_h;
    gh = h - l->status_h - l->nav_h;
    if (gh < MUI_MIN_TARGET + 16) return -1;
    l->cell_w = w / l->cols;
    l->rows = (MUI_APPS + l->cols - 1) / l->cols;
    l->cell_h = gh / l->rows;
    if (l->cell_h > l->cell_w + 24) l->cell_h = l->cell_w + 24;   /* square-ish cells */
    if (l->cell_h < MUI_MIN_TARGET + 16) return -1;
    l->icon = (l->cell_w < l->cell_h ? l->cell_w : l->cell_h) - 16;
    if (l->icon > 96) l->icon = 96;
    for (i = 0; i < MUI_APPS; i++) {
        int c = i % l->cols, r = i / l->cols;
        int x = c * l->cell_w + (l->cell_w - l->icon) / 2;
        int y = gy + r * l->cell_h + 8;
        add(l, i, x, y, l->icon, l->icon);
    }
    nav_w = w / 3;
    add(l, MUI_NAV_BACK, 0, h - l->nav_h, nav_w, l->nav_h);
    add(l, MUI_NAV_HOME, nav_w, h - l->nav_h, nav_w, l->nav_h);
    add(l, MUI_NAV_RECENTS, 2 * nav_w, h - l->nav_h, w - 2 * nav_w, l->nav_h);
    return 0;
}

int mui_hit(const mui_layout_t* l, int x, int y) {
    int i;
    if (!l) return -1;
    for (i = 0; i < l->ntargets; i++) {
        const mui_target_t* t = &l->t[i];
        if (x >= t->x && x < t->x + t->w && y >= t->y && y < t->y + t->h) return t->id;
    }
    return -1;
}

void mui_init(mui_state_t* s) {
    int i;
    if (!s) return;
    s->screen = MUI_SCREEN_HOME; s->app = -1; s->depth = 0; s->shade = 0;
    s->opens = 0U; s->backs = 0U;
    for (i = 0; i < MUI_STACK; i++) s->stack[i] = -1;
}

static int go_back(mui_state_t* s) {
    s->backs++;
    if (s->screen == MUI_SCREEN_RECENTS || s->screen == MUI_SCREEN_HOME) {
        s->screen = s->app >= 0 && s->screen == MUI_SCREEN_RECENTS ? MUI_SCREEN_APP : MUI_SCREEN_HOME;
        return MUI_ACT_BACK;
    }
    if (s->depth > 0) { s->app = s->stack[--s->depth]; s->stack[s->depth] = -1; }
    else { s->screen = MUI_SCREEN_HOME; s->app = -1; }
    return MUI_ACT_BACK;
}

static int open_app(mui_state_t* s, int id) {
    if (s->screen == MUI_SCREEN_APP && s->app >= 0 && s->app != id) {
        if (s->depth == MUI_STACK) {           /* drop the oldest */
            int i;
            for (i = 1; i < MUI_STACK; i++) s->stack[i - 1] = s->stack[i];
            s->depth--;
        }
        s->stack[s->depth++] = s->app;
    }
    s->screen = MUI_SCREEN_APP; s->app = id; s->opens++;
    return MUI_ACT_OPEN;
}

int mui_tap(mui_state_t* s, const mui_layout_t* l, int x, int y) {
    int id;
    if (!s || !l) return MUI_ACT_NONE;
    if (s->shade) {                            /* any tap closes the shade */
        s->shade = 0;
        return MUI_ACT_SHADE_CLOSE;
    }
    id = mui_hit(l, x, y);
    if (id == MUI_NAV_BACK) return go_back(s);
    if (id == MUI_NAV_HOME) { s->screen = MUI_SCREEN_HOME; return MUI_ACT_HOME; }
    if (id == MUI_NAV_RECENTS) { s->screen = MUI_SCREEN_RECENTS; return MUI_ACT_RECENTS; }
    /* app icons are only live on the home screen */
    if (id >= 0 && id < MUI_APPS && s->screen == MUI_SCREEN_HOME) return open_app(s, id);
    return MUI_ACT_NONE;
}

int mui_gesture(mui_state_t* s, const mui_layout_t* l, const plat_point_t* p, int n) {
    int g, i, first = -1;
    if (!s || !l || !p || n < 2) return MUI_ACT_NONE;
    for (i = 0; i < n; i++) if (p[i].down) { first = i; break; }
    if (first < 0) return MUI_ACT_NONE;
    g = plat_gesture(p, n);
    if (g == PLAT_G_TAP) return mui_tap(s, l, p[first].x, p[first].y);
    if (g == PLAT_G_SWIPE_DOWN && p[first].y < l->status_h && !s->shade) { s->shade = 1; return MUI_ACT_SHADE_OPEN; }
    if (g == PLAT_G_SWIPE_UP && s->shade) { s->shade = 0; return MUI_ACT_SHADE_CLOSE; }
    if (g == PLAT_G_SWIPE_UP && p[first].y >= l->h - l->nav_h) { s->screen = MUI_SCREEN_HOME; return MUI_ACT_HOME; }
    if (g == PLAT_G_SWIPE_RIGHT && p[first].x < 16) return go_back(s);
    return MUI_ACT_NONE;
}

/* Pixel model of one frame: status bar, home grid or app screen, nav bar,
 * shade over the top third. */
uint32_t mui_pixel(const mui_layout_t* l, const mui_state_t* s, int x, int y) {
    int i;
    if (!l || !s || x < 0 || y < 0 || x >= l->w || y >= l->h) return 0U;
    if (s->shade && y < l->h / 3) return y >= l->h / 3 - 2 ? 0xFFFFFFU : 0x37474FU;
    if (y < l->status_h) return 0x101010U;
    if (y >= l->h - l->nav_h) {
        int c = x / (l->w / 3);
        int cx = (c * (l->w / 3)) + l->w / 6;
        int cy = l->h - l->nav_h / 2;
        int dx = x - cx, dy = y - cy;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        return (dx < 10 && dy < 10) ? 0xE0E0E0U : 0x000000U;
    }
    if (s->screen == MUI_SCREEN_APP && s->app >= 0) {
        if (y < l->status_h + 48) return k_colors[s->app];          /* app bar */
        return 0xFAFAFAU;
    }
    if (s->screen == MUI_SCREEN_RECENTS) {
        int card = l->status_h + 24 + s->depth * 12;
        return (x > 24 && x < l->w - 24 && y > card && y < card + l->h / 3) ? 0xCFD8DCU : 0x212121U;
    }
    for (i = 0; i < l->ntargets; i++) {
        const mui_target_t* t = &l->t[i];
        if (t->id < MUI_APPS && x >= t->x && x < t->x + t->w && y >= t->y && y < t->y + t->h)
            return k_colors[t->id];
    }
    return 0x0D47A1U;                                               /* wallpaper */
}

uint32_t mui_frame_hash(const mui_layout_t* l, const mui_state_t* s) {
    uint32_t h = 2166136261U;
    int x, y;
    if (!l || !s) return 0U;
    for (y = 0; y < l->h; y++)
        for (x = 0; x < l->w; x++) {
            uint32_t p = mui_pixel(l, s, x, y);
            h = (h ^ (p & 0xFFU)) * 16777619U;
            h = (h ^ ((p >> 8) & 0xFFU)) * 16777619U;
            h = (h ^ ((p >> 16) & 0xFFU)) * 16777619U;
        }
    return h;
}

/* ------------------------------------------- Lot 9: US-077 display list */
static int dl_rect(os_fb_web_item_t* o, int n, int max, int x, int y, int w, int h, uint32_t c) {
    os_fb_web_item_t* it;
    if (n >= max || w <= 0 || h <= 0) return n;
    it = &o[n];
    it->kind = OS_FB_WEB_RECT; it->bold = 0;
    it->x = (int16_t)x; it->y = (int16_t)y; it->w = (uint16_t)w; it->h = (uint16_t)h;
    it->pad = 0; it->color = c; it->text[0] = 0;
    return n + 1;
}

static int dl_text(os_fb_web_item_t* o, int n, int max, int x, int y, const char* t, uint32_t c) {
    os_fb_web_item_t* it;
    int k = 0;
    if (n >= max) return n;
    it = &o[n];
    it->kind = OS_FB_WEB_TEXT; it->bold = 0;
    it->x = (int16_t)x; it->y = (int16_t)y; it->h = 8; it->pad = 0; it->color = c;
    while (t[k] && k < OS_FB_WEB_TEXT_LEN - 1) { it->text[k] = t[k]; k++; }
    it->text[k] = 0;
    it->w = (uint16_t)(k * 8);
    return n + 1;
}

int mui_display_list(const mui_layout_t* l, const mui_state_t* s, os_fb_web_item_t* o, int max) {
    int n = 0, i, w, h, third;
    if (!l || !s || !o || max <= 0) return -1;
    w = l->w; h = l->h; third = w / 3;
    if (s->screen == MUI_SCREEN_APP && s->app >= 0) {
        n = dl_rect(o, n, max, 0, l->status_h, w, h - l->status_h - l->nav_h, 0xFAFAFAU);
        n = dl_rect(o, n, max, 0, l->status_h, w, 48, k_colors[s->app]);
        n = dl_text(o, n, max, 16, l->status_h + 20, mui_app_name(s->app), 0xFFFFFFU);
        n = dl_text(o, n, max, 16, l->status_h + 72, "app surface (simule)", 0x212121U);
    } else if (s->screen == MUI_SCREEN_RECENTS) {
        int card = l->status_h + 24 + s->depth * 12;
        n = dl_rect(o, n, max, 0, l->status_h, w, h - l->status_h - l->nav_h, 0x212121U);
        n = dl_rect(o, n, max, 25, card + 1, w - 49, h / 3 - 1, 0xCFD8DCU);
        if (s->app >= 0) n = dl_text(o, n, max, 33, card + 9, mui_app_name(s->app), 0x212121U);
    } else {
        n = dl_rect(o, n, max, 0, l->status_h, w, h - l->status_h - l->nav_h, 0x0D47A1U);
        for (i = 0; i < l->ntargets; i++) {
            const mui_target_t* t = &l->t[i];
            if (t->id >= MUI_APPS) continue;
            n = dl_rect(o, n, max, t->x, t->y, t->w, t->h, k_colors[t->id]);
            n = dl_text(o, n, max, t->x + t->w / 2 - 4 * slen_mui(mui_app_name(t->id)),
                        t->y + t->h + 2, mui_app_name(t->id), 0xFFFFFFU);
        }
    }
    n = dl_rect(o, n, max, 0, 0, w, l->status_h, 0x101010U);
    n = dl_text(o, n, max, 8, 8, "mohhdy mobile", 0xFFFFFFU);
    n = dl_rect(o, n, max, 0, h - l->nav_h, w, l->nav_h, 0x000000U);
    for (i = 0; i < 3; i++) {
        int cx = i * third + w / 6, cy = h - l->nav_h / 2;
        n = dl_rect(o, n, max, cx - 9, cy - 9, 19, 19, 0xE0E0E0U);
    }
    if (s->shade) {
        n = dl_rect(o, n, max, 0, 0, w, h / 3 - 2, 0x37474FU);
        n = dl_rect(o, n, max, 0, h / 3 - 2, w, 2, 0xFFFFFFU);
        n = dl_text(o, n, max, 16, 32, "notifications", 0xFFFFFFU);
    }
    return n;
}
