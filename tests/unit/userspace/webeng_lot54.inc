/* Lot 54: main-axis geometry for sized columns in the one engine. */
static void test_l54_column_sizing_matrix(void) {
    static const struct { const char *a, *b; int height, gap, ah, bh; } cases[] = {
        {"height:80px;flex-shrink:1", "height:80px", 100, 0, 50, 50},
        {"height:80px;flex-shrink:0", "height:80px;flex-shrink:1", 100, 0, 80, 20},
        {"height:80px;flex-shrink:1", "height:80px;flex-shrink:0", 100, 0, 20, 80},
        {"flex-basis:80px", "flex-basis:40px", 90, 0, 60, 30},
        {"flex-basis:80px;flex-shrink:2", "flex-basis:80px", 100, 0, 40, 60},
        {"flex-basis:80px;min-height:70px", "flex-basis:80px", 100, 0, 70, 30},
        {"height:10px;flex-basis:80px", "height:10px;flex-basis:80px", 100, 0, 50, 50},
        {"flex-basis:80%", "flex-basis:80%", 100, 0, 50, 50},
        {"height:80px;flex-basis:auto", "height:80px", 100, 0, 50, 50},
        {"height:80px;flex-basis:0%;flex-grow:1", "flex-basis:0;flex-grow:1", 100, 0, 50, 50},
        {"flex-basis:80px", "flex-basis:80px", 100, 10, 45, 45},
        {"flex-basis:10px;flex-grow:1", "flex-basis:10px;flex-grow:3", 100, 0, 30, 70},
        {"flex-basis:10px;flex-grow:0", "flex-basis:10px;flex-grow:1", 100, 0, 10, 90},
        {"flex-basis:0;flex-grow:1;max-height:20px", "flex-basis:0;flex-grow:1", 100, 0, 20, 80},
        {"flex-basis:80px;max-height:60px", "flex-basis:80px", 100, 0, 40, 60},
        {"flex-basis:80px;min-height:70px;max-height:40px", "flex-basis:80px", 100, 0, 70, 30},
        {"flex-basis:80px;padding:2px;border:1px solid red", "flex-basis:80px", 100, 0, 53, 47},
        {"flex-basis:80px;margin:2px", "flex-basis:80px", 100, 0, 48, 48},
        {"flex-basis:80px;flex-shrink:1000", "flex-basis:80px;flex-shrink:1000", 100, 0, 50, 50},
        {"height:80px;flex-shrink:0", "height:80px;flex-shrink:0", 100, 0, 80, 80},
        {"flex-basis:80px", "flex-basis:80px", 0, 0, 0, 0},
        {"flex-basis:0px", "flex-basis:0px", 100, 0, 0, 0},
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[900];
        const we_node_t *a, *b;
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div id=col style='display:flex;flex-direction:column;width:100px;height:%dpx;gap:%dpx'>"
            "<div id=a style='%s'></div><div id=b style='%s'></div></div></body>",
            cases[i].height, cases[i].gap, cases[i].a, cases[i].b);
        lot52_render(html, 320);
        a = lot52_node("a"); b = lot52_node("b");
        TEST_ASSERT_EQUAL(cases[i].ah, a->h);
        TEST_ASSERT_EQUAL(cases[i].bh, b->h);
        TEST_ASSERT_EQUAL(a->y + a->h + a->margin[2] + b->margin[0] + cases[i].gap, b->y);
        TEST_ASSERT_EQUAL(a->x - a->margin[3], b->x - b->margin[3]);
        TEST_ASSERT_EQUAL(100 - a->margin[1] - a->margin[3], a->w);
        TEST_ASSERT_EQUAL(100 - b->margin[1] - b->margin[3], b->w);
        TEST_ASSERT_EQUAL(cases[i].height, lot52_node("col")->h);
        TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    }
}

static void test_l54_column_minimum_overflow_diagnostic(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;height:100px'>"
        "<div id=a style='flex-basis:80px;min-height:70px'></div>"
        "<div id=b style='flex-basis:80px;min-height:60px'></div></div></body>", 200);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("flex minimum prevents fit"));
}

static void test_l54_column_wrap_basis_and_grow(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:100px;gap:5px'>"
        "<div id=a style='width:30px;flex-basis:60px;flex-grow:1'></div>"
        "<div id=b style='width:40px;flex-basis:60px;flex-grow:1'></div></div></body>", 200);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l54_column_wrap_percent_cross_size_once(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:50px;gap:4px'>"
        "<div id=a style='width:40%;flex-basis:80px'></div>"
        "<div id=b style='width:40%;flex-basis:80px'></div></div></body>", 200);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l54_column_wrap_horizontal_bound(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;flex-wrap:wrap;width:64px;height:20px;gap:5px'>"
        "<div id=a style='width:40px;flex-basis:20px;background:red'></div>"
        "<div id=b style='width:40px;flex-basis:20px;background:blue'></div></div></body>", 200);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("column wrap width exceeded"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l54_column_indefinite_height_percent_refused(void) {
    lot52_render("<body style='margin:0'><div id=col style='display:flex;flex-direction:column;flex-wrap:wrap;gap:3px'>"
        "<div id=a style='height:10px;flex-basis:80%;flex-grow:1'></div>"
        "<div id=b style='height:20px;flex-basis:30px'></div></div></body>", 200);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(43, lot52_node("col")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("column percent requires height"));
    TEST_ASSERT_TRUE(lot52_has_diag("column wrap requires height"));
}

static void test_l54_block_height_limits_and_invalid_values(void) {
    lot52_render("<body style='margin:0'><div id=a style='height:10px;min-height:30px;min-height:bogus'></div>"
        "<div id=b style='height:80px;max-height:20px;max-height:-1px'></div>"
        "<div id=c style='min-height:40px;max-height:20px'></div>"
        "<div id=d style='min-height:2001px;max-height:999999999999px'></div>"
        "<div id=e style='min-height:0;max-height:0'>word</div></body>", 200);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("d")->min_height);
    TEST_ASSERT_EQUAL(-1, lot52_node("d")->max_height);
    TEST_ASSERT_EQUAL(0, lot52_node("e")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("height limit refused"));
}

static void test_l54_column_parent_maximum_is_main_size(void) {
    lot52_render("<body style='margin:0'><div id=col style='display:flex;flex-direction:column;height:150px;max-height:100px'>"
        "<div id=a style='height:80px'></div><div id=b style='height:80px'></div></div></body>", 200);
    TEST_ASSERT_EQUAL(100, lot52_node("col")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
}

static void test_l54_nested_axes_and_relayout_restore_styles(void) {
    lot52_render("<body style='margin:0'><div id=col style='display:flex;flex-direction:column;height:100px'>"
        "<div id=row style='display:flex;flex-basis:80px;min-height:20px;max-height:90px'>"
        "<div id=a style='flex-basis:80px'></div><div id=b style='flex-basis:80px'></div></div>"
        "<div id=c style='flex-basis:80px'></div></div></body>", 100);
    TEST_ASSERT_EQUAL(50, lot52_node("row")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(-1, lot52_node("row")->height);
    TEST_ASSERT_EQUAL(20, lot52_node("row")->min_height);
    TEST_ASSERT_EQUAL(90, lot52_node("row")->max_height);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(50, lot52_node("row")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
}

static void test_l54_column_rounding_and_painted_positions(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;height:100px'>"
        "<div id=a style='flex-basis:80px'>one</div><div id=b style='flex-basis:80px'>two</div>"
        "<div id=c style='flex-basis:80px'>three</div></div></body>", 200);
    TEST_ASSERT_EQUAL(33, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(34, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(66, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(1, find_text("one")->y);
    TEST_ASSERT_EQUAL(34, find_text("two")->y);
    TEST_ASSERT_EQUAL(67, find_text("three")->y);
}

static void test_l54_column_maximum_unsigned_weights(void) {
    char html[8192];
    unsigned i, total = 0, count = 0;
    int col;
    strcpy(html, "<style>.x{flex-basis:2000%;flex-shrink:1000}</style>"
        "<div id=col style='display:flex;flex-direction:column'>");
    for (i = 0; i < WE_MAX_NODES - 3; i++) strcat(html, "<div class=x></div>");
    strcat(html, "</div>");
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    col = we_find_id(&g_doc, "col");
    TEST_ASSERT_TRUE(col >= 0);
    g_doc.nodes[col].height = WE_LAYOUT_MAX;
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    for (i = 0; i < g_doc.nnodes; i++) {
        const we_node_t *n = &g_doc.nodes[i];
        if (strcmp(n->cls, "x")) continue;
        TEST_ASSERT_TRUE(n->h >= 0);
        TEST_ASSERT_TRUE(n->y >= 0);
        TEST_ASSERT_TRUE(n->y + n->h <= WE_LAYOUT_MAX);
        total += (unsigned)n->h;
        count++;
    }
    TEST_ASSERT_EQUAL(WE_MAX_NODES - 3, count);
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, total);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
