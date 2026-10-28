/* Definite containing heights and border boxes keep style fields intact. */
static void test_l55_height_percent_definite_parent(void) {
    lot52_render("<body style='margin:0'><div id=p style='height:200px'>"
        "<div id=a style='height:50%;background:red'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(200, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->height_percent);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_nested_percentages(void) {
    lot52_render("<body style='margin:0'><div id=p style='height:200px'>"
        "<div id=a style='height:50%'><div id=b style='height:25%'>B</div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(200, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_percent_ignores_parent_padding(void) {
    lot52_render("<body style='margin:0'><div id=p style='height:100px;padding:5px;border:2px solid red'>"
        "<div id=a style='height:50%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(114, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(7, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(8, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_percent_indefinite_is_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=p><div id=a style='height:50%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->height_pct);
    TEST_ASSERT_TRUE(lot52_has_diag("percent height indefinite"));
    TEST_ASSERT_NOT_NULL(find_text("A"));
}

static void test_l55_height_zero_percent_is_zero(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='height:0%'>A</div><div id=b style='height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->height_percent);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_percent_above_hundred_is_bounded(void) {
    lot52_render("<body style='margin:0'><div id=p style='height:100px'>"
        "<div id=a style='height:200%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->height_percent);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_percent_large_product_clamps(void) {
    int p;
    lot52_render("<body style='margin:0'><div id=p style='height:2000px'><div id=a style='height:2000%'></div></div></body>", 320);
    p = we_find_id(&g_doc, "p");
    TEST_ASSERT_TRUE(p >= 0);
    g_doc.nodes[p].height = WE_LAYOUT_MAX;
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(2000, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_invalid_preserves_percent(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='height:50%;height:-1px;height:2001%;height:50%junk'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->height_percent);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_TRUE(lot52_has_diag("height refused"));
}

static void test_l55_height_auto_clears_percent(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='height:50%;height:auto'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->height_percent);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_pixels_clear_percent(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='height:50%;height:20px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->height_percent);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_minimum_percentage(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='height:10px;min-height:40%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->min_height);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->min_height_pct);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->min_height_percent);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_maximum_percentage(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='height:80px;max-height:30%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->max_height);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->max_height_pct);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->max_height_percent);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_minimum_beats_percentage_maximum(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='height:10px;min-height:70%;max-height:30%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->min_height_pct);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->max_height_pct);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_percentage_limit_on_auto_content(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='min-height:40%'>A</div><div id=b style='max-height:0%'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(-1, lot52_node("b")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_indefinite_limits_ignore_with_diagnostic(void) {
    lot52_render("<body style='margin:0'><div id=a style='min-height:70%;max-height:30%'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->min_height_pct);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->max_height_pct);
    TEST_ASSERT_TRUE(lot52_has_diag("percent height indefinite"));
    TEST_ASSERT_NOT_NULL(find_text("A"));
}

static void test_l55_height_auto_parent_minimum_is_not_definite(void) {
    lot52_render("<body style='margin:0'><div id=p style='min-height:100px'>"
        "<div id=a style='height:50%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(-1, lot52_node("p")->height);
    TEST_ASSERT_TRUE(lot52_has_diag("percent height indefinite"));
}

static void test_l55_height_percentage_column_minimum_freeze(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;height:100px'>"
        "<div id=a style='flex-basis:80px;min-height:70%'>A</div>"
        "<div id=b style='flex-basis:80px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->min_height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_percentage_column_grow_cap(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;height:100px'>"
        "<div id=a style='flex-basis:0;flex-grow:1;max-height:20%'>A</div>"
        "<div id=b style='flex-basis:0;flex-grow:1'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->max_height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_height_percentage_basis_auto_column(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;height:100px'>"
        "<div id=a style='height:40%;flex-basis:auto'>A</div>"
        "<div id=b style='height:20%;flex-basis:auto'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_width_and_height(void) {
    lot52_render("<body style='margin:0'><div id=a style='box-sizing:border-box;width:100px;height:60px;padding:4px;border:2px solid red'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->border_box);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_content_box_retains_additive_extras(void) {
    lot52_render("<body style='margin:0'><div id=a style='box-sizing:content-box;width:100px;height:60px;padding:4px;border:2px solid red'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(112, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->border_box);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_percentage_width(void) {
    lot52_render("<body style='margin:0'><div style='width:200px'>"
        "<div id=a style='box-sizing:border-box;width:50%;padding:4px;border:2px solid red'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(22, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->width_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_parent_content_height(void) {
    lot52_render("<body style='margin:0'><div id=p style='box-sizing:border-box;height:100px;padding:4px;border:2px solid red'>"
        "<div id=a style='height:50%'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_child_percent_height(void) {
    lot52_render("<body style='margin:0'><div style='height:100px'>"
        "<div id=a style='box-sizing:border-box;height:50%;padding:4px;border:2px solid red'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->height_percent);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_minimum_limits(void) {
    lot52_render("<body style='margin:0'><div id=a style='box-sizing:border-box;width:20px;height:10px;min-width:60px;min-height:40px;padding:4px;border:2px solid red'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_maximum_limits(void) {
    lot52_render("<body style='margin:0'><div id=a style='box-sizing:border-box;width:200px;height:100px;max-width:60px;max-height:40px;padding:4px;border:2px solid red'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_extras_cannot_be_negative_content(void) {
    lot52_render("<body style='margin:0'><div id=a style='box-sizing:border-box;width:2px;height:2px;padding:4px;border:2px solid red'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(6, find_text("A")->x);
    TEST_ASSERT_EQUAL(7, find_text("A")->y);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->width);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->height);
}

static void test_l55_border_box_margins_remain_external(void) {
    lot52_render("<body style='margin:0'><div id=a style='box-sizing:border-box;width:100px;height:60px;padding:4px;border:2px solid red;margin:5px'>A</div>"
        "<div id=b style='height:10px'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(11, find_text("A")->x);
    TEST_ASSERT_EQUAL(12, find_text("A")->y);
}

static void test_l55_border_box_flex_basis_uses_content_weight(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px'>"
        "<div id=a style='box-sizing:border-box;flex-basis:80px;padding:5px;height:10px'></div>"
        "<div id=b style='flex-basis:80px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(52, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->flex_basis);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_flex_column_basis(void) {
    lot52_render("<body style='margin:0'><div style='display:flex;flex-direction:column;height:100px'>"
        "<div id=a style='box-sizing:border-box;flex-basis:80px;padding:5px'></div>"
        "<div id=b style='flex-basis:80px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(52, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(52, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->flex_basis);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->border_box);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_border_box_invalid_value_preserves_prior(void) {
    lot52_render("<body style='margin:0'><div id=a style='box-sizing:border-box;box-sizing:padding-box;width:100px;height:60px;padding:5px'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->border_box);
    TEST_ASSERT_EQUAL(5, find_text("A")->x);
    TEST_ASSERT_EQUAL(6, find_text("A")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("box sizing refused"));
}

static void test_l55_border_box_not_inherited(void) {
    lot52_render("<body style='margin:0'><div id=p style='box-sizing:border-box;width:100px;padding:5px'>"
        "<div id=a style='width:40px;height:10px;padding:3px'>A</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(46, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->border_box);
    TEST_ASSERT_EQUAL(8, find_text("A")->x);
    TEST_ASSERT_EQUAL(9, find_text("A")->y);
}

static void test_l55_height_relayout_restores_percentage_style(void) {
    lot52_render("<body style='margin:0'><div id=p style='height:100px'>"
        "<div id=a style='height:50%;min-height:20%;max-height:80%'>A</div></div></body>", 100);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 200));
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->height);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->min_height_pct);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->max_height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
