/* Grid assertions use independent literal track and placement coordinates. */
static void test_l55_grid_mixed_fixed_fraction_columns(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:20px 1fr'>"
        "<div id=a style='height:10px'>A</div><div id=b style='height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, find_text("B")->x);
    TEST_ASSERT_EQUAL(1, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_weighted_fraction_columns(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:20px 1fr 3fr'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->x + lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_weighted_fraction_rounding(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:101px;grid-template-columns:1fr 3fr'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(26, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(75, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(101, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_repeat_fixed_columns(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:repeat(3,20px);column-gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(48, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_repeat_weighted_fraction(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:101px;grid-template-columns:repeat(3,2fr)'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div>"
        "<div id=c style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(68, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(101, lot52_node("c")->x + lot52_node("c")->w);
}

static void test_l55_grid_auto_track_intrinsic_width(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:auto 1fr'>"
        "<div id=a>one</div><div id=b>two</div></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(76, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_auto_tracks_take_maximum_per_column(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:200px;grid-template-columns:auto auto;gap:3px'>"
        "<div id=a>one</div><div id=b>xx</div>"
        "<div id=c>seven</div><div id=d>four</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(43, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(13, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l55_grid_fixed_overflow_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:80px 80px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("grid width overflow"));
    TEST_ASSERT_EQUAL(160, lot52_node("b")->x + lot52_node("b")->w);
}

static void test_l55_grid_fixed_track_justify_matrix(void) {
    static const struct { const char *mode; int a, b; } cases[] = {
        {"start", 0, 20}, {"end", 60, 80}, {"center", 30, 50},
        {"space-between", 0, 80}, {"space-around", 15, 65}, {"space-evenly", 20, 60}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[600];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:20px 20px;justify-content:%s'>"
            "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>", cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].a, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(cases[i].b, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_grid_mixed_weighted_rows(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-rows:10px 1fr 3fr;row-gap:4px;align-items:stretch'>"
        "<div id=a></div><div id=b></div><div id=c></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(21, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(61, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(39, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("c")->y + lot52_node("c")->h);
    TEST_ASSERT_EQUAL(3, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_fraction_rows_indefinite_use_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;grid-template-rows:2fr 3fr;row-gap:4px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(14, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(34, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_axis_gaps_are_independent(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:repeat(2,1fr);column-gap:10px;row-gap:7px'>"
        "<div id=a style='height:10px'></div><div id=b style='height:10px'></div>"
        "<div id=c style='height:20px'></div><div id=d style='height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(55, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(55, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(17, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(37, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_explicit_row_and_column(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:40px 60px;grid-template-rows:20px 30px'>"
        "<div id=a style='grid-column:2;grid-row:2;height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(21, find_text("A")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_explicit_items_reserve_before_auto(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:120px;grid-template-columns:repeat(3,1fr)'>"
        "<div id=a style='height:10px'>A</div><div id=b style='height:10px'>B</div>"
        "<div id=c style='grid-column:1;grid-row:1;height:10px'>C</div>"
        "<div id=d style='height:10px'>D</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_column_span_consumes_tracks(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:120px;grid-template-columns:repeat(3,1fr)'>"
        "<div id=a style='grid-column:span 2;height:10px'>A</div>"
        "<div id=b style='height:10px'>B</div><div id=c style='height:20px'>C</div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_start_span_shorthand(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:120px;grid-template-columns:repeat(3,1fr);grid-template-rows:20px 30px'>"
        "<div id=a style='grid-column:2 / span 2;grid-row:2;height:10px'>A</div>"
        "<div id=b style='height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
}

static void test_l55_grid_end_line_shorthand(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:120px;grid-template-columns:repeat(3,1fr)'>"
        "<div id=a style='grid-column:1 / 3;height:10px'>A</div><div id=b style='height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_row_span_fixed_tracks(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:50px;grid-template-columns:20px 30px;grid-template-rows:20px 30px;align-items:stretch'>"
        "<div id=a style='grid-column:1;grid-row:1 / span 2'>A</div>"
        "<div id=b style='grid-column:2;grid-row:1'>B</div>"
        "<div id=c style='grid-column:2;grid-row:2'>C</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_row_span_auto_tracks(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:50px;grid-template-columns:20px 30px;row-gap:4px'>"
        "<div id=a style='grid-column:1;grid-row:span 2;height:40px'>A</div>"
        "<div id=b style='grid-column:2;grid-row:1;height:10px'>B</div>"
        "<div id=c style='grid-column:2;grid-row:2;height:10px'>C</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_explicit_collision_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:repeat(2,1fr)'>"
        "<div id=a style='grid-column:1;grid-row:1;height:10px'>A</div>"
        "<div id=b style='grid-column:1;grid-row:1;height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->h);
    TEST_ASSERT_NULL(find_text("B"));
    TEST_ASSERT_TRUE(lot52_has_diag("grid occupied or outside"));
}

static void test_l55_grid_implicit_column_is_bounded(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:80px;grid-template-columns:20px'>"
        "<div id=a style='grid-column:3;height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_invalid_position_preserves_previous(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:80px;grid-template-columns:repeat(4,1fr)'>"
        "<div id=a style='grid-column:2 / span 2;grid-column:8 / span 2;grid-row:0;height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_row_start);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
}

static void test_l55_grid_auto_clears_previous_placement(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:80px;grid-template-columns:repeat(4,1fr)'>"
        "<div id=a style='grid-column:3 / span 2;grid-column:auto;height:10px'>A</div>"
        "<div id=b style='height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_item_alignment_matrix(void) {
    static const struct { const char *mode; int x, y; } cases[] = {
        {"start", 0, 0}, {"center", 40, 25}, {"end", 80, 50}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char html[600];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div style='display:grid;width:100px;grid-template-rows:60px;justify-items:%s;align-items:%s'>"
            "<div id=a style='width:20px;height:10px'>A</div></div></body>", cases[i].mode, cases[i].mode);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(cases[i].x, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(cases[i].y, lot52_node("a")->y);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
        TEST_ASSERT_EQUAL(cases[i].x, find_text("A")->x);
        TEST_ASSERT_EQUAL(cases[i].y + 1, find_text("A")->y);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_grid_self_alignment_overrides(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-rows:60px;justify-items:end;align-items:end'>"
        "<div id=a style='width:20px;height:10px;justify-self:center;align-self:start'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, find_text("A")->x);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_stretch_respects_explicit_dimensions(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:repeat(2,1fr);grid-template-rows:60px;justify-items:stretch;align-items:stretch'>"
        "<div id=a>A</div><div id=b style='width:20px;height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->width);
}

static void test_l55_grid_center_uses_intrinsic_auto_width(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-rows:40px;justify-items:center;align-items:center'>"
        "<div id=a>one</div></div></body>", 320);
    TEST_ASSERT_EQUAL(38, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(38, find_text("one")->x);
    TEST_ASSERT_EQUAL(16, find_text("one")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_row_packing_end(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-rows:10px 20px;align-content:end'>"
        "<div id=a style='height:10px'></div><div id=b style='height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->y + lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_auto_rows_stretch(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-rows:auto auto;align-content:stretch;align-items:stretch'>"
        "<div id=a>one</div><div id=b>two</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, find_text("one")->y);
    TEST_ASSERT_EQUAL(51, find_text("two")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_hidden_display_does_not_occupy(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:repeat(2,1fr)'>"
        "<div id=a style='display:none;grid-column:span 2'>hidden</div>"
        "<div id=b style='height:10px'>B</div><div id=c style='height:10px'>C</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_grid_repeat_count_and_weight_bounds(void) {
    static const char *invalid[] = {"repeat(0,1fr)", "repeat(9,1fr)", "repeat(2,0fr)",
        "repeat(2,9fr)", "repeat(2,20%)", "20px 0fr", "-1px 1fr", "999999999999px"};
    unsigned i;
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        char html[600];
        snprintf(html, sizeof(html),
            "<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:20px 1fr;grid-template-columns:%s'>"
            "<div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>", invalid[i]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
        TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
        TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
    }
}

static void test_l55_grid_maximum_spans_capacity(void) {
    char html[4096];
    unsigned i, kept = 0, dropped = 0;
    strcpy(html, "<style>.x{grid-row:span 8;grid-column:span 8;height:1px}</style>"
        "<div style='display:grid;grid-template-columns:repeat(8,1fr)'>");
    for (i = 0; i < 30; i++) strcat(html, "<div class=x></div>");
    strcat(html, "</div>");
    lot52_render(html, 320);
    for (i = 0; i < g_doc.nnodes; i++) {
        if (strcmp(g_doc.nodes[i].cls, "x")) continue;
        if (g_doc.nodes[i].w) kept++; else dropped++;
        TEST_ASSERT_TRUE(g_doc.nodes[i].x >= 0);
        TEST_ASSERT_TRUE(g_doc.nodes[i].y >= 0);
    }
    TEST_ASSERT_EQUAL(WE_GRID_ROWS_MAX / 8, kept);
    TEST_ASSERT_EQUAL(30 - WE_GRID_ROWS_MAX / 8, dropped);
    TEST_ASSERT_TRUE(lot52_has_diag("grid placement capacity"));
    TEST_ASSERT_TRUE(g_doc.ndiag <= WE_MAX_DIAG);
}

static void test_l55_grid_relayout_replans_occupancy(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;grid-template-columns:20px 1fr'>"
        "<div id=a style='grid-column:2;grid-row:1;height:10px'>A</div>"
        "<div id=b style='height:10px'>B</div></div></body>", 100);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(180, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
