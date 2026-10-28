/* Lot 53: row flex basis and bounded weighted shrinking. */
static void test_l53_row_sizing_matrix(void) {
    static const struct {
        const char *a, *b;
        int width, gap, aw, bw;
    } cases[] = {
        {"width:80px;flex-shrink:1", "width:80px;flex-shrink:1", 100, 0, 50, 50},
        {"width:80px;flex-shrink:0", "width:80px;flex-shrink:1", 100, 0, 80, 20},
        {"width:80px;flex-shrink:1", "width:80px;flex-shrink:0", 100, 0, 20, 80},
        {"width:80px;flex-shrink:1", "width:40px;flex-shrink:1", 90, 0, 60, 30},
        {"width:80px;flex-shrink:2", "width:80px;flex-shrink:1", 100, 0, 40, 60},
        {"width:80px;flex-shrink:1;min-width:70px", "width:80px;flex-shrink:1", 100, 0, 70, 30},
        {"flex-basis:80px;flex-shrink:1", "flex-basis:80px", 100, 0, 50, 50},
        {"width:10px;flex-basis:80px", "width:10px;flex-basis:80px", 100, 0, 50, 50},
        {"flex-basis:80%", "flex-basis:80%", 100, 0, 50, 50},
        {"width:80px;flex-basis:auto", "width:80px", 100, 0, 50, 50},
        {"flex-basis:0%;flex-grow:1", "flex-basis:0px;flex-grow:1", 100, 0, 50, 50},
        {"flex-basis:80px", "flex-basis:80px", 100, 10, 45, 45},
        {"flex-basis:10px;flex-grow:1", "flex-basis:10px;flex-grow:3", 100, 0, 30, 70},
        {"flex-basis:80px;max-width:60px", "flex-basis:80px", 100, 0, 40, 60},
        {"width:80px;flex-shrink:1000", "width:80px;flex-shrink:1000", 100, 0, 50, 50},
        {"flex-basis:80px;padding:2px;border:1px solid red", "flex-basis:80px", 100, 0, 53, 47},
        {"flex-basis:80px;margin:2px", "flex-basis:80px", 100, 0, 48, 48},
        {"flex-basis:80px;min-width:75%", "flex-basis:80px", 100, 0, 75, 25},
        {"width:80px;flex-shrink:0", "width:80px;flex-shrink:0", 100, 0, 80, 80},
    };
    unsigned k;
    for (k = 0; k < sizeof(cases) / sizeof(cases[0]); k++) {
        char html[900];
        const we_node_t *a, *b;
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div id=row style='display:flex;width:%dpx;gap:%dpx'>"
            "<div id=a style='%s;height:10px'>A</div>"
            "<div id=b style='%s;height:20px'>B</div></div></body>",
            cases[k].width, cases[k].gap, cases[k].a, cases[k].b);
        lot52_render(html, 320);
        a = lot52_node("a"); b = lot52_node("b");
        TEST_ASSERT_EQUAL(cases[k].aw, a->w);
        TEST_ASSERT_EQUAL(cases[k].bw, b->w);
        TEST_ASSERT_EQUAL(a->x + a->w + cases[k].gap + a->margin[1] + b->margin[3], b->x);
        TEST_ASSERT_EQUAL(a->y - a->margin[0], b->y - b->margin[0]);
        TEST_ASSERT_EQUAL(20, lot52_node("row")->h);
        TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    }
}

static void test_l53_minimum_freeze_and_unavoidable_overflow(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='flex-basis:80px;min-width:70px'></div>"
        "<div id=b style='flex-basis:80px;min-width:60px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("flex minimum prevents fit"));
}

static void test_l53_wrap_partitions_before_shrinking(void) {
    const char *html =
        "<body style='margin:0'><div id=row style='display:flex;flex-wrap:wrap;width:100px;gap:4px'>"
        "<div id=a style='flex-basis:60px;height:10px'></div>"
        "<div id=b style='flex-basis:60px;height:20px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("row")->h);
}

static void test_l53_invalid_values_do_not_replace_prior_declarations(void) {
    const char *html =
        "<body><div style='display:flex'><div id=a style='flex-basis:80px;flex-basis:-1px;flex-shrink:2;flex-shrink:0.5'></div>"
        "<div id=b style='flex-basis:999999999999px;flex-shrink:1001'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->flex_basis);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->flex_shrink);
    TEST_ASSERT_EQUAL(-1, lot52_node("b")->flex_basis);
    TEST_ASSERT_EQUAL(-1, lot52_node("b")->flex_shrink);
    TEST_ASSERT_TRUE(lot52_has_diag("flex-basis refused"));
    TEST_ASSERT_TRUE(lot52_has_diag("flex-shrink refused"));
}

static void test_l53_weight_arithmetic_large_container(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex'>"
        "<div id=a style='flex-basis:2000%;flex-shrink:1000'></div>"
        "<div id=b style='flex-basis:2000%;flex-shrink:1000'></div></div></body>";
    lot52_render(html, WE_LAYOUT_MAX);
    TEST_ASSERT_EQUAL(8000, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(8000, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(8000, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l53_nested_sizing_and_relayout(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex'>"
        "<div id=inner style='display:flex;flex-basis:80px'><div id=a style='flex-basis:80px'></div>"
        "<div id=b style='flex-basis:80px'></div></div><div id=c style='flex-basis:80px'></div></div></body>";
    lot52_render(html, 100);
    TEST_ASSERT_EQUAL(50, lot52_node("inner")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(80, lot52_node("inner")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->w);
}

static void test_l53_column_sizing_is_explicitly_refused(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;flex-direction:column'>"
        "<div id=a style='height:10px;flex-basis:80px;flex-shrink:1'></div></div></body>";
    lot52_render(html, 100);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("column basis/shrink refused"));
}

static void test_l53_shrink_rounding_exhausts_deficit(void) {
    const char *html =
        "<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='flex-basis:80px'></div><div id=b style='flex-basis:80px'></div>"
        "<div id=c style='flex-basis:80px'></div></div></body>";
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(33, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(66, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x + lot52_node("c")->w);
}

static void test_l53_maximum_node_weight_sum_is_unsigned(void) {
    char html[8192];
    unsigned i, total = 0, count = 0;
    strcpy(html, "<style>.x{flex-basis:2000%;flex-shrink:1000}</style><div id=row style='display:flex'>");
    for (i = 0; i < WE_MAX_NODES - 3; i++) strcat(html, "<div class=x></div>");
    strcat(html, "</div>");
    lot52_render(html, WE_LAYOUT_MAX);
    for (i = 0; i < g_doc.nnodes; i++) {
        const we_node_t *n = &g_doc.nodes[i];
        if (strcmp(n->cls, "x")) continue;
        TEST_ASSERT_TRUE(n->w >= 0);
        TEST_ASSERT_TRUE(n->x >= 0);
        TEST_ASSERT_TRUE(n->x + n->w <= WE_LAYOUT_MAX);
        total += (unsigned)n->w;
        count++;
    }
    TEST_ASSERT_EQUAL(WE_MAX_NODES - 3, count);
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, total);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
