static void test_l57_empty_selector_distinguishes_spaces_and_comments(void) {
    lot52_render("<style>div{height:10px}div:empty{width:32px;background:red}</style><body style='margin:0'>"
        "<div id=a></div><div id=b> </div><div id=c><!-- comment --></div><div id=d><span></span></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(320, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_empty_attributes_are_present_and_equal_empty(void) {
    lot52_render("<style>div{height:10px}[class='']{width:32px}[href]{color:red}</style><body style='margin:0'>"
        "<div id=a class='' href=''>A</div><div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_id_specificity_exceeds_all_supported_classes(void) {
    lot52_render("<style>#a{width:32px;color:red}body.x.y.z main.x.y.z section.x.y.z div.x.y.z{width:48px;color:blue}"
        "</style><body class='x y z' style='margin:0'><main class='x y z'><section class='x y z'>"
        "<div id=a class='x y z'>A</div></section></main></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_dense_named_area_and_order_keep_link_hits(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);"
        "grid-auto-flow:dense;grid-template-areas:\". . end\"'>"
        "<div id=a style='grid-column:span 2;height:10px;order:1'></div>"
        "<div id=b style='grid-column:span 2;height:10px;order:2'></div>"
        "<div id=c style='grid-area:end;height:10px'><a href='dense'>hit</a></div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 81, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("dense", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 79, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_named_areas_relayout_without_pool_growth(void) {
    uint32_t pool;
    int pass;
    lot52_render("<style>body{margin:0}#g{display:grid;width:100%;grid-template-columns:minmax(20px,1fr) 1fr;"
        "grid-template-areas:\"a b\"}#a{grid-area:a;height:10px}#b{grid-area:b;height:10px}</style>"
        "<body><div id=g><div id=a></div><div id=b></div></div></body>", 100);
    pool = g_doc.pool_used;
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    for (pass = 0; pass < 8; pass++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 120));
        TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
        TEST_ASSERT_EQUAL(pool, g_doc.pool_used);
    }
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_column_capacity_is_eight_and_diagnosed(void) {
    char html[2048];
    int i;
    strcpy(html, "<body style='margin:0'><div style='display:grid;width:160px;grid-auto-flow:column;grid-auto-columns:20px'>");
    for (i = 0; i < 9; i++) strcat(html, "<div style='height:10px;background:red'></div>");
    strcat(html, "</div></body>");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(8, g_doc.nitems);
    TEST_ASSERT_EQUAL(140, g_doc.items[7].x);
    TEST_ASSERT_EQUAL(20, g_doc.items[7].w);
    TEST_ASSERT_EQUAL(10, g_doc.items[7].h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement capacity"));
}

static void test_l57_eight_named_areas_and_exact_track_limit(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:160px;grid-template-columns:repeat(8,minmax(20px,1fr));"
        "grid-template-areas:\"a b c d e f g h\"'>"
        "<div id=a style='grid-area:a;height:10px'></div><div id=h style='grid-area:h;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(8, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(8, lot52_node("g")->area_count);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(140, lot52_node("h")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("h")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_minmax_intrinsic_width_uses_floor_and_cap(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:min-content;grid-template-columns:minmax(40px,60px) 20px'>"
        "<div id=a>abc def ghi</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->min_content);
    TEST_ASSERT_EQUAL(80, lot52_node("g")->max_content);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flex_cross_auto_margin_keeps_intrinsic_height(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:80px;height:40px;align-items:stretch'>"
        "<div id=a style='width:24px;margin-top:auto'>abc def</div></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(21, find_text("abc")->y);
    TEST_ASSERT_EQUAL(31, find_text("def")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_flex_cross_auto_negative_space_uses_start(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:80px;height:10px;align-items:end'>"
        "<div id=a style='width:24px;height:20px;margin-top:auto;margin-bottom:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_image_fit_pixel_reference_for_entire_content_box(void) {
    static const struct { const char *fit; int rw, rh, ox, oy; } cases[] = {
        {"fill",24,24,0,0}, {"contain",24,16,0,4}, {"cover",36,24,-6,0},
        {"none",48,32,-12,-4}, {"scale-down",24,16,0,4}
    };
    char html[512];
    unsigned k;
    int x, y, painted;
    for (k = 0; k < sizeof(cases) / sizeof(cases[0]); k++) {
        snprintf(html, sizeof(html), "<body style='margin:0'><img id=im src='/images/mire-g1.png' style='width:24px;height:24px;object-fit:%s;background:red'></body>", cases[k].fit);
        lot57_image(html);
        TEST_ASSERT_EQUAL(1, g_doc.perf.images);
        TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
        for (y = 0; y < 24; y++) for (x = 0; x < 24; x++) {
            unsigned expected = 0xff0000U;
            int px = x - cases[k].ox, py = y - cases[k].oy;
            if (px >= 0 && px < cases[k].rw && py >= 0 && py < cases[k].rh)
                expected = (px * 48 / cases[k].rw) & 1 ? 0xffffffU : 0U;
            TEST_ASSERT_EQUAL(expected, lot52_pixel(x, y, &painted));
            TEST_ASSERT_EQUAL(1, painted);
        }
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l57_image_intrinsic_width_in_flex_content_basis(void) {
    lot57_image("<body style='margin:0'><div style='display:flex;width:100px;align-items:start'>"
        "<img id=im src='/images/mire-g1.png' style='object-fit:contain;flex-basis:content;flex-grow:0'>"
        "<div id=b style='width:20px;height:10px;flex-grow:1'></div></div></body>");
    TEST_ASSERT_EQUAL(48, lot52_node("im")->min_content);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->max_content);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_image_in_auto_grid_track_measures_same_as_paint(void) {
    lot57_image("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:auto 1fr'>"
        "<img id=im src='/images/mire-g1.png' style='object-fit:contain'>"
        "<div id=b style='height:10px'></div></div></body>");
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(32, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_image_fit_clipped_pixels_and_link_use_visible_box(void) {
    char href[32];
    int painted;
    lot57_image("<body style='margin:0'><div style='width:12px;height:12px;overflow:hidden'>"
        "<a href='image'><img id=im src='/images/mire-g1.png' style='object-fit:cover;width:24px;height:24px'></a></div></body>");
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    (void)lot52_pixel(13, 0, &painted);
    TEST_ASSERT_EQUAL(0, painted);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 2, 2, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("image", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 13, 2, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_combined_guest_page_geometry(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot57.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot57.html", "rb");
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file);
    fclose(file);
    html[count] = 0;
    lot57_image(html);
    TEST_ASSERT_EQUAL(14, lot52_node("wrapped")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("floor")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("hole")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("hole")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("side")->x);
    TEST_ASSERT_EQUAL(64, lot52_node("side")->y);
    TEST_ASSERT_EQUAL(0x0000ffU, lot52_node("side")->bg);
    TEST_ASSERT_EQUAL(80, lot52_node("span")->w);
    TEST_ASSERT_EQUAL(84, lot52_node("span")->y);
    TEST_ASSERT_EQUAL(124, lot52_node("auto")->y);
    TEST_ASSERT_EQUAL(134, lot52_node("contain")->y);
    TEST_ASSERT_EQUAL(158, lot52_node("cover")->y);
    TEST_ASSERT_EQUAL(182, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.images);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_named_area_case_is_preserved(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;"
        "grid-template-areas:\"Head head\"'><div id=a style='grid-area:Head;height:10px'></div>"
        "<div id=b style='grid-area:head;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->area_count);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_selector_backtracking_budget_is_diagnosed(void) {
    char html[4096];
    int i;
    strcpy(html, "<style>.missing~div~div~div{width:10px}</style><body style='margin:0'>");
    for (i = 0; i < 140; i++) strcat(html, "<div></div>");
    strcat(html, "</body>");
    lot52_render(html, 320);
    TEST_ASSERT_TRUE(lot52_has_diag("selector work budget"));
    TEST_ASSERT_TRUE(g_doc.ndiag <= WE_MAX_DIAG);
    TEST_ASSERT_TRUE(g_doc.diag_dropped > 0);
    TEST_ASSERT_TRUE(g_doc.nnodes <= WE_MAX_NODES);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_numeric_margin_shorthand_clears_cross_auto_bits(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:80px;height:40px;align-items:start'>"
        "<div id=a style='width:20px;height:10px;margin:auto;margin:0'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->margin_auto);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
