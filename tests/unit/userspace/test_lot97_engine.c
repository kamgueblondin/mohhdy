/* Lot 97 (US-031): the engine paints a repeating gradient whose period is
 * 0 in its average colour, paints a gradient along an axis per pixel run,
 * and paints cells in a vertical-rl box from the physical left with a
 * repeating radial centre mirrored. Hand cases next to the generated
 * test_lot97_cells.c; expected values follow docs/lot97_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

#define TEST_ASSERT_EQUAL_HEX32(e, a) TEST_ASSERT_EQUAL((long)(e), (long)(a))

static we_doc_t g_doc;
static char g_html[2048], g_out[4096];

static int diag_count(const char *what) {
    int i, n = 0;
    for (i = 0; i < (int)g_doc.ndiag; i++) if (strstr(g_doc.diag[i], what)) n++;
    return n;
}

static const char *items(void) {
    int i, n = 0;
    g_out[0] = 0;
    for (i = 0; i < (int)g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind != OS_FB_WEB_RECT) continue;
        n += snprintf(g_out + n, sizeof(g_out) - (size_t)n, "%s%d %d %d %d %08X", n ? ";" : "", it->x, it->y,
                      (int)it->w, (int)it->h, (unsigned)it->color);
    }
    return g_out;
}

static int rects(void) {
    int i, n = 0;
    for (i = 0; i < (int)g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_RECT) n++;
    return n;
}

static void render(const char *body) {
    snprintf(g_html, sizeof(g_html), "<html><body style='margin:0'>%s</body></html>", body);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, g_html, (int)strlen(g_html), 320));
}

static const we_node_t *node(const char *id) {
    int idx = we_find_id(&g_doc, id);
    return idx >= 0 ? &g_doc.nodes[idx] : 0;
}

static long colour_at(int x, int y) {
    int i;
    for (i = 0; i < (int)g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind == OS_FB_WEB_RECT && x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h)
            return (long)it->color;
    }
    return -1;
}

/* the parsed gradient of one div: "dir angle repeat count c0 c1" */
static const char *parsed(const char *bg) {
    const we_node_t *n;
    snprintf(g_out, sizeof(g_out), "<div id=a style='width:40px;height:20px;background:%s'></div>", bg);
    render(g_out);
    n = node("a");
    if (!n) return "NO NODE";
    snprintf(g_out, sizeof(g_out), "%s %d %d %d %08X %08X",
             n->grad_dir == WE_GRAD_ANGLE ? "ANGLE" : n->grad_dir == WE_GRAD_RADIAL ? "RADIAL" : n->grad_dir ? "OTHER" : "NONE",
             n->grad_angle * 100 + n->grad_frac, n->grad_repeat, n->grad_count, (unsigned)n->grad_color[0],
             (unsigned)n->grad_color[1]);
    return g_out;
}

static void test_average(void) {
    /* two stops: weights 1 and 1, half up */
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 00800080 00800080", parsed("repeating-linear-gradient(red, blue 0%)"));
    /* three stops: weights 1, 2, 1 */
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 00408040 00408040", parsed("repeating-linear-gradient(red, lime 0%, blue 0%)"));
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 002B002B 002B002B",
                             parsed("repeating-linear-gradient(red 50%, #000000 50%, #000000 40%, blue 10%)"));
    /* premultiplied: a transparent stop weighs nothing in the colour, half
     * in the opacity (127.5 up to 128) */
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 7FFF0000 7FFF0000", parsed("repeating-linear-gradient(red, transparent 0%)"));
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 FF000000 FF000000", parsed("repeating-linear-gradient(transparent, transparent 0%)"));
    /* 3 stops, opacity (255 + 2) / 4 = 64 */
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 BF0000FF BF0000FF",
                             parsed("repeating-linear-gradient(transparent, transparent 0%, blue 0%)"));
    /* a side, an angle, a corner, a radial and a conic one */
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 00800080 00800080", parsed("repeating-linear-gradient(to right, red 30%, blue 30%)"));
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 00800080 00800080", parsed("repeating-linear-gradient(33deg, red 30%, blue 20%)"));
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 00800080 00800080", parsed("repeating-linear-gradient(to top left, red, blue 0%)"));
    TEST_ASSERT_EQUAL(0, node("a")->grad_corner);
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 00800080 00800080",
                             parsed("repeating-radial-gradient(circle 5px at 2px 3px, red 40%, blue 40%)"));
    TEST_ASSERT_EQUAL(0, node("a")->grad_shape);
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 0 2 00800080 00800080",
                             parsed("repeating-conic-gradient(from 10deg at 10% 20%, red 50deg, blue 50deg)"));
    TEST_ASSERT_EQUAL(0, node("a")->grad_at[0]);
    TEST_ASSERT_EQUAL(0, node("a")->grad_at[1]);
    /* painted as one rectangle, nothing refused */
    TEST_ASSERT_EQUAL(0, diag_count("refused"));
    TEST_ASSERT_EQUAL_STRING("0 0 40 20 00800080", items());
    /* a period above 0 still repeats; a gradient that does not repeat keeps
     * its stops */
    TEST_ASSERT_EQUAL_STRING("ANGLE 18000 1 2 00FF0000 000000FF", parsed("repeating-linear-gradient(red, blue 0.1%)"));
    TEST_ASSERT_EQUAL_STRING("OTHER 0 0 2 00FF0000 000000FF", parsed("linear-gradient(red, blue 0%)"));
}

/* along an axis: one rectangle per run of equal colours */
static void test_axis_runs(void) {
    render("<div style='width:100px;height:6px;background:repeating-linear-gradient(to right, red 0%, red 25%, blue 25%, blue 50%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 25 6 00FF0000;25 0 25 6 000000FF;50 0 25 6 00FF0000;75 0 25 6 000000FF", items());
    render("<div style='width:100px;height:6px;background:repeating-linear-gradient(to left, red 0%, red 25%, blue 25%, blue 50%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 25 6 000000FF;25 0 25 6 00FF0000;50 0 25 6 000000FF;75 0 25 6 00FF0000", items());
    render("<div style='width:6px;height:100px;background:repeating-linear-gradient(red 0%, red 25%, blue 25%, blue 50%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 6 25 00FF0000;0 25 6 25 000000FF;0 50 6 25 00FF0000;0 75 6 25 000000FF", items());
    render("<div style='width:6px;height:100px;background:repeating-linear-gradient(to top, red 0%, red 25%, blue 25%, blue 50%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 6 25 000000FF;0 25 6 25 00FF0000;0 50 6 25 000000FF;0 75 6 25 00FF0000", items());
    /* the pixel centre decides: 30% of 10px is the edge of pixel 3 */
    render("<div style='width:10px;height:2px;background:repeating-linear-gradient(90deg, red 0%, red 30%, blue 30%, blue 100%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 3 2 00FF0000;3 0 7 2 000000FF", items());
    render("<div style='width:10px;height:2px;background:repeating-linear-gradient(90deg, red 0%, red 35%, blue 35%, blue 100%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 4 2 00FF0000;4 0 6 2 000000FF", items());
    render("<div style='width:10px;height:2px;background:repeating-linear-gradient(90deg, red 0%, red 34.9%, blue 34.9%, blue 100%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 3 2 00FF0000;3 0 7 2 000000FF", items());
    /* the origin box: padding moves the gradient line, the clip keeps it */
    render("<div style='width:8px;height:2px;padding:0 1px;background-clip:content-box;background-origin:border-box;"
           "background-image:repeating-linear-gradient(to right, red 0%, red 50%, blue 50%, blue 100%)'></div>");
    TEST_ASSERT_EQUAL_STRING("1 0 4 2 00FF0000;5 0 4 2 000000FF", items());
    /* 32 runs fit (the third one 11px wide); 33 do not and the 32 cells
     * paint (10px each) */
    render("<div style='width:320px;height:10px;background:repeating-linear-gradient(to right, red 0%, red 3.2%, blue 3.2%, blue 6.4%)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
    TEST_ASSERT_EQUAL(20, g_doc.items[2].x);
    TEST_ASSERT_EQUAL(11, g_doc.items[2].w);
    render("<div style='width:320px;height:10px;background:repeating-linear-gradient(to right, red 0%, red 3.1%, blue 3.1%, blue 6.2%)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
    TEST_ASSERT_EQUAL(20, g_doc.items[2].x);
    TEST_ASSERT_EQUAL(10, g_doc.items[2].w);
    /* an angle off the axes keeps its cells */
    render("<div style='width:100px;height:6px;background:repeating-linear-gradient(91deg, red 0%, red 25%, blue 25%, blue 50%)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
}

/* a vertical-rl box paints as the same box in horizontal flow */
static void test_vertical_rl(void) {
    static const char *const grads[] = {
        "linear-gradient(30.5deg, #000000, #ffffff)", "repeating-linear-gradient(100.75deg, #000000, #ffffff 30%)",
        "repeating-linear-gradient(to right, #000000, #ffffff 20%)", "repeating-linear-gradient(to top, #000000, #ffffff 20%)",
        "repeating-linear-gradient(to bottom left, #000000, #ffffff 20%)",
        "repeating-radial-gradient(circle 9px at 10px 5px, #000000, #ffffff 30%)",
        "repeating-radial-gradient(ellipse 30% 20% at 70% 30%, #000000, #ffffff 30%)",
        "repeating-radial-gradient(#000000, #ffffff 0%)" };
    static long flat[14][7];
    char body[384];
    int g, x, y, diff;
    for (g = 0; g < 8; g++) {
        snprintf(body, sizeof(body), "<div><div style='width:40px;height:20px;background:%s'></div></div>", grads[g]);
        render(body);
        for (x = 0; x < 14; x++) for (y = 0; y < 7; y++) flat[x][y] = colour_at(1 + 3 * x, 1 + 3 * y);
        if (g < 7) TEST_ASSERT(flat[0][0] != flat[13][6] || flat[0][0] != flat[6][3]);
        snprintf(body, sizeof(body), "<div style='writing-mode:vertical-rl'><div id=c style='width:40px;height:20px;background:%s'></div></div>",
                 grads[g]);
        render(body);
        TEST_ASSERT_EQUAL(0, diag_count("refused"));
        diff = 0;
        for (x = 0; x < 14; x++) for (y = 0; y < 7; y++) diff += colour_at(1 + 3 * x, 1 + 3 * y) != flat[x][y];
        TEST_ASSERT_EQUAL(0, diff);
        /* the marks are gone after the layout */
        TEST_ASSERT(node("c")->grad_repeat <= 1);
    }
    /* "to right" in rl: per pixel runs along the logical axis, so 32 runs
     * of the full height from the physical right; pixel 0 is 1/16 of the
     * way into the first period */
    render("<div style='writing-mode:vertical-rl'><div style='width:40px;height:20px;background:repeating-linear-gradient(to right, #000000, #ffffff 20%)'></div></div>");
    TEST_ASSERT_EQUAL(32, rects());
    for (x = 0; x < 32; x++) TEST_ASSERT_EQUAL(20, g_doc.items[x].h);
    TEST_ASSERT_EQUAL(38, g_doc.items[0].x);
    TEST_ASSERT_EQUAL_HEX32(0x00101010, colour_at(0, 0));
    TEST_ASSERT_EQUAL_HEX32(0x00DFDFDF, colour_at(39, 19));
    /* a radial gradient that does not repeat keeps its rings there */
    render("<div style='writing-mode:vertical-rl'><div id=c style='width:40px;height:20px;background:radial-gradient(circle 9px at 10px 5px, #000000, #ffffff)'></div></div>");
    TEST_ASSERT_EQUAL(0, node("c")->grad_repeat);
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, node("c")->grad_dir);
}

int main(void) {
    RUN_TEST(test_average);
    RUN_TEST(test_axis_runs);
    RUN_TEST(test_vertical_rl);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
