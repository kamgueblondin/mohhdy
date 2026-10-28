/* Spans are resolved once into the table's shared bounded columns. */
static void test_l55_table_two_column_span_geometry(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:120px'><tr>"
        "<td id=a colspan=2>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->colspan);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_shared_with_following_row(void) {
    lot52_render("<body style='margin:0'><table style='width:120px'>"
        "<tr><td id=a colspan=2>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td><td id=e>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("e")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("e")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_shared_with_previous_fixed_cells(void) {
    lot52_render("<body style='margin:0'><table style='width:120px'>"
        "<tr><td id=a style='width:20px'>A</td><td id=b style='width:40px'>B</td><td id=c>C</td></tr>"
        "<tr><td id=d colspan=2>D</td><td id=e>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("e")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_fixed_width_request(void) {
    lot52_render("<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=2 style='width:80px'>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td><td id=e>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_minimum_width_request(void) {
    lot52_render("<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=2 style='min-width:80px'>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td><td id=e>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->min_width);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_percentage_width_request(void) {
    lot52_render("<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=2 style='width:80%'>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td><td id=e>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->width_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_overlapping_width_requests(void) {
    lot52_render("<body style='margin:0'><table style='width:120px'>"
        "<tr><td id=a colspan=2 style='width:60px'>A</td><td>B</td></tr>"
        "<tr><td>C</td><td id=d colspan=2 style='width:90px'>D</td></tr>"
        "<tr><td id=e>E</td><td id=f>F</td><td id=g>G</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(3, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_all_columns_eight(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:80px'>"
        "<tr><td id=a colspan=8>A</td></tr>"
        "<tr><td id=b>B</td><td id=c>C</td><td>D</td><td>E</td><td>F</td><td>G</td><td>H</td><td id=i>I</td></tr>"
        "</table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("i")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("i")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->colspan);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_total_columns_overflow_refused(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:100px'><tr>"
        "<td id=a colspan=8>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
    TEST_ASSERT_NULL(find_text("A"));
    TEST_ASSERT_TRUE(lot52_has_diag("table columns exceed limit"));
}

static void test_l55_table_span_invalid_attribute_values(void) {
    static const char *invalid[] = {"0", "9", "-1", "1.5", "x", "", "999999999999999999999"};
    unsigned i;
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        char html[600];
        snprintf(html, sizeof(html), "<body style='margin:0'><table id=t><tr>"
            "<td id=a colspan='%s'>A</td><td id=b>B</td></tr></table></body>", invalid[i]);
        lot52_render(html, 320);
        TEST_ASSERT_EQUAL(1, lot52_node("a")->span_refused);
        TEST_ASSERT_EQUAL(0, lot52_node("t")->h);
        TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(0, g_doc.perf.table_rows);
        TEST_ASSERT_NULL(find_text("A"));
        TEST_ASSERT_TRUE(lot52_has_diag("table span refused"));
    }
}

static void test_l55_table_rowspan_with_colspan_places(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:90px'><tr>"
        "<td id=a colspan=2 rowspan=2>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->colspan);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->rowspan);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_rowspan_one_accepted(void) {
    lot52_render("<body style='margin:0'><table style='width:90px'><tr>"
        "<td id=a colspan=2 rowspan=1>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->span_refused);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_hidden_span_does_not_advance(void) {
    lot52_render("<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=8 style='display:none'>hidden</td>"
        "<td id=b>B</td><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->w);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_spans_across_sections(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:120px'>"
        "<thead><tr><th id=a colspan=3>Title</th></tr></thead>"
        "<tbody><tr><td id=b>B</td><td id=c>C</td><td id=d>D</td></tr></tbody>"
        "<tfoot><tr><td id=e colspan=2>E</td><td id=f>F</td></tr></tfoot></table></body>", 320);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("e")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(3, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_rounding_exhausts_width(void) {
    lot52_render("<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=2>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td><td id=e>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(67, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(34, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(33, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(67, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_scaled_oversized_requests(void) {
    lot52_render("<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=2 style='width:180px'>A</td><td id=b style='width:20px'>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td><td id=e>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("d")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_height_follows_wrapped_text(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:60px'><tr>"
        "<td id=a colspan=2>one two three four</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td>D</td><td>E</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(31, find_text("four")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_nested_flex(void) {
    lot52_render("<body style='margin:0'><table style='width:120px'><tr><td id=a colspan=2>"
        "<div style='display:flex;justify-content:space-between'>"
        "<div id=x style='width:20px;height:10px'>X</div><div id=y style='width:20px;height:10px'>Y</div>"
        "</div></td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("x")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("y")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("y")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_nested_grid(void) {
    lot52_render("<body style='margin:0'><table style='width:120px'><tr><td id=a colspan=2>"
        "<div style='display:grid;grid-template-columns:20px 1fr'>"
        "<div id=x style='height:10px'>X</div><div id=y style='height:10px'>Y</div>"
        "</div></td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("x")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("y")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("y")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(1, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_border_box_width(void) {
    lot52_render("<body style='margin:0'><table style='width:100px'><tr>"
        "<td id=a colspan=2 style='box-sizing:border-box;width:80px;padding:3px;border:2px solid red'>A</td>"
        "<td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(5, find_text("A")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_link_uses_merged_cell(void) {
    char href[32];
    lot52_render("<body style='margin:0'><table style='width:120px'><tr>"
        "<td id=a colspan=2><a href='/next'>one</a></td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 70, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 90, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_table_span_empty_cell_still_has_tracks(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:120px'><tr>"
        "<td id=a colspan=2></td><td id=b style='height:10px'>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
}

static void test_l55_table_span_implied_cell_closes(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:120px'>"
        "<tr><td id=a colspan=2>A<td id=b>B<tr><td id=c>C<td>D<td>E</table></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_NOT_NULL(find_text("B"));
}

static void test_l55_table_span_relayout_recomputes_shared_widths(void) {
    lot52_render("<body style='margin:0'><table><tr><td id=a colspan=2>A</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td><td id=d>D</td><td id=e>E</td></tr></table></body>", 120);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 240));
    TEST_ASSERT_EQUAL(160, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(160, lot52_node("e")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->colspan);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
