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
#endif
