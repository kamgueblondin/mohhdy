static void test_l59_composition_responsive_fit_and_insets(void) {
    uint32_t pool;
    int pass;
    lot52_render("<body style='margin:0'><div id=p style='height:80px'>"
        "<div id=a style='width:fit-content(20%);height:20px;position:relative;inset-inline-start:10%'>AB CDEF</div>"
        "</div></body>", 200);
    pool = g_doc.pool_used;
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    for (pass = 0; pass < 8; pass++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 100));
        TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
        TEST_ASSERT_EQUAL(1, find_text("AB")->y);
        TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 400));
        TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(40, lot52_node("a")->x);
        TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
        TEST_ASSERT_EQUAL(pool, g_doc.pool_used);
    }
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_caller_document_canaries(void) {
    static struct { uint32_t before; we_doc_t doc; uint32_t after; } guarded;
    const char *html = "<body style='margin:0'><div id=a style='width:fit-content(calc(20% + 4px));min-width:min-content;word-break:break-all'>abcdefgh</div></body>";
    int a;
    guarded.before = 0x12345678U; guarded.after = 0xabcdef01U;
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(0, we_render(&guarded.doc, html, (int)strlen(html), 100));
    a = we_find_id(&guarded.doc, "a");
    TEST_ASSERT_TRUE(a > 0);
    TEST_ASSERT_EQUAL(24, guarded.doc.nodes[a].w);
    TEST_ASSERT_EQUAL(30, guarded.doc.nodes[a].h);
    TEST_ASSERT_EQUAL(8, guarded.doc.nodes[a].min_content);
    TEST_ASSERT_EQUAL(64, guarded.doc.nodes[a].max_content);
    TEST_ASSERT_EQUAL(0x12345678U, guarded.before);
    TEST_ASSERT_EQUAL(0xabcdef01U, guarded.after);
    TEST_ASSERT_EQUAL(0, guarded.doc.ndiag);
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
}

static void test_l59_composition_independent_documents(void) {
    static we_doc_t other;
    const char *html = "<body style='margin:0'><div id=a style='width:fit-content(50%);word-break:break-all'>abcdefgh</div></body>";
    int a;
    lot52_render("<body style='margin:0'><div id=a style='width:fit-content(40px)'>AB CDEF</div></body>", 320);
    TEST_ASSERT_EQUAL(0, we_render(&other, html, (int)strlen(html), 64));
    a = we_find_id(&other, "a");
    TEST_ASSERT_EQUAL(32, other.nodes[a].w);
    TEST_ASSERT_EQUAL(20, other.nodes[a].h);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, we_layout(&other, 100));
    TEST_ASSERT_EQUAL(50, other.nodes[a].w);
    TEST_ASSERT_EQUAL(20, other.nodes[a].h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(8, other.nodes[a].min_content);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, other.ndiag);
}

static void test_l59_composition_break_all_inherited_and_reset(void) {
    lot52_render("<body style='margin:0'><div id=p style='word-break:break-all;width:24px'>"
        "<div id=a>abcd</div><div id=b style='word-break:normal'>efgh</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->min_content);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_NOT_NULL(find_text("d"));
    TEST_ASSERT_NOT_NULL(find_text("efgh"));
    TEST_ASSERT_EQUAL(21, find_text("efgh")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_anywhere_inherited_and_reset(void) {
    lot52_render("<body style='margin:0'><div id=p style='overflow-wrap:anywhere;width:24px'>"
        "<div id=a>abcd</div><div id=b style='overflow-wrap:normal'>efgh</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->min_content);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_NOT_NULL(find_text("d"));
    TEST_ASSERT_NOT_NULL(find_text("efgh"));
    TEST_ASSERT_EQUAL(21, find_text("efgh")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_break_all_fills_float_band(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:64px;word-break:break-all'>"
        "<div id=f style='float:left;width:24px;height:20px'></div>abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("f")->h);
    TEST_ASSERT_NOT_NULL(find_text("abcde"));
    TEST_ASSERT_NOT_NULL(find_text("fgh"));
    TEST_ASSERT_EQUAL(24, find_text("abcde")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcde")->y);
    TEST_ASSERT_EQUAL(24, find_text("fgh")->x);
    TEST_ASSERT_EQUAL(11, find_text("fgh")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_anywhere_uses_float_band(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:64px;overflow-wrap:anywhere'>"
        "<div id=f style='float:left;width:24px;height:20px'></div>abcdefgh</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("abcde"));
    TEST_ASSERT_NOT_NULL(find_text("fgh"));
    TEST_ASSERT_EQUAL(24, find_text("abcde")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcde")->y);
    TEST_ASSERT_EQUAL(24, find_text("fgh")->x);
    TEST_ASSERT_EQUAL(11, find_text("fgh")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_word_break_invalid_keeps_previous(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;word-break:break-all;word-break:keep-everything'>abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->break_all);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("word break refused"));
}

static void test_l59_composition_relative_moves_descendants_and_links(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px'>"
        "<div style='height:20px'></div><div id=a style='position:relative;margin-left:30px;width:40px;height:20px;right:10px;bottom:10px'>"
        "<a href=/next id=link>Hi</a><div id=abs style='position:absolute;inset:10px auto auto 20px;width:10px;height:5px;background:red'></div>"
        "</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, find_text("Hi")->x);
    TEST_ASSERT_EQUAL(11, find_text("Hi")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("abs")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("abs")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 22, 13, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp("/next", href));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 32, 23, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_relative_bottom_requires_definite_height(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px'>"
        "<div style='height:20px'></div><div id=a style='position:relative;width:20px;height:10px;bottom:-20px'></div>"
        "<div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_absolute_auto_releases_stretch(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:100px;height:80px'>"
        "<div id=a style='position:absolute;inset:10px;left:auto;top:auto;width:fit-content(40px);height:20px'>AB CDEF</div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, find_text("AB")->x);
    TEST_ASSERT_EQUAL(51, find_text("AB")->y);
    TEST_ASSERT_EQUAL(61, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(WE_INSET_RIGHT | WE_INSET_BOTTOM, lot52_node("a")->inset_flags);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_absolute_logical_inset_stretch(void) {
    lot52_render("<body style='margin:0'><div id=p style='position:relative;width:100px;height:80px'>"
        "<div id=a style='position:absolute;inset-inline:10% 20%;inset-block:10px 20px;background:red'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(15, lot52_node("a")->inset_flags);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_grid_limit_changes_actual_width(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:120px;grid-template-columns:repeat(2,1fr);grid-template-rows:30px;align-items:stretch'>"
        "<div id=a style='max-width:min-content'>AB CDEF</div><div id=b>X</div></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(60, find_text("X")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_combined_guest_fixture(void) {
    static char html[4096];
    FILE *file = fopen("initrd_content/web/lot59.html", "rb");
    int count;
    if (!file) file = fopen("../initrd_content/web/lot59.html", "rb");
    TEST_ASSERT_NOT_NULL(file);
    if (!file) return;
    count = (int)fread(html, 1, sizeof(html) - 1, file); fclose(file); html[count] = 0;
    lot58_image(html);
    TEST_ASSERT_EQUAL(40, lot52_node("fit")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("fit")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(34, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("cell")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("cell")->w);
    TEST_ASSERT_EQUAL(42, lot52_node("relative")->x);
    TEST_ASSERT_EQUAL(79, lot52_node("relative")->y);
    TEST_ASSERT_EQUAL(104, lot52_node("table")->y);
    TEST_ASSERT_EQUAL(114, lot52_node("image")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("image")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("image")->h);
    TEST_ASSERT_EQUAL(146, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_NULL(find_text("decode_failed"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_column_intrinsic_basis_ignores_declared_height(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:40px;height:80px'>"
        "<div id=a style='height:60px;flex-basis:min-content;flex-grow:0'>AB CDEF</div>"
        "<div id=b style='height:10px;flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_column_max_basis_uses_wrapped_block_height(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:40px;height:80px'>"
        "<div id=a style='height:60px;flex-basis:max-content;flex-grow:0'>AB CDEF</div>"
        "<div id=b style='height:10px;flex-grow:1'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_intrinsic_fit_dynamic_overflow_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:fit-content(calc(2000% * 1000));height:20px'>AB CDEF</div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_TRUE(lot52_has_diag("math evaluation refused"));
}

static void test_l59_composition_hidden_position_does_not_evaluate_insets(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:40px;height:20px;inset:calc(2000% * 1000)'>AB</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
}

static void test_l59_composition_anywhere_minimum_allows_flex_shrink(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:48px'>"
        "<div id=a style='flex-basis:64px;min-width:min-content;overflow-wrap:anywhere'>abcdefgh</div>"
        "<div id=b style='flex-basis:32px;min-width:0'>X</div></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->w);
    TEST_ASSERT_NOT_NULL(find_text("abcd"));
    TEST_ASSERT_NOT_NULL(find_text("efgh"));
    TEST_ASSERT_EQUAL(11, find_text("efgh")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_relative_auto_drops_stale_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px;height:80px'>"
        "<div style='height:20px'></div><div id=a style='position:relative;width:20px;height:10px;margin-left:30px;inset:calc(20%);inset:auto;right:10px;bottom:10px'>A</div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(WE_INSET_RIGHT | WE_INSET_BOTTOM, lot52_node("a")->inset_flags);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_LEFT].count);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->length[WE_LEN_TOP].count);
    TEST_ASSERT_EQUAL(20, find_text("A")->x);
    TEST_ASSERT_EQUAL(11, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_break_all_after_full_atomic_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:32px;word-break:break-all'>"
        "<span id=a style='display:inline-block;width:32px;height:10px;background:red'></span>abcd</div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abcd"));
    TEST_ASSERT_EQUAL(0, find_text("abcd")->x);
    TEST_ASSERT_EQUAL(11, find_text("abcd")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_composition_anywhere_after_full_atomic_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:32px;overflow-wrap:anywhere'>"
        "<span id=a style='display:inline-block;width:32px;height:10px;background:red'></span>abcd</div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abcd"));
    TEST_ASSERT_EQUAL(0, find_text("abcd")->x);
    TEST_ASSERT_EQUAL(11, find_text("abcd")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
