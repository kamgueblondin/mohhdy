/* wf_cssbg.h - Lot 90 (US-031): background and list marker images for the
 * reader sheet.
 *
 * The reader passed one background colour to the engine; images,
 * gradients and positions were left out, and list-style-image was ignored.
 * These functions check a value and give it back in one normal form
 * (lower case keywords, single spaces, ", " between layers, url() without
 * quotes) that the reader writes into the style attribute it hands to the
 * engine. The grammar is the part the engine draws:
 *   - an image layer is none, url(...), linear-gradient(...) or
 *     radial-gradient(...); at most WF_CSSBG_LAYERS layers, a gradient only
 *     in the last one (it paints under every image);
 *   - linear-gradient: an optional "to top|right|bottom|left" or an angle
 *     that is a multiple of 90deg (or a quarter turn), then 2 to 4 colour
 *     stops "colour [N%]" (0..100);
 *   - radial-gradient: an optional shape argument (circle or ellipse, an
 *     extent keyword, "at" and a position), then 2 to 4 stops; explicit
 *     radii are refused;
 *   - a position is one or two values (left, center, right, top, bottom,
 *     a length in px, a percentage, 0), given back as "X Y";
 *   - a size is cover, contain, auto, or one or two lengths (auto allowed);
 *   - a repeat is repeat-x, repeat-y, or one or two of repeat, no-repeat,
 *     space and round (two equal words, or repeat no-repeat, come back as
 *     the one-word form);
 *   - a url is 1..100 printable bytes without spaces, quotes, parentheses,
 *     backslash, ';', '<' or '>' (a quoted url loses its quotes).
 * The background shorthand is split into its longhands; attachment
 * (scroll, fixed, local) and box keywords (border-box, padding-box,
 * content-box) are accepted and not passed on. A colour is only allowed in
 * the last layer and is given back as text for the caller to check.
 * Freestanding, no heap. Host tested (test_lot90_bg*.c). */
#ifndef WF_CSSBG_H
#define WF_CSSBG_H
#include <stdint.h>

#define WF_CSSBG_LAYERS 4
#define WF_CSSBG_VAL 128          /* longest normal form, NUL included */
#define WF_CSSBG_URL 100          /* longest url inside url() */

typedef struct {
    char image[WF_CSSBG_VAL];
    char position[WF_CSSBG_VAL];
    char size[WF_CSSBG_VAL];
    char repeat[WF_CSSBG_VAL];
    char color[32];               /* "" when no layer gave one */
    int layers;
    /* 1 when a layer gave that part; 0: every layer has its initial
     * value (none, 0% 0%, auto, repeat) */
    uint8_t has_image, has_position, has_size, has_repeat;
} wf_cssbg_t;

/* Each returns the length of the normal form written to out (1..cap-1),
 * 0 when the value is not valid (out is then ""). */
int wf_cssbg_url(const char *v, int n, char *out, int cap);
int wf_cssbg_gradient(const char *v, int n, char *out, int cap);
int wf_cssbg_image(const char *v, char *out, int cap);
int wf_cssbg_position(const char *v, char *out, int cap);
int wf_cssbg_size(const char *v, char *out, int cap);
int wf_cssbg_repeat(const char *v, char *out, int cap);
/* list-style-image: none or one url() */
int wf_cssbg_list_image(const char *v, char *out, int cap);
/* 1 when the text is a colour the engine draws: #rgb, #rgba, #rrggbb,
 * #rrggbbaa, rgb()/rgba() with 3 or 4 numbers, transparent, or one of
 * the engine's fifteen names. */
int wf_cssbg_color(const char *v, int n);
/* The background shorthand: 1 and out filled, 0 invalid. */
int wf_cssbg_shorthand(const char *v, wf_cssbg_t *out);
#endif
