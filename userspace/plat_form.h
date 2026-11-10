/* plat_form.h - Lot 68 (US-080): form-factor plan for the real desktop.
 * plat_layout() (platform.c) only classified a size; this turns a screen
 * (pixels, optional diagonal in tenths of an inch) into the settings the
 * desktop applies: text scale (quarters, include/os_ui_scale.h), touch
 * target size, chat window mode, dock side, menu style and the CSS page
 * width of the web view. `form-factor apply` sets the shared ui-scale, so
 * the console and the GUI change together. Pure integer code, host
 * testable; no heap. Lot 69: the physical size comes from the EDID of
 * the QEMU std VGA (SYS_DISPLAY_INFO) when present; without it the plan
 * uses pixels only, or the diagonal the operator gives. */
#ifndef PLAT_FORM_H
#define PLAT_FORM_H
#include <stdint.h>

enum { PF_PHONE = 1, PF_TABLET = 2, PF_DESKTOP = 3, PF_TV = 4 };
enum { PF_CHAT_FULL = 0, PF_CHAT_CENTER = 1, PF_CHAT_FLOAT = 2 };
enum { PF_DOCK_BOTTOM = 0, PF_DOCK_LEFT = 1 };

typedef struct {
    int w, h;              /* pixels */
    int diag_tenths;       /* diagonal, 0.1 inch; 0 = unknown */
    int ppi;               /* pixels per inch, 0 = unknown */
    int form;              /* PF_* */
    int portrait;
    int ui_q;              /* recommended text scale, quarters (4 = 1x) */
    int glyph_px;          /* 8x8 cell height at that scale */
    int glyph_mm10;        /* glyph height in 0.1 mm, 0 if ppi unknown */
    int touch_px;          /* minimum hit target (44 pt at the ppi) */
    int chat_mode;         /* PF_CHAT_* */
    int dock;              /* PF_DOCK_* */
    int compact_menu;      /* short menu labels */
    int columns;           /* text columns at ui_q */
    int rows;
    int web_css_w;         /* page layout width at ui_q */
    int forced;            /* form given by the operator */
} plat_form_plan_t;

/* Plan for a screen. form = 0 picks from size (and ppi when known). */
int plat_form_plan(int w, int h, int diag_tenths, int form, plat_form_plan_t* out);
const char* plat_form_kind_name(int form);
int plat_form_kind_parse(const char* s);
const char* plat_form_chat_name(int mode);
/* one line "form-factor ok ..." */
int plat_form_report(const plat_form_plan_t* p, char* out, int cap);
/* integer square root (diagonal in pixels) */
uint32_t plat_form_isqrt(uint32_t v);
#include "../include/os_syscalls.h"
/* Lot 69 (US-080): "display physical ..." line from SYS_DISPLAY_INFO;
 * sets *diag_tenths (0 when unknown). 0 known, 1 unknown, -1 args. */
int plat_form_physical(const os_display_info_t* d, int* diag_tenths, char* out, int cap);
/* Lot 85: a second line for the EDID extras SYS_DISPLAY_INFO now carries.
 * Writes nothing and returns 0 when every extra field is zero, so a
 * display with only the old size line stays on that one line. */
int plat_form_edid_detail(const os_display_info_t* d, char* out, int cap);
/* Lot 87 (US-080): the CEA-861 line of display-info.
 * "display cea rev 3 svd 6 vics 16,4,... best 1920x1080@60 (native) audio
 *  2ch hdmi 1.0.0.0 tmds 340 hdr ycc444 underscan sink tv|monitor"
 * or "display cea none" / "display cea not-cea tag 0xNN" /
 * "display cea refused". Returns the length. */
int plat_form_cea_line(const os_display_info_t* d, char* out, int cap);
/* Lot 87: the UI scale hint for a television sink (10-foot viewing):
 * 0 when the sink is not a TV or nothing is known, else a quarter-step
 * scale (4 = 1.0) at least 6 (1.5x) and larger for a 4K panel. */
int plat_form_tv_scale_q(const os_display_info_t* d);
/* Lot 88 (US-080): the TV scale the desktop may take on its own, or 0.
 * Only for a parsed CEA block that says TV, never when the test harness
 * pinned a scale (ui_hint_q), and stepped down (3x, 2x, 1.5x) until the
 * logical screen keeps at least 640 x 360 CSS px. why gets a short
 * reason ("tv 1080p", "not a tv", "pinned", "screen too small", ...). */
int plat_form_tv_safe_q(const os_display_info_t* d, const char **why);
/* the mode the gui command sets (kernel GFX_FB_WIDTH x GFX_FB_HEIGHT): the
 * TV hint is computed before the desktop starts, while fb_w is still 0 */
#define PLAT_FORM_DESK_W 1024
#define PLAT_FORM_DESK_H 768
/* Lot 88 (US-080): "display tv-hint scale_q 8 safe_q 6 (...)\n" plus an
 * overscan note for a TV without the underscan bit; length, 0 none. */
int plat_form_tv_line(const os_display_info_t* d, int applied_q, char* out, int cap);
#endif
