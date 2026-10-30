/* Lot 61: repeat(auto-fill|auto-fit, fixed or minmax(fixed, fr)), 1..8 tracks. */
#define L61_ITEM(id) "<div id=" id " style='height:10px'></div>"

static void test_l61_repeat_auto_fill_fixed_count(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:130px;gap:10px;grid-template-columns:repeat(auto-fill, 30px)'>"
        L61_ITEM("a") L61_ITEM("b") L61_ITEM("c") "<div id=d style='height:10px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->auto_count[0]);
    l61_box("a", 0, 0, 30, 10);
    l61_box("b", 40, 0, 30, 10);
    l61_box("c", 80, 0, 30, 10);
    l61_box("d", 0, 20, 30, 10);
    l61_rect_is(0xFF0000, 0, 20, 30, 10);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_auto_fill_follows_each_layout_width(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;gap:10px;grid-template-columns:repeat(auto-fill, 30px)'>"
        L61_ITEM("a") L61_ITEM("b") L61_ITEM("c") "</div></body>", 130);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    l61_box("c", 80, 0, 30, 10);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 70));
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    l61_box("c", 0, 20, 30, 10);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 130));
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    l61_box("c", 80, 0, 30, 10);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_auto_fill_minmax_fr_stretches(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:130px;column-gap:10px;"
        "grid-template-columns:repeat(auto-fill, minmax(30px, 1fr))'>" L61_ITEM("a") L61_ITEM("b") L61_ITEM("c") "</div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    l61_box("a", 0, 0, 37, 10);
    l61_box("b", 47, 0, 37, 10);
    l61_box("c", 94, 0, 36, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_auto_fit_collapses_empty_tracks(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:130px;column-gap:10px;"
        "grid-template-columns:20px repeat(auto-fit, minmax(30px, 1fr))'>" L61_ITEM("e") L61_ITEM("f") "</div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    l61_box("e", 0, 0, 20, 10);
    l61_box("f", 30, 0, 100, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_auto_fill_keeps_empty_tracks(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:130px;column-gap:10px;"
        "grid-template-columns:20px repeat(auto-fill, minmax(30px, 1fr))'>" L61_ITEM("e") L61_ITEM("f") "</div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->grid_cols);
    l61_box("e", 0, 0, 20, 10);
    l61_box("f", 30, 0, 45, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_auto_fit_collapsed_middle_gap(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:200px;column-gap:10px;"
        "grid-template-columns:20px repeat(auto-fit, 30px) 20px'><div id=a style='height:10px'></div>"
        "<div id=z style='grid-column:-2 / -1;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(5, lot52_node("p")->grid_cols);
    l61_box("a", 0, 0, 20, 10);
    l61_box("z", 30, 0, 20, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_count_is_capped_at_eight_tracks(void) {
    char html[1024];
    int k, at;
    at = snprintf(html, sizeof(html), "<body style='margin:0'><div id=p style='display:grid;width:300px;grid-template-columns:repeat(auto-fill, 10px)'>");
    for (k = 0; k < 9; k++) at += snprintf(html + at, sizeof(html) - (size_t)at, "<div id=i%d style='height:10px'></div>", k);
    snprintf(html + at, sizeof(html) - (size_t)at, "</div></body>");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(8, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(8, lot52_node("p")->auto_count[0]);
    l61_box("i7", 70, 0, 10, 10);
    l61_box("i8", 0, 10, 10, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_other_tracks_reduce_the_count(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:300px;grid-template-columns:100px repeat(auto-fill, 10px) 150px'>"
        L61_ITEM("a") "</div></body>", 320);
    TEST_ASSERT_EQUAL(7, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(5, lot52_node("p")->auto_count[0]);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_rows_need_definite_height(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:40px;grid-template-rows:repeat(auto-fill, 10px)'>"
        L61_ITEM("a") L61_ITEM("b") "</div><div id=q style='display:grid;width:40px;height:35px;grid-template-rows:repeat(auto-fill, 10px)'>"
        "<div id=c style='grid-row:-2 / -1;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("p")->grid_rows);
    l61_box("b", 0, 10, 40, 10);
    TEST_ASSERT_EQUAL(3, lot52_node("q")->grid_rows);
    l61_box("c", 0, 40, 40, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_auto_fit_rows_collapse(void) {
    lot52_render("<body style='margin:0'><div id=q style='display:grid;width:40px;height:40px;row-gap:5px;"
        "grid-template-rows:repeat(auto-fit, 10px) 5px'><div id=c style='height:10px'></div>"
        "<div id=z style='grid-row:-2 / -1;height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("q")->grid_rows);
    l61_box("c", 0, 0, 40, 10);
    l61_box("z", 0, 15, 40, 5);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_names_shift_after_expansion(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:130px;column-gap:10px;"
        "grid-template-columns:[s] repeat(auto-fill, 30px) [e] 10px [z]'><div id=a style='grid-column:s / e;height:10px'></div>"
        "<div id=b style='grid-column:e / z;grid-row:2;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("p")->auto_count[0]);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->grid_col_span);
    l61_box("a", 0, 0, 110, 10);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->grid_col_start);
    l61_box("b", 120, 10, 10, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_refusals_keep_previous_template(void) {
    static const char *bad[] = { "repeat(auto-fill, 10px) repeat(auto-fit, 10px)", "auto repeat(auto-fill, 10px)",
        "1fr repeat(auto-fill, 10px)", "repeat(auto-fill, auto)", "repeat(auto-fill, 1fr)", "repeat(auto-fill, 10px 20px)",
        "repeat(auto-fill, [a] 10px)", "repeat(auto-fill, minmax(auto, 1fr))", "repeat(auto-fill, fit-content(20px))",
        "repeat(auto-fill)", "repeat(auto-fills, 10px)", 0 };
    int k;
    char html[512];
    for (k = 0; bad[k]; k++) {
        snprintf(html, sizeof(html), "<body style='margin:0'><div id=p style='display:grid;width:120px;grid-template-columns:20px 40px;"
            "grid-template-columns:%s'><div id=a style='height:10px'></div><div id=b style='height:10px'></div></div></body>", bad[k]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
        TEST_ASSERT_EQUAL(0, lot52_node("p")->auto_mode[0]);
        l61_box("b", 20, 0, 40, 10);
        TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
    }
}

static void test_l61_repeat_restyle_replaces_automatic_template(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:130px;grid-template-columns:repeat(auto-fill, 30px);"
        "grid-template-columns:50px 50px'>" L61_ITEM("a") L61_ITEM("b") L61_ITEM("c") "</div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->auto_mode[0]);
    l61_box("c", 0, 10, 50, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_repeat_inside_measured_flex_item(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:200px;align-items:start'>"
        "<div id=p style='display:grid;width:70px;column-gap:10px;grid-template-columns:repeat(auto-fill, 30px)'>"
        L61_ITEM("a") L61_ITEM("b") L61_ITEM("c") "</div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("p")->grid_cols);
    l61_box("c", 0, 10, 30, 10);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot61_repeat_run(void) {
    RUN_TEST(test_l61_repeat_auto_fill_fixed_count);
    RUN_TEST(test_l61_repeat_auto_fill_follows_each_layout_width);
    RUN_TEST(test_l61_repeat_auto_fill_minmax_fr_stretches);
    RUN_TEST(test_l61_repeat_auto_fit_collapses_empty_tracks);
    RUN_TEST(test_l61_repeat_auto_fill_keeps_empty_tracks);
    RUN_TEST(test_l61_repeat_auto_fit_collapsed_middle_gap);
    RUN_TEST(test_l61_repeat_count_is_capped_at_eight_tracks);
    RUN_TEST(test_l61_repeat_other_tracks_reduce_the_count);
    RUN_TEST(test_l61_repeat_rows_need_definite_height);
    RUN_TEST(test_l61_repeat_auto_fit_rows_collapse);
    RUN_TEST(test_l61_repeat_names_shift_after_expansion);
    RUN_TEST(test_l61_repeat_refusals_keep_previous_template);
    RUN_TEST(test_l61_repeat_restyle_replaces_automatic_template);
    RUN_TEST(test_l61_repeat_inside_measured_flex_item);
}
