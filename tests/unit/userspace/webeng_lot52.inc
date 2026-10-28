/* Lot 52: observable geometry, pixels, limits and repeated layout. */
#include <stdio.h>
#include "../../../userspace/imgdec.h"

static int lot52_read_calls;
static int lot52_read_mode;
static unsigned char lot52_bytes[WE_IMAGE_SOURCE_MAX];
static int lot52_bytes_len;
static char lot52_last_path[256];

static int lot52_reader(void *ctx, const char *path, unsigned char *dst, int cap) {
    FILE *file;
    long size;
    int count;
    char local[320];
    (void)ctx;
    lot52_read_calls++;
    strncpy(lot52_last_path, path, sizeof(lot52_last_path) - 1);
    if (lot52_read_mode == 1) return cap + 1;
    if (lot52_read_mode == 2) return -1;
    if (lot52_read_mode == 3) {
        memset(dst, 0, 16);
        return 16;
    }
    if (lot52_read_mode == 4) {
        if (lot52_bytes_len > cap) return cap + 1;
        memcpy(dst, lot52_bytes, (size_t)lot52_bytes_len);
        return lot52_bytes_len;
    }
    if (strncmp(path, "/images/", 8)) return -1;
    snprintf(local, sizeof(local), "initrd_content%s", path);
    file = fopen(local, "rb");
    if (!file) {
        snprintf(local, sizeof(local), "../initrd_content%s", path);
        file = fopen(local, "rb");
    }
    if (!file) return -1;
    if (fseek(file, 0, SEEK_END)) { fclose(file); return -1; }
    size = ftell(file);
    if (size < 0 || size > cap) { fclose(file); return cap + 1; }
    rewind(file);
    count = (int)fread(dst, 1, (size_t)size, file);
    fclose(file);
    return count;
}

static void lot52_render(const char *html, int width) {
    we_set_image_reader(0, 0);
    lot52_read_calls = 0;
    lot52_read_mode = 0;
    lot52_last_path[0] = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), width));
}

static const we_node_t *lot52_node(const char *id) {
    int idx = we_find_id(&g_doc, id);
    if (idx < 0) {
        unity_test_fail("missing layout node", __FILE__, __LINE__);
        return &g_doc.nodes[0];
    }
    return &g_doc.nodes[idx];
}

static int lot52_has_diag(const char *fragment) {
    uint32_t i;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], fragment)) return 1;
    return 0;
}

static unsigned lot52_pixel(int x, int y, int *painted) {
    uint32_t i;
    unsigned color = 0;
    *painted = 0;
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind != OS_FB_WEB_RECT) continue;
        if (x >= it->x && y >= it->y && x < it->x + it->w && y < it->y + it->h) {
            color = it->color;
            *painted = 1;
        }
    }
    return color;
}

static void lot52_image(const char *html) {
    lot52_read_calls = 0;
    lot52_read_mode = 0;
    lot52_last_path[0] = 0;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 320));
}

static void test_l52_flex_grow_zero_keeps_basis(void) {
    const we_node_t *a, *b, *row;
    const char *html =
        "<body style='margin:0'><div id=row style='display:flex;width:120px;gap:8px'>"
        "<div id=a style='width:24px;height:12px;flex-grow:0'></div>"
        "<div id=b style='width:24px;height:20px;flex-grow:1'></div></div></body>";
    lot52_render(html, 320);
    row = lot52_node("row");
    a = lot52_node("a");
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(120, row->w);
    TEST_ASSERT_EQUAL(24, a->w);
    TEST_ASSERT_EQUAL(88, b->w);
    TEST_ASSERT_EQUAL(0, a->x);
    TEST_ASSERT_EQUAL(32, b->x);
    TEST_ASSERT_EQUAL(0, a->y);
    TEST_ASSERT_EQUAL(0, b->y);
    TEST_ASSERT_EQUAL(20, row->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_flex_weighted_grow(void) {
    const we_node_t *a, *b, *c;
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:160px;gap:5px'>"
        "<div id=a style='width:10px;height:10px;flex-grow:1'></div>"
        "<div id=b style='width:10px;height:10px;flex-grow:2'></div>"
        "<div id=c style='width:10px;height:10px;flex-grow:0'></div></div></body>";
    lot52_render(html, 320);
    a = lot52_node("a");
    b = lot52_node("b");
    c = lot52_node("c");
    TEST_ASSERT_EQUAL(50, a->w);
    TEST_ASSERT_EQUAL(90, b->w);
    TEST_ASSERT_EQUAL(10, c->w);
    TEST_ASSERT_EQUAL(55, b->x);
    TEST_ASSERT_EQUAL(150, c->x);
    TEST_ASSERT_EQUAL(160, c->x + c->w);
    TEST_ASSERT_EQUAL(a->y, b->y);
    TEST_ASSERT_EQUAL(a->y, c->y);
}

static void test_l52_flex_wrap_crosses_container(void) {
    const we_node_t *a, *b, *c;
    const char *html =
        "<body style='margin:0'><div id=row style='display:flex;flex-wrap:wrap;width:100px;gap:8px'>"
        "<div id=a style='width:48px;height:10px'></div>"
        "<div id=b style='width:48px;height:20px'></div>"
        "<div id=c style='width:40px;height:12px'></div></div></body>";
    lot52_render(html, 320);
    a = lot52_node("a");
    b = lot52_node("b");
    c = lot52_node("c");
    TEST_ASSERT_EQUAL(0, a->x);
    TEST_ASSERT_EQUAL(0, a->y);
    TEST_ASSERT_EQUAL(48, a->w);
    TEST_ASSERT_EQUAL(0, b->x);
    TEST_ASSERT_EQUAL(18, b->y);
    TEST_ASSERT_EQUAL(56, c->x);
    TEST_ASSERT_EQUAL(18, c->y);
    TEST_ASSERT_EQUAL(38, lot52_node("row")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_flex_wrap_exact_fit(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=a style='width:48px;height:12px'></div>"
        "<div id=b style='width:48px;height:10px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_wrap_then_grow_per_line(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=a style='width:60px;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:60px;height:20px;flex-grow:1'></div>"
        "<div id=c style='width:20px;height:10px;flex-grow:0'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(76, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_flex_all_zero_leaves_free_space(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px;gap:3px'>"
        "<div id=a style='width:20px;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:20px;height:10px;flex-grow:0'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(23, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(43, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_flex_fractional_grow_refused(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex'>"
        "<div id=a style='height:10px;flex-grow:0.5'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->flex_grow);
    TEST_ASSERT_TRUE(lot52_has_diag("flex-grow refused"));
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
}

static void test_l52_flex_grow_overflow_refused(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex'>"
        "<div id=a style='height:10px;flex-grow:999999999999999'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->flex_grow);
    TEST_ASSERT_TRUE(lot52_has_diag("flex-grow refused"));
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
}

static void test_l52_grid_explicit_rows(void) {
    const we_node_t *a, *b, *c, *d;
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(2, 1fr);grid-template-rows:20px 40px;gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:12px'></div>"
        "<div id=c style='height:10px'></div><div id=d style='height:10px'></div></div></body>";
    lot52_render(html, 320);
    a = lot52_node("a");
    b = lot52_node("b");
    c = lot52_node("c");
    d = lot52_node("d");
    TEST_ASSERT_EQUAL(48, a->w);
    TEST_ASSERT_EQUAL(52, b->x);
    TEST_ASSERT_EQUAL(0, a->y);
    TEST_ASSERT_EQUAL(0, b->y);
    TEST_ASSERT_EQUAL(24, c->y);
    TEST_ASSERT_EQUAL(24, d->y);
    TEST_ASSERT_EQUAL(0, c->x);
    TEST_ASSERT_EQUAL(52, d->x);
    TEST_ASSERT_EQUAL(64, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l52_grid_fraction_rows_definite_height(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;width:100px;height:100px;grid-template-columns:repeat(2, 1fr);grid-template-rows:repeat(2, 1fr);gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b></div>"
        "<div id=c style='height:10px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(-1, lot52_node("g")->row_size[0]);
    TEST_ASSERT_EQUAL(-1, lot52_node("g")->row_size[1]);
}

static void test_l52_grid_mixed_rows(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;width:90px;height:90px;grid-template-rows:20px 1fr 1fr;gap:5px'>"
        "<div id=a style='height:5px'></div><div id=b style='height:5px'></div>"
        "<div id=c style='height:5px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(3, g_doc.perf.grid_lines);
}

static void test_l52_grid_rejects_nine_columns(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-columns:repeat(9, 1fr)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l52_grid_rejects_nine_rows(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-rows:repeat(9, 1fr)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l52_float_left_text_beside(void) {
    const os_fb_web_item_t *text;
    const char *html =
        "<body style='margin:0'><div id=box style='width:120px'>"
        "<div id=f style='float:left;width:40px;height:30px;background:red'></div>"
        "hello world</div></body>";
    lot52_render(html, 320);
    text = find_text("hello");
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL(40, text->x);
    TEST_ASSERT_EQUAL(1, text->y);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("box")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_right_text_beside(void) {
    const os_fb_web_item_t *text;
    const char *html =
        "<body style='margin:0'><div id=box style='width:120px'>"
        "<div id=f style='float:right;width:40px;height:30px;background:blue'></div>"
        "hello world</div></body>";
    lot52_render(html, 320);
    text = find_text("hello");
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL(0, text->x);
    TEST_ASSERT_EQUAL(1, text->y);
    TEST_ASSERT_EQUAL(80, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->y);
    TEST_ASSERT_TRUE(text->x + text->w <= lot52_node("f")->x);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_expires_and_text_uses_full_width(void) {
    const os_fb_web_item_t *a, *b, *c;
    const char *html =
        "<body style='margin:0'><div style='width:96px'>"
        "<div style='float:left;width:48px;height:20px'></div>"
        "aaaa bbbb cccc dddd eeee</div></body>";
    lot52_render(html, 320);
    a = find_text("aaaa");
    b = find_text("bbbb");
    c = find_text("cccc dddd");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL(48, a->x);
    TEST_ASSERT_EQUAL(1, a->y);
    TEST_ASSERT_EQUAL(48, b->x);
    TEST_ASSERT_EQUAL(11, b->y);
    TEST_ASSERT_EQUAL(0, c->x);
    TEST_ASSERT_EQUAL(21, c->y);
}

static void test_l52_float_full_width_waits_for_bottom(void) {
    const os_fb_web_item_t *text;
    const char *html =
        "<body style='margin:0'><div style='width:80px'>"
        "<div id=f style='float:left;width:80px;height:25px'></div>word</div></body>";
    lot52_render(html, 320);
    text = find_text("word");
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL(0, text->x);
    TEST_ASSERT_EQUAL(26, text->y);
    TEST_ASSERT_EQUAL(80, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_margins_do_not_collapse(void) {
    const os_fb_web_item_t *text;
    const char *html =
        "<body style='margin:0'><div id=box style='width:120px'>"
        "<div id=f style='float:left;width:24px;height:10px;margin:4px;padding:2px;border:1px solid red'></div>"
        "word</div></body>";
    lot52_render(html, 320);
    text = find_text("word");
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL(4, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("f")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("f")->h);
    TEST_ASSERT_EQUAL(38, text->x);
    TEST_ASSERT_EQUAL(1, text->y);
    TEST_ASSERT_EQUAL(24, lot52_node("box")->h);
}

static void test_l52_table_rows_and_columns(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:120px'>"
        "<tr id=r1><td id=a>A</td><td id=b>B</td></tr>"
        "<tr id=r2><td id=c>C</td><td id=d>D</td></tr></table></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(WE_DISP_TABLE, lot52_node("t")->display);
    TEST_ASSERT_EQUAL(WE_DISP_ROW, lot52_node("r1")->display);
    TEST_ASSERT_EQUAL(WE_DISP_CELL, lot52_node("a")->display);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
}

static void test_l52_table_column_widths_shared_across_rows(void) {
    const char *html =
        "<body style='margin:0'><table style='width:120px'>"
        "<tr><td id=a style='width:40px'>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td></tr></table></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->y);
}

static void test_l52_table_spans_are_refused(void) {
    const char *html =
        "<body style='margin:0'><table id=t><tr>"
        "<td id=a colspan=2>forbidden</td><td id=b>B</td></tr></table></body>";
    lot52_render(html, 320);
    TEST_ASSERT_TRUE(lot52_has_diag("table span refused"));
    TEST_ASSERT_TRUE(lot52_has_diag("table refused after span"));
    TEST_ASSERT_EQUAL(1, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
    TEST_ASSERT_NULL(find_text("forbidden"));
    TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
}

static void test_l52_table_oversized_columns_are_bounded(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'><tr>"
        "<td id=a style='width:200px'>A</td><td id=b style='width:200px'>B</td>"
        "</tr></table></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->width);
}

static void test_l52_image_initrd_pixels(void) {
    static unsigned char source[8192], scratch[8192];
    imgdec_t expected;
    const we_node_t *img;
    int bytes, x, y, painted;
    unsigned color;
    unsigned char rgb[3];
    const char *html =
        "<body style='margin:0'><img id=im src='/images/mire-g1.png' alt='failed'></body>";
    lot52_image(html);
    img = lot52_node("im");
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(48, img->w);
    TEST_ASSERT_EQUAL(32, img->h);
    TEST_ASSERT_EQUAL(48, g_doc.nitems);
    TEST_ASSERT_NULL(find_text("[failed]"));
    bytes = lot52_reader(0, "/images/mire-g1.png", source, sizeof(source));
    TEST_ASSERT_TRUE(bytes > 0);
    TEST_ASSERT_EQUAL(0, imgdec_open(source, bytes, scratch, sizeof(scratch), &expected));
    for (y = 0; y < expected.h; y++) {
        for (x = 0; x < expected.w; x++) {
            imgdec_pixel(&expected, x, y, rgb);
            color = lot52_pixel(img->x + x, img->y + y, &painted);
            TEST_ASSERT_EQUAL(1, painted);
            TEST_ASSERT_EQUAL(((unsigned)rgb[0] << 16) | ((unsigned)rgb[1] << 8) | rgb[2], color);
        }
    }
    we_set_image_reader(0, 0);
}

static void test_l52_image_too_complex_is_atomic(void) {
    const char *html =
        "<body style='margin:0'><img id=im src='/images/mire420.jpg' alt='complex'></body>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_NOT_NULL(find_text("[complex]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image display budget exceeded"));
    TEST_ASSERT_EQUAL(OS_FB_WEB_TEXT, g_doc.items[0].kind);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    we_set_image_reader(0, 0);
}

static void test_l52_image_network_path_never_read(void) {
    const char *html =
        "<body><img src='https://example.com/a.png' alt='remote'></body>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_NOT_NULL(find_text("[remote]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image local path refused"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_large_source_refused(void) {
    const char *html = "<img src='/images/large.png' alt='large'>";
    lot52_render(html, 320);
    lot52_read_mode = 1;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_NOT_NULL(find_text("[large]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image source exceeds limit"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_bad_decode_keeps_alt(void) {
    const char *html = "<img src='/images/bad.png' alt='bad'>";
    lot52_render(html, 320);
    lot52_read_mode = 3;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_NOT_NULL(find_text("[bad]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image format refused"));
    we_set_image_reader(0, 0);
}

static void test_l52_min_width_expands_content_box(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='width:20px;min-width:60px;padding:3px;border:2px solid red'>x</div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->min_width);
    TEST_ASSERT_EQUAL(5, find_text("x")->x);
    TEST_ASSERT_EQUAL(6, find_text("x")->y);
}

static void test_l52_max_width_limits_auto_box(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='max-width:80px'>aaaa bbbb cccc</div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_NOT_NULL(find_text("aaaa bbbb"));
    TEST_ASSERT_NOT_NULL(find_text("cccc"));
    TEST_ASSERT_EQUAL(11, find_text("cccc")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
}

static void test_l52_min_beats_conflicting_max(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='width:20px;min-width:100px;max-width:40px'></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->min_width);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->max_width);
}

static void test_l52_relative_moves_paint_and_descendants(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:100px;height:80px'>"
        "<div id=a style='width:40px;height:20px;position:relative;left:12px;top:9px;background:red'><span>word</span></div>"
        "<div id=b style='height:10px'></div></div></body>";
    int painted;
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(9, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(12, find_text("word")->x);
    TEST_ASSERT_EQUAL(10, find_text("word")->y);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_pixel(12, 9, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
}

static void test_l52_relative_clamps_to_containing_box(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:100px;height:80px;padding:4px'>"
        "<div id=a style='width:40px;height:20px;position:relative;left:1000px;top:1000px'></div>"
        "</div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(104, lot52_node("a")->x + lot52_node("a")->w);
    TEST_ASSERT_EQUAL(84, lot52_node("a")->y + lot52_node("a")->h);
    TEST_ASSERT_EQUAL(108, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(88, lot52_node("p")->h);
}

static void test_l52_disc_list_marker(void) {
    const char *html =
        "<body style='margin:0'><ul style='margin:0'><li id=a>first</li><li id=b>second</li></ul></body>";
    lot52_render(html, 320);
    TEST_ASSERT_NOT_NULL(find_text("*"));
    TEST_ASSERT_EQUAL(0, find_text("*")->x);
    TEST_ASSERT_EQUAL(1, find_text("*")->y);
    TEST_ASSERT_EQUAL(16, find_text("first")->x);
    TEST_ASSERT_EQUAL(16, find_text("second")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
}

static void test_l52_decimal_list_marker(void) {
    const char *html =
        "<body style='margin:0'><ol style='margin:0;padding-left:24px'><li id=a>first</li><li id=b>second</li></ol></body>";
    lot52_render(html, 320);
    TEST_ASSERT_NOT_NULL(find_text("1."));
    TEST_ASSERT_NOT_NULL(find_text("2."));
    TEST_ASSERT_EQUAL(0, find_text("1.")->x);
    TEST_ASSERT_EQUAL(1, find_text("1.")->y);
    TEST_ASSERT_EQUAL(11, find_text("2.")->y);
    TEST_ASSERT_EQUAL(24, find_text("first")->x);
    TEST_ASSERT_EQUAL(24, find_text("second")->x);
}

static void test_l52_legacy_auto_equal_columns(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;gap:6px'>"
        "<div id=a style='height:10px'>A</div>"
        "<div id=b style='height:20px'>B</div>"
        "<div id=c style='height:15px'>C</div></div></body>";
    const we_node_t *a, *b, *c;
    lot52_render(html, 120);
    a = lot52_node("a");
    b = lot52_node("b");
    c = lot52_node("c");
    TEST_ASSERT_EQUAL(36, a->w);
    TEST_ASSERT_EQUAL(36, b->w);
    TEST_ASSERT_EQUAL(36, c->w);
    TEST_ASSERT_EQUAL(42, b->x);
    TEST_ASSERT_EQUAL(84, c->x);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, b->y);
    TEST_ASSERT_EQUAL(0, c->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_legacy_column_with_margins(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-direction:column;gap:5px'>"
        "<div id=a style='height:10px;margin:2px'></div>"
        "<div id=b style='height:20px;margin:3px'></div></div></body>";
    const we_node_t *a, *b;
    lot52_render(html, 120);
    a = lot52_node("a");
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(2, a->x);
    TEST_ASSERT_EQUAL(2, a->y);
    TEST_ASSERT_EQUAL(116, a->w);
    TEST_ASSERT_EQUAL(3, b->x);
    TEST_ASSERT_EQUAL(22, b->y);
    TEST_ASSERT_EQUAL(114, b->w);
    TEST_ASSERT_EQUAL(45, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_column_grow_keeps_zero_fixed(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-direction:column;height:100px;gap:10px'>"
        "<div id=a style='height:20px;flex-grow:0'></div>"
        "<div id=b style='height:20px;flex-grow:1'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->height);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_column_wrap_moves_next_column(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:50px;gap:5px'>"
        "<div id=a style='width:30px;height:20px'></div>"
        "<div id=b style='width:30px;height:20px'></div>"
        "<div id=c style='width:40px;height:20px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_column_wrap_requires_definite_height(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-direction:column;flex-wrap:wrap;gap:3px'>"
        "<div id=a style='height:10px'></div>"
        "<div id=b style='height:20px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_TRUE(lot52_has_diag("column wrap requires height"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(13, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(33, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_column_wrap_reports_horizontal_limit(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;flex-direction:column;flex-wrap:wrap;width:64px;height:20px;gap:5px'>"
        "<div id=a style='width:40px;height:20px'>A</div>"
        "<div id=b style='width:40px;height:20px'>B</div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_NULL(find_text("B"));
    TEST_ASSERT_TRUE(lot52_has_diag("column wrap width exceeded"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
}

static void test_l52_unset_grow_does_not_steal_explicit_grow(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px;gap:4px'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px;flex-grow:1'></div></div></body>";
    const we_node_t *a, *b;
    lot52_render(html, 120);
    a = lot52_node("a");
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(-1, a->flex_grow);
    TEST_ASSERT_EQUAL(1, b->flex_grow);
    TEST_ASSERT_EQUAL(20, a->w);
    TEST_ASSERT_EQUAL(76, b->w);
    TEST_ASSERT_EQUAL(24, b->x);
    TEST_ASSERT_EQUAL(100, b->x + b->w);
}

static void test_l52_flex_hidden_item_has_no_gap(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=a style='width:48px;height:10px'></div>"
        "<div id=h style='display:none;width:100px;height:100px'>hidden</div>"
        "<div id=b style='width:48px;height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("h")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_flex_gap_zero_overrides_rule(void) {
    const char *html =
        "<style>div.row{display:flex;gap:20px}</style>"
        "<body style='margin:0'><div class=row id=r style='gap:0'>"
        "<div id=a style='width:20px;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:20px;height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 100);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->gap);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
}

static void test_l52_flex_inline_zero_wins_cascade(void) {
    const char *html =
        "<style>.grow{flex-grow:1}</style><body style='margin:0'>"
        "<div style='display:flex;width:100px'>"
        "<div id=a class=grow style='flex-grow:0;width:25px;height:10px'></div>"
        "<div id=b class=grow style='width:25px;height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->flex_grow);
    TEST_ASSERT_EQUAL(1, lot52_node("b")->flex_grow);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l52_grow_auto_basis_is_zero(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px;gap:4px'>"
        "<div id=a style='height:10px;flex-grow:1'></div>"
        "<div id=b style='height:10px;flex-grow:3'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(72, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(28, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(-1, lot52_node("b")->width);
}

static void test_l52_grow_percent_basis_uses_container(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:200px'>"
        "<div id=a style='width:25%;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:25%;height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(150, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->width_pct);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->width_pct);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l52_grow_accounts_for_box_extras(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px;gap:4px'>"
        "<div id=a style='width:20px;padding:2px;border:1px solid red;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:20px;padding:2px;border:1px solid blue;height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(26, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l52_grow_cap_redistributes_unused_share(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='width:10px;max-width:20px;height:10px;flex-grow:1'></div>"
        "<div id=b style='width:10px;height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->max_width);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l52_all_grow_caps_leave_space(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='width:10px;max-width:20px;height:10px;flex-grow:1'></div>"
        "<div id=b style='width:10px;max-width:30px;height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_grow_rounding_is_deterministic(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:101px'>"
        "<div id=a style='height:10px;flex-grow:1'></div>"
        "<div id=b style='height:10px;flex-grow:1'></div>"
        "<div id=c style='height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(68, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(101, lot52_node("c")->x + lot52_node("c")->w);
}

static void test_l52_negative_free_space_does_not_shrink(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px;gap:4px'>"
        "<div id=a style='width:70px;height:10px;flex-grow:0'></div>"
        "<div id=b style='width:70px;height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(74, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_nested_flex_keeps_outer_workspace(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:120px'>"
        "<div id=inner style='display:flex;width:40px;flex-grow:0'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div>"
        "<div id=c style='height:20px;flex-grow:1'></div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(40, lot52_node("inner")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_wrap_uses_tallest_line_item(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:30px'></div>"
        "<div id=c style='width:40px;height:12px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(46, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_wrap_oversized_first_item_terminates(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=a style='width:150px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(150, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_wrap_accounts_for_margins(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=a style='width:44px;height:10px;margin:2px'></div>"
        "<div id=b style='width:44px;height:10px;margin:2px'></div>"
        "<div id=c style='width:44px;height:10px;margin:2px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(54, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_grid_eight_columns_and_rows(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;width:80px;height:80px;grid-template-columns:repeat(8, 1fr);grid-template-rows:repeat(8, 1fr)'>"
        "<div id=a style='height:5px'></div><div id=b style='height:5px'></div>"
        "<div></div><div></div><div></div><div></div><div></div><div id=h></div>"
        "<div id=i style='height:5px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(8, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(8, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("h")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("i")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("i")->y);
    TEST_ASSERT_EQUAL(8, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(80, lot52_node("g")->h);
}

static void test_l52_grid_auto_fraction_rows_use_content(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(2, 1fr);grid-template-rows:repeat(2, 1fr);gap:3px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:20px'></div>"
        "<div id=c style='height:12px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grid_implicit_rows_after_template(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-rows:20px;gap:3px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:12px'></div>"
        "<div id=c style='height:14px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(3, g_doc.perf.grid_lines);
}

static void test_l52_grid_empty_explicit_rows_keep_space(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-rows:10px 20px;gap:4px'></div>"
        "<div id=after style='height:5px'></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("after")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grid_zero_pixel_row_is_not_auto(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-rows:0px 20px;gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:5px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->row_size[0]);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->row_size[1]);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l52_grid_fixed_rows_larger_than_height(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;height:20px;grid-template-rows:30px 1fr;gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:5px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grid_rejects_zero_tracks(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-columns:repeat(0, 1fr);grid-template-rows:repeat(0, 1fr)'>"
        "<div id=a style='height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.ndiag);
}

static void test_l52_grid_malformed_repeat_does_not_replace_valid(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-rows:20px 30px;grid-template-rows:repeat(2, 1fr)junk;gap:4px'>"
        "<div id=a style='height:5px'></div><div id=b style='height:5px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->row_size[0]);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->row_size[1]);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(54, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l52_grid_rejects_fraction_weight_other_than_one(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-rows:10px 2fr'>"
        "<div id=a style='height:10px'></div><div id=b style='height:12px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l52_grid_rejects_fixed_column_syntax(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-columns:20px 1fr'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("grid columns require 1fr"));
}

static void test_l52_grid_track_integer_overflow_refused(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-rows:999999999999999999px'>"
        "<div id=a style='height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l52_grid_remainder_stays_inside_container(void) {
    const char *html =
        "<body style='margin:0'><div style='display:grid;width:101px;grid-template-columns:repeat(3, 1fr);gap:2px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(33, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(69, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(101, lot52_node("c")->x + lot52_node("c")->w);
}

static void test_l52_grid_padding_defines_track_origin(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;width:100px;padding:3px;border:2px solid red;grid-template-columns:repeat(2, 1fr);grid-template-rows:20px 30px;gap:4px'>"
        "<div id=a style='height:5px'></div><div></div><div id=c style='height:5px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(29, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(110, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(64, lot52_node("g")->h);
}

static void test_l52_grid_hidden_item_does_not_take_cell(void) {
    const char *html =
        "<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:repeat(2, 1fr);grid-template-rows:20px 30px;gap:4px'>"
        "<div id=a style='height:10px'></div><div id=h style='display:none'>hidden</div>"
        "<div id=b style='height:10px'></div><div id=c style='height:10px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("h")->w);
    TEST_ASSERT_NULL(find_text("hidden"));
}

static void test_l52_grid_nested_tracks_keep_outer_indexes(void) {
    const char *html =
        "<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:repeat(2, 1fr);grid-template-rows:20px 30px;gap:4px'>"
        "<div id=inner style='display:grid;grid-template-rows:5px 7px'><div id=a></div><div id=b></div></div>"
        "<div id=c></div><div id=d></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(48, lot52_node("inner")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(52, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(4, g_doc.perf.grid_lines);
}

static void test_l52_grid_repeat_accepts_spaces_and_case(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-columns:REPEAT(2, 1FR);grid-template-rows:repeat(2 , 1fr );gap:2px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>";
    lot52_render(html, 100);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(51, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(12, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grid_row_cascade_uses_inline_value(void) {
    const char *html =
        "<style>#g{display:grid;grid-template-rows:10px 20px}</style>"
        "<body style='margin:0'><div id=g style='grid-template-rows:30px 40px;gap:5px'>"
        "<div id=a style='height:5px'></div><div id=b style='height:5px'></div></div></body>";
    lot52_render(html, 100);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_rows);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->row_size[0]);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->row_size[1]);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(75, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grid_layout_twice_resets_counters(void) {
    const char *html =
        "<body style='margin:0'><div style='display:grid;grid-template-columns:repeat(2, 1fr);grid-template-rows:20px 30px;gap:4px'>"
        "<div id=a>one</div><div id=b>two</div><div id=c>three</div></div></body>";
    uint32_t first_items;
    lot52_render(html, 100);
    first_items = g_doc.nitems;
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(98, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(102, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(first_items, g_doc.nitems);
    TEST_ASSERT_EQUAL(200, g_doc.perf.page_w);
    TEST_ASSERT_EQUAL(54, g_doc.perf.page_h);
}

static void test_l52_two_opposing_floats_leave_middle(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=a style='float:left;width:24px;height:30px'></div>"
        "<div id=b style='float:right;width:32px;height:30px'></div>"
        "word</div></body>";
    const os_fb_web_item_t *text;
    lot52_render(html, 160);
    text = find_text("word");
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(88, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(24, text->x);
    TEST_ASSERT_EQUAL(1, text->y);
    TEST_ASSERT_TRUE(text->x + text->w <= 88);
    TEST_ASSERT_EQUAL(2, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
}

static void test_l52_two_left_floats_share_line(void) {
    const char *html =
        "<body style='margin:0'><div style='width:120px'>"
        "<div id=a style='float:left;width:24px;height:30px'></div>"
        "<div id=b style='float:left;width:32px;height:20px'></div>"
        "word</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(56, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.floats);
}

static void test_l52_two_right_floats_share_line(void) {
    const char *html =
        "<body style='margin:0'><div style='width:120px'>"
        "<div id=a style='float:right;width:24px;height:30px'></div>"
        "<div id=b style='float:right;width:32px;height:20px'></div>"
        "word</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(96, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(64, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(0, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.floats);
}

static void test_l52_second_float_waits_for_first_bottom(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:100px'>"
        "<div id=a style='float:left;width:60px;height:20px'></div>"
        "<div id=b style='float:right;width:60px;height:30px'></div>"
        "word</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, find_text("word")->x);
    TEST_ASSERT_EQUAL(21, find_text("word")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("p")->h);
}

static void test_l52_float_scope_does_not_leak_to_next_block(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:100px'>"
        "<div id=f style='float:left;width:40px;height:30px'></div>one</div>"
        "<div id=next>two</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(40, find_text("one")->x);
    TEST_ASSERT_EQUAL(1, find_text("one")->y);
    TEST_ASSERT_EQUAL(0, find_text("two")->x);
    TEST_ASSERT_EQUAL(31, find_text("two")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("next")->y);
    TEST_ASSERT_EQUAL(160, lot52_node("next")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_inside_inline_wrapper(void) {
    const char *html =
        "<body style='margin:0'><div style='width:100px'><span>"
        "<span id=f style='float:left;width:40px;height:30px;background:red'></span>"
        "word</span></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("f")->h);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(40, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_hidden_float_does_not_exclude_text(void) {
    const char *html =
        "<body style='margin:0'><div style='width:100px'>"
        "<div id=f style='display:none;float:left;width:40px;height:30px'></div>"
        "word</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(0, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(0, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(10, g_doc.perf.page_h);
}

static void test_l52_oversized_float_is_bounded(void) {
    const char *html =
        "<body style='margin:0'><div style='width:80px'>"
        "<div id=f style='float:right;width:500px;height:20px'></div>word</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(500, lot52_node("f")->width);
    TEST_ASSERT_EQUAL(21, find_text("word")->y);
    TEST_ASSERT_EQUAL(0, find_text("word")->x);
    TEST_ASSERT_EQUAL(30, g_doc.perf.page_h);
}

static void test_l52_float_center_aligns_in_remaining_space(void) {
    const char *html =
        "<body style='margin:0'><div style='width:120px;text-align:center'>"
        "<div style='float:left;width:40px;height:30px'></div>word</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(64, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(32, find_text("word")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_right_aligns_in_remaining_space(void) {
    const char *html =
        "<body style='margin:0'><div style='width:120px;text-align:right'>"
        "<div style='float:right;width:40px;height:30px'></div>word</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(48, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(80, find_text("word")->x + find_text("word")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_link_hit_uses_excluded_position(void) {
    const char *html =
        "<body style='margin:0'><div style='width:120px'>"
        "<div style='float:left;width:40px;height:30px'></div>"
        "<a href='next.html'>word</a></div></body>";
    char href[32];
    lot52_render(html, 160);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(40, find_text("word")->x);
    TEST_ASSERT_EQUAL(9, we_link_at(&g_doc, 42, 3, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("next.html", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 2, 3, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("", href);
}

static void test_l52_float_limit_reports_refusal(void) {
    char html[2048];
    int i, len;
    strcpy(html, "<body style='margin:0'><div style='width:100px'>");
    for (i = 0; i < WE_FLOATS_MAX + 1; i++)
        strcat(html, "<div style='float:left;width:1px;height:1px'></div>");
    strcat(html, "word</div></body>");
    len = (int)strlen(html);
    TEST_ASSERT_TRUE(len < (int)sizeof(html));
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(WE_FLOATS_MAX, g_doc.perf.floats);
    TEST_ASSERT_TRUE(lot52_has_diag("float capacity exceeded"));
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l52_table_sections_share_columns(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:120px'>"
        "<thead><tr><th id=a>A</th><th id=b>B</th></tr></thead>"
        "<tbody><tr><td id=c>C</td><td id=d>D</td></tr></tbody>"
        "<tfoot><tr><td id=e>E</td><td id=f>F</td></tr></tfoot></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("e")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->bold);
    TEST_ASSERT_EQUAL(0, find_text("C")->bold);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(3, g_doc.perf.table_rows);
}

static void test_l52_table_implied_cell_and_row_end(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'>"
        "<tr id=r1><td id=a>A<td id=b>B<tr id=r2><td id=c>C<td id=d>D</table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_table_tall_cell_sets_next_row(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'>"
        "<tr id=r1><td id=a style='height:30px'>A</td><td id=b>B</td></tr>"
        "<tr id=r2><td id=c>C</td><td id=d>D</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(30, lot52_node("r1")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
}

static void test_l52_table_row_background_behind_cells(void) {
    const char *html =
        "<body style='margin:0'><table style='width:100px'><tr id=r style='height:20px;background:red'>"
        "<td id=a style='background:blue'>A</td><td id=b>B</td></tr></table></body>";
    int painted;
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0x0000ffU, lot52_pixel(10, 5, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_pixel(70, 15, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_pixel(10, 15, &painted));
    TEST_ASSERT_EQUAL(1, painted);
}

static void test_l52_table_span_one_is_supported(void) {
    const char *html =
        "<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=1 rowspan=1>A</td><td id=b>B</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
}

static void test_l52_table_rowspan_two_refuses_whole_table(void) {
    const char *html =
        "<body style='margin:0'><table id=t><tr>"
        "<td id=a rowspan=2>A</td><td id=b>B</td></tr><tr><td>C</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_TRUE(lot52_has_diag("table span refused"));
    TEST_ASSERT_TRUE(lot52_has_diag("table refused after span"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
    TEST_ASSERT_NULL(find_text("A"));
    TEST_ASSERT_NULL(find_text("C"));
}

static void test_l52_table_nine_columns_refused(void) {
    const char *html =
        "<body style='margin:0'><table id=t><tr>"
        "<td>A</td><td>B</td><td>C</td><td>D</td><td>E</td>"
        "<td>F</td><td>G</td><td>H</td><td>I</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_TRUE(lot52_has_diag("table columns exceed limit"));
    TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
    TEST_ASSERT_NULL(find_text("A"));
    TEST_ASSERT_NULL(find_text("I"));
}

static void test_l52_table_eight_columns_are_inside(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:80px'><tr>"
        "<td id=a>A</td><td>B</td><td>C</td><td>D</td><td>E</td>"
        "<td>F</td><td>G</td><td id=h>H</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("h")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("h")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("h")->x + lot52_node("h")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_table_missing_cell_leaves_empty_column(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'>"
        "<tr><td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
}

static void test_l52_table_nested_inside_cell(void) {
    const char *html =
        "<body style='margin:0'><table style='width:120px'><tr>"
        "<td id=outer><table id=inner><tr><td id=a>A</td><td id=b>B</td></tr></table></td>"
        "<td id=c>C</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(60, lot52_node("outer")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("inner")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
}

static void test_l52_table_percent_columns_use_table_width(void) {
    const char *html =
        "<body style='margin:0'><table style='width:200px'><tr>"
        "<td id=a style='width:25%'>A</td><td id=b>B</td></tr></table></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(150, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->width_pct);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
}

static void test_l52_table_text_wrap_changes_row_height(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:80px'><tr id=r1>"
        "<td id=a>aaaa bbbb</td><td id=b>B</td></tr>"
        "<tr id=r2><td>C</td><td>D</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, find_text("aaaa")->y);
    TEST_ASSERT_EQUAL(11, find_text("bbbb")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r1")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("r2")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
}

static void test_l52_image_no_reader_keeps_alt(void) {
    const char *html = "<img src='/images/mire-g1.png' alt='offline'>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_NOT_NULL(find_text("[offline]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image reader unavailable"));
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
}

static void test_l52_image_read_failure_keeps_alt(void) {
    const char *html = "<img src='/images/missing.png' alt='missing'>";
    lot52_render(html, 120);
    we_set_image_reader(lot52_reader, 0);
    lot52_read_mode = 2;
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 120));
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL_STRING("/images/missing.png", lot52_last_path);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[missing]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image read failed"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_parent_traversal_never_read(void) {
    const char *html = "<img src='/images/../mire-g1.png' alt='parent'>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[parent]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image local path refused"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_encoded_traversal_never_read(void) {
    const char *html = "<img src='/images/%2e%2e/a.png' alt='encoded'>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_NOT_NULL(find_text("[encoded]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image local path refused"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_protocol_relative_never_read(void) {
    const char *html = "<img src='//example.com/image.png' alt='host'>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_NOT_NULL(find_text("[host]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image local path refused"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_relative_path_diagnosed(void) {
    const char *html = "<img src='mire.png' alt='relative'>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_NOT_NULL(find_text("[relative]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image local path refused"));
    we_set_image_reader(0, 0);
}

static void test_l52_hidden_image_does_not_call_decoder(void) {
    const char *html =
        "<body><div style='display:none'><img src='/images/mire-g1.png' alt='hidden'></div></body>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_NULL(find_text("[hidden]"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_output_cap_diagnosed(void) {
    const char *html =
        "<img id=im src='/images/mire-g1.png' alt='too_wide' style='width:65px'>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[too_wide]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    we_set_image_reader(0, 0);
}

static void test_l52_image_zero_height_diagnosed(void) {
    const char *html =
        "<img src='/images/mire-g1.png' alt='zero' style='height:0'>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[zero]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    we_set_image_reader(0, 0);
}

static void test_l52_image_scaled_jpeg_uses_shared_decoder(void) {
    static unsigned char source[8192], scratch[65536];
    imgdec_t expected;
    const char *html =
        "<body style='margin:0'><img id=im src='/images/mire420.jpg' alt='jpeg' style='width:4px;height:4px'></body>";
    int bytes, x, y, painted;
    unsigned char rgb[3];
    unsigned color;
    lot52_image(html);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(4, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(4, lot52_node("im")->h);
    TEST_ASSERT_NULL(find_text("[jpeg]"));
    bytes = lot52_reader(0, "/images/mire420.jpg", source, sizeof(source));
    TEST_ASSERT_TRUE(bytes > 0);
    TEST_ASSERT_EQUAL(0, imgdec_open(source, bytes, scratch, sizeof(scratch), &expected));
    for (y = 0; y < 4; y++) for (x = 0; x < 4; x++) {
        imgdec_pixel(&expected, x * expected.w / 4, y * expected.h / 4, rgb);
        color = lot52_pixel(x, y, &painted);
        TEST_ASSERT_EQUAL(1, painted);
        TEST_ASSERT_EQUAL(((unsigned)rgb[0] << 16) | ((unsigned)rgb[1] << 8) | rgb[2], color);
    }
    TEST_ASSERT_TRUE(g_doc.nitems <= 16);
    we_set_image_reader(0, 0);
}

static void test_l52_progressive_image_is_not_second_jpeg_engine(void) {
    const char *html =
        "<body style='margin:0'><img id=im src='/images/mire-prog.jpg' alt='progressive' style='width:4px;height:4px'></body>";
    int painted;
    lot52_image(html);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(4, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(4, lot52_node("im")->h);
    (void)lot52_pixel(3, 3, &painted);
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_NULL(find_text("[progressive]"));
    TEST_ASSERT_TRUE(g_doc.nitems <= 16);
    we_set_image_reader(0, 0);
}

static void test_l52_two_images_have_independent_paint(void) {
    const char *html =
        "<body style='margin:0'><img id=a src='/images/mire-g1.png' alt='a' style='width:8px;height:4px'>"
        "<img id=b src='/images/mire-g2-i.png' alt='b' style='width:8px;height:4px'></body>";
    int painted;
    lot52_image(html);
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
    TEST_ASSERT_EQUAL(2, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_pixel(0, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    (void)lot52_pixel(7, 7, &painted);
    TEST_ASSERT_EQUAL(1, painted);
    we_set_image_reader(0, 0);
}

static void test_l52_image_in_table_cell_uses_cell_origin(void) {
    const char *html =
        "<body style='margin:0'><table style='width:120px'><tr><td>A</td>"
        "<td id=cell><img id=im src='/images/mire-g1.png' alt='image'></td></tr></table></body>";
    int painted;
    lot52_image(html);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(60, lot52_node("cell")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("cell")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(61, 5, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    we_set_image_reader(0, 0);
}

static void test_l52_image_block_natural_width(void) {
    const char *html =
        "<body style='margin:0'><img id=im src='/images/mire-g1.png' alt='block' style='display:block'>"
        "<div id=next>after</div></body>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("next")->y);
    TEST_ASSERT_EQUAL(33, find_text("after")->y);
    TEST_ASSERT_NULL(find_text("[block]"));
    we_set_image_reader(0, 0);
}

static void test_l52_image_budget_exhaustion_does_not_corrupt_prior_items(void) {
    char html[2048];
    int i;
    strcpy(html, "<body style='margin:0'>");
    for (i = 0; i < 55; i++) strcat(html, "<b>x</b>");
    strcat(html, "<img src='/images/mire-g1.png' alt='budget'></body>");
    lot52_image(html);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_TRUE(lot52_has_diag("image display budget exceeded"));
    TEST_ASSERT_EQUAL(56, g_doc.nitems);
    TEST_ASSERT_NOT_NULL(find_text("[budget]"));
    TEST_ASSERT_EQUAL(OS_FB_WEB_TEXT, g_doc.items[0].kind);
    TEST_ASSERT_EQUAL_STRING("x", g_doc.items[0].text);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    we_set_image_reader(0, 0);
}

static void test_l52_relayout_image_resets_geometry_and_counters(void) {
    const char *html =
        "<body style='margin:0'><img id=im src='/images/mire-g1.png' alt='repeat'></body>";
    lot52_image(html);
    TEST_ASSERT_EQUAL(48, g_doc.nitems);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 120));
    TEST_ASSERT_EQUAL(48, g_doc.nitems);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(120, g_doc.perf.page_w);
    we_set_image_reader(0, 0);
}

static void test_l52_min_width_percentage_uses_parent_content(void) {
    const char *html =
        "<body style='margin:0'><div style='width:200px;padding:10px'>"
        "<div id=a style='width:20px;min-width:50%'>x</div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->min_pct);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->min_width);
    TEST_ASSERT_EQUAL(10, find_text("x")->x);
}

static void test_l52_max_width_percentage_limits_auto(void) {
    const char *html =
        "<body style='margin:0'><div style='width:200px;padding:10px'>"
        "<div id=a style='max-width:25%;padding:2px;border:1px solid red'>x</div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->max_pct);
    TEST_ASSERT_EQUAL(13, find_text("x")->x);
    TEST_ASSERT_EQUAL(14, find_text("x")->y);
}

static void test_l52_negative_min_width_refused(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='width:40px;min-width:-10px'></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->min_width);
    TEST_ASSERT_TRUE(lot52_has_diag("width constraint refused"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->min_pct);
}

static void test_l52_negative_percentage_constraint_refused(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='width:40px;max-width:-10%'></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->max_width);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->max_pct);
    TEST_ASSERT_TRUE(lot52_has_diag("width constraint refused"));
}

static void test_l52_width_constraints_reset_to_none(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='min-width:100px;max-width:50px;min-width:auto;max-width:none;width:40px'></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->min_width);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->max_width);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->min_pct);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->max_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grow_respects_min_width_before_distribution(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='min-width:40px;height:10px;flex-grow:0'></div>"
        "<div id=b style='height:10px;flex-grow:1'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->min_width);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l52_relative_negative_offsets_clamp_at_origin(void) {
    const char *html =
        "<body style='margin:0'><div style='width:100px;height:80px;padding:4px'>"
        "<div id=a style='position:relative;left:-50px;top:-50px;width:40px;height:20px'>word</div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(4, find_text("word")->x);
    TEST_ASSERT_EQUAL(5, find_text("word")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
}

static void test_l52_relative_keeps_normal_flow_height(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='height:100px;width:100px'>"
        "<div id=a style='height:20px;width:40px;position:relative;top:30px'></div>"
        "<div id=b style='height:10px'></div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(100, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
}

static void test_l52_relative_auto_height_does_not_escape_unknown_bottom(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:100px'>"
        "<div id=a style='height:20px;width:40px;position:relative;top:30px;left:10px'>word</div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
}

static void test_l52_relative_link_hit_moves_with_paint(void) {
    const char *html =
        "<body style='margin:0'><div style='width:120px;height:80px'>"
        "<div id=a style='width:40px;height:20px;position:relative;left:20px;top:10px'>"
        "<a href='next.html'>word</a></div></div></body>";
    char href[32];
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(20, find_text("word")->x);
    TEST_ASSERT_EQUAL(11, find_text("word")->y);
    TEST_ASSERT_EQUAL(9, we_link_at(&g_doc, 22, 13, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("next.html", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 2, 3, href, sizeof(href)));
}

static void test_l52_relative_position_static_overrides_rule(void) {
    const char *html =
        "<style>#a{position:relative;left:20px;top:10px}</style>"
        "<body style='margin:0'><div style='width:100px;height:80px'>"
        "<div id=a style='position:static;width:40px;height:20px'>word</div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->relative);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
}

static void test_l52_absolute_position_is_diagnosed(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='position:absolute;left:20px;top:10px;width:40px;height:20px'></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->relative);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("position refused"));
}

static void test_l52_list_none_does_not_paint_marker(void) {
    const char *html =
        "<body style='margin:0'><ul style='list-style-type:none;margin:0'>"
        "<li id=a>first</li><li id=b>second</li></ul></body>";
    lot52_render(html, 160);
    TEST_ASSERT_NULL(find_text("*"));
    TEST_ASSERT_NULL(find_text("1."));
    TEST_ASSERT_NOT_NULL(find_text("first"));
    TEST_ASSERT_NOT_NULL(find_text("second"));
    TEST_ASSERT_EQUAL(WE_LIST_NONE, lot52_node("a")->list_style);
    TEST_ASSERT_EQUAL(2, g_doc.nitems);
}

static void test_l52_list_square_marker_inherits_color(void) {
    const char *html =
        "<body style='margin:0'><ul style='list-style-type:square;color:red;margin:0'>"
        "<li id=a>first</li></ul></body>";
    lot52_render(html, 160);
    TEST_ASSERT_NOT_NULL(find_text("#"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("#")->color);
    TEST_ASSERT_EQUAL(0xff0000U, find_text("first")->color);
    TEST_ASSERT_EQUAL(0, find_text("#")->x);
    TEST_ASSERT_EQUAL(16, find_text("first")->x);
    TEST_ASSERT_EQUAL(WE_LIST_SQUARE, lot52_node("a")->list_style);
}

static void test_l52_hidden_list_item_does_not_advance_number(void) {
    const char *html =
        "<body style='margin:0'><ol style='margin:0'>"
        "<li id=a>first</li><li id=h style='display:none'>hidden</li><li id=b>second</li></ol></body>";
    lot52_render(html, 160);
    TEST_ASSERT_NOT_NULL(find_text("1."));
    TEST_ASSERT_NOT_NULL(find_text("2."));
    TEST_ASSERT_NULL(find_text("3."));
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_EQUAL(11, find_text("2.")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("h")->h);
}

static void test_l52_nested_list_counter_is_local(void) {
    const char *html =
        "<body style='margin:0'><ol style='margin:0'>"
        "<li id=a>outer<ul style='margin:0'><li id=b>inner</li></ul></li>"
        "<li id=c>second</li></ol></body>";
    lot52_render(html, 200);
    TEST_ASSERT_NOT_NULL(find_text("1."));
    TEST_ASSERT_NOT_NULL(find_text("2."));
    TEST_ASSERT_NOT_NULL(find_text("*"));
    TEST_ASSERT_EQUAL(WE_LIST_DECIMAL, lot52_node("a")->list_style);
    TEST_ASSERT_EQUAL(WE_LIST_DISC, lot52_node("b")->list_style);
    TEST_ASSERT_EQUAL(WE_LIST_DECIMAL, lot52_node("c")->list_style);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
}

static void test_l52_list_counter_reaches_two_digits(void) {
    const char *html =
        "<body style='margin:0'><ol style='margin:0'>"
        "<li>a</li><li>b</li><li>c</li><li>d</li><li>e</li>"
        "<li>f</li><li>g</li><li>h</li><li>i</li><li id=last>ten</li></ol></body>";
    lot52_render(html, 200);
    TEST_ASSERT_NOT_NULL(find_text("10."));
    TEST_ASSERT_EQUAL(90, lot52_node("last")->y);
    TEST_ASSERT_EQUAL(91, find_text("10.")->y);
    TEST_ASSERT_EQUAL(8, find_text("10.")->x);
    TEST_ASSERT_EQUAL(40, find_text("ten")->x);
    TEST_ASSERT_EQUAL(20, g_doc.nitems);
}

static void test_l52_image_source_dimensions_over_cap(void) {
    const char *html = "<img src='/images/wide.pnm' alt='dimensions'>";
    const char *header = "P6\n65 1\n255\n";
    lot52_render(html, 160);
    memset(lot52_bytes, 0, sizeof(lot52_bytes));
    memcpy(lot52_bytes, header, strlen(header));
    lot52_bytes_len = (int)strlen(header) + 65 * 3;
    lot52_read_mode = 4;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 160));
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[dimensions]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image dimensions exceed limit"));
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    we_set_image_reader(0, 0);
}

static void test_l52_image_max_source_size_is_accepted(void) {
    const char *html = "<img id=im src='/images/exact.pnm' alt='exact'>";
    const char *header = "P6\n1 1\n255\n";
    int offset = (int)strlen(header), painted;
    lot52_render(html, 160);
    memset(lot52_bytes, 0, sizeof(lot52_bytes));
    memcpy(lot52_bytes, header, (size_t)offset);
    lot52_bytes[offset] = 12;
    lot52_bytes[offset + 1] = 34;
    lot52_bytes[offset + 2] = 56;
    lot52_bytes_len = WE_IMAGE_SOURCE_MAX;
    lot52_read_mode = 4;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 160));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(1, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(1, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0x0c2238U, lot52_pixel(0, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    we_set_image_reader(0, 0);
}

static void test_l52_image_max_dimensions_can_be_one_rectangle(void) {
    const char *html = "<img id=im src='/images/max.pnm' alt='max'>";
    const char *header = "P6\n64 64\n255\n";
    lot52_render(html, 160);
    memset(lot52_bytes, 0, sizeof(lot52_bytes));
    memcpy(lot52_bytes, header, strlen(header));
    lot52_bytes_len = (int)strlen(header) + 64 * 64 * 3;
    lot52_read_mode = 4;
    we_set_image_reader(lot52_reader, 0);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 160));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(64, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(64, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(64, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(64, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(0, g_doc.items[0].color);
    we_set_image_reader(0, 0);
}

static void test_l52_table_all_fixed_widths_share_remaining_space(void) {
    const char *html =
        "<body style='margin:0'><table style='width:120px'><tr>"
        "<td id=a style='width:20px'>A</td><td id=b style='width:40px'>B</td>"
        "</tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->width);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
}

static void test_l52_table_rounding_does_not_expand_fixed_column(void) {
    const char *html =
        "<body style='margin:0'><table style='width:101px'><tr>"
        "<td id=a style='width:20px'>A</td><td id=b>B</td><td id=c>C</td>"
        "</tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(41, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(61, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(101, lot52_node("c")->x + lot52_node("c")->w);
}

static void test_l52_layout_clamps_viewport_before_narrowing(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='background:red;height:10px'>bounded</div></body>";
    lot52_render(html, 1000000000);
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, g_doc.perf.page_w);
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, g_doc.items[0].w);
    TEST_ASSERT_EQUAL(10, g_doc.items[0].h);
    TEST_ASSERT_EQUAL(10, g_doc.perf.page_h);
}

static void test_l52_deep_padding_never_wraps_coordinates(void) {
    char html[4096];
    uint32_t i;
    strcpy(html, "<body style='margin:0'>");
    for (i = 0; i < 15; i++) strcat(html, "<div style='padding:2000px;background:red'>");
    strcat(html, "end");
    for (i = 0; i < 15; i++) strcat(html, "</div>");
    strcat(html, "</body>");
    lot52_render(html, 120);
    for (i = 0; i < g_doc.nitems; i++) {
        TEST_ASSERT_TRUE(g_doc.items[i].x >= -WE_LAYOUT_MAX);
        TEST_ASSERT_TRUE(g_doc.items[i].x <= WE_LAYOUT_MAX);
        TEST_ASSERT_TRUE(g_doc.items[i].y >= -WE_LAYOUT_MAX);
        TEST_ASSERT_TRUE(g_doc.items[i].y <= WE_LAYOUT_MAX);
        TEST_ASSERT_TRUE(g_doc.items[i].w <= WE_LAYOUT_MAX);
        TEST_ASSERT_TRUE(g_doc.items[i].h <= WE_LAYOUT_MAX);
    }
    TEST_ASSERT_TRUE(g_doc.perf.page_h <= WE_LAYOUT_MAX);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l52_grow_percentage_cap_redistributes(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;width:200px;gap:10px'>"
        "<div id=a style='width:10px;max-width:25%;height:10px;flex-grow:1'></div>"
        "<div id=b style='width:10px;height:20px;flex-grow:1'></div></div></body>";
    const we_node_t *a, *b;
    lot52_render(html, 320);
    a = lot52_node("a");
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(50, a->w);
    TEST_ASSERT_EQUAL(140, b->w);
    TEST_ASSERT_EQUAL(0, a->x);
    TEST_ASSERT_EQUAL(60, b->x);
    TEST_ASSERT_EQUAL(0, a->y);
    TEST_ASSERT_EQUAL(0, b->y);
    TEST_ASSERT_EQUAL(200, b->x + b->w);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(25, a->max_pct);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l52_grow_minimum_wins_lower_maximum(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;width:100px'>"
        "<div id=a style='width:10px;min-width:40px;max-width:20px;height:10px;flex-grow:1'></div>"
        "<div id=b style='width:10px;height:20px;flex-grow:1'></div></div></body>";
    const we_node_t *a, *b;
    lot52_render(html, 160);
    a = lot52_node("a");
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(40, a->w);
    TEST_ASSERT_EQUAL(60, b->w);
    TEST_ASSERT_EQUAL(40, b->x);
    TEST_ASSERT_EQUAL(0, a->y);
    TEST_ASSERT_EQUAL(0, b->y);
    TEST_ASSERT_EQUAL(40, a->min_width);
    TEST_ASSERT_EQUAL(20, a->max_width);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(100, b->x + b->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_largest_grow_coefficient_stays_bounded(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;width:100px'>"
        "<div id=a style='height:10px;flex-grow:1000'></div>"
        "<div id=b style='height:20px;flex-grow:1000'></div></div></body>";
    const we_node_t *a, *b;
    lot52_render(html, 160);
    a = lot52_node("a");
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(1000, a->flex_grow);
    TEST_ASSERT_EQUAL(1000, b->flex_grow);
    TEST_ASSERT_EQUAL(50, a->w);
    TEST_ASSERT_EQUAL(50, b->w);
    TEST_ASSERT_EQUAL(50, b->x);
    TEST_ASSERT_EQUAL(0, a->y);
    TEST_ASSERT_EQUAL(0, b->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_wrap_origin_includes_parent_border_and_padding(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;padding:3px;border:2px solid red;gap:4px'>"
        "<div id=a style='width:60px;height:10px'></div>"
        "<div id=b style='width:60px;height:20px'></div></div></body>";
    const we_node_t *a, *b, *row;
    lot52_render(html, 160);
    a = lot52_node("a");
    b = lot52_node("b");
    row = lot52_node("r");
    TEST_ASSERT_EQUAL(5, a->x);
    TEST_ASSERT_EQUAL(5, a->y);
    TEST_ASSERT_EQUAL(5, b->x);
    TEST_ASSERT_EQUAL(19, b->y);
    TEST_ASSERT_EQUAL(60, a->w);
    TEST_ASSERT_EQUAL(60, b->w);
    TEST_ASSERT_EQUAL(110, row->w);
    TEST_ASSERT_EQUAL(44, row->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l52_empty_flex_has_no_phantom_line(void) {
    const char *html =
        "<body style='margin:0'><div id=a style='display:flex;flex-wrap:wrap;width:100px;gap:20px'></div>"
        "<div id=b style='display:flex;flex-direction:column;gap:20px'></div>"
        "<div id=c style='height:10px'>after</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(1, find_text("after")->y);
    TEST_ASSERT_EQUAL(0, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(10, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grid_max_width_changes_both_columns(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;max-width:100px;grid-template-columns:repeat(2, 1fr);grid-template-rows:20px 30px;gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>";
    const we_node_t *a, *b, *c;
    lot52_render(html, 320);
    a = lot52_node("a");
    b = lot52_node("b");
    c = lot52_node("c");
    TEST_ASSERT_EQUAL(100, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(48, a->w);
    TEST_ASSERT_EQUAL(48, b->w);
    TEST_ASSERT_EQUAL(52, b->x);
    TEST_ASSERT_EQUAL(0, c->x);
    TEST_ASSERT_EQUAL(24, c->y);
    TEST_ASSERT_EQUAL(54, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l52_grid_ninth_pixel_track_refuses_whole_declaration(void) {
    const char *html =
        "<body style='margin:0'><div id=g style='display:grid;grid-template-rows:1px 2px 3px 4px 5px 6px 7px 8px 9px'>"
        "<div id=a style='height:10px'></div>"
        "<div id=b style='height:20px'></div></div></body>";
    const we_node_t *grid;
    lot52_render(html, 160);
    grid = lot52_node("g");
    TEST_ASSERT_EQUAL(0, grid->grid_rows);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, grid->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
    TEST_ASSERT_EQUAL(1, g_doc.ndiag);
}

static void test_l52_float_auto_width_uses_bounded_half_container(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=f style='float:left;height:20px'></div>word</div></body>";
    const we_node_t *floating;
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    floating = lot52_node("f");
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(-1, floating->width);
    TEST_ASSERT_EQUAL(60, floating->w);
    TEST_ASSERT_EQUAL(20, floating->h);
    TEST_ASSERT_EQUAL(0, floating->x);
    TEST_ASSERT_EQUAL(60, word->x);
    TEST_ASSERT_EQUAL(1, word->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_max_width_constrains_exclusion(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=f style='float:left;width:60px;max-width:24px;height:20px'></div>word</div></body>";
    const we_node_t *floating;
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    floating = lot52_node("f");
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(24, floating->w);
    TEST_ASSERT_EQUAL(60, floating->width);
    TEST_ASSERT_EQUAL(24, floating->max_width);
    TEST_ASSERT_EQUAL(24, word->x);
    TEST_ASSERT_EQUAL(1, word->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_none_overrides_stylesheet(void) {
    const char *html =
        "<style>#f{float:left}</style><body style='margin:0'><div style='width:120px'>"
        "<div id=f style='float:none;width:40px;height:20px'></div>word</div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(WE_FLOAT_NONE, lot52_node("f")->float_side);
    TEST_ASSERT_EQUAL(40, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("f")->h);
    TEST_ASSERT_EQUAL(0, word->x);
    TEST_ASSERT_EQUAL(21, word->y);
    TEST_ASSERT_EQUAL(0, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(30, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_float_br_uses_exclusion_on_next_line(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=f style='float:left;width:40px;height:30px'></div>one<br>two</div></body>";
    const os_fb_web_item_t *one, *two;
    lot52_render(html, 160);
    one = find_text("one");
    two = find_text("two");
    TEST_ASSERT_NOT_NULL(one);
    TEST_ASSERT_NOT_NULL(two);
    TEST_ASSERT_EQUAL(40, one->x);
    TEST_ASSERT_EQUAL(40, two->x);
    TEST_ASSERT_EQUAL(1, one->y);
    TEST_ASSERT_EQUAL(11, two->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_float_after_queued_words_starts_after_line(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>before"
        "<div id=f style='float:left;width:40px;height:20px'></div>after</div></body>";
    const os_fb_web_item_t *before, *after;
    lot52_render(html, 160);
    before = find_text("before");
    after = find_text("after");
    TEST_ASSERT_NOT_NULL(before);
    TEST_ASSERT_NOT_NULL(after);
    TEST_ASSERT_EQUAL(0, before->x);
    TEST_ASSERT_EQUAL(1, before->y);
    TEST_ASSERT_EQUAL(10, lot52_node("f")->y);
    TEST_ASSERT_EQUAL(40, after->x);
    TEST_ASSERT_EQUAL(11, after->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
}

static void test_l52_nested_float_context_uses_available_box(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=f style='float:left;width:20px;height:30px'></div>"
        "<div id=inner><div id=g style='float:left;width:24px;height:20px'></div>word</div>"
        "</div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(20, lot52_node("inner")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("inner")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(44, word->x);
    TEST_ASSERT_EQUAL(1, word->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
}

static void test_l52_table_minimum_request_is_shared_by_column(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:120px'>"
        "<tr><td id=a style='min-width:40px'>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td></tr></table></body>";
    const we_node_t *a, *b, *c, *d;
    lot52_render(html, 160);
    a = lot52_node("a");
    b = lot52_node("b");
    c = lot52_node("c");
    d = lot52_node("d");
    TEST_ASSERT_EQUAL(40, a->w);
    TEST_ASSERT_EQUAL(40, c->w);
    TEST_ASSERT_EQUAL(80, b->w);
    TEST_ASSERT_EQUAL(80, d->w);
    TEST_ASSERT_EQUAL(40, b->x);
    TEST_ASSERT_EQUAL(40, d->x);
    TEST_ASSERT_EQUAL(10, d->y);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
}

static void test_l52_table_padding_sets_cell_origin(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px;padding:3px;border:2px solid red'>"
        "<tr id=r><td id=a>A</td><td id=b>B</td></tr></table></body>";
    const we_node_t *a, *b, *table;
    lot52_render(html, 160);
    a = lot52_node("a");
    b = lot52_node("b");
    table = lot52_node("t");
    TEST_ASSERT_EQUAL(5, a->x);
    TEST_ASSERT_EQUAL(5, a->y);
    TEST_ASSERT_EQUAL(55, b->x);
    TEST_ASSERT_EQUAL(5, b->y);
    TEST_ASSERT_EQUAL(50, a->w);
    TEST_ASSERT_EQUAL(50, b->w);
    TEST_ASSERT_EQUAL(110, table->w);
    TEST_ASSERT_EQUAL(20, table->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
}

static void test_l52_css_table_display_uses_same_layout(void) {
    const char *html =
        "<body style='margin:0'><div id=t style='display:table;width:100px'>"
        "<div id=r style='display:table-row'><span id=a style='display:table-cell'>A</span>"
        "<span id=b style='display:table-cell'>B</span></div></div></body>";
    const we_node_t *a, *b;
    lot52_render(html, 160);
    a = lot52_node("a");
    b = lot52_node("b");
    TEST_ASSERT_EQUAL(WE_DISP_TABLE, lot52_node("t")->display);
    TEST_ASSERT_EQUAL(WE_DISP_ROW, lot52_node("r")->display);
    TEST_ASSERT_EQUAL(WE_DISP_CELL, a->display);
    TEST_ASSERT_EQUAL(50, a->w);
    TEST_ASSERT_EQUAL(50, b->w);
    TEST_ASSERT_EQUAL(50, b->x);
    TEST_ASSERT_EQUAL(0, b->y);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
}

static void test_l52_hidden_table_row_is_not_painted(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'>"
        "<tr style='display:none'><td id=h>hidden</td></tr>"
        "<tr id=r><td id=a>A</td><td id=b>B</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("h")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_hidden_table_cell_does_not_take_column(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'><tr>"
        "<td id=a>A</td><td id=h style='display:none'>hidden</td><td id=b>B</td>"
        "</tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(0, lot52_node("h")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_empty_table_row_explicit_height_is_kept(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'>"
        "<tr id=empty style='height:20px'></tr>"
        "<tr id=r><td id=a>A</td><td id=b>B</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(20, lot52_node("empty")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("empty")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_zero_colspan_is_refused_not_treated_as_one(void) {
    const char *html =
        "<body style='margin:0'><table id=t><tr>"
        "<td id=a colspan=0>A</td><td id=b>B</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
    TEST_ASSERT_TRUE(lot52_has_diag("table span refused"));
    TEST_ASSERT_TRUE(lot52_has_diag("table refused after span"));
    TEST_ASSERT_NULL(find_text("A"));
    TEST_ASSERT_NULL(find_text("B"));
}

static void test_l52_non_numeric_colspan_is_diagnosed(void) {
    const char *html =
        "<body style='margin:0'><table id=t><tr>"
        "<td id=a colspan=nope>A</td><td id=b>B</td></tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
    TEST_ASSERT_TRUE(lot52_has_diag("table span refused"));
    TEST_ASSERT_TRUE(lot52_has_diag("table refused after span"));
    TEST_ASSERT_EQUAL(2, g_doc.ndiag);
}

static void test_l52_header_font_weight_can_be_overridden(void) {
    const char *html =
        "<body style='margin:0'><table style='width:100px'><tr>"
        "<th id=a style='font-weight:normal'>plain</th><th id=b>bold</th></tr></table></body>";
    const os_fb_web_item_t *plain, *bold;
    lot52_render(html, 160);
    plain = find_text("plain");
    bold = find_text("bold");
    TEST_ASSERT_NOT_NULL(plain);
    TEST_ASSERT_NOT_NULL(bold);
    TEST_ASSERT_EQUAL(0, plain->bold);
    TEST_ASSERT_EQUAL(1, bold->bold);
    TEST_ASSERT_EQUAL(0, plain->x);
    TEST_ASSERT_EQUAL(50, bold->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
}

static void test_l52_link_in_table_uses_row_and_column_geometry(void) {
    const char *html =
        "<body style='margin:0'><table style='width:120px'><tr><td>A</td><td>B</td></tr>"
        "<tr><td>C</td><td id=link><a href='next.html'>word</a></td></tr></table></body>";
    const os_fb_web_item_t *word;
    char href[32];
    lot52_render(html, 160);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(60, word->x);
    TEST_ASSERT_EQUAL(11, word->y);
    TEST_ASSERT_EQUAL(60, lot52_node("link")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("link")->y);
    TEST_ASSERT_EQUAL(9, we_link_at(&g_doc, 62, 13, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("next.html", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 2, 13, href, sizeof(href)));
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
}

static void test_l52_image_missing_src_uses_default_alt(void) {
    const char *html = "<body style='margin:0'><img id=im></body>";
    const os_fb_web_item_t *alt;
    lot52_image(html);
    alt = find_text("[img]");
    TEST_ASSERT_NOT_NULL(alt);
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, alt->x);
    TEST_ASSERT_EQUAL(1, alt->y);
    TEST_ASSERT_EQUAL(40, alt->w);
    TEST_ASSERT_EQUAL(10, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    we_set_image_reader(0, 0);
}

static void test_l52_image_in_float_shares_word_exclusion(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<img id=im src='/images/mire-g1.png' alt='float' style='float:left;width:48px;height:32px'>"
        "word</div></body>";
    const os_fb_web_item_t *word;
    int painted;
    lot52_image(html);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(48, word->x);
    TEST_ASSERT_EQUAL(1, word->y);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 2, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    we_set_image_reader(0, 0);
}

static void test_l52_relative_container_moves_image_pixels(void) {
    const char *html =
        "<body style='margin:0'><div style='width:100px;height:100px'>"
        "<div id=holder style='width:48px;height:32px;position:relative;left:8px;top:8px'>"
        "<img id=im src='/images/mire-g1.png' alt='relative'></div></div></body>";
    int painted;
    lot52_image(html);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(8, lot52_node("holder")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("holder")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(9, 10, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    (void)lot52_pixel(1, 2, &painted);
    TEST_ASSERT_EQUAL(0, painted);
    we_set_image_reader(0, 0);
}

static void test_l52_second_image_failure_keeps_first_image(void) {
    const char *html =
        "<body style='margin:0;background:red'>"
        "<img id=a src='/images/mire-g1.png' alt='first'>"
        "<img id=b src='/images/mire-g1.png' alt='second'></body>";
    int painted;
    lot52_image(html);
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("[second]"));
    TEST_ASSERT_NULL(find_text("[first]"));
    TEST_ASSERT_EQUAL(50, g_doc.nitems);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 2, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_TRUE(lot52_has_diag("image display budget exceeded"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    we_set_image_reader(0, 0);
}

static void test_l52_image_anchor_hit_uses_image_box(void) {
    const char *html =
        "<body style='margin:0'><a href='next.html'>"
        "<img id=im src='/images/mire-g1.png' alt='link'></a></body>";
    char href[32];
    lot52_image(html);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(9, we_link_at(&g_doc, 10, 10, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("next.html", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 60, 10, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 10, 40, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("", href);
    we_set_image_reader(0, 0);
}

static int lot52_reader_with_context(void *ctx, const char *path, unsigned char *dst, int cap) {
    int *counter = (int *)ctx;
    (*counter)++;
    return lot52_reader(0, path, dst, cap);
}

static void test_l52_image_reader_context_survives_parse_reset(void) {
    const char *html =
        "<body style='margin:0'><img id=im src='/images/mire-g1.png' alt='context'></body>";
    int context_calls = 0;
    lot52_read_calls = 0;
    lot52_read_mode = 0;
    we_set_image_reader(lot52_reader_with_context, &context_calls);
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 160));
    TEST_ASSERT_EQUAL(1, context_calls);
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 160));
    TEST_ASSERT_EQUAL(2, context_calls);
    TEST_ASSERT_EQUAL(2, lot52_read_calls);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    we_set_image_reader(0, 0);
}

static void test_l52_relative_nested_offsets_compose_once(void) {
    const char *html =
        "<body style='margin:0'><div style='width:200px;height:100px'>"
        "<div id=a style='width:100px;height:60px;position:relative;left:10px;top:10px'>"
        "<div id=b style='width:40px;height:20px;position:relative;left:5px;top:5px'>word</div>"
        "</div></div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 320);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(15, word->x);
    TEST_ASSERT_EQUAL(16, word->y);
    TEST_ASSERT_EQUAL(100, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relative_oversized_box_does_not_shift_to_negative_x(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:80px;height:100px'>"
        "<div id=a style='width:100px;height:20px;position:relative;left:10px;top:10px'>word</div>"
        "</div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, word->x);
    TEST_ASSERT_EQUAL(11, word->y);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relative_border_is_translated_with_background(void) {
    const char *html =
        "<body style='margin:0'><div style='width:100px;height:100px'>"
        "<div id=a style='width:40px;height:20px;border:2px solid blue;background:red;position:relative;left:10px;top:10px'></div>"
        "</div></body>";
    uint32_t i;
    int borders = 0, backgrounds = 0;
    lot52_render(html, 160);
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *item = &g_doc.items[i];
        if (item->kind == OS_FB_WEB_BORDER || item->kind == OS_FB_WEB_RECT) {
            TEST_ASSERT_EQUAL(10, item->x);
            TEST_ASSERT_EQUAL(10, item->y);
            TEST_ASSERT_EQUAL(44, item->w);
            TEST_ASSERT_EQUAL(24, item->h);
            if (item->kind == OS_FB_WEB_BORDER) borders++;
            else backgrounds++;
        }
    }
    TEST_ASSERT_EQUAL(1, borders);
    TEST_ASSERT_EQUAL(1, backgrounds);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->h);
}

static void test_l52_relative_relayout_does_not_accumulate_offsets(void) {
    const char *html =
        "<body style='margin:0'><div style='width:100px;height:80px'>"
        "<div id=a style='width:40px;height:20px;position:relative;left:10px;top:10px'>word</div>"
        "</div></body>";
    uint32_t items;
    lot52_render(html, 160);
    items = g_doc.nitems;
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 160));
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, find_text("word")->x);
    TEST_ASSERT_EQUAL(11, find_text("word")->y);
    TEST_ASSERT_EQUAL(items, g_doc.nitems);
    TEST_ASSERT_EQUAL(80, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_adjacent_ordered_lists_restart_number(void) {
    const char *html =
        "<body style='margin:0'><ol style='margin:0'><li id=a>first</li><li>second</li></ol>"
        "<ol style='margin:0'><li id=b>third</li></ol></body>";
    uint32_t i;
    int ones = 0, twos = 0;
    lot52_render(html, 160);
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *item = &g_doc.items[i];
        if (item->kind != OS_FB_WEB_TEXT) continue;
        if (!strcmp(item->text, "1.")) ones++;
        if (!strcmp(item->text, "2.")) twos++;
    }
    TEST_ASSERT_EQUAL(2, ones);
    TEST_ASSERT_EQUAL(1, twos);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(6, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_single_list_item_can_override_inherited_marker(void) {
    const char *html =
        "<body style='margin:0'><ul style='margin:0'>"
        "<li id=a style='list-style-type:none'>first</li><li id=b>second</li></ul></body>";
    uint32_t i;
    int markers = 0;
    lot52_render(html, 160);
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && !strcmp(g_doc.items[i].text, "*")) markers++;
    TEST_ASSERT_EQUAL(1, markers);
    TEST_ASSERT_EQUAL(WE_LIST_NONE, lot52_node("a")->list_style);
    TEST_ASSERT_EQUAL(WE_LIST_DISC, lot52_node("b")->list_style);
    TEST_ASSERT_EQUAL(11, find_text("*")->y);
    TEST_ASSERT_EQUAL(16, find_text("first")->x);
    TEST_ASSERT_EQUAL(16, find_text("second")->x);
    TEST_ASSERT_EQUAL(20, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(3, g_doc.nitems);
}

static void test_l52_non_li_child_does_not_increment_list_number(void) {
    const char *html =
        "<body style='margin:0'><ol style='margin:0'>"
        "<li id=a>first</li><div id=aside>aside</div><li id=b>second</li></ol></body>";
    lot52_render(html, 160);
    TEST_ASSERT_NOT_NULL(find_text("1."));
    TEST_ASSERT_NOT_NULL(find_text("2."));
    TEST_ASSERT_NULL(find_text("3."));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("aside")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(21, find_text("2.")->y);
    TEST_ASSERT_EQUAL(30, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(5, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_multiline_list_item_has_one_marker(void) {
    const char *html =
        "<body style='margin:0'><ul style='width:64px;margin:0'>"
        "<li id=a>aaaa bbbb cccc</li><li id=b>last</li></ul></body>";
    uint32_t i;
    int markers = 0;
    lot52_render(html, 160);
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && !strcmp(g_doc.items[i].text, "*")) markers++;
    TEST_ASSERT_EQUAL(2, markers);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(16, find_text("aaaa")->x);
    TEST_ASSERT_EQUAL(1, find_text("aaaa")->y);
    TEST_ASSERT_EQUAL(11, find_text("bbbb")->y);
    TEST_ASSERT_EQUAL(21, find_text("cccc")->y);
    TEST_ASSERT_EQUAL(31, find_text("last")->y);
    TEST_ASSERT_EQUAL(40, g_doc.perf.page_h);
}

static void test_l52_marker_display_budget_remains_bounded(void) {
    char html[2048];
    int i;
    strcpy(html, "<body style='margin:0'><ul style='margin:0'>");
    for (i = 0; i < 60; i++) strcat(html, "<li>x</li>");
    strcat(html, "</ul></body>");
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(OS_FB_WEB_ITEMS, g_doc.nitems);
    TEST_ASSERT_EQUAL(24, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(600, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(OS_FB_WEB_TEXT, g_doc.items[0].kind);
    TEST_ASSERT_EQUAL_STRING("x", g_doc.items[0].text);
    TEST_ASSERT_EQUAL(OS_FB_WEB_TEXT, g_doc.items[1].kind);
    TEST_ASSERT_EQUAL_STRING("*", g_doc.items[1].text);
    TEST_ASSERT_EQUAL(0, g_doc.items[1].x);
    TEST_ASSERT_EQUAL(1, g_doc.items[1].y);
    TEST_ASSERT_TRUE(g_doc.nnodes < WE_MAX_NODES);
}

static void test_l52_long_inline_style_keeps_flex_properties(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;height:auto;gap:4px;background:white'>"
        "<div id=a style='width:60px;height:10px;flex-grow:0;background:red'></div>"
        "<div id=b style='width:60px;height:10px;flex-grow:1;background:blue'></div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_TRUE(lot52_node("r")->style_len > 63);
    TEST_ASSERT_EQUAL(WE_DISP_FLEX, lot52_node("r")->display);
    TEST_ASSERT_EQUAL(1, lot52_node("r")->flex_wrap);
    TEST_ASSERT_EQUAL(4, lot52_node("r")->gap);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_node("r")->bg);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_overlong_src_is_not_read_as_a_prefix(void) {
    char html[512];
    int i;
    strcpy(html, "<img alt='long' src='/images/");
    for (i = 0; i < 280; i++) strcat(html, "a");
    strcat(html, ".png'>");
    lot52_image(html);
    TEST_ASSERT_TRUE(lot52_has_diag("attribute exceeds limit"));
    TEST_ASSERT_EQUAL(0, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_NOT_NULL(find_text("[long]"));
    TEST_ASSERT_EQUAL(1, g_doc.nitems);
    TEST_ASSERT_EQUAL(OS_FB_WEB_TEXT, g_doc.items[0].kind);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    we_set_image_reader(0, 0);
}

static void test_l52_overlong_style_is_refused_atomically(void) {
    char html[600];
    int i;
    strcpy(html, "<body style='margin:0'><div id=a style='width:10px;");
    for (i = 0; i < 280; i++) strcat(html, " ");
    strcat(html, "height:40px'>word</div></body>");
    lot52_render(html, 160);
    TEST_ASSERT_TRUE(lot52_has_diag("attribute exceeds limit"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->style_len);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(160, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l52_multiple_full_css_blocks_stay_in_css_buffer(void) {
    char html[5000];
    int i, n = 0;
    memcpy(html + n, "<style>", 7);
    n += 7;
    for (i = 0; i < 2100; i++) html[n++] = ' ';
    memcpy(html + n, "</style><style>", 15);
    n += 15;
    for (i = 0; i < 2100; i++) html[n++] = ' ';
    strcpy(html + n, "</style><body><div id=a>word</div></body>");
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(WE_CSS_MAX, g_doc.css_len);
    TEST_ASSERT_TRUE(lot52_has_diag("css source exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.nrules);
    TEST_ASSERT_NOT_NULL(find_text("word"));
    TEST_ASSERT_EQUAL(144, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l52_float_percent_width_uses_parent_before_exclusion(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=a style='float:left;width:25%;height:30px'></div>"
        "<div id=b style='float:right;width:25%;height:20px'></div>word</div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, word->x);
    TEST_ASSERT_EQUAL(1, word->y);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->width_pct);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->width_pct);
    TEST_ASSERT_EQUAL(2, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
}

static void test_l52_float_zero_height_does_not_exclude_words(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=f style='float:left;width:40px;height:0'></div>word</div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(40, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->h);
    TEST_ASSERT_EQUAL(0, word->x);
    TEST_ASSERT_EQUAL(1, word->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relative_table_moves_rows_and_cells_together(void) {
    const char *html =
        "<body style='margin:0'><div style='width:160px;height:100px'>"
        "<table id=t style='width:100px;position:relative;left:10px;top:10px'>"
        "<tr id=r><td id=a>A</td><td id=b>B</td></tr></table></div></body>";
    lot52_render(html, 200);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(60, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(100, g_doc.perf.page_h);
}

static void test_l52_flex_in_table_cell_keeps_column_bounds(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:120px'><tr>"
        "<td id=cell><div style='display:flex;gap:4px'><div id=a style='height:10px;flex-grow:0;width:20px'></div>"
        "<div id=b style='height:10px;flex-grow:1;width:20px'></div></div></td><td id=c>C</td>"
        "</tr></table></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(60, lot52_node("cell")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_grid_in_wrapped_flex_keeps_row_offsets(void) {
    const char *html =
        "<body style='margin:0'><div id=outer style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=first style='width:60px;height:10px'></div>"
        "<div id=g style='display:grid;width:60px;grid-template-rows:10px 20px;gap:2px'>"
        "<div id=a>A</div><div id=b>B</div></div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(14, lot52_node("g")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(14, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(32, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(46, lot52_node("outer")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_list_in_table_has_one_marker_per_cell_item(void) {
    const char *html =
        "<body style='margin:0'><table style='width:160px'><tr>"
        "<td id=cell><ol style='margin:0;padding-left:24px'><li id=a>one</li><li id=b>two</li></ol></td>"
        "<td id=c>C</td></tr></table></body>";
    lot52_render(html, 200);
    TEST_ASSERT_NOT_NULL(find_text("1."));
    TEST_ASSERT_NOT_NULL(find_text("2."));
    TEST_ASSERT_EQUAL(80, lot52_node("cell")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("cell")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(24, find_text("one")->x);
    TEST_ASSERT_EQUAL(24, find_text("two")->x);
    TEST_ASSERT_EQUAL(0, find_text("1.")->x);
    TEST_ASSERT_EQUAL(11, find_text("2.")->y);
    TEST_ASSERT_EQUAL(20, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relayout_float_does_not_keep_old_exclusions(void) {
    const char *html =
        "<body style='margin:0'><div id=p><div id=f style='float:right;width:40px;height:30px'></div>word</div></body>";
    uint32_t items;
    lot52_render(html, 120);
    items = g_doc.nitems;
    TEST_ASSERT_EQUAL(80, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(0, find_text("word")->x);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(160, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(0, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(items, g_doc.nitems);
    TEST_ASSERT_EQUAL(200, g_doc.perf.page_w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relayout_table_recomputes_column_widths(void) {
    const char *html =
        "<body style='margin:0'><table id=t><tr><td id=a>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td></tr></table></body>";
    uint32_t items;
    lot52_render(html, 120);
    items = g_doc.nitems;
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(items, g_doc.nitems);
    TEST_ASSERT_EQUAL(20, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relayout_wrap_recomputes_line_partition(void) {
    const char *html =
        "<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;gap:4px'>"
        "<div id=a style='width:60px;height:10px'></div>"
        "<div id=b style='width:60px;height:20px'></div></div></body>";
    lot52_render(html, 120);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(64, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(200, g_doc.perf.page_w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relative_list_moves_marker_and_words(void) {
    const char *html =
        "<body style='margin:0'><div style='width:160px;height:100px'>"
        "<ul id=list style='margin:0;width:80px;position:relative;left:10px;top:10px'>"
        "<li id=a>word</li></ul></div></body>";
    const os_fb_web_item_t *marker, *word;
    lot52_render(html, 200);
    marker = find_text("*");
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(marker);
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(10, lot52_node("list")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("list")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, marker->x);
    TEST_ASSERT_EQUAL(11, marker->y);
    TEST_ASSERT_EQUAL(26, word->x);
    TEST_ASSERT_EQUAL(11, word->y);
    TEST_ASSERT_EQUAL(100, g_doc.perf.page_h);
}


static void test_l52_empty_table_rows_keep_definite_heights(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'>"
        "<tr id=a style='height:20px'></tr><tr id=b style='height:10px'></tr>"
        "</table><div id=next>after</div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(100, lot52_node("t")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("next")->y);
    TEST_ASSERT_EQUAL(31, find_text("after")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(40, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_empty_table_has_no_phantom_row(void) {
    const char *html =
        "<body style='margin:0'><table id=t style='width:100px'></table>"
        "<div id=next style='height:10px'>after</div></body>";
    const we_node_t *table, *next;
    lot52_render(html, 160);
    table = lot52_node("t");
    next = lot52_node("next");
    TEST_ASSERT_EQUAL(WE_DISP_TABLE, table->display);
    TEST_ASSERT_EQUAL(100, table->w);
    TEST_ASSERT_EQUAL(0, table->h);
    TEST_ASSERT_EQUAL(0, table->y);
    TEST_ASSERT_EQUAL(0, next->y);
    TEST_ASSERT_EQUAL(10, next->h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(10, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, find_text("after")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l52_relative_later_child_clamps_to_parent_top(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:100px;height:100px;padding:4px'>"
        "<div id=a style='height:20px'></div>"
        "<div id=b style='width:40px;height:20px;position:relative;top:-1000px'>word</div>"
        "<div id=c style='height:10px'></div></div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    word = find_text("word");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(44, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(4, word->x);
    TEST_ASSERT_EQUAL(5, word->y);
    TEST_ASSERT_EQUAL(108, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(108, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}


static void test_l52_column_grow_auto_basis_is_zero(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='display:flex;flex-direction:column;height:100px;gap:10px'>"
        "<div id=a style='flex-grow:1'>one</div>"
        "<div id=b style='flex-grow:1'>two</div></div></body>";
    lot52_render(html, 160);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(55, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, find_text("one")->y);
    TEST_ASSERT_EQUAL(56, find_text("two")->y);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(-1, lot52_node("b")->height);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}


static void test_l52_overlong_word_clears_float_before_overflow(void) {
    const char *html =
        "<body style='margin:0'><div id=p style='width:80px'>"
        "<div id=f style='float:left;width:80px;height:20px'></div>abcdefghijkl</div></body>";
    const os_fb_web_item_t *word;
    lot52_render(html, 160);
    word = find_text("abcdefghijkl");
    TEST_ASSERT_NOT_NULL(word);
    TEST_ASSERT_EQUAL(80, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("f")->h);
    TEST_ASSERT_EQUAL(0, word->x);
    TEST_ASSERT_EQUAL(21, word->y);
    TEST_ASSERT_EQUAL(96, word->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
