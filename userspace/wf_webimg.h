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
 * reused by URL. Nothing is allocated; the state is caller owned.
 *
 * Lot 73: animated GIFs. When a GIF has more than one frame, every frame
 * is composed (imgdec_anim_*: delays, disposal, loop count), scaled to
 * the slot size and kept in a frame store (WF_WEBIMG_ANIM_MAX animations,
 * WF_WEBIMG_ANIM_FRAMES frames each, WF_WEBIMG_ANIM_PIXELS pixels in all).
 * wf_webimg_tick(now) copies the frame that is due into the atlas slot and
 * bumps the atlas generation, so the compositor redraws it. Delays are
 * GIF centiseconds (the 100 Hz system ticks); a delay of 0 or 1 is played
 * as 10 (as browsers do). Loop count: no extension plays once, 0 forever,
 * N plays N + 1 times; the animation then stays on its last frame. A GIF
 * whose frames do not all fit keeps the frames that fit (stats anim_cut). */
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
#define WF_WEBIMG_ANIM_MAX 4
#define WF_WEBIMG_ANIM_FRAMES 32
#define WF_WEBIMG_ANIM_PIXELS (192 * 1024)
#define WF_WEBIMG_ANIM_MIN_DELAY 10   /* cs: delays 0 and 1 are played as this */

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
    int animated;     /* Lot 73: GIFs kept with all their frames */
    int anim_frames;  /* frames stored for them */
    int anim_cut;     /* animations kept with fewer frames (store full, too big) */
    char last_err[96];
} wf_webimg_stats_t;

typedef struct {
    uint8_t slot;                     /* atlas slot (0: unused entry) */
    uint8_t nframes, cur, stopped;
    int16_t loop;                     /* -1 play once, 0 forever, N: N more plays */
    uint16_t plays;                   /* complete plays so far */
    uint16_t w, h;                    /* stored frame size (the slot size) */
    uint16_t delay[WF_WEBIMG_ANIM_FRAMES];   /* cs, after the minimum */
    uint32_t off[WF_WEBIMG_ANIM_FRAMES];     /* pixel offset in anim_rgb */
    uint32_t due;                     /* tick of the next frame change */
    uint8_t started;
} wf_webimg_anim_t;

typedef struct {
    os_fb_webimg_t atlas;
    char url[OS_FB_WEBIMG_SLOTS][WF_WEBIMG_URL];
    uint16_t disp_w[OS_FB_WEBIMG_SLOTS], disp_h[OS_FB_WEBIMG_SLOTS];   /* display size (stored pixels may be fewer) */
    uint32_t used;                    /* pixels allocated in atlas.rgb */
    uint32_t gen;
    wf_webimg_anim_t anim[WF_WEBIMG_ANIM_MAX];
    uint32_t anim_used;               /* pixels used in anim_rgb */
    uint32_t anim_changes;            /* frames copied into the atlas */
    uint8_t anim_rgb[WF_WEBIMG_ANIM_PIXELS * 3];
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
/* Lot 73: advances the animations to tick now (100 Hz); 1 when a frame
 * changed in the atlas (atlas.gen bumped), else 0. */
int wf_webimg_tick(wf_webimg_t* w, uint32_t now);
/* Lot 73: one line per animation ("anim slot K frames F cur C plays P
 * loop L delays D1,D2,.. stopped S"); returns the length. */
int wf_webimg_anim_text(const wf_webimg_t* w, char* out, int cap);
/* Size of a slot for the web engine (we_set_image_atlas callback). */
int wf_webimg_dims(void* w, int slot, int* width, int* height);
/* Display box of an image: the attributes when given (one of them keeps
 * the ratio of nat_w x nat_h), else the natural size, then shrunk to
 * WF_WEBIMG_BOX_W x WF_WEBIMG_BOX_H keeping the ratio. */
void wf_webimg_box(int attr_w, int attr_h, int nat_w, int nat_h, int* w, int* h);
#endif
