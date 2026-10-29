static void test_l60_composition_self_baseline_overrides_start(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:start'>"
        "<div id=a style='width:40px;padding-top:6px;align-self:baseline'>A</div>"
        "<div id=b style='width:40px;align-self:first baseline'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_auto_margin_excludes_baseline_group(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;height:50px;align-items:baseline'>"
        "<div id=a style='width:40px;padding-top:6px;margin-top:auto'>A</div>"
        "<div id=b style='width:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(34, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(41, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_baseline_uses_first_wrapped_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:24px;padding-top:6px;overflow-wrap:anywhere'>abcd</div>"
        "<div id=b style='width:40px'>B</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(26, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(7, find_text("abc")->y);
    TEST_ASSERT_EQUAL(17, find_text("d")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(26, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_nested_first_line_exports_baseline(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px'><div style='height:5px'></div><div style='padding-top:3px'>A</div></div>"
        "<div id=b style='width:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(9, find_text("A")->y);
    TEST_ASSERT_EQUAL(9, find_text("B")->y);
    TEST_ASSERT_EQUAL(18, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_empty_item_synthesizes_bottom_edge(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px;height:20px'></div><div id=b style='width:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(12, find_text("B")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_overflow_hidden_synthesizes_bottom_edge(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px;height:20px;overflow:hidden'>A</div><div id=b style='width:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(12, find_text("B")->y);
    TEST_ASSERT_EQUAL(21, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_wrapped_lines_have_independent_baselines(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-wrap:wrap;width:80px;align-items:baseline;gap:4px'>"
        "<div id=a style='width:36px;padding-top:6px'>A</div><div id=b style='width:36px'>B</div>"
        "<div id=c style='width:36px'>C</div><div id=d style='width:36px;padding-top:3px'>D</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(23, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(24, find_text("C")->y);
    TEST_ASSERT_EQUAL(24, find_text("D")->y);
    TEST_ASSERT_EQUAL(33, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_reversed_row_retains_baseline(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-direction:row-reverse;width:120px;align-items:baseline'>"
        "<div id=a style='width:40px;padding-top:6px'>A</div><div id=b style='width:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(7, find_text("B")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_column_baseline_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:80px;align-items:baseline'>"
        "<div id=a style='width:40px'>A</div><div id=b style='width:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("column baseline unsupported"));
}

static void test_l60_composition_grid_row_spanning_baseline_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:40px 40px;grid-template-rows:20px 20px;align-items:baseline'>"
        "<div id=a style='grid-row:1 / span 2'>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("spanning baseline unsupported"));
}

static void test_l60_composition_unknown_line_is_diagnosed_then_auto(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:[start] 40px [end] 40px'>"
        "<div id=a style='grid-column:unknown / end;height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_span);
    TEST_ASSERT_TRUE(lot52_has_diag("grid line unresolved"));
}

static void test_l60_composition_negative_line_before_start_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:80px;grid-template-columns:40px 40px'>"
        "<div id=a style='grid-column:-4 / -1;height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_TRUE(lot52_has_diag("grid line unresolved"));
}

static void test_l60_composition_restyle_and_resize_preserve_names(void) {
    int pass; uint32_t pool;
    lot52_render("<body style='margin:0'><div id=p style='display:grid;grid-template-columns:[a] 1fr [b] 1fr [c]'>"
        "<div id=a style='grid-column:b / c;height:10px'>A</div></div></body>", 320);
    pool = g_doc.pool_used;
    TEST_ASSERT_EQUAL(160, lot52_node("a")->x);
    for (pass = 0; pass < 8; pass++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 100));
        TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
        TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(pool, g_doc.pool_used);
    }
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_resolved_name_replaced_by_numeric(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;grid-template-columns:[a] 40px [b] 40px [c] 40px [d]'>"
        "<div id=a style='grid-column:c / d;grid-column:1 / span 2;height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_request[0].active);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grid_col_start);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grid_col_span);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_names_cleared_by_numeric_track_list(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;grid-template-columns:[a] 40px [b] 40px [c];grid-template-columns:40px 40px'>"
        "<div id=a style='grid-column:b / c;height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->line_names[0].name[1][0][0]);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grid_col_start);
    TEST_ASSERT_TRUE(lot52_has_diag("grid line unresolved"));
}

static void test_l60_composition_caller_canaries(void) {
    static struct { uint32_t before; we_doc_t doc; uint32_t after; } guarded;
    const char *html = "<body style='margin:0'><div style='display:grid;grid-template-columns:[a b] repeat(8,20px) [end finish]'><div id=a style='grid-column:a / finish;height:10px'>A</div></div></body>";
    int a;
    guarded.before = 0x12345678U; guarded.after = 0xabcdef01U;
    TEST_ASSERT_EQUAL(0, we_render(&guarded.doc, html, (int)strlen(html), 320));
    a = we_find_id(&guarded.doc, "a");
    TEST_ASSERT_TRUE(a > 0);
    TEST_ASSERT_EQUAL(160, guarded.doc.nodes[a].w);
    TEST_ASSERT_EQUAL(8, guarded.doc.nodes[a].grid_col_span);
    TEST_ASSERT_EQUAL(0x12345678U, guarded.before);
    TEST_ASSERT_EQUAL(0xabcdef01U, guarded.after);
    TEST_ASSERT_EQUAL(0, guarded.doc.ndiag);
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
}

static void test_l60_composition_combined_guest_fixture(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot60.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot60.html", "rb");
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file); fclose(file); html[count] = 0;
    lot58_image(html);
    TEST_ASSERT_EQUAL(0, lot52_node("fa")->y);
    TEST_ASSERT_EQUAL(6, lot52_node("fb")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("grid")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("ga")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("gb")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("gb")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("span")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("span")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("last")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("last")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("image")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("image")->w);
    TEST_ASSERT_EQUAL(21, lot52_node("image")->h);
    TEST_ASSERT_EQUAL(91, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l60_composition_image_exports_decoded_bottom_baseline(void) {
    int painted;
    lot58_image("<body style='margin:0'><div id=p style='display:flex;width:120px;align-items:baseline'>"
        "<img id=a src=/images/mire-g1.png alt=fallback style='object-fit:fill;flex-grow:0'>"
        "<div id=b style='width:40px;flex-grow:0'>B</div></div></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(23, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(24, find_text("B")->y);
    TEST_ASSERT_EQUAL(33, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 10, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
