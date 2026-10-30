/* Lot 61: bounded subgrid on both axes (tracks, gaps, names, intrinsic). */
#define L61_SUB_COLS "<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:[a] 40px [b] auto [c] 30px [d];column-gap:5px'>" \
    "<div id=h style='grid-column:1 / 4;height:5px;background:#ff0000'></div>" \
    "<div id=s style='display:grid;grid-column:1 / 4;grid-template-columns:subgrid'>" \
    "<div id=x style='grid-column:b / c'>WIDEWORD</div><div id=y>Y</div><div id=z style='grid-column:c'>Z</div></div></div></body>"
#define L61_SUB_PARENT "<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:50px 50px'>"

static void test_l61_subgrid_columns_inherit_tracks_gap_names(void) {
    lot52_render(L61_SUB_COLS, 320);
    TEST_ASSERT_EQUAL(1, lot52_node("s")->subgrid[0]);
    TEST_ASSERT_EQUAL(3, lot52_node("s")->grid_cols);
    l61_box("h", 0, 0, 144, 5);
    l61_box("s", 0, 5, 144, 10);
    l61_box("x", 45, 5, 64, 10);
    l61_box("y", 0, 5, 40, 10);
    l61_box("z", 114, 5, 30, 10);
    l61_rect_is(0xFF0000, 0, 0, 144, 5);
    l61_text_at("WIDEWORD", 45, 6);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_subgrid_successive_layouts_are_stable(void) {
    int pass;
    lot52_render(L61_SUB_COLS, 320);
    for (pass = 0; pass < 3; pass++) {
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
        l61_box("x", 45, 5, 64, 10);
        l61_box("z", 114, 5, 30, 10);
        TEST_ASSERT_EQUAL(3, lot52_node("s")->grid_cols);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l61_subgrid_rows_feed_parent_rows(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:60px 60px;grid-template-rows:auto auto;row-gap:4px'>"
        "<div id=s style='display:grid;grid-row:1 / 3;grid-template-rows:subgrid'><div id=a>A</div><div id=b style='height:30px'>B</div></div>"
        "<div id=q>Q</div><div id=r style='grid-column:2;grid-row:2'>R</div></div></body>", 320);
    l61_box("s", 0, 0, 60, 44);
    l61_box("a", 0, 0, 60, 10);
    l61_box("b", 0, 14, 60, 30);
    l61_box("q", 60, 0, 60, 10);
    l61_box("r", 60, 14, 60, 10);
    TEST_ASSERT_EQUAL(44, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_subgrid_both_axes_with_names(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:[l] 50px [m] 50px [r];grid-template-rows:[t] 20px [u] auto [v];gap:6px'>"
        "<div id=s style='display:grid;grid-column:1 / 3;grid-row:1 / 3;grid-template-columns:subgrid;grid-template-rows:subgrid'>"
        "<div id=a style='grid-column:m;grid-row:u;height:25px;background:#00ff00'>A</div><div id=b style='background:#0000ff'>B</div></div></div></body>", 320);
    l61_box("s", 0, 0, 106, 51);
    l61_box("a", 56, 26, 50, 25);
    l61_box("b", 0, 0, 50, 10);
    l61_rect_is(0x00FF00, 56, 26, 50, 25);
    l61_rect_is(0x0000FF, 0, 0, 50, 10);
    TEST_ASSERT_EQUAL(51, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_subgrid_area_aliases_are_inherited(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:30px 40px 50px;grid-template-areas:\"x main main\"'>"
        "<div id=s style='display:grid;grid-column:1 / 4;grid-template-columns:subgrid'>"
        "<div id=a style='grid-column:main-start / main-end;height:10px'></div><div id=b style='grid-column:main;height:10px'></div></div></div></body>", 320);
    l61_box("a", 30, 0, 90, 10);
    l61_box("b", 30, 10, 90, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_subgrid_intrinsic_width_sums_items(void) {
    lot52_render("<body style='margin:0'><div id=w style='display:inline-block'><div id=p style='display:grid;grid-template-columns:auto auto;column-gap:3px'>"
        "<div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid'><div id=a>AAAA</div><div id=b>BB</div></div></div></div></body>", 320);
    l61_box("w", 0, 0, 51, 10);
    l61_box("p", 0, 0, 51, 10);
    l61_box("a", 0, 0, 32, 10);
    l61_box("b", 35, 0, 16, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_subgrid_items_are_clamped_into_the_span(void) {
    lot52_render(L61_SUB_PARENT "<div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid'>"
        "<div id=a style='grid-column:2 / span 3'>A</div><div id=b style='grid-column:5'>B</div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->grid_col_start);
    l61_box("a", 0, 0, 100, 10);
    l61_box("b", 50, 10, 50, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_subgrid_unresolved_name_reported_once(void) {
    uint32_t i, seen = 0;
    lot52_render(L61_SUB_PARENT "<div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid'>"
        "<div id=a style='grid-column:nope'>A</div></div></div></body>", 320);
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], "grid line unresolved")) seen++;
    TEST_ASSERT_EQUAL(1, seen);
    l61_box("a", 0, 0, 50, 10);
}

static void test_l61_subgrid_line_name_list_is_refused(void) {
    lot52_render(L61_SUB_PARENT "<div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid [a] [b]'>"
        "<div id=a>A</div><div id=b>B</div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("s")->subgrid[0]);
    l61_box("a", 0, 0, 100, 10);
    l61_box("b", 0, 10, 100, 10);
    TEST_ASSERT_TRUE(lot52_has_diag("subgrid line names refused"));
}

static void test_l61_subgrid_without_grid_parent_is_refused(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=s style='display:grid;grid-template-columns:subgrid'>"
        "<div id=a>A</div><div id=b>B</div></div></div></body>", 320);
    l61_box("a", 0, 0, 200, 10);
    l61_box("b", 0, 10, 200, 10);
    TEST_ASSERT_TRUE(lot52_has_diag("subgrid requires grid parent"));
}

static void test_l61_subgrid_on_a_block_is_refused(void) {
    lot52_render(L61_SUB_PARENT "<div id=s style='display:block;grid-column:1 / 3;grid-template-columns:subgrid'><div id=a>A</div></div></div></body>", 320);
    l61_box("s", 0, 0, 100, 10);
    TEST_ASSERT_TRUE(lot52_has_diag("subgrid requires grid parent"));
}

static void test_l61_subgrid_nested_is_refused(void) {
    lot52_render(L61_SUB_PARENT "<div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid'>"
        "<div id=t style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid'><div id=a>A</div><div id=b>B</div></div></div></div></body>", 320);
    l61_box("t", 0, 0, 100, 20);
    l61_box("a", 0, 0, 100, 10);
    l61_box("b", 0, 10, 100, 10);
    TEST_ASSERT_TRUE(lot52_has_diag("nested subgrid refused"));
}

static void test_l61_subgrid_padding_on_axis_is_refused(void) {
    lot52_render(L61_SUB_PARENT "<div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid;padding-left:4px'>"
        "<div id=a>A</div><div id=b>B</div></div></div></body>", 320);
    l61_box("a", 4, 0, 96, 10);
    l61_box("b", 4, 10, 96, 10);
    TEST_ASSERT_TRUE(lot52_has_diag("subgrid box offsets refused"));
}

static void test_l61_subgrid_padding_on_other_axis_is_accepted(void) {
    lot52_render(L61_SUB_PARENT "<div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid;padding-top:4px'>"
        "<div id=a>A</div><div id=b>B</div></div></div></body>", 320);
    l61_box("a", 0, 4, 50, 10);
    l61_box("b", 50, 4, 50, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_subgrid_restyle_to_tracks_drops_inheritance(void) {
    lot52_render("<style>#s{grid-template-columns:subgrid}#s{grid-template-columns:20px 20px}</style>" L61_SUB_PARENT
        "<div id=s style='display:grid;grid-column:1 / 3'><div id=a>A</div><div id=b>B</div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("s")->subgrid[0]);
    l61_box("a", 0, 0, 20, 10);
    l61_box("b", 20, 0, 20, 10);
}

/* A lone custom ident names both edges; equal edges collapse to span 1. */
static void test_l61_names_lone_area_ident_spans_the_area(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:[k] 30px 40px 50px;grid-template-areas:\"x main main\"'>"
        "<div id=b style='grid-column:main;height:10px'></div><div id=c style='grid-column:k / k;height:10px'></div></div></body>", 320);
    l61_box("b", 30, 0, 90, 10);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->grid_col_span);
    l61_box("c", 0, 0, 30, 10);
    TEST_ASSERT_EQUAL(1, lot52_node("c")->grid_col_span);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot61_subgrid_run(void) {
    RUN_TEST(test_l61_names_lone_area_ident_spans_the_area);
    RUN_TEST(test_l61_subgrid_columns_inherit_tracks_gap_names);
    RUN_TEST(test_l61_subgrid_successive_layouts_are_stable);
    RUN_TEST(test_l61_subgrid_rows_feed_parent_rows);
    RUN_TEST(test_l61_subgrid_both_axes_with_names);
    RUN_TEST(test_l61_subgrid_area_aliases_are_inherited);
    RUN_TEST(test_l61_subgrid_intrinsic_width_sums_items);
    RUN_TEST(test_l61_subgrid_items_are_clamped_into_the_span);
    RUN_TEST(test_l61_subgrid_unresolved_name_reported_once);
    RUN_TEST(test_l61_subgrid_line_name_list_is_refused);
    RUN_TEST(test_l61_subgrid_without_grid_parent_is_refused);
    RUN_TEST(test_l61_subgrid_on_a_block_is_refused);
    RUN_TEST(test_l61_subgrid_nested_is_refused);
    RUN_TEST(test_l61_subgrid_padding_on_axis_is_refused);
    RUN_TEST(test_l61_subgrid_padding_on_other_axis_is_accepted);
    RUN_TEST(test_l61_subgrid_restyle_to_tracks_drops_inheritance);
}
