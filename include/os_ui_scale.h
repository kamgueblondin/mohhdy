/* os_ui_scale.h - Lot 68: text / UI scale shared by the kernel desktop
 * renderer, the Ring 3 OS-UI and the host tests. A scale is kept in
 * quarters: 4 = 1x, 6 = 1.5x, 8 = 2x, 12 = 3x. Header only, no libc. */
#ifndef OS_UI_SCALE_H
#define OS_UI_SCALE_H
#include <stdint.h>

#define OS_UI_Q1 4            /* 1x */
#define OS_UI_Q_MIN 4
#define OS_UI_Q_MAX 16        /* 4x */
#define OS_UI_Q_AUTO 0        /* ui_scale_q = 0: chosen from the screen */
/* Window title bars and the menu bar have fixed heights (30-40 px): their
 * text stops at 2x so it stays inside the bar. */
#define OS_UI_Q_CHROME_MAX 8

/* zoom slots of os_fb_scene_t.zoom_q */
#define OS_UI_Z_WEB 0
#define OS_UI_Z_CHAT 1
#define OS_UI_Z_SHELL 2
#define OS_UI_Z_PANE 3
#define OS_UI_Z_IDE 4
#define OS_UI_Z_APP0 5        /* + app slot (0..OS_FB_APPS-1) */
#define OS_UI_Z_COUNT 12

static inline int os_ui_q_clamp(int q) {
    return q < OS_UI_Q_MIN ? OS_UI_Q_MIN : q > OS_UI_Q_MAX ? OS_UI_Q_MAX : q;
}

/* Default scale from the framebuffer. Lot 69: 1x 8x8 glyphs proved too
 * small even on 1024x768 (owner report), so 1.5x from a 760 px side. The shorter side decides so a tall or wide
 * screen is not over scaled. */
static inline int os_ui_auto_q(int w, int h) {
    int s = w < h ? w : h;
    (void)w;
    if (s >= 2000) return 12;   /* 4K class: 3x */
    if (s >= 1400) return 8;    /* 2560x1440: 2x */
    if (s >= 760) return 6;     /* 1024x768 .. 1920x1080: 1.5x (Lot 69: 1x was too small) */
    return 4;                   /* 800x600 and below */
}

/* Effective scale: global (or auto) times the window zoom (4 = none). */
static inline int os_ui_effective_q(int ui_q, int zoom_q, int w, int h) {
    int g = ui_q == OS_UI_Q_AUTO ? os_ui_auto_q(w, h) : os_ui_q_clamp(ui_q);
    int z = zoom_q <= 0 ? OS_UI_Q1 : zoom_q;
    return os_ui_q_clamp((g * z + 2) / 4);
}

/* Ctrl+'+' / Ctrl+'-' steps */
static inline int os_ui_q_step(int q, int dir) {
    static const uint8_t steps[] = {4, 5, 6, 7, 8, 10, 12, 14, 16};
    int i, n = (int)sizeof(steps);
    q = os_ui_q_clamp(q);
    if (dir > 0) { for (i = 0; i < n; i++) if (steps[i] > q) return steps[i]; return steps[n - 1]; }
    for (i = n - 1; i >= 0; i--) if (steps[i] < q) return steps[i];
    return steps[0];
}

/* Glyph metrics of the 8x8 font at a scale (cell 8 wide, line 10 high). */
static inline int os_ui_px(int px, int q) { return (px * q + 2) / 4; }
static inline int os_ui_char_w(int q) { return os_ui_px(8, q); }
static inline int os_ui_line_h(int q) { return os_ui_px(10, q); }

/* "1", "1.5", "2x", "150%", "auto" -> quarters; -1 if invalid. */
static inline int os_ui_q_parse(const char *s) {
    int ip = 0, frac = -1, digits = 0, pct = 0, i = 0, q;
    if (!s || !s[0]) return -1;
    if (s[0] == 'a' && s[1] == 'u' && s[2] == 't' && s[3] == 'o' && !s[4]) return OS_UI_Q_AUTO;
    while (s[i] >= '0' && s[i] <= '9') { ip = ip * 10 + (s[i] - '0'); i++; digits++; if (digits > 4) return -1; }
    if (!digits) return -1;
    if (s[i] == '.' || s[i] == ',') {
        i++;
        if (s[i] < '0' || s[i] > '9') return -1;
        frac = s[i] - '0';
        i++;
        if (s[i] >= '0' && s[i] <= '9') { frac = frac * 10 + (s[i] - '0'); i++; } else frac *= 10;
        while (s[i] >= '0' && s[i] <= '9') i++;
    }
    if (s[i] == '%') { pct = 1; i++; }
    else if (s[i] == 'x' || s[i] == 'X') i++;
    if (s[i]) return -1;
    if (pct) { if (frac >= 0) return -1; q = (ip * 4 + 50) / 100; }
    else q = ip * 4 + ((frac < 0 ? 0 : frac) * 4 + 50) / 100;
    if (q < OS_UI_Q_MIN || q > OS_UI_Q_MAX) return -1;
    return q;
}

/* quarters -> "1.5x" (buf >= 8) */
static inline void os_ui_q_format(int q, char *buf) {
    int i = 0, ip = q / 4, rem = (q % 4) * 25;
    if (ip >= 10) buf[i++] = (char)('0' + ip / 10);
    buf[i++] = (char)('0' + ip % 10);
    if (rem) {
        buf[i++] = '.';
        buf[i++] = (char)('0' + rem / 10);
        if (rem % 10) buf[i++] = (char)('0' + rem % 10);
    }
    buf[i++] = 'x';
    buf[i] = 0;
}
#endif
