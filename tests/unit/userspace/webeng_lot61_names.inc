/* Lot 61: grid line occurrences, named spans and implicit area aliases. */
#define L61_LINES "<body style='margin:0'><div id=p style='display:grid;width:120px;grid-template-columns:[a] 20px [a] 20px [b] 20px [a] 20px'>"

static void test_l61_names_occurrence_start_and_end(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:a 2 / a 3;height:10px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_col_span);
    l61_box("x", 20, 0, 40, 10);
    l61_rect_is(0xFF0000, 20, 0, 40, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_occurrence_number_first(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:2 a / 3 a;height:10px'></div></div></body>", 320);
    l61_box("x", 20, 0, 40, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_negative_occurrence_counts_from_end(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:a -2 / a -1;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_col_start);
    l61_box("x", 20, 0, 40, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_negative_occurrence_single_track(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:b / a -1;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("x")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->grid_col_span);
    l61_box("x", 40, 0, 20, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_missing_occurrence_is_diagnosed(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:a 4 / b;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("x")->grid_col_start);
    l61_box("x", 0, 0, 20, 10);
    TEST_ASSERT_TRUE(lot52_has_diag("grid line unresolved"));
}

static void test_l61_names_occurrence_on_rows(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:40px;grid-template-columns:40px;"
        "grid-template-rows:[r] 10px [r] 12px [r] 14px [r];align-items:stretch'><div id=x style='grid-row:r 2 / r 4'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_row_start);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_row_span);
    l61_box("x", 0, 10, 40, 26);
    TEST_ASSERT_EQUAL(36, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_span_name_after_start(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:b / span a;height:10px'></div>"
        "<div id=y style='grid-row:2;grid-column:1 / span 2 a;height:10px'></div></div></body>", 320);
    l61_box("x", 40, 0, 20, 10);
    TEST_ASSERT_EQUAL(3, lot52_node("x")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->grid_col_span);
    TEST_ASSERT_EQUAL(1, lot52_node("y")->grid_col_start);
    TEST_ASSERT_EQUAL(3, lot52_node("y")->grid_col_span);
    l61_box("y", 0, 10, 60, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_span_name_before_end(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:span a / 5;height:10px'></div>"
        "<div id=y style='grid-row:2;grid-column:span 2 a / b;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("x")->grid_col_start);
    l61_box("x", 60, 0, 20, 10);
    TEST_ASSERT_EQUAL(1, lot52_node("y")->grid_col_start);
    l61_box("y", 0, 10, 40, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_span_name_with_auto_start_is_span_one(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:auto / span a;height:10px'></div>"
        "<div id=y style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->grid_col_span);
    l61_box("x", 0, 0, 20, 10);
    l61_box("y", 20, 0, 20, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_span_name_missing_is_diagnosed(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:b / span 3 a;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("x")->grid_col_start);
    l61_box("x", 0, 0, 20, 10);
    TEST_ASSERT_TRUE(lot52_has_diag("grid line unresolved"));
}

#define L61_AREAS "<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:30px 50px;" \
    "grid-template-rows:10px 20px;align-items:stretch;grid-template-areas:\"h h\" \"s m\"'>"

static void test_l61_names_area_start_end_aliases(void) {
    lot52_render(L61_AREAS "<div id=x style='grid-column:h-start / h-end;grid-row:s-start / s-end;background:#00ff00'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_col_span);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_row_start);
    l61_box("x", 0, 10, 80, 20);
    l61_rect_is(0x00FF00, 0, 10, 80, 20);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_bare_area_name_uses_start_alias(void) {
    lot52_render(L61_AREAS "<div id=x style='grid-column:m;grid-row:m'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_row_start);
    l61_box("x", 30, 10, 50, 20);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_bare_area_name_on_end_uses_end_alias(void) {
    lot52_render(L61_AREAS "<div id=x style='grid-column:1 / s;grid-row:1'></div>"
        "<div id=y style='grid-column:s / m;grid-row:2'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("x")->grid_col_span);
    l61_box("x", 0, 0, 30, 10);
    TEST_ASSERT_EQUAL(1, lot52_node("y")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("y")->grid_col_span);
    l61_box("y", 0, 10, 80, 20);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_alias_wins_over_explicit_bare_name(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[m] 30px 50px;"
        "grid-template-areas:\"s m\"'><div id=x style='grid-column:m;height:10px'></div>"
        "<div id=y style='grid-column:m 1;grid-row:2;height:10px'></div></div></body>", 320);
    l61_box("x", 30, 0, 50, 10);
    TEST_ASSERT_EQUAL(1, lot52_node("y")->grid_col_start);
    l61_box("y", 0, 10, 30, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_alias_occurrence_counts_implicit_lines(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:90px;grid-template-columns:30px 30px 30px;"
        "grid-template-areas:\"a . b\"'><div id=x style='grid-column:a-end / b-start;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_col_start);
    l61_box("x", 30, 0, 30, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_names_refusals_keep_previous_request(void) {
    static const char *bad[] = { "a 0 / b", "span a b / 5", "span 0 a / 5", "a 2 3 / 5", "a b / 5",
        "span / 5", "b / span 9 a", "abcdefghijklmno-start / 3", "span a / span b", "2 3 / 5", 0 };
    int k;
    char html[512];
    for (k = 0; bad[k]; k++) {
        snprintf(html, sizeof(html), L61_LINES "<div id=x style='grid-column:a 2 / b;grid-column:%s;height:10px'></div></div></body>", bad[k]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(2, lot52_node("x")->grid_col_start);
        TEST_ASSERT_EQUAL(1, lot52_node("x")->grid_col_span);
        l61_box("x", 20, 0, 20, 10);
        TEST_ASSERT_TRUE(lot52_has_diag("grid placement refused"));
    }
}

static void test_l61_names_successive_layouts_and_restyle(void) {
    lot52_render(L61_LINES "<div id=x style='grid-column:a 2 / a -1;height:10px'></div></div></body>", 320);
    l61_box("x", 20, 0, 40, 10);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    l61_box("x", 20, 0, 40, 10);
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
    l61_box("x", 20, 0, 40, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot61_names_run(void) {
    RUN_TEST(test_l61_names_occurrence_start_and_end);
    RUN_TEST(test_l61_names_occurrence_number_first);
    RUN_TEST(test_l61_names_negative_occurrence_counts_from_end);
    RUN_TEST(test_l61_names_negative_occurrence_single_track);
    RUN_TEST(test_l61_names_missing_occurrence_is_diagnosed);
    RUN_TEST(test_l61_names_occurrence_on_rows);
    RUN_TEST(test_l61_names_span_name_after_start);
    RUN_TEST(test_l61_names_span_name_before_end);
    RUN_TEST(test_l61_names_span_name_with_auto_start_is_span_one);
    RUN_TEST(test_l61_names_span_name_missing_is_diagnosed);
    RUN_TEST(test_l61_names_area_start_end_aliases);
    RUN_TEST(test_l61_names_bare_area_name_uses_start_alias);
    RUN_TEST(test_l61_names_bare_area_name_on_end_uses_end_alias);
    RUN_TEST(test_l61_names_alias_wins_over_explicit_bare_name);
    RUN_TEST(test_l61_names_alias_occurrence_counts_implicit_lines);
    RUN_TEST(test_l61_names_refusals_keep_previous_request);
    RUN_TEST(test_l61_names_successive_layouts_and_restyle);
}
