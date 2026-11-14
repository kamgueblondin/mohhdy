/* Lot 96 (US-031): fractional degrees and repeating gradients in the
 * engine, hand cases (generated cases: test_lot96_cells.c,
 * test_lot96_sheet.c). Expected values follow docs/lot96_plan.md. */
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

static int centi(const char *id) {
    const we_node_t *n = node(id);
    return n ? n->grad_angle * 100 + n->grad_frac : -1;
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

static void test_fractional_angles(void) {
    render("<div id=a style='background:linear-gradient(90.01deg, red, blue)'></div>"
           "<div id=b style='background:linear-gradient(90.004deg, red, blue)'></div>"
           "<div id=c style='background:linear-gradient(-0.005deg, red, blue)'></div>"
           "<div id=d style='background:linear-gradient(1rad, red, blue)'></div>"
           "<div id=e style='background:linear-gradient(0.3333turn, red, blue)'></div>"
           "<div id=f style='background:linear-gradient(100.05grad, red, blue)'></div>"
           "<div id=g style='background:conic-gradient(from 0.25turn, red 1rad, blue 10.25deg, lime 359.999deg)'></div>"
           "<div id=h style='background:linear-gradient(44.995deg, red, blue)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("gradient refused"));
    TEST_ASSERT_EQUAL(WE_GRAD_ANGLE, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(9001, centi("a"));
    /* 90.004deg is 9000 hundredths: a quarter turn keeps its bands */
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, node("b")->grad_dir);
    TEST_ASSERT_EQUAL(0, centi("b"));
    /* -0.005deg: -5 thousandths, half away from zero: -1 hundredth */
    TEST_ASSERT_EQUAL(35999, centi("c"));
    TEST_ASSERT_EQUAL(5730, centi("d"));                    /* the engine's 57.3 */
    TEST_ASSERT_EQUAL(11988, centi("e"));                   /* 333 * 360 = 119880 thousandths */
    TEST_ASSERT_EQUAL(9005, centi("f"));                    /* 100050 * 9 / 10 = 90045 */
    TEST_ASSERT_EQUAL(WE_GRAD_CONIC, node("g")->grad_dir);
    TEST_ASSERT_EQUAL(9000, centi("g"));
    TEST_ASSERT_EQUAL(1592, node("g")->grad_pos[0]);         /* (5730 * 5 + 9) / 18 */
    TEST_ASSERT_EQUAL(1592, node("g")->grad_pos[1]);         /* 285 raised to the first */
    TEST_ASSERT_EQUAL(10000, node("g")->grad_pos[2]);        /* 36000 hundredths */
    TEST_ASSERT_EQUAL(4500, centi("h"));
    /* the sin between two degrees: 44.5deg paints other cells than 44deg
     * and 45deg (test_lot96_cells.c checks the values) */
    {
        static char c44[4096], c45[4096];
        render("<div style='width:100px;height:100px;background:linear-gradient(44deg, #000000, #ffffff)'></div>");
        strcpy(c44, items());
        render("<div style='width:100px;height:100px;background:linear-gradient(45deg, #000000, #ffffff)'></div>");
        strcpy(c45, items());
        render("<div style='width:100px;height:100px;background:linear-gradient(44.5deg, #000000, #ffffff)'></div>");
        TEST_ASSERT(strcmp(c44, c45) != 0);
        TEST_ASSERT(strcmp(c44, items()) != 0);
        TEST_ASSERT(strcmp(c45, items()) != 0);
    }
}

static void test_repeat_by_hand(void) {
    /* 1x8, to bottom, black 0 to white 50%: p = (y + 0.5) / 8 = 625 + 1250 y,
     * mapped into 0..5000: t = 1250, 3750, 6250, 8750 -> 32, 96, 159, 223 */
    render("<div style='width:1px;height:8px;background:repeating-linear-gradient(#000000, #ffffff 50%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 00202020;0 1 1 1 00606060;0 2 1 1 009F9F9F;0 3 1 1 00DFDFDF;"
                             "0 4 1 1 00202020;0 5 1 1 00606060;0 6 1 1 009F9F9F;0 7 1 1 00DFDFDF", items());
    /* the same to the right on 8x1 */
    render("<div style='width:8px;height:1px;background:repeating-linear-gradient(to right, #000000, #ffffff 50%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 00202020;1 0 1 1 00606060;2 0 1 1 009F9F9F;3 0 1 1 00DFDFDF;"
                             "4 0 1 1 00202020;5 0 1 1 00606060;6 0 1 1 009F9F9F;7 0 1 1 00DFDFDF", items());
    /* a first stop past 0: p below it is mapped up a period: 625 -> 5625
     * (t 6250), 1875 -> 6875 (t 8750), 3125 (t 1250), 4375 (t 3750), ... */
    render("<div style='width:1px;height:8px;background:repeating-linear-gradient(#000000 25%, #ffffff 75%)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 009F9F9F;0 1 1 1 00DFDFDF;0 2 1 1 00202020;0 3 1 1 00606060;"
                             "0 4 1 1 009F9F9F;0 5 1 1 00DFDFDF;0 6 1 1 00202020;0 7 1 1 00606060", items());
    /* an axis gradient on a big box: 32 rows of one column, 32 columns of
     * one row */
    render("<div id=a style='width:100px;height:100px;background:repeating-linear-gradient(red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
    TEST_ASSERT_EQUAL(100, g_doc.items[0].w);
    render("<div style='width:100px;height:10px;background:repeating-linear-gradient(270deg, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
    TEST_ASSERT_EQUAL(10, g_doc.items[0].h);
    render("<div style='width:100px;height:10px;background:repeating-linear-gradient(-0.25turn, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
    /* a small box: no more cells than pixels. Lot 97 (converted): painted
     * per pixel, the five pixel centres all fall on the period's start, so
     * one rectangle (was 5 cells of the same colour) */
    render("<div style='width:3px;height:5px;background:repeating-linear-gradient(to top, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, rects());
    TEST_ASSERT_EQUAL(3, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(5, g_doc.items[0].h);
    /* radial: 9x1, circle 2px at 4px: |dx| 0..4 -> 0, 2500 (0.5px), ...;
     * period 5000 (black to white at 50%) */
    render("<div style='width:9px;height:1px;background:repeating-radial-gradient(circle 2px at 4px 0, #000000, #ffffff 50%)'></div>");
    TEST_ASSERT_EQUAL(9, rects());
    TEST_ASSERT_EQUAL_HEX32(colour_at(3, 0), colour_at(4, 0));      /* centres 3.5 and 4.5 */
    TEST_ASSERT_EQUAL_HEX32(colour_at(2, 0), colour_at(5, 0));
    TEST_ASSERT_EQUAL_HEX32(colour_at(0, 0), colour_at(7, 0));
    /* cell 3: doubled offsets (-1, 1), rx = ry = 2: N = 8, s = isqrt(512) =
     * 22, R = 8, p = (220000 + 32) / 64 = 3437 (the exact 3536 less than
     * 10000 / 64), t = 6874, 255 * 0.6874 = 175 */
    TEST_ASSERT_EQUAL_HEX32(0x00AFAFAF, colour_at(3, 0));
}

static void test_repeat_parse(void) {
    render("<div id=a style='background:repeating-linear-gradient(red, blue 10%) no-repeat'></div>"
           "<div id=b style='background-image:url(a.png), repeating-conic-gradient(red, blue 10%)'></div>"
           "<div id=c style='background:url(a.png), REPEATING-RADIAL-GRADIENT(red, blue 10%)'></div>"
           "<div id=d style='background:repeating-linear-gradient(to top left, red, blue 10%)'></div>"
           "<div id=e style='background:repeating-linear-gradient(to left, red, blue 10%)'></div>"
           "<div id=f style='background:linear-gradient(to left, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("gradient refused"));
    TEST_ASSERT_EQUAL(1, node("a")->grad_repeat);
    TEST_ASSERT_EQUAL(WE_GRAD_ANGLE, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(18000, centi("a"));
    TEST_ASSERT_EQUAL(1, node("b")->grad_repeat);
    TEST_ASSERT_EQUAL(WE_GRAD_CONIC, node("b")->grad_dir);
    TEST_ASSERT_EQUAL(1, node("c")->grad_repeat);
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, node("c")->grad_dir);
    TEST_ASSERT_EQUAL(WE_GRAD_ANGLE, node("d")->grad_dir);
    TEST_ASSERT_EQUAL(2, node("d")->grad_corner);
    TEST_ASSERT_EQUAL(0, centi("d"));
    TEST_ASSERT_EQUAL(27000, centi("e"));
    TEST_ASSERT_EQUAL(0, node("f")->grad_repeat);
    TEST_ASSERT_EQUAL(WE_GRAD_LEFT, node("f")->grad_dir);
    /* a period of 0 is refused, the earlier gradient stays */
    {
        static const char *const bad[] = {
            "repeating-linear-gradient(red 50%, blue 50%)", "repeating-linear-gradient(red 100%, blue)",
            "repeating-conic-gradient(red 90deg, blue 25%)", "repeating-radial-gradient(red 60%, lime 50%, blue 20%)",
            "repeating-linear-gradient(red)", "repeating-gradient(red, blue)", "repeating-linear-gradient(red, blue",
            "repeating-repeating-linear-gradient(red, blue)", "repeating-conic-gradient(red 0.35deg, blue 0.1%)",
            "repeating-linear-gradient(red, blue 0%)" };
        char body[256];
        int k;
        for (k = 0; k < 10; k++) {
            snprintf(body, sizeof(body), "<div id=a style='width:4px;height:4px;background:linear-gradient(red, blue);background-image:%s'></div>", bad[k]);
            render(body);
            TEST_ASSERT_EQUAL(0, node("a")->grad_repeat);
            TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, node("a")->grad_dir);
        }
    }
    /* 0.34deg is at 9/10000 of the turn, 0.35deg at 10/10000 (rounded) */
    render("<div id=a style='background:repeating-conic-gradient(red 0.34deg, blue 0.1%)'></div>");
    TEST_ASSERT_EQUAL(1, node("a")->grad_repeat);
    TEST_ASSERT_EQUAL(9, node("a")->grad_pos[0]);
    /* repeating-radial-gradient(red 60%, lime 70%, blue 70.1%): taken */
    render("<div id=a style='background:repeating-radial-gradient(red 60%, lime 70%, blue 70.1%)'></div>");
    TEST_ASSERT_EQUAL(1, node("a")->grad_repeat);
    TEST_ASSERT_EQUAL(6000, node("a")->grad_pos[0]);
    TEST_ASSERT_EQUAL(7010, node("a")->grad_pos[2]);
}

static void test_guards(void) {
    /* lengths stop at 2000px; a percentage radius reaches past 8191:
     * 1000% of 820px is 8200, of 819px 8190 */
    render("<div style='width:820px;height:20px;background:repeating-radial-gradient(1000% 10%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("repeating gradient too large"));
    TEST_ASSERT_EQUAL(0, rects());
    render("<div style='width:819px;height:819px;background:repeating-radial-gradient(1000% 1000%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("repeating gradient too large"));
    TEST_ASSERT_EQUAL(30, rects());                         /* 5 rows of 6 */
    render("<div style='width:20px;height:820px;background:repeating-radial-gradient(10% 1000%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("repeating gradient too large"));
    /* the centre far from the box: the doubled offsets pass 32767 (at
     * 1000% of 1639px is 16390, doubled 32780; of 1638px 32760) */
    render("<div style='width:1639px;height:20px;background:repeating-radial-gradient(circle 10px at 1000% 0, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("repeating gradient too large"));
    TEST_ASSERT_EQUAL(0, rects());
    render("<div style='width:1638px;height:20px;background:repeating-radial-gradient(circle 10px at 1000% 0, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("repeating gradient too large"));
    TEST_ASSERT(rects() > 0);
    render("<div style='width:20px;height:1639px;background:repeating-radial-gradient(circle 10px at 0 1000%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("repeating gradient too large"));
    /* 800% of 1024px is 8192: past 8191 */
    render("<div style='width:1024px;height:20px;background:repeating-radial-gradient(800% 10%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("repeating gradient too large"));
    /* the centre far left of a wide box: only the right edge passes
     * (at -750% of 2000px: 2 * 2000 + 30000 > 32767, at -700% 32000) */
    render("<div style='width:2000px;height:20px;background:repeating-radial-gradient(circle 10px at -750% 0, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("repeating gradient too large"));
    render("<div style='width:2000px;height:20px;background:repeating-radial-gradient(circle 10px at -700% 0, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("repeating gradient too large"));
    TEST_ASSERT(rects() > 0);
    render("<div style='width:20px;height:2000px;background:repeating-radial-gradient(circle 10px at 0 -750%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("repeating gradient too large"));
    render("<div style='width:20px;height:2000px;background:repeating-radial-gradient(circle 10px at 0 -700%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("repeating gradient too large"));
    /* an angle past 2e9 thousandths of a degree: refused (5556turn), 5555turn taken */
    render("<div id=a style='background:linear-gradient(red, blue);background-image:linear-gradient(5556turn, red, blue)'></div>"
           "<div id=b style='background:linear-gradient(5555turn, red, blue)'></div>");
    TEST_ASSERT_EQUAL(WE_GRAD_BOTTOM, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(WE_GRAD_TOP, node("b")->grad_dir);    /* 5555turn is a whole number of turns */
    /* a radial gradient that does not repeat keeps its rings at any size */
    render("<div style='width:820px;height:20px;background:radial-gradient(1000% 10%, red, blue 10%)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("repeating gradient too large"));
    TEST_ASSERT(rects() > 0);
}

static void test_vertical(void) {
    static const char *const grads[] = {
        "linear-gradient(30.5deg, #000000, #ffffff)", "linear-gradient(200.25deg, #000000, #ffffff)",
        "repeating-linear-gradient(100.75deg, #000000, #ffffff 30%)", "repeating-linear-gradient(to top left, #000000, #ffffff 20%)",
        "repeating-linear-gradient(#000000, #ffffff 20%)", "repeating-linear-gradient(to right, #000000, #ffffff 20%)",
        "repeating-radial-gradient(circle 9px at 10px 5px, #000000, #ffffff 30%)" };
    /* vertical-rl rotates the box (x = W - y'): the cells of "to right" are
     * cut from the other end, and a repeating radial gradient is refused
     * there; both are checked in vertical-lr only */
    static const char *const modes[2] = { "vertical-rl", "vertical-lr" };
    static long flat[14][7];
    char body[384];
    int g, m, x, y, diff;
    for (g = 0; g < 7; g++) {
        snprintf(body, sizeof(body), "<div><div style='width:40px;height:20px;background:%s'></div></div>", grads[g]);
        render(body);
        for (x = 0; x < 14; x++) for (y = 0; y < 7; y++) flat[x][y] = colour_at(1 + 3 * x, 1 + 3 * y);
        TEST_ASSERT(flat[0][0] != flat[13][6] || flat[0][0] != flat[6][3]);
        for (m = g >= 5; m < 2; m++) {
            snprintf(body, sizeof(body), "<div style='writing-mode:%s'><div id=c style='width:40px;height:20px;background:%s'></div></div>",
                     modes[m], grads[g]);
            render(body);
            diff = 0;
            for (x = 0; x < 14; x++) for (y = 0; y < 7; y++) diff += colour_at(1 + 3 * x, 1 + 3 * y) != flat[x][y];
            TEST_ASSERT_EQUAL(0, diff);
        }
    }
    render("<div style='writing-mode:vertical-rl'><div style='width:40px;height:20px;background:repeating-linear-gradient(to right, #000000, #ffffff 20%)'></div></div>");
    TEST_ASSERT_EQUAL(32, rects());
    for (x = 0; x < 32; x++) TEST_ASSERT_EQUAL(20, g_doc.items[x].h);
    /* mapped back after the layout, to 1/100 degree */
    render("<div style='writing-mode:vertical-rl'><div id=a style='width:40px;height:20px;background:linear-gradient(30.25deg, red, blue)'></div></div>"
           "<div style='writing-mode:vertical-lr'><div id=b style='width:40px;height:20px;background:repeating-linear-gradient(100.75deg, red, blue 10%)'></div></div>");
    TEST_ASSERT_EQUAL(3025, centi("a"));
    TEST_ASSERT_EQUAL(10075, centi("b"));
    TEST_ASSERT_EQUAL(1, node("b")->grad_repeat);
    /* a repeating conic child is refused there like a conic one */
    render("<div style='writing-mode:vertical-rl'><div id=a style='width:40px;height:20px;background:#123456 repeating-conic-gradient(#000000, #ffffff 10%)'></div></div>");
    TEST_ASSERT_EQUAL(1, diag_count("conic refused in vertical flow"));
    TEST_ASSERT_EQUAL(1, rects());
    TEST_ASSERT_EQUAL_HEX32(0x00123456, g_doc.items[0].color);
    /* a repeating radial one in vertical-rl. Lot 97 (converted): painted
     * in cells with its centre mirrored (was refused, one rectangle and
     * grad_dir 0); the rl marks are gone after the layout. A radial one
     * that does not repeat keeps its rings */
    render("<div style='writing-mode:vertical-rl'><div id=a style='width:40px;height:20px;background:#123456 repeating-radial-gradient(#000000, #ffffff 10%)'></div></div>");
    TEST_ASSERT_EQUAL(0, diag_count("repeating radial refused in rl"));
    TEST_ASSERT_EQUAL(33, rects());
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(1, node("a")->grad_repeat);
    render("<div style='writing-mode:vertical-rl'><div id=a style='width:40px;height:20px;background:radial-gradient(#000000, #ffffff 10%)'></div></div>");
    TEST_ASSERT_EQUAL(0, diag_count("refused in vertical"));
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, node("a")->grad_dir);
    render("<div style='writing-mode:vertical-lr'><div id=a style='width:40px;height:20px;background:repeating-radial-gradient(#000000, #ffffff 10%)'></div></div>");
    TEST_ASSERT_EQUAL(0, diag_count("refused in vertical"));
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, node("a")->grad_dir);
}

int main(void) {
    unity_init();
    RUN_TEST(test_fractional_angles);
    RUN_TEST(test_repeat_by_hand);
    RUN_TEST(test_repeat_parse);
    RUN_TEST(test_guards);
    RUN_TEST(test_vertical);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
