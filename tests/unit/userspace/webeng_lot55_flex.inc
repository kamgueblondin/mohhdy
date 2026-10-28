/* Both axes exercise one grow/shrink and packing implementation. */
static void test_l55_flex_justify_matrix(void) {
    static const struct { const char *mode; int a, b, c; } cases[] = {
        {"start", 0, 20, 40}, {"flex-start", 0, 20, 40},
        {"end", 60, 80, 100}, {"flex-end", 60, 80, 100},
        {"center", 30, 50, 70}, {"space-between", 0, 50, 100},
        {"space-around", 10, 50, 90}, {"space-evenly", 15, 50, 85}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[800];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div id=r style='display:flex;width:120px;justify-content:%s'>"
            "<div id=a style='width:20px;height:10px'></div>"
            "<div id=b style='width:20px;height:10px'></div>"
            "<div id=c style='width:20px;height:10px'></div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].a, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(cases[i].b, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(cases[i].c, lot52_node("c")->x);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
        TEST_ASSERT_EQUAL(20, lot52_node("c")->w);
        TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
        TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_flex_column_justify_matrix(void) {
    static const struct { const char *mode; int a, b, c; } cases[] = {
        {"start", 0, 20, 40}, {"end", 60, 80, 100}, {"center", 30, 50, 70},
        {"space-between", 0, 50, 100}, {"space-around", 10, 50, 90}, {"space-evenly", 15, 50, 85}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[800];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div id=r style='display:flex;flex-direction:column;width:100px;height:120px;justify-content:%s'>"
            "<div id=a style='height:20px'></div><div id=b style='height:20px'></div>"
            "<div id=c style='height:20px'></div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].a, lot52_node("a")->y);
        TEST_ASSERT_EQUAL(cases[i].b, lot52_node("b")->y);
        TEST_ASSERT_EQUAL(cases[i].c, lot52_node("c")->y);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
        TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(120, lot52_node("r")->h);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_flex_single_item_distributions(void) {
    static const struct { const char *mode; int x; } cases[] = {
        {"space-between", 0}, {"space-around", 40}, {"space-evenly", 40}, {"center", 40}, {"end", 80}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[400];
        snprintf(html, sizeof(html), "<body style='margin:0'><div style='display:flex;width:100px;justify-content:%s'>"
            "<div id=a style='width:20px;height:10px'>A</div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].x, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(cases[i].x, find_text("A")->x);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_flex_justify_integer_remainder(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:101px;justify-content:space-between'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div>"
        "<div id=c style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(81, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(101, lot52_node("c")->x + lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_justify_with_fixed_gap(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:120px;gap:10px;justify-content:space-between'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div>"
        "<div id=c style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l55_flex_grow_exhausts_before_justify(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;justify-content:center'>"
        "<div id=a style='flex-basis:20px;flex-grow:1;height:10px'></div>"
        "<div id=b style='flex-basis:20px;flex-grow:1;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_capped_grow_leaves_packed_space(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;justify-content:center'>"
        "<div id=a style='flex-basis:10px;flex-grow:1;max-width:20px;height:10px'></div>"
        "<div id=b style='flex-basis:10px;flex-grow:1;max-width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_overflow_alignment_is_safe(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;justify-content:end'>"
        "<div id=a style='flex-basis:80px;flex-shrink:0;height:10px'></div>"
        "<div id=b style='flex-basis:80px;flex-shrink:0;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_TRUE(lot52_has_diag("flex minimum prevents fit"));
}

static void test_l55_flex_row_cross_alignment_matrix(void) {
    static const struct { const char *mode; int a, b; } cases[] = {
        {"start", 0, 0}, {"center", 25, 20}, {"end", 50, 40}, {"flex-end", 50, 40}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[600];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div style='display:flex;width:100px;height:60px;align-items:%s'>"
            "<div id=a style='width:20px;height:10px'>A</div>"
            "<div id=b style='width:20px;height:20px'>B</div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].a, lot52_node("a")->y);
        TEST_ASSERT_EQUAL(cases[i].b, lot52_node("b")->y);
        TEST_ASSERT_EQUAL(cases[i].a + 1, find_text("A")->y);
        TEST_ASSERT_EQUAL(cases[i].b + 1, find_text("B")->y);
        TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_flex_column_cross_alignment_matrix(void) {
    static const struct { const char *mode; int a, b; } cases[] = {
        {"start", 0, 0}, {"center", 40, 30}, {"end", 80, 60}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[600];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div style='display:flex;flex-direction:column;width:100px;height:60px;align-items:%s'>"
            "<div id=a style='width:20px;height:10px'>A</div>"
            "<div id=b style='width:40px;height:20px'>B</div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].a, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(cases[i].b, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(cases[i].a, find_text("A")->x);
        TEST_ASSERT_EQUAL(cases[i].b, find_text("B")->x);
        TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
        TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_flex_row_stretch_auto_only(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;height:60px;align-items:stretch'>"
        "<div id=a style='width:20px'>A</div><div id=b style='width:20px;height:20px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->height);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_stretch_honors_maximum(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;height:60px;align-items:stretch'>"
        "<div id=a style='width:20px;max-height:30px'>A</div>"
        "<div id=b style='width:20px;min-height:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->max_height);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->min_height);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_column_stretch_auto_only(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;width:100px;align-items:stretch'>"
        "<div id=a style='height:10px'>A</div><div id=b style='height:20px;width:30px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->width);
}

static void test_l55_flex_align_self_overrides_parent(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;height:60px;align-items:end'>"
        "<div id=a style='width:20px;height:10px;align-self:start'>A</div>"
        "<div id=b style='width:20px;height:20px;align-self:center'>B</div>"
        "<div id=c style='width:20px;height:10px;align-self:auto'>C</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(21, find_text("B")->y);
    TEST_ASSERT_EQUAL(51, find_text("C")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_cross_alignment_with_margins(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;height:60px;align-items:center'>"
        "<div id=a style='width:20px;height:10px;margin:5px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(26, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_intrinsic_auto_centered_column(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;width:100px;align-items:center'>"
        "<div id=a>one</div><div id=b>seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(38, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
}

static void test_l55_flex_axis_gaps_row_wrap(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;column-gap:10px;row-gap:7px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:10px'></div>"
        "<div id=c style='width:40px;height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(37, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_axis_gaps_column_wrap(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:50px;row-gap:5px;column-gap:9px'>"
        "<div id=a style='width:30px;flex-basis:20px'></div>"
        "<div id=b style='width:30px;flex-basis:20px'></div>"
        "<div id=c style='width:40px;flex-basis:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l55_flex_two_value_gap_shorthand(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;gap:7px 10px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:10px'></div>"
        "<div id=c style='width:40px;height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(37, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(7, lot52_node("r")->row_gap);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->col_gap);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_gap_cascade_resets_longhands(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;row-gap:7px;column-gap:10px;gap:4px'>"
        "<div id=a style='width:48px;height:10px'></div>"
        "<div id=b style='width:48px;height:10px'></div>"
        "<div id=c style='width:48px;height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("r")->row_gap);
    TEST_ASSERT_EQUAL(-1, lot52_node("r")->col_gap);
}

static void test_l55_flex_invalid_gap_preserves_axes(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;gap:4px 8px;gap:1px 2px 3px;column-gap:10%'>"
        "<div id=a style='width:20px;height:10px'></div><div id=b style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("r")->row_gap);
    TEST_ASSERT_EQUAL(8, lot52_node("r")->col_gap);
    TEST_ASSERT_EQUAL(28, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("gap refused"));
    TEST_ASSERT_TRUE(lot52_has_diag("axis gap refused"));
}

static void test_l55_flex_wrap_line_justify_independent(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap;width:100px;justify-content:end;row-gap:4px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:10px'></div>"
        "<div id=c style='width:40px;height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(14, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
}

static void test_l55_flex_align_content_matrix(void) {
    static const struct { const char *mode; int first, second; } cases[] = {
        {"start", 0, 10}, {"end", 70, 80}, {"center", 35, 45},
        {"space-between", 0, 80}, {"space-around", 17, 62}, {"space-evenly", 23, 56}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[600];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div style='display:flex;flex-wrap:wrap;width:60px;height:100px;align-content:%s'>"
            "<div id=a style='width:40px;height:10px'></div>"
            "<div id=b style='width:40px;height:20px'></div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].first, lot52_node("a")->y);
        TEST_ASSERT_EQUAL(cases[i].second, lot52_node("b")->y);
        TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
        TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
        TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_flex_align_content_stretch_lines(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-wrap:wrap;width:60px;height:100px;align-content:stretch;align-items:stretch'>"
        "<div id=a style='width:40px'>one</div>"
        "<div id=b style='width:40px'>one seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(55, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(45, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(56, find_text("seven")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_column_align_content_end(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;flex-wrap:wrap;width:100px;height:50px;align-content:end;column-gap:5px'>"
        "<div id=a style='width:20px;flex-basis:40px'></div>"
        "<div id=b style='width:30px;flex-basis:40px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_empty_alignment_no_phantom_line(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;justify-content:center;align-items:stretch'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_invalid_alignment_keeps_valid(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;height:60px;justify-content:end;justify-content:baseline;align-items:center;align-items:space-between'>"
        "<div id=a style='width:20px;height:10px;align-self:inherit'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("alignment refused"));
}

static void test_l55_flex_nested_wrapped_measurement(void) {
    lot52_render("<body style='margin:0'><div id=outer style='display:flex;width:60px;align-items:center'>"
        "<div id=inner style='display:flex;flex-wrap:wrap;flex-basis:60px;gap:3px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:20px'></div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(33, lot52_node("inner")->h);
    TEST_ASSERT_EQUAL(33, lot52_node("outer")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("inner")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_nested_column_basis_from_wrapped_row(void) {
    lot52_render("<body style='margin:0'><div id=outer style='display:flex;flex-direction:column;width:60px;gap:4px'>"
        "<div id=inner style='display:flex;flex-wrap:wrap;flex-basis:auto;gap:3px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:20px'></div></div>"
        "<div id=c style='height:10px;flex-basis:auto'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(33, lot52_node("inner")->h);
    TEST_ASSERT_EQUAL(37, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(47, lot52_node("outer")->h);
    TEST_ASSERT_EQUAL(13, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(-1, lot52_node("inner")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_flex_relayout_preserves_alignment_and_basis(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;justify-content:center;align-items:end;height:40px'>"
        "<div id=a style='flex-basis:20px;height:10px'></div>"
        "<div id=b style='flex-basis:20px;height:20px'></div></div></body>", 100);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->flex_basis);
    TEST_ASSERT_EQUAL(WE_PACK_CENTER, lot52_node("r")->justify_content);
    TEST_ASSERT_EQUAL(WE_PACK_END, lot52_node("r")->align_items);
}
