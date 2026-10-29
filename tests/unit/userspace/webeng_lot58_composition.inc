static void test_l58_composition_dynamic_overflow_uses_auto_and_diagnostic(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:200px'><div id=a style='width:calc(2000% * 1000);height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("math evaluation refused"));
}

static void test_l58_composition_dynamic_height_overflow_uses_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='height:100px'><div id=a style='height:calc(2000% * 1000)'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("math evaluation refused"));
}

static void test_l58_composition_dynamic_maximum_overflow_is_ignored(void) {
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='width:80px;max-width:calc(2000% * 1000);height:10px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math evaluation refused"));
    TEST_ASSERT_EQUAL(0, find_text("A")->x);
}

static void test_l58_composition_static_box_does_not_evaluate_unused_insets(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:80px;height:10px;left:calc(2000% * 1000)'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_responsive_math_relayout_preserves_style(void) {
    int pass;
    uint32_t pool;
    lot52_render("<body style='margin:0'><div id=a style='width:clamp(20px,calc(50% - 10px),100px);height:10px;margin-inline:auto'>A</div></body>", 100);
    pool = g_doc.pool_used;
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->x);
    for (pass = 0; pass < 8; pass++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
        TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(55, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(pool, g_doc.pool_used);
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 300));
        TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(100, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(pool, g_doc.pool_used);
    }
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_exact_expression_node_capacity(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:clamp(1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px,2px,3px);height:10px'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_MATH_NODES_MAX, lot52_node("a")->length[WE_LEN_WIDTH].count);
    TEST_ASSERT_EQUAL(11, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_expression_capacity_refuses_atomically(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:calc(40px + 10px);width:calc(1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px + 1px);height:10px'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->length[WE_LEN_WIDTH].count);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
}

static void test_l58_composition_exact_expression_depth(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:calc(calc(calc(calc(40px))));height:10px'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->length[WE_LEN_WIDTH].count);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_caller_document_canaries(void) {
    static struct { uint32_t before; we_doc_t doc; uint32_t after; } guarded;
    const char *html = "<body style='margin:0'><div id=a style='width:calc(50% - 20px);aspect-ratio:2;margin-inline:auto'></div></body>";
    int a;
    guarded.before = 0x12345678U; guarded.after = 0x89abcdefU;
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(0, we_render(&guarded.doc, html, (int)strlen(html), 200));
    a = we_find_id(&guarded.doc, "a");
    TEST_ASSERT_TRUE(a > 0);
    TEST_ASSERT_EQUAL(80, guarded.doc.nodes[a].w);
    TEST_ASSERT_EQUAL(40, guarded.doc.nodes[a].h);
    TEST_ASSERT_EQUAL(60, guarded.doc.nodes[a].x);
    TEST_ASSERT_EQUAL(0x12345678U, guarded.before);
    TEST_ASSERT_EQUAL(0x89abcdefU, guarded.after);
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
}

static void test_l58_composition_expression_state_belongs_to_each_document(void) {
    static we_doc_t other;
    const char *a = "<body style='margin:0'><div id=a style='width:calc(50% - 10px);height:10px'><a href='first'>A</a></div></body>";
    const char *b = "<body style='margin:0'><div id=b style='width:calc(25% - 5px);height:20px'>B</div></body>";
    char href[32];
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, a, (int)strlen(a), 200));
    TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, we_render(&other, b, (int)strlen(b), 200));
    TEST_ASSERT_EQUAL(45, other.nodes[we_find_id(&other, "b")].w);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 300));
    TEST_ASSERT_EQUAL(140, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 2, 2, href, sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("first", href);
    TEST_ASSERT_EQUAL(45, other.nodes[we_find_id(&other, "b")].w);
    TEST_ASSERT_EQUAL(0, other.ndiag);
}

static void test_l58_composition_math_inside_float_keeps_text_exclusion(void) {
    lot52_render("<body style='margin:0'><div id=g style='width:120px'><div id=a style='float:left;width:calc(25% + 10px);aspect-ratio:2;background:red'></div>word</div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, find_text("word")->x);
    TEST_ASSERT_EQUAL(1, find_text("word")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_grid_ratio_uses_actual_child_width_for_rows(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:1fr'><div id=a style='width:calc(50%);aspect-ratio:2'></div><div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_table_math_width_is_one_shared_track_request(void) {
    lot52_render("<body style='margin:0'><table id=g style='width:calc(100px + 20px)'><tr><td id=a style='width:calc(50% - 10px)'>A</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(120, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_flex_column_indefinite_basis_math_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:flex;flex-direction:column;justify-content:start'><div id=a style='height:10px;flex-basis:calc(50% - 10px)'></div><div id=b style='height:20px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("g")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("column percent requires height"));
}

static void test_l58_composition_math_cascade_important_beats_inline_normal(void) {
    lot52_render("<style>body{margin:0}#a{inline-size:calc(50% - 10px)!important;aspect-ratio:2!important;margin-inline:auto}</style><body><div id=a style='width:20px;aspect-ratio:1'></div></body>", 200);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(55, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->aspect_num);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_ratio_content_minimum_keeps_all_lines(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;aspect-ratio:4'>abc def ghi</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_EQUAL(21, find_text("ghi")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_border_box_cannot_be_smaller_than_edges(void) {
    lot52_render("<body style='margin:0'><div id=a style='inline-size:calc(0px);block-size:calc(0px);padding:4px;border:2px solid red;box-sizing:border-box'></div></body>", 320);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_composition_combined_guest_fixture(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot58.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot58.html", "rb");
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file); fclose(file); html[count] = 0;
    lot58_image(html);
    TEST_ASSERT_EQUAL(35, lot52_node("ratio")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("ratio")->w);
    TEST_ASSERT_EQUAL(25, lot52_node("ratio")->h);
    TEST_ASSERT_EQUAL(39, lot52_node("wrapped")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("limited")->x);
    TEST_ASSERT_EQUAL(49, lot52_node("limited")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("limited")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("limited")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("cell")->x);
    TEST_ASSERT_EQUAL(94, lot52_node("cell")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("absolute")->x);
    TEST_ASSERT_EQUAL(134, lot52_node("absolute")->y);
    TEST_ASSERT_EQUAL(179, lot52_node("image")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("image")->h);
    TEST_ASSERT_EQUAL(203, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
