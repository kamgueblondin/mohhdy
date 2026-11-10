/* osui_frame.h - Lot 88 (US-081): the desktop frame pacer.
 *
 * The desktop loop wakes about ten times a second when no key comes
 * (the kernel getc gives up after about 10 ticks) and used to fill and
 * blit a whole 1024x768 scene each time. The pacer lets an idle wake
 * paint only when the energy refresh budget (plat_energy refresh_hz,
 * which the idle dim halves and then quarters) allows it. A key or a
 * window event always paints at once. Times are milliseconds from the
 * caller's tick source; differences are unsigned so a wrap is harmless.
 * No heap, host testable. */
#ifndef OSUI_FRAME_H
#define OSUI_FRAME_H
#include <stdint.h>

typedef struct {
    int started;
    uint32_t last_draw_ms;    /* last painted frame */
    uint32_t last_input_ms;   /* last key or window event */
    uint32_t hz;              /* budget of the last call, 0 = unpaced */
    int dim;                  /* idle dim level of the last call */
    uint32_t drawn;           /* every painted frame */
    uint32_t skipped;         /* idle wakes that did not paint */
    uint32_t input_frames;    /* frames painted for an input */
    uint32_t idle_drawn;      /* since the last input */
    uint32_t idle_skipped;    /* since the last input */
    uint32_t wakes;           /* inputs that ended a dim */
} osui_frame_t;

void osui_frame_init(osui_frame_t *f, uint32_t now_ms);
/* Milliseconds since the last input (0 before the first call). */
uint32_t osui_frame_idle_ms(const osui_frame_t *f, uint32_t now_ms);
/* One loop pass: input is 1 for a key or event. hz is the budget (0 = paint
 * every pass), dim the energy dim level. Returns 1 when the pass paints. */
int osui_frame_due(osui_frame_t *f, uint32_t now_ms, int input, uint32_t hz, int dim);
/* Frames painted per second over the idle time since the last input,
 * in tenths (73 = 7.3 fps); 0 when less than one second passed. */
uint32_t osui_frame_idle_fps10(const osui_frame_t *f, uint32_t now_ms);
/* "frames drawn D skipped S input I idle_drawn A idle_skipped B" */
int osui_frame_report(const osui_frame_t *f, char *out, int cap);
#endif
