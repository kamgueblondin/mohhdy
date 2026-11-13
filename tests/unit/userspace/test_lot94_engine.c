/* Lot 94 (US-031): angled linear and conic gradients in the engine, hand
 * cases (generated cases: test_lot94_cells.c, test_lot94_sheet.c).
 * Expected rectangles follow docs/lot94_plan.md; the colours of the
 * small boxes can be checked by hand (45deg on 4x4: the anti-diagonal
 * cells are at the middle of the line, #808080). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

#define TEST_ASSERT_EQUAL_HEX32(e, a) TEST_ASSERT_EQUAL((long)(e), (long)(a))

static we_doc_t g_doc;
static int diag_count(const char *what);
static char g_html[2048], g_out[4096];

static int refused(void) {
    int i, n = 0;
    for (i = 0; i < (int)g_doc.ndiag; i++) if (strstr(g_doc.diag[i], "gradient refused")) n++;
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

static void test_small_boxes(void) {
    render("<div style='width:4px;height:4px;background:linear-gradient(45deg, #000000, #ffffff)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 00808080;1 0 1 1 009F9F9F;2 0 1 1 00BFBFBF;3 0 1 1 00DFDFDF;"
                             "0 1 1 1 00606060;1 1 1 1 00808080;2 1 1 1 009F9F9F;3 1 1 1 00BFBFBF;"
                             "0 2 1 1 00404040;1 2 1 1 00606060;2 2 1 1 00808080;3 2 1 1 009F9F9F;"
                             "0 3 1 1 00202020;1 3 1 1 00404040;2 3 1 1 00606060;3 3 1 1 00808080", items());
    /* to a corner: the line is perpendicular to the other diagonal, so the
     * two other corners (top left, bottom right) are on its middle */
    render("<div style='width:4px;height:2px;background:linear-gradient(to top right, #000000, #ffffff)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 00707070;1 0 1 1 008F8F8F;2 0 1 1 00AFAFAF;3 0 1 1 00CFCFCF;"
                             "0 1 1 1 00303030;1 1 1 1 00505050;2 1 1 1 00707070;3 1 1 1 008F8F8F", items());
    render("<div style='width:4px;height:2px;background:linear-gradient(to left bottom, #000000, #ffffff)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 008F8F8F;1 0 1 1 00707070;2 0 1 1 00505050;3 0 1 1 00303030;"
                             "0 1 1 1 00CFCFCF;1 1 1 1 00AFAFAF;2 1 1 1 008F8F8F;3 1 1 1 00707070", items());
}

static void test_conic_small(void) {
    render("<div style='width:4px;height:4px;background:conic-gradient(#ff0000, #0000ff)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 002000DF;1 0 1 1 000D00F2;2 0 1 1 00F2000D;3 0 1 1 00DF0020;"
                             "0 1 1 1 003300CC;1 1 1 1 002000DF;2 1 1 1 00DF0020;3 1 1 1 00CC0033;"
                             "0 2 1 1 004D00B2;1 2 1 1 0060009F;2 2 1 1 009F0060;3 2 1 1 00B2004D;"
                             "0 3 1 1 0060009F;1 3 1 1 0072008D;2 3 1 1 008D0072;3 3 1 1 009F0060", items());
    /* from 90deg: the turn starts on the right; the four cells around the
     * centre are at 1/8, 3/8, 5/8, 7/8 of it */
    render("<div style='width:2px;height:2px;background:conic-gradient(from 90deg, #ff0000, #0000ff)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 0060009F;1 0 1 1 002000DF;0 1 1 1 009F0060;1 1 1 1 00DF0020", items());
    /* the centre on a cell centre (direction 0) and on the axes: exact */
    render("<div style='width:3px;height:1px;background:conic-gradient(at 1px 0px, #ff0000, #0000ff)'></div>");
    TEST_ASSERT_EQUAL_STRING("0 0 1 1 0060009F;1 0 1 1 009F0060;2 0 1 1 00B2004D", items());
    /* 64x2: one row of 2px cells, centres (1, 1), (3, 1) ... */
    render("<div style='width:64px;height:2px;background:conic-gradient(at 3px 1px, #ff0000, #0000ff 360deg)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
    TEST_ASSERT_EQUAL_HEX32(0x004000BF, g_doc.items[0].color);   /* left: 3/4 */
    TEST_ASSERT_EQUAL_HEX32(0x00FF0000, g_doc.items[1].color);   /* the centre itself: 0 */
    TEST_ASSERT_EQUAL_HEX32(0x00BF0040, g_doc.items[2].color);   /* right: 1/4 */
    TEST_ASSERT_EQUAL_HEX32(0x00BF0040, g_doc.items[31].color);
    /* 2x64: one column of 2px cells */
    render("<div style='width:2px;height:64px;background:conic-gradient(at 1px 3px, #ff0000, #0000ff 360deg)'></div>");
    TEST_ASSERT_EQUAL(32, rects());
    TEST_ASSERT_EQUAL_HEX32(0x00FF0000, g_doc.items[0].color);   /* straight up: 0 */
    TEST_ASSERT_EQUAL_HEX32(0x00FF0000, g_doc.items[1].color);   /* the centre */
    TEST_ASSERT_EQUAL_HEX32(0x00800080, g_doc.items[2].color);   /* down: 1/2 */
    /* from 90deg: up is 3/4 of the turn, right is 0 */
    render("<div style='width:64px;height:2px;background:conic-gradient(from 90deg at 3px 1px, #ff0000, #0000ff 360deg)'></div>");
    TEST_ASSERT_EQUAL_HEX32(0x00800080, g_doc.items[0].color);   /* left: 1/2 */
    TEST_ASSERT_EQUAL_HEX32(0x00FF0000, g_doc.items[2].color);   /* right: 0 */
}

static void test_grid_shapes(void) {
    render("<div style='width:1px;height:40px;background:linear-gradient(30deg, red, blue)'></div>");
    TEST_ASSERT_EQUAL(32, rects());                               /* one column, 32 rows */
    TEST_ASSERT_EQUAL(1, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(1, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(38, g_doc.items[31].y);                     /* 31 * 40 / 32 */
    TEST_ASSERT_EQUAL(2, g_doc.items[31].h);
    render("<div style='width:300px;height:1px;background:linear-gradient(30deg, red, blue)'></div>");
    TEST_ASSERT_EQUAL(32, rects());                               /* one row, 32 columns */
    TEST_ASSERT_EQUAL(0, g_doc.items[0].y);
    TEST_ASSERT_EQUAL(9, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(290, g_doc.items[31].x);                    /* 31 * 300 / 32 */
    render("<div style='width:2px;height:100px;background:linear-gradient(30deg, red, blue)'></div>");
    TEST_ASSERT_EQUAL(32, rects());                               /* isqrt(1600) = 40, at most 32 rows */
    TEST_ASSERT_EQUAL(2, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(3, g_doc.items[0].h);
    render("<div style='width:10px;height:200px;background:linear-gradient(30deg, red, blue)'></div>");
    TEST_ASSERT_EQUAL(25, rects());                               /* isqrt(640) = 25 rows, 1 column */
    render("<div style='width:3px;height:3px;background:conic-gradient(red, blue)'></div>");
    TEST_ASSERT_EQUAL(9, rects());                                /* never more columns than pixels */
    render("<div style='width:0;height:20px;background:conic-gradient(red, blue)'></div>");
    TEST_ASSERT_EQUAL(0, rects());                                /* nothing to paint */
}

static void test_origin_and_clip(void) {
    /* origin content-box, clip border-box: the cells cover the content box */
    render("<div style='width:20px;height:10px;padding:5px;background-origin:content-box;"
           "background-image:linear-gradient(60deg, red, blue)'></div>");
    TEST_ASSERT(rects() > 0);
    TEST_ASSERT_EQUAL(5, g_doc.items[0].x);
    TEST_ASSERT_EQUAL(5, g_doc.items[0].y);
    /* origin border-box, clip content-box: the cells cover the content box */
    render("<div style='width:20px;height:10px;padding:5px;background-clip:content-box;"
           "background-image:conic-gradient(red, blue)'></div>");
    TEST_ASSERT_EQUAL(5, g_doc.items[0].x);
    TEST_ASSERT_EQUAL(25, g_doc.items[rects() - 1].x + g_doc.items[rects() - 1].w);
    TEST_ASSERT_EQUAL(15, g_doc.items[rects() - 1].y + g_doc.items[rects() - 1].h);
}

static void test_parse(void) {
    const we_node_t *n;
    render("<div id=a style='background:linear-gradient(-30deg, red, blue)'></div>"
           "<div id=b style='background:linear-gradient(0.1turn, red, blue)'></div>"
           "<div id=c style='background:linear-gradient(to BOTTOM right, red, blue)'></div>"
           "<div id=d style='background:conic-gradient(from -90deg at 10px 20%, red, blue 50%, lime 270deg)'></div>"
           "<div id=e style='background:conic-gradient(at right bottom, red, blue)'></div>"
           "<div id=f style='background:linear-gradient(450deg, red, blue)'></div>"
           "<div id=g style='background:linear-gradient(44.5deg, red, blue)'></div>");
    TEST_ASSERT_EQUAL(0, refused());
    n = node("a");
    TEST_ASSERT_EQUAL(WE_GRAD_ANGLE, n->grad_dir);
    TEST_ASSERT_EQUAL(330, n->grad_angle);
    TEST_ASSERT_EQUAL(36, node("b")->grad_angle);
    n = node("c");
    TEST_ASSERT_EQUAL(WE_GRAD_ANGLE, n->grad_dir);
    TEST_ASSERT_EQUAL(3, n->grad_corner);                          /* bottom right: bit 1 */
    n = node("d");
    TEST_ASSERT_EQUAL(WE_GRAD_CONIC, n->grad_dir);
    TEST_ASSERT_EQUAL(270, n->grad_angle);
    TEST_ASSERT_EQUAL(10, n->grad_at[0]);
    TEST_ASSERT_EQUAL(20, n->grad_at[1]);
    TEST_ASSERT_EQUAL(2, n->grad_at_pct);
    TEST_ASSERT_EQUAL(3, n->grad_count);
    TEST_ASSERT_EQUAL(5000, n->grad_pos[1]);
    TEST_ASSERT_EQUAL(7500, n->grad_pos[2]);
    n = node("e");
    TEST_ASSERT_EQUAL(100, n->grad_at[0]);
    TEST_ASSERT_EQUAL(100, n->grad_at[1]);
    TEST_ASSERT_EQUAL(3, n->grad_at_pct);
    TEST_ASSERT_EQUAL(WE_GRAD_RIGHT, node("f")->grad_dir);          /* a quarter turn keeps its bands */
    TEST_ASSERT_EQUAL(45, node("g")->grad_angle);                   /* half away from zero */
    TEST_ASSERT_EQUAL(0, node("g")->grad_corner);
    /* a conic layer after an image layer, and the shorthand with a repeat */
    render("<div id=a style='background-image:url(a.png), conic-gradient(from 90deg, red, blue)'></div>"
           "<div id=b style='background:conic-gradient(from 10deg, red, blue) no-repeat'></div>");
    TEST_ASSERT_EQUAL(0, refused());
    TEST_ASSERT_EQUAL(WE_GRAD_CONIC, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(90, node("a")->grad_angle);
    TEST_ASSERT_EQUAL(WE_GRAD_CONIC, node("b")->grad_dir);
    TEST_ASSERT_EQUAL(10, node("b")->grad_angle);
}

/* the colour painted at a physical point */
static long colour_at(int x, int y) {
    int i;
    for (i = 0; i < (int)g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind == OS_FB_WEB_RECT && x >= it->x && x < it->x + (int)it->w && y >= it->y && y < it->y + (int)it->h)
            return (long)it->color;
    }
    return -1;
}

static void test_vertical_children(void) {
    /* a child of a vertical box is painted in the logical frame: its angle
     * and corner are mapped there and back, the physical colours are the
     * ones of the same box in a horizontal flow */
    static const char *const grads[] = {
        "linear-gradient(30deg, #000000, #ffffff)", "linear-gradient(200deg, #000000, #ffffff)",
        "linear-gradient(100deg, #000000, #ffffff)", "linear-gradient(to top left, #000000, #ffffff)",
        "linear-gradient(to bottom left, #000000, #ffffff)", "linear-gradient(to top right, #000000, #ffffff)",
        "linear-gradient(to bottom right, #000000, #ffffff)" };
    static const char *const modes[2] = { "vertical-rl", "vertical-lr" };
    static long flat[14][7];
    char body[384];
    int g, m, x, y, diff;
    for (g = 0; g < 7; g++) {
        snprintf(body, sizeof(body), "<div><div style='width:40px;height:20px;background:%s'></div></div>", grads[g]);
        render(body);
        for (x = 0; x < 14; x++) for (y = 0; y < 7; y++) flat[x][y] = colour_at(1 + 3 * x, 1 + 3 * y);
        for (m = 0; m < 2; m++) {
            snprintf(body, sizeof(body), "<div style='writing-mode:%s'><div id=c style='width:40px;height:20px;background:%s'></div></div>",
                     modes[m], grads[g]);
            render(body);
            diff = 0;
            for (x = 0; x < 14; x++) for (y = 0; y < 7; y++) diff += colour_at(1 + 3 * x, 1 + 3 * y) != flat[x][y];
            TEST_ASSERT_EQUAL(0, diff);
            TEST_ASSERT(flat[0][0] != flat[13][6]);
        }
    }
    /* mapped back after the layout */
    render("<div style='writing-mode:vertical-rl'><div id=a style='width:40px;height:20px;background:linear-gradient(30deg, red, blue)'></div>"
           "<div id=b style='width:40px;height:20px;background:linear-gradient(to top left, red, blue)'></div></div>"
           "<div style='writing-mode:vertical-lr'><div id=c style='width:40px;height:20px;background:linear-gradient(100deg, red, blue)'></div>"
           "<div id=d style='width:40px;height:20px;background:linear-gradient(to bottom left, red, blue)'></div></div>");
    TEST_ASSERT_EQUAL(30, node("a")->grad_angle);
    TEST_ASSERT_EQUAL(2, node("b")->grad_corner);
    TEST_ASSERT_EQUAL(100, node("c")->grad_angle);
    TEST_ASSERT_EQUAL(4, node("d")->grad_corner);
    /* a conic child is refused there, its colour stays */
    render("<div style='writing-mode:vertical-rl'><div id=a style='width:40px;height:20px;background:#123456 conic-gradient(#000000, #ffffff)'></div></div>");
    TEST_ASSERT_EQUAL(1, diag_count("conic refused in vertical flow"));
    TEST_ASSERT_EQUAL(0, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(1, rects());
    TEST_ASSERT_EQUAL_HEX32(0x00123456, g_doc.items[0].color);
    render("<div style='writing-mode:vertical-lr'><div id=a style='width:40px;height:20px;background:conic-gradient(#000000, #ffffff)'></div></div>");
    TEST_ASSERT_EQUAL(1, diag_count("conic refused in vertical flow"));
    TEST_ASSERT_EQUAL(0, rects());
}

static void test_refusals(void) {
    static const char *const bad[] = {
        "linear-gradient(to top top, red, blue)", "linear-gradient(to left right, red, blue)",
        "linear-gradient(to top left bottom, red, blue)", "conic-gradient(from 45deg left, red, blue)",
        "conic-gradient(from, red, blue)", "conic-gradient(at, red, blue)", "conic-gradient(from 45px, red, blue)",
        "conic-gradient(red 361deg, blue)", "conic-gradient(red -1deg, blue)", "conic-gradient(red)",
        "conic-gradient(from 45deg at, red, blue)", "conic-gradient(at left right, red, blue)",
        "conic-gradient(from 45deg to left, red, blue)", "linear-gradient(red 45deg, blue)" };
    char body[256];
    int k;
    for (k = 0; k < 14; k++) {
        snprintf(body, sizeof(body), "<div id=a style='width:4px;height:4px;background:%s'></div>", bad[k]);
        render(body);
        TEST_ASSERT_EQUAL(1, refused());
        TEST_ASSERT_EQUAL(0, node("a")->grad_dir);
        TEST_ASSERT_EQUAL(0, rects());
    }
    /* a refused conic gradient leaves the earlier one as it was */
    render("<div id=a style='width:8px;height:8px;background:radial-gradient(at 3px 2px, red, blue);"
           "background:conic-gradient(at 1px 1px, red 400deg, blue)'></div>");
    TEST_ASSERT_EQUAL(1, refused());
    TEST_ASSERT_EQUAL(WE_GRAD_RADIAL, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(3, node("a")->grad_at[0]);
    TEST_ASSERT_EQUAL(2, node("a")->grad_at[1]);
    TEST_ASSERT_EQUAL(0, node("a")->grad_at_pct);
}

static void test_budget_and_modes(void) {
    static const int widths[6] = { 6, 7, 7, 6, 7, 7 };   /* 0 6 13 20 26 33 40 */
    int i;
    /* each 40x40 box reserves 32 items and paints 30 (5 rows of 6); the
     * unused ones are dropped, the fourth box finds no room */
    render("<div style='width:40px;height:40px;background:conic-gradient(red, blue)'></div>"
           "<div style='width:40px;height:40px;background:conic-gradient(red, blue)'></div>"
           "<div style='width:40px;height:40px;background:conic-gradient(red, blue)'></div>"
           "<div style='width:40px;height:40px;background:conic-gradient(red, blue)'></div>");
    TEST_ASSERT(g_doc.nitems <= OS_FB_WEB_ITEMS);
    TEST_ASSERT_EQUAL(90, rects());
    for (i = 0; i < 90; i++) TEST_ASSERT(g_doc.items[i].y < 120 && g_doc.items[i].w == widths[i % 6]);
    for (i = 0; i < (int)g_doc.ndiag && !strstr(g_doc.diag[i], "budget exceeded"); i++) {}
    TEST_ASSERT(i < (int)g_doc.ndiag);
    /* backgrounds are physical: a vertical writing mode paints the same cells */
    {
        static char flat[4096];
        render("<div style='width:20px;height:10px;background:linear-gradient(30deg, #000000, #ffffff)'></div>");
        strcpy(flat, items());
        render("<div style='writing-mode:vertical-rl;width:20px;height:10px;background:linear-gradient(30deg, #000000, #ffffff)'></div>");
        TEST_ASSERT_EQUAL_STRING(flat, items());
        render("<div style='width:20px;height:10px;background:conic-gradient(at 5px 2px, #000000, #ffffff)'></div>");
        strcpy(flat, items());
        render("<div style='writing-mode:vertical-lr;width:20px;height:10px;background:conic-gradient(at 5px 2px, #000000, #ffffff)'></div>");
        TEST_ASSERT_EQUAL_STRING(flat, items());
    }
    /* vertical writing modes keep the angle and the corner */
    render("<div id=a style='writing-mode:vertical-rl;width:20px;height:20px;background:linear-gradient(30deg, red, blue)'></div>"
           "<div id=b style='writing-mode:vertical-lr;width:20px;height:20px;background:linear-gradient(to top left, red, blue)'></div>");
    TEST_ASSERT_EQUAL(WE_GRAD_ANGLE, node("a")->grad_dir);
    TEST_ASSERT_EQUAL(30, node("a")->grad_angle);
    TEST_ASSERT_EQUAL(WE_GRAD_ANGLE, node("b")->grad_dir);
    TEST_ASSERT_EQUAL(2, node("b")->grad_corner);
    /* translucent stops are composed on the page as the bands are: the top
     * cell is at p = 0.5 + 0.5 cos 10 / (sin 10 + 2 cos 10) = 0.7298, red 69
     * blue 186 at half alpha over white: a2 7f dc */
    render("<div style='width:1px;height:2px;background:linear-gradient(10deg, rgba(255, 0, 0, 0.5), rgba(0, 0, 255, 0.5))'></div>");
    TEST_ASSERT_EQUAL(2, rects());
    TEST_ASSERT_EQUAL_HEX32(0x00A27FDC, g_doc.items[0].color);
    TEST_ASSERT_EQUAL_HEX32(0x00DC7FA2, g_doc.items[1].color);
}

static int diag_count(const char *what) {
    int i, n = 0;
    for (i = 0; i < (int)g_doc.ndiag; i++) if (strstr(g_doc.diag[i], what)) n++;
    return n;
}

static void test_diag_and_transform(void) {
    /* cells of two colours are an approximation, one colour is exact */
    render("<div style='width:20px;height:10px;background:conic-gradient(#000000, #ffffff)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("gradient painted in bands"));
    render("<div style='width:20px;height:10px;background:linear-gradient(20deg, #000000, #ffffff)'></div>");
    TEST_ASSERT_EQUAL(1, diag_count("gradient painted in bands"));
    render("<div style='width:20px;height:10px;background:conic-gradient(#123456, #123456 50%, #123456)'></div>");
    TEST_ASSERT_EQUAL(0, diag_count("gradient painted in bands"));
    TEST_ASSERT_EQUAL(32, rects());
    /* a transform takes the cells like the bands */
    render("<div style='width:20px;height:10px;transform:rotate(30deg);background:conic-gradient(#000000, #ffffff)'></div>");
    TEST_ASSERT_EQUAL(0, refused());
    TEST_ASSERT(rects() > 32);
}

int main(void) {
    unity_init();
    RUN_TEST(test_small_boxes);
    RUN_TEST(test_conic_small);
    RUN_TEST(test_grid_shapes);
    RUN_TEST(test_origin_and_clip);
    RUN_TEST(test_parse);
    RUN_TEST(test_vertical_children);
    RUN_TEST(test_refusals);
    RUN_TEST(test_budget_and_modes);
    RUN_TEST(test_diag_and_transform);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
