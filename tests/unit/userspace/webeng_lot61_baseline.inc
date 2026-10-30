/* Lot 61: last baselines, nested baseline export and vertical-align:baseline. */
#define L61_NEST(align) "<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:" align "'>" \
    "<div id=a style='width:40px;padding-top:12px'>A</div>" \
    "<div id=f style='display:flex;width:60px;padding-top:4px'><div id=g style='width:30px;padding-top:5px'>G</div></div>" \
    "<div id=t style='display:table;width:40px'><div style='display:table-row'><div id=tc style='display:table-cell;padding-top:2px'>T</div></div></div>" \
    "<div id=r style='display:grid;width:20px;padding-top:1px'><div id=ri style='padding-top:3px'>R</div></div></div></body>"

static void test_l61_flex_last_baseline_aligns_last_lines(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:last baseline'>"
        "<div id=a style='width:40px'>A</div><div id=b style='width:40px;padding-top:7px'>B<br>C</div>"
        "<div id=c style='width:40px;align-self:baseline;padding-top:3px'>D</div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_PACK_LAST_BASELINE, lot52_node("p")->align_items);
    l61_box("a", 0, 17, 40, 10);
    l61_box("b", 40, 0, 40, 27);
    l61_box("c", 80, 0, 40, 13);
    l61_text_at("A", 0, 18);
    l61_text_at("C", 40, 18);
    l61_text_at("D", 80, 4);
    TEST_ASSERT_EQUAL(27, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_flex_last_baseline_per_wrapped_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-wrap:wrap;width:80px;align-items:last baseline'>"
        "<div id=a style='width:40px'>A</div><div id=b style='width:40px'>B<br>C</div>"
        "<div id=c style='width:40px;padding-bottom:4px'>D</div><div id=e style='width:40px'>E</div></div></body>", 320);
    l61_box("a", 0, 10, 40, 10);
    l61_box("b", 40, 0, 40, 20);
    l61_box("c", 0, 20, 40, 14);
    l61_box("e", 40, 20, 40, 10);
    l61_text_at("A", 0, 11);
    l61_text_at("C", 40, 11);
    l61_text_at("D", 0, 21);
    l61_text_at("E", 40, 21);
    TEST_ASSERT_EQUAL(34, lot52_node("p")->h);
}

static void test_l61_flex_last_baseline_column_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:80px;align-items:last baseline'>"
        "<div id=a style='width:40px'>A</div><div id=b style='width:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("column baseline unsupported"));
}

static void test_l61_grid_last_baseline_with_row_span(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;grid-template-columns:40px 40px 40px;align-items:last baseline'>"
        "<div id=a style='grid-row:1 / span 2'>A<br>B<br>C<br>D</div><div id=b>B</div>"
        "<div id=c style='grid-row:2;padding-bottom:5px'>C</div><div id=e style='grid-row:2;grid-column:3'>E</div></div></body>", 320);
    l61_box("a", 0, 0, 40, 40);
    l61_box("b", 40, 13, 40, 10);
    l61_box("c", 40, 30, 40, 15);
    l61_box("e", 80, 30, 40, 10);
    TEST_ASSERT_EQUAL(31, g_doc.items[3].y);
    l61_text_at("E", 80, 31);
    TEST_ASSERT_EQUAL(45, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

/* The lot 60 refusal fixture, now accepted: the spanning item joins row 1. */
static void test_l61_grid_spanning_first_baseline_is_accepted(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:40px 40px;grid-template-rows:20px 20px;align-items:baseline'>"
        "<div id=a style='grid-row:1 / span 2'>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_FALSE(lot52_has_diag("spanning baseline unsupported"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_grid_spanning_first_baseline_moves_to_the_group(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:40px 40px;align-items:baseline'>"
        "<div id=a style='grid-row:1 / span 2'>A<br>B<br>C</div><div id=b style='grid-row:1;grid-column:2;padding-top:10px'>B</div>"
        "<div id=c style='grid-row:2;grid-column:2'>C</div></div></body>", 320);
    l61_box("a", 0, 10, 40, 30);
    l61_box("b", 40, 0, 40, 20);
    l61_box("c", 40, 20, 40, 10);
    l61_text_at("A", 0, 11);
    TEST_ASSERT_EQUAL(11, g_doc.items[3].y);
    TEST_ASSERT_EQUAL(40, g_doc.items[3].x);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
}

static void test_l61_export_nested_flex_grid_table(void) {
    lot52_render(L61_NEST("baseline"), 320);
    l61_box("a", 0, 0, 40, 22);
    l61_box("f", 40, 3, 60, 19);
    l61_box("g", 40, 7, 30, 15);
    l61_box("t", 100, 10, 40, 12);
    l61_box("r", 140, 8, 20, 14);
    l61_text_at("A", 0, 13);
    l61_text_at("G", 40, 13);
    l61_text_at("T", 100, 13);
    l61_text_at("R", 140, 13);
    TEST_ASSERT_EQUAL(22, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

/* The probe layout leaves no item, counter or diagnostic behind. */
static void test_l61_export_probe_restores_the_document(void) {
    uint32_t items, boxes, lines, rows;
    lot52_render(L61_NEST("flex-start"), 320);
    items = g_doc.nitems; boxes = g_doc.perf.boxes; lines = g_doc.perf.lines; rows = g_doc.perf.table_rows;
    lot52_render(L61_NEST("baseline"), 320);
    TEST_ASSERT_EQUAL(items, g_doc.nitems);
    TEST_ASSERT_EQUAL(boxes, g_doc.perf.boxes);
    TEST_ASSERT_EQUAL(lines, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(rows, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, g_doc.diag_dropped);
}

static void test_l61_export_successive_layouts_are_stable(void) {
    int pass;
    lot52_render(L61_NEST("baseline"), 320);
    for (pass = 0; pass < 3; pass++) {
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
        l61_box("f", 40, 3, 60, 19);
        l61_box("t", 100, 10, 40, 12);
        l61_text_at("G", 40, 13);
        l61_text_at("R", 140, 13);
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l61_export_nested_last_uses_main_end_item(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:last baseline'>"
        "<div id=a style='width:40px'>A</div><div id=f style='display:flex;width:80px;align-items:flex-start'>"
        "<div id=g style='width:40px'>G<br>H</div><div id=k style='width:40px;padding-bottom:6px'>K</div></div></div></body>", 320);
    l61_box("a", 0, 0, 40, 10);
    l61_box("f", 40, 0, 80, 20);
    l61_text_at("A", 0, 1);
    l61_text_at("K", 80, 1);
    l61_text_at("H", 40, 11);
}

static void test_l61_export_column_flex_uses_first_item(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px'>A</div><div id=f style='display:flex;flex-direction:column;width:40px'>"
        "<div id=g style='padding-top:8px'>G</div><div id=h>H</div></div></div></body>", 320);
    l61_box("a", 0, 8, 40, 10);
    l61_box("f", 40, 0, 40, 28);
    l61_text_at("A", 0, 9);
    l61_text_at("G", 40, 9);
}

static void test_l61_export_empty_container_synthesizes_bottom(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px;padding-top:4px'>A</div>"
        "<div id=f style='display:flex;width:40px;height:30px;padding-bottom:3px'></div></div></body>", 320);
    l61_box("a", 0, 20, 40, 14);
    l61_box("f", 40, 0, 40, 33);
    l61_text_at("A", 0, 25);
}

static void test_l61_export_overflow_hidden_uses_bottom_edge(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px'>A</div><div id=f style='display:flex;width:40px;overflow:hidden;padding-bottom:3px'>"
        "<div>G</div></div></div></body>", 320);
    l61_box("a", 0, 4, 40, 10);
    l61_box("f", 40, 0, 40, 13);
    l61_text_at("A", 0, 5);
    l61_text_at("G", 40, 1);
}

/* Probes nest at most WE_PROBE_DEPTH levels: the fourth nested
 * baseline-aligned container falls back to its synthesized edge. */
static void test_l61_export_probe_depth_is_capped(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:200px;align-items:baseline'><div id=a style='width:20px'>A</div>"
        "<div id=f1 style='display:flex;width:100px;align-items:baseline'><div id=f2 style='display:flex;width:90px;padding-top:2px;align-items:baseline'>"
        "<div id=f3 style='display:flex;width:80px;padding-top:2px;align-items:baseline'><div id=f4 style='display:flex;width:70px;padding-top:2px;align-items:baseline'>"
        "<div id=z style='padding-top:2px'>Z</div></div></div></div></div></div></body>", 320);
    l61_box("a", 0, 9, 20, 10);
    l61_box("z", 20, 6, 8, 12);
    l61_text_at("A", 0, 10);
    l61_text_at("Z", 20, 9);
    TEST_ASSERT_EQUAL(19, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_export_probe_budget_is_diagnosed(void) {
    static char html[6000];
    int i, n;
    n = snprintf(html, sizeof(html), "<body style='margin:0'><div id=p style='display:flex;flex-wrap:wrap;width:640px;align-items:baseline'>");
    for (i = 1; i <= 70; i++)
        n += snprintf(html + n, sizeof(html) - (size_t)n, "<div style='display:flex;width:8px;height:%dpx'></div>", i);
    snprintf(html + n, sizeof(html) - (size_t)n, "<div id=t>T</div></div></body>");
    lot52_render(html, 640);
    TEST_ASSERT_TRUE(lot52_has_diag("baseline probe budget"));
    l61_box("t", 560, 61, 8, 10);
    TEST_ASSERT_EQUAL(71, lot52_node("p")->h);
}

static void test_l61_export_block_wrapping_a_flex_container(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px'>A</div><div id=w style='width:60px;padding-top:6px'>"
        "<div id=f style='display:flex'><div style='padding-top:4px'>F</div></div></div></div></body>", 320);
    l61_box("a", 0, 10, 40, 10);
    l61_box("w", 40, 0, 60, 20);
    l61_text_at("A", 0, 11);
    l61_text_at("F", 40, 11);
}

static void test_l61_inline_block_baseline_shares_the_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'>ab"
        "<div id=b style='display:inline-block;width:30px;vertical-align:baseline;padding-top:6px'>X<br>Y</div>cd"
        "<div id=c style='display:inline-block;width:20px;height:30px;vertical-align:baseline;background:#ff0000'></div>"
        "<div id=t style='display:inline-block;width:10px;height:5px;background:#00ff00'></div></div><div id=n>next</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_VALIGN_BASELINE, lot52_node("b")->valign);
    l61_box("b", 16, 5, 30, 26);
    l61_box("c", 62, 0, 20, 30);
    l61_box("t", 82, 0, 10, 5);
    l61_text_at("ab", 0, 22);
    l61_text_at("X", 16, 12);
    l61_text_at("Y", 16, 22);
    l61_text_at("cd", 46, 22);
    l61_rect_is(0xFF0000, 62, 0, 20, 30);
    l61_rect_is(0x00FF00, 82, 0, 10, 5);
    TEST_ASSERT_EQUAL(31, lot52_node("p")->h);
    l61_box("n", 0, 31, 320, 10);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_inline_block_baseline_measured_equals_layout(void) {
    lot52_render("<body style='margin:0'><div id=q style='display:flex;width:300px'><div id=p style='width:200px'>ab"
        "<div id=b style='display:inline-block;width:30px;vertical-align:baseline;padding-top:6px'>X<br>Y</div>cd</div>"
        "<div id=s style='width:20px;align-self:stretch;background:#0000ff'></div></div></body>", 320);
    l61_box("p", 0, 0, 200, 26);
    l61_box("b", 16, 0, 30, 26);
    l61_text_at("ab", 0, 17);
    l61_text_at("Y", 16, 17);
    TEST_ASSERT_EQUAL(26, lot52_node("q")->h);
    l61_rect_is(0x0000FF, 200, 0, 20, 26);
}

static void test_l61_inline_block_overflow_hidden_uses_margin_edge(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'>ab"
        "<div id=b style='display:inline-block;width:30px;vertical-align:baseline;overflow:hidden;margin-bottom:4px'>X<br>Y</div>cd</div></body>", 320);
    l61_box("b", 16, 0, 30, 20);
    l61_text_at("ab", 0, 16);
    l61_text_at("X", 16, 1);
    l61_text_at("cd", 46, 16);
    TEST_ASSERT_EQUAL(25, lot52_node("p")->h);
}

static void test_l61_inline_block_baseline_capacity_is_diagnosed(void) {
    static char html[2000];
    int i, n = snprintf(html, sizeof(html), "<body style='margin:0'><div id=p style='width:300px'>a");
    for (i = 1; i <= 9; i++)
        n += snprintf(html + n, sizeof(html) - (size_t)n,
            "<div id=i%d style='display:inline-block;width:10px;height:%dpx;vertical-align:baseline'></div>", i, i);
    snprintf(html + n, sizeof(html) - (size_t)n, "</div></body>");
    lot52_render(html, 320);
    l61_box("i1", 8, 8, 10, 1);
    l61_box("i8", 78, 1, 10, 8);
    l61_box("i9", 88, 0, 10, 9);
    l61_text_at("a", 0, 1);
    TEST_ASSERT_TRUE(lot52_has_diag("inline baseline capacity"));
}

static void test_l61_table_cell_baseline_row(void) {
    lot52_render("<body style='margin:0'><table id=tb style='width:120px;border-spacing:0'><tr>"
        "<td id=a style='vertical-align:baseline;padding-top:10px'>A</td><td id=b style='vertical-align:baseline'>B<br>C</td>"
        "<td id=c>D</td><td id=e style='vertical-align:baseline'><div style='height:30px'></div></td></tr></table></body>", 320);
    l61_box("a", 0, 0, 30, 41);
    l61_box("b", 30, 0, 30, 41);
    l61_box("c", 60, 0, 30, 10);
    l61_box("e", 90, 0, 30, 41);
    l61_text_at("A", 0, 22);
    l61_text_at("B", 30, 22);
    l61_text_at("C", 30, 32);
    l61_text_at("D", 60, 1);
    TEST_ASSERT_EQUAL(41, lot52_node("tb")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_table_baseline_exports_to_flex(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:160px;align-items:baseline'>"
        "<div id=a style='width:40px'>A</div><table id=tb style='width:80px;border-spacing:0'><tr>"
        "<td id=x style='vertical-align:baseline;padding-top:10px'>X</td><td id=y style='vertical-align:baseline'>Y</td></tr></table></div></body>", 320);
    l61_box("a", 0, 10, 40, 10);
    l61_text_at("A", 0, 11);
    l61_text_at("X", 40, 11);
    l61_text_at("Y", 80, 11);
}

static void test_l61_table_baseline_keeps_middle_cells(void) {
    lot52_render("<body style='margin:0'><table id=tb style='width:80px;border-spacing:0'><tr>"
        "<td id=a style='vertical-align:baseline;padding-top:20px'>A</td><td id=b style='vertical-align:middle'>B</td></tr></table></body>", 320);
    l61_box("a", 0, 0, 40, 30);
    l61_box("b", 40, 0, 40, 30);
    l61_text_at("A", 0, 21);
    l61_text_at("B", 40, 11);
}

static void lot61_baseline_run(void) {
    RUN_TEST(test_l61_flex_last_baseline_aligns_last_lines);
    RUN_TEST(test_l61_flex_last_baseline_per_wrapped_line);
    RUN_TEST(test_l61_flex_last_baseline_column_is_diagnosed);
    RUN_TEST(test_l61_grid_last_baseline_with_row_span);
    RUN_TEST(test_l61_grid_spanning_first_baseline_is_accepted);
    RUN_TEST(test_l61_grid_spanning_first_baseline_moves_to_the_group);
    RUN_TEST(test_l61_export_nested_flex_grid_table);
    RUN_TEST(test_l61_export_probe_restores_the_document);
    RUN_TEST(test_l61_export_successive_layouts_are_stable);
    RUN_TEST(test_l61_export_nested_last_uses_main_end_item);
    RUN_TEST(test_l61_export_column_flex_uses_first_item);
    RUN_TEST(test_l61_export_empty_container_synthesizes_bottom);
    RUN_TEST(test_l61_export_overflow_hidden_uses_bottom_edge);
    RUN_TEST(test_l61_export_probe_depth_is_capped);
    RUN_TEST(test_l61_export_probe_budget_is_diagnosed);
    RUN_TEST(test_l61_export_block_wrapping_a_flex_container);
    RUN_TEST(test_l61_inline_block_baseline_shares_the_line);
    RUN_TEST(test_l61_inline_block_baseline_measured_equals_layout);
    RUN_TEST(test_l61_inline_block_overflow_hidden_uses_margin_edge);
    RUN_TEST(test_l61_inline_block_baseline_capacity_is_diagnosed);
    RUN_TEST(test_l61_table_cell_baseline_row);
    RUN_TEST(test_l61_table_baseline_exports_to_flex);
    RUN_TEST(test_l61_table_baseline_keeps_middle_cells);
}
