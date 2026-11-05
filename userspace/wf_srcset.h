/* wf_srcset.h - Lot 73: responsive image selection for the guest browser.
 *
 * <img srcset sizes> and <picture><source srcset sizes media type>: the
 * reduced page (wf_html) keeps one URL per image, chosen here as a
 * browser does for a viewport of vw CSS pixels and a device pixel ratio
 * of dpr100 / 100:
 *  - srcset: comma separated candidates "URL [Nx | Nw]" (no descriptor is
 *    1x). A width descriptor is a density of N / slot width, the slot width
 *    coming from sizes (default 100vw). The candidate with the smallest
 *    density >= dpr wins, else the largest one; the first one on a tie.
 *    A candidate with two descriptors or a bad one is skipped.
 *  - sizes: "(media) length, ..., length": the first entry whose media
 *    matches; lengths in px, vw, em / rem (16 px); a bad entry is skipped.
 *  - media: all, screen, (min-width: Npx), (max-width: Npx), joined by
 *    "and"; print and anything else do not match.
 *  - type: image/png, image/jpeg, image/gif, image/bmp, image/x-icon,
 *    image/vnd.microsoft.icon (what imgdec decodes), or no type.
 * Freestanding, no allocation, bounded loops. */
#ifndef WF_SRCSET_H
#define WF_SRCSET_H

#define WF_SRCSET_MAX_CAND 16
#define WF_SRCSET_DEFAULT_VW 640

/* the URL chosen from srcset into out (cap bytes); returns its length,
 * 0 when srcset has no usable candidate (out empty) */
int wf_srcset_pick(const char *srcset, const char *sizes, int vw, int dpr100, char *out, int cap);
/* slot width in CSS px from sizes (NULL or empty: vw) */
int wf_sizes_width(const char *sizes, int vw);
/* 1 when the media query list matches a screen of vw px */
int wf_media_match(const char *media, int vw);
/* 1 when an image of MIME type `type' (NULL or empty: unknown, accepted) is decoded */
int wf_image_type_ok(const char *type);

#endif
