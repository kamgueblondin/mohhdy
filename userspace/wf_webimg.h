/* wf_webimg.h - Lot 72: images of the guest browser.
 *
 * A section of a reduced page (wf_html with images on, wf_section) holds
 * <img src="https://..." width height alt>. wf_webimg_section fetches each
 * image through a caller transport (wf_fetch in the guest, a table in the
 * host tests), decodes it in Ring 3 (imgdec: PNG, JPEG baseline and
 * progressive, GIF, BMP, ICO), scales it to its display box and stores the
 * pixels in the web atlas (os_fb_webimg_t) that the compositor draws. The
 * tag is rewritten to <img src="mohhdy-img:K" width height> for the web
 * engine (webeng.h WE_ATLAS_SCHEME); an image that fails becomes its alt
 * text in brackets. Slot 0 holds the favicon (wf_webimg_favicon).
 *
 * Bounds: WF_WEBIMG_SRC_MAX bytes per file, WF_WEBIMG_SCRATCH decode
 * bytes (large JPEGs are decoded at 1/2..1/8, imgdec_open_scaled),
 * WF_WEBIMG_FETCH_MAX fetches per section, OS_FB_WEBIMG_SLOTS - 1 images
 * and OS_FB_WEBIMG_PIXELS stored pixels. Images already in the atlas are
 * reused by URL. Nothing is allocated; the state is caller owned. */
#ifndef WF_WEBIMG_H
#define WF_WEBIMG_H
#include <stdint.h>
#include "../include/os_syscalls.h"

#define WF_WEBIMG_SRC_MAX (512 * 1024)
#define WF_WEBIMG_SCRATCH (1024 * 1024)
#define WF_WEBIMG_FETCH_MAX 8
#define WF_WEBIMG_BOX_W 480        /* largest display box, page pixels */
#define WF_WEBIMG_BOX_H 360
#define WF_WEBIMG_URL 256
#define WF_WEBIMG_FAVICON_PX 16

typedef struct {
    void* ctx;
    /* GET url into buf; returns the body length (> cap: cut), or < 0 */
    int (*fetch)(void* ctx, const char* url, uint8_t* buf, int cap);
} wf_webimg_io_t;

typedef struct {
    int found;        /* <img> in the section */
    int shown;        /* rewritten to an atlas slot */
    int fetched;      /* network fetches done */
    int reused;       /* already in the atlas */
    int failed;       /* fetch, format or decode error */
    int skipped;      /* fetch budget, slot or pixel budget */
    int scaled;       /* JPEG decoded below full size */
    char last_err[96];
} wf_webimg_stats_t;

typedef struct {
    os_fb_webimg_t atlas;
    char url[OS_FB_WEBIMG_SLOTS][WF_WEBIMG_URL];
    uint16_t disp_w[OS_FB_WEBIMG_SLOTS], disp_h[OS_FB_WEBIMG_SLOTS];   /* display size (stored pixels may be fewer) */
    uint32_t used;                    /* pixels allocated in atlas.rgb */
    uint32_t gen;
    uint8_t src[WF_WEBIMG_SRC_MAX];
    uint8_t scratch[WF_WEBIMG_SCRATCH];
} wf_webimg_t;

void wf_webimg_reset(wf_webimg_t* w);
/* Rewrites the section in[0..n) into out (cap bytes, NUL-terminated),
 * filling the atlas. Returns the output length or -1 (out too small). */
int wf_webimg_section(wf_webimg_t* w, const char* in, int n, char* out, int cap, const wf_webimg_io_t* io,
                      wf_webimg_stats_t* st);
/* Favicon of a page into slot 0 (16x16): icon_url when given, else
 * /favicon.ico of page_url. 0 when shown, < 0 otherwise (slot cleared). */
int wf_webimg_favicon(wf_webimg_t* w, const char* page_url, const char* icon_url, const wf_webimg_io_t* io,
                      wf_webimg_stats_t* st);
/* Size of a slot for the web engine (we_set_image_atlas callback). */
int wf_webimg_dims(void* w, int slot, int* width, int* height);
/* Display box of an image: the attributes when given (one of them keeps
 * the ratio of nat_w x nat_h), else the natural size, then shrunk to
 * WF_WEBIMG_BOX_W x WF_WEBIMG_BOX_H keeping the ratio. */
void wf_webimg_box(int attr_w, int attr_h, int nat_w, int nat_h, int* w, int* h);
#endif
