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

#define OSUI_FRAME_LEVELS 3

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
    /* Lot 89: what the idle paints cost. Per dim level (0 awake,
     * 1 dim, 2 deep), since the last input: idle time spent at that level
     * (wall) and time spent filling and blitting idle frames there
     * (paint), with the frame count. cost_ms / cost_frames is the running
     * cost of one frame (every frame, input ones included). */
    uint32_t last_call_ms;
    uint32_t wall_ms[OSUI_FRAME_LEVELS];
    uint32_t paint_ms[OSUI_FRAME_LEVELS];
    uint32_t paint_frames[OSUI_FRAME_LEVELS];
    uint32_t cost_ms;
    uint32_t cost_frames;
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

/* Lot 89 (US-081): the measured cost of painting. The caller
 * reads its tick source before and after one fill and blit and passes
 * both (milliseconds; an end before the start counts as 0). idle is 1 when
 * the frame was an idle one (not a key, event or new content); such a
 * frame is charged to the dim level of the last osui_frame_due call. */
void osui_frame_cost(osui_frame_t *f, uint32_t start_ms, uint32_t end_ms, int idle);
/* Average cost of one frame in microseconds (0 before the first frame). */
uint32_t osui_frame_cost_us(const osui_frame_t *f);
/* Paint time not spent since the last input, estimated: idle wakes
 * skipped times the average frame cost, in milliseconds. */
uint32_t osui_frame_saved_ms(const osui_frame_t *f);
/* Share of the idle time at one dim level spent painting, in thousandths
 * (0 when no time was spent at that level; at most 1000). */
uint32_t osui_frame_busy_permille(const osui_frame_t *f, int level);
/* "paint_ms A B C wall_ms X Y Z busy_permille P Q R cost_us U saved_ms S" */
int osui_frame_cost_report(const osui_frame_t *f, char *out, int cap);
#endif
