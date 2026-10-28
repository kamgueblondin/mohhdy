/* Intrinsic sizes are observed through actual boxes and text positions. */
static void test_l55_intrinsic_longest_word(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(0, find_text("one")->x);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_max_content_keeps_line(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:max-content'>one seven</div></body>", 64);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("one seven"));
    TEST_ASSERT_EQUAL(72, find_text("one seven")->w);
    TEST_ASSERT_EQUAL(1, find_text("one seven")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_fit_content_roomy(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:fit-content'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("one seven"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l55_intrinsic_fit_content_shrinks(void) {
    lot52_render("<body style='margin:0'><div style='width:56px'><div id=a style='width:fit-content'>one seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, find_text("one")->x);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_fit_content_minimum_overflow(void) {
    lot52_render("<body style='margin:0'><div style='width:16px'><div id=a style='width:fit-content'>seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, find_text("seven")->w);
    TEST_ASSERT_EQUAL(0, find_text("seven")->x);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l55_intrinsic_box_extras(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content;padding:3px;border:2px solid red;margin:4px'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(9, find_text("one")->x);
    TEST_ASSERT_EQUAL(10, find_text("one")->y);
    TEST_ASSERT_EQUAL(20, find_text("seven")->y);
    TEST_ASSERT_EQUAL(38, g_doc.perf.page_h);
}

static void test_l55_intrinsic_inline_word_crosses_span(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content'>ab<span>cd</span> ef</div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(16, find_text("cd")->x);
    TEST_ASSERT_EQUAL(11, find_text("ef")->y);
}

static void test_l55_intrinsic_inline_space_crosses_span(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content'>ab <span>cd</span> ef</div></body>", 320);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(11, find_text("cd")->y);
    TEST_ASSERT_EQUAL(21, find_text("ef")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
}

static void test_l55_intrinsic_br_splits_maximum(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:max-content'>one two<br>seven</div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
}

static void test_l55_intrinsic_empty_element_zero(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:max-content'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(0, g_doc.nitems);
    TEST_ASSERT_EQUAL(0, g_doc.perf.lines);
}

static void test_l55_intrinsic_hidden_child_has_no_size(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:max-content'>ab<span style='display:none'>longlong</span>cd</div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->max_content);
    TEST_ASSERT_NULL(find_text("longlong"));
    TEST_ASSERT_EQUAL(16, find_text("cd")->x);
}

static void test_l55_intrinsic_nested_block_maximum(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:max-content'><div>one two</div><div>seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
}

static void test_l55_intrinsic_declared_child_width(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:max-content'><div id=b style='width:80px;padding:2px;margin:3px'>ab</div></div></body>", 320);
    TEST_ASSERT_EQUAL(90, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(84, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(5, find_text("ab")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
}

static void test_l55_intrinsic_max_width_caps_keyword(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:max-content;max-width:40px'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
}

static void test_l55_intrinsic_min_width_expands_keyword(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content;min-width:80px'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->min_content);
    TEST_ASSERT_NOT_NULL(find_text("one seven"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l55_intrinsic_invalid_width_preserves_keyword(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content;width:bogus'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_SIZE_MIN_CONTENT, lot52_node("a")->width_kind);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
}

static void test_l55_intrinsic_numeric_width_replaces_keyword(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content;width:100px'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_SIZE_AUTO, lot52_node("a")->width_kind);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("one seven"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_nowrap_min_equals_max(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:min-content;white-space:nowrap'>one seven</div></body>", 320);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l55_intrinsic_flex_auto_row_basis(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:120px'>"
        "<div id=a style='flex-basis:auto'>one</div><div id=b style='flex-basis:auto'>seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_content_basis_overrides_width(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:120px'>"
        "<div id=a style='width:100px;flex-basis:content'>one</div>"
        "<div id=b style='width:100px;flex-basis:content'>seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->basis_content);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_auto_basis_uses_declared_width(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:120px'>"
        "<div id=a style='width:50px;flex-basis:auto'>one</div>"
        "<div id=b style='width:60px;flex-basis:auto'>seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_auto_basis_scaled_shrink(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:96px'>"
        "<div id=a style='flex-basis:auto'>abcdefgh</div>"
        "<div id=b style='flex-basis:auto'>abcdefghijklmnop</div></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(64, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(96, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_auto_column_text_height(void) {
    lot52_render("<body style='margin:0'><div id=col style='display:flex;flex-direction:column;width:40px'>"
        "<div id=a style='flex-basis:auto'>one seven</div>"
        "<div id=b style='flex-basis:auto'>two</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("col")->h);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(21, find_text("two")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_intrinsic_content_column_ignores_height(void) {
    lot52_render("<body style='margin:0'><div id=col style='display:flex;flex-direction:column;width:40px'>"
        "<div id=a style='height:80px;flex-basis:content'>one seven</div>"
        "<div id=b style='height:90px;flex-basis:content'>two</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("col")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->height);
}

static void test_l55_intrinsic_invalid_basis_preserves_content(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:120px'>"
        "<div id=a style='width:100px;flex-basis:content;flex-basis:-1px'>one</div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->basis_content);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->width);
    TEST_ASSERT_TRUE(lot52_has_diag("flex-basis refused"));
}

static void test_l55_intrinsic_relayout_fit_content(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:fit-content'>one two three four</div></body>", 160);
    TEST_ASSERT_EQUAL(144, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 64));
    TEST_ASSERT_EQUAL(64, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(144, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(WE_SIZE_FIT_CONTENT, lot52_node("a")->width_kind);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
