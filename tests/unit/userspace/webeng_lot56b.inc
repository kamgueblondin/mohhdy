/* Lot 56 continued: pre-line, two-axis table spacing, clear, wrap-reverse,
 * line-height, text-indent, table vertical-align, inline-block, z-index.
 * Each case checks a box, a glyph, or a paint order. No filler. */

static int lot56_rect_at(uint32_t color) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == color) return (int)i;
    return -1;
}

static void test_l56_pre_line_min_content_is_the_longest_line_word(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line;width:min-content'>ab\ncdef</div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->min_content);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, find_text("ab")->x);
    TEST_ASSERT_EQUAL(11, find_text("cdef")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_pre_line_wraps_and_keeps_the_newline(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line;width:24px'>abc def\ng</div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_EQUAL(21, find_text("g")->y);
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
}

static void test_l56_pre_line_collapses_a_run_of_spaces(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line'>a   b</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("a b"));
    TEST_ASSERT_EQUAL(24, find_text("a b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_NULL(find_text("a   b"));
}

static void test_l56_pre_line_leading_newline_is_an_empty_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line'>\nab</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(11, find_text("ab")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
}

static void test_l56_pre_line_inherits_through_a_span(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line'>a<span id=s>\nb</span></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE_LINE, lot52_node("s")->ws_mode);
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_EQUAL(0, find_text("b")->x);
}

static void test_l56_pre_line_resets_pre(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre;white-space:pre-line;width:24px'>abc def</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE_LINE, lot52_node("p")->ws_mode);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->nowrap);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_break_spaces_is_refused(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line;white-space:break-spaces'>a\nb</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_WS_PRE_LINE, lot52_node("p")->ws_mode);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("white space refused"));
}

static void test_l56_spacing_two_values_split_the_axes(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:10px 20px'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td><td id=d>D</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->border_spacing);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->border_spacing_y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("d")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("d")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_spacing_one_value_sets_both_axes(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:6px'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(6, lot52_node("t")->border_spacing);
    TEST_ASSERT_EQUAL(6, lot52_node("t")->border_spacing_y);
    TEST_ASSERT_EQUAL(38, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(16, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("t")->h);
}

static void test_l56_spacing_horizontal_zero_keeps_rows_apart(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:40px;border-spacing:0 10px'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
}

static void test_l56_spacing_vertical_zero_keeps_columns_apart(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:50px;border-spacing:10px 0'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
}

static void test_l56_spacing_collapse_zeros_both_axes(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:10px 20px;border-collapse:collapse'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(35, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
}

static void test_l56_spacing_second_value_refused_keeps_both(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:4px 8px;border-spacing:10px 99px'><tr>"
        "<td id=a>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("t")->border_spacing);
    TEST_ASSERT_EQUAL(8, lot52_node("t")->border_spacing_y);
    TEST_ASSERT_EQUAL(18, lot52_node("c")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("border spacing refused"));
}

static void test_l56_spacing_rowspan_uses_the_vertical_axis(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:70px;border-spacing:10px 20px'><tr>"
        "<td id=a rowspan=2>A</td><td id=b>B</td></tr><tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_clear_left_moves_below_the_float(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px'>"
        "<div id=f style='float:left;width:40px;height:30px;background:#ff0000'></div>"
        "<div id=b style='clear:left;width:20px;height:10px;background:#0000ff'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("f")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("f")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_no_clear_sits_beside_the_float(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px'>"
        "<div id=f style='float:left;width:40px;height:30px;background:#ff0000'></div>"
        "<div id=b style='width:20px;height:10px;background:#0000ff'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
}

static void test_l56_clear_right_ignores_a_left_float(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:100px'>"
        "<div id=f style='float:left;width:40px;height:30px'></div>"
        "<div id=b style='clear:right;width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_CLEAR_RIGHT, lot52_node("b")->clear_side);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
}

static void test_l56_clear_both_uses_the_taller_float(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=l style='float:left;width:30px;height:20px;background:#ff0000'></div>"
        "<div id=r style='float:right;width:30px;height:40px;background:#0000ff'></div>"
        "<div id=b style='clear:both;height:10px;background:#008000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("l")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("r")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("p")->h);
}

static void test_l56_clear_left_stops_at_the_left_float(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=l style='float:left;width:30px;height:20px'></div>"
        "<div id=r style='float:right;width:30px;height:40px'></div>"
        "<div id=b style='clear:left;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
}

static void test_l56_clear_right_stops_at_the_right_float(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:120px'>"
        "<div id=l style='float:left;width:30px;height:20px'></div>"
        "<div id=r style='float:right;width:30px;height:40px'></div>"
        "<div id=b style='clear:right;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("p")->h);
}

static void test_l56_second_float_clears_the_first(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:80px'>"
        "<div id=a style='float:left;width:20px;height:20px;background:#ff0000'></div>"
        "<div id=b style='float:left;clear:left;width:20px;height:10px;background:#0000ff'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
}

static void test_l56_invalid_clear_keeps_left(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'>"
        "<div id=f style='float:left;width:40px;height:30px'></div>"
        "<div id=b style='clear:left;clear:middle;width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_CLEAR_LEFT, lot52_node("b")->clear_side);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("clear refused"));
}

static void test_l56_clear_none_resets_left(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'>"
        "<div style='float:left;width:40px;height:30px'></div>"
        "<div id=b style='clear:left;clear:none;width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(WE_CLEAR_NONE, lot52_node("b")->clear_side);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
}

static void test_l56_wrap_reverse_packs_lines_from_the_cross_end(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap-reverse;width:80px;height:50px'>"
        "<div id=a style='width:40px;height:10px;background:#ff0000'></div>"
        "<div id=b style='width:40px;height:10px;background:#0000ff'></div>"
        "<div id=c style='width:40px;height:10px;background:#008000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("r")->flex_wrap);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_wrap_reverse_single_line_uses_the_cross_end(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap-reverse;width:40px;height:30px'>"
        "<div id=a style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
}

static void test_l56_wrap_reverse_row_gap_mirrors_between_lines(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap-reverse;width:40px;height:50px;row-gap:4px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:10px'></div>"
        "<div id=c style='width:40px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(26, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("c")->y);
}

static void test_l56_wrap_reverse_justify_end_stays_on_the_main_axis(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap-reverse;justify-content:flex-end;width:80px;height:30px'>"
        "<div id=a style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
}

static void test_l56_wrap_reverse_align_end_packs_toward_the_start(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap-reverse;align-content:flex-end;width:40px;height:50px'>"
        "<div id=a style='width:40px;height:10px'></div>"
        "<div id=b style='width:40px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
}

static void test_l56_column_wrap_reverse_mirrors_x(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-direction:column;flex-wrap:wrap-reverse;width:80px;height:30px'>"
        "<div id=a style='width:10px;height:20px;background:#ff0000'></div>"
        "<div id=b style='width:10px;height:20px;background:#0000ff'></div>"
        "<div id=c style='width:10px;height:20px;background:#008000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_invalid_wrap_keeps_wrap_reverse(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap-reverse;flex-wrap:sideways;width:40px;height:30px'>"
        "<div id=a style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("r")->flex_wrap);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("flex-wrap refused"));
}

static void test_l56_nowrap_clears_wrap_reverse(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;flex-wrap:wrap-reverse;flex-wrap:nowrap;width:40px;height:30px'>"
        "<div id=a style='width:20px;height:10px'></div>"
        "<div id=b style='width:20px;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("r")->flex_wrap);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
}

static void test_l56_line_height_grows_one_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:20px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->line_height);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_EQUAL(16, find_text("ab")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_line_height_spaces_the_second_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:20px;width:16px'>ab cd</div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_EQUAL(21, find_text("cd")->y);
    TEST_ASSERT_EQUAL(0, find_text("cd")->x);
}

static void test_l56_line_height_unitless_is_a_multiple(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:2'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->line_height);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
}

static void test_l56_line_height_inherits(void) {
    lot52_render("<body style='margin:0'><div style='line-height:20px'><div id=p>ab</div></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->line_height);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
}

static void test_l56_line_height_normal_resets(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:20px;line-height:normal'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->line_height);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
}

static void test_l56_line_height_too_small_is_refused(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:20px;line-height:9px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->line_height);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("line height refused"));
}

static void test_l56_line_height_unitless_above_four_is_refused(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:2;line-height:5'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("line height refused"));
}

static void test_l56_line_height_above_the_cap_is_refused(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:20px;line-height:41px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("line height refused"));
}

static void test_l56_line_height_with_pre_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='white-space:pre-line;line-height:20px'>a\nb</div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_EQUAL(21, find_text("b")->y);
}

static void test_l56_line_height_br_uses_the_same_step(void) {
    lot52_render("<body style='margin:0'><div id=p style='line-height:20px'>ab<br>cd</div></body>", 320);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_EQUAL(21, find_text("cd")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
}

static void test_l56_text_indent_shifts_only_the_first_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='text-indent:16px;width:48px'>abcd efgh</div></body>", 320);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->text_indent);
    TEST_ASSERT_EQUAL(16, find_text("abcd")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcd")->y);
    TEST_ASSERT_EQUAL(0, find_text("efgh")->x);
    TEST_ASSERT_EQUAL(11, find_text("efgh")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_text_indent_center_uses_the_shortened_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='text-indent:16px;text-align:center;width:80px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(40, find_text("ab")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
}

static void test_l56_text_indent_max_content_includes_the_indent(void) {
    lot52_render("<body style='margin:0'><div id=p style='text-indent:16px;width:max-content'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->max_content);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(16, find_text("ab")->x);
}

static void test_l56_text_indent_inherits_and_zero_resets(void) {
    lot52_render("<body style='margin:0'><div style='text-indent:16px'><div id=p style='width:80px'>ab</div></div></body>", 320);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->text_indent);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->x);
    TEST_ASSERT_EQUAL(16, find_text("ab")->x);
}

static void test_l56_text_indent_zero_resets(void) {
    lot52_render("<body style='margin:0'><div id=p style='text-indent:16px;text-indent:0;width:80px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("p")->text_indent);
    TEST_ASSERT_EQUAL(0, find_text("ab")->x);
}

static void test_l56_text_indent_percent_is_refused(void) {
    lot52_render("<body style='margin:0'><div id=p style='text-indent:16px;text-indent:10%;width:80px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(16, lot52_node("p")->text_indent);
    TEST_ASSERT_EQUAL(16, find_text("ab")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("text indent refused"));
}

static void test_l56_text_indent_negative_is_refused(void) {
    lot52_render("<body style='margin:0'><div id=p style='text-indent:16px;text-indent:-4px;width:80px'>ab</div></body>", 320);
    TEST_ASSERT_EQUAL(16, find_text("ab")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("text indent refused"));
}

static void test_l56_text_indent_after_a_break_starts_at_zero(void) {
    lot52_render("<body style='margin:0'><div id=p style='text-indent:16px;width:80px'>ab<br>cd</div></body>", 320);
    TEST_ASSERT_EQUAL(16, find_text("ab")->x);
    TEST_ASSERT_EQUAL(0, find_text("cd")->x);
    TEST_ASSERT_EQUAL(11, find_text("cd")->y);
}

static void test_l56_valign_middle_centers_the_line_in_the_row(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:80px'><tr style='height:40px'>"
        "<td id=a style='vertical-align:middle'>M</td><td id=b>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(WE_VALIGN_MIDDLE, lot52_node("a")->valign);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(16, find_text("M")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(1, find_text("B")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_valign_bottom_pins_the_line_to_the_row(void) {
    lot52_render("<body style='margin:0'><table style='width:40px'><tr style='height:40px'>"
        "<td id=a style='vertical-align:bottom'>M</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(31, find_text("M")->y);
}

static void test_l56_valign_top_keeps_the_content_box(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:40px'><tr style='height:40px'>"
        "<td id=a style='vertical-align:top'>M</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(WE_VALIGN_TOP, lot52_node("a")->valign);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, find_text("M")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("t")->h);
}

static void test_l56_valign_middle_on_a_rowspan(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:80px'><tr>"
        "<td id=a rowspan=2 style='vertical-align:middle'>M</td><td id=b>B</td></tr>"
        "<tr><td id=c>C</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(6, find_text("M")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
}

static void test_l56_valign_baseline_is_refused(void) {
    lot52_render("<body style='margin:0'><table style='width:40px'><tr style='height:40px'>"
        "<td id=a style='vertical-align:middle;vertical-align:baseline'>M</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(WE_VALIGN_MIDDLE, lot52_node("a")->valign);
    TEST_ASSERT_EQUAL(16, find_text("M")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("vertical align refused"));
}

static void test_l56_inline_block_sits_between_words(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'>ab"
        "<div id=b style='display:inline-block;width:20px;height:10px;background:#ff0000'></div>cd</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_DISP_INLINE_BLOCK, lot52_node("b")->display);
    TEST_ASSERT_EQUAL(0, find_text("ab")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(36, find_text("cd")->x);
    TEST_ASSERT_EQUAL(1, find_text("cd")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_inline_block_taller_than_the_line(void) {
    lot52_render("<body style='margin:0'><div id=p>ab"
        "<div id=b style='display:inline-block;width:12px;height:30px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
}

static void test_l56_two_inline_blocks_pack_in_order(void) {
    lot52_render("<body style='margin:0'><div id=p>"
        "<div id=a style='display:inline-block;width:20px;height:10px;background:#ff0000'></div>"
        "<div id=b style='display:inline-block;width:12px;height:10px;background:#0000ff'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
}

static void test_l56_inline_block_wraps_when_it_does_not_fit(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:30px'>ab"
        "<div id=b style='display:inline-block;width:24px;height:10px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
}

static void test_l56_inline_block_shrinks_to_its_text(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'>"
        "<div id=b style='display:inline-block;background:#ff0000'>abcd</div></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, find_text("abcd")->x);
}

static void test_l56_inline_block_width_wraps_its_text(void) {
    lot52_render("<body style='margin:0'><div id=p>"
        "<div id=b style='display:inline-block;width:16px'>ab cd</div></div></body>", 320);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_EQUAL(11, find_text("cd")->y);
}

static void test_l56_inline_block_margin_pushes_the_border_box(void) {
    lot52_render("<body style='margin:0'><div id=p>ab"
        "<div id=b style='display:inline-block;width:10px;height:10px;margin-left:8px;background:#ff0000'></div>cd</div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(34, find_text("cd")->x);
}

static void test_l56_inline_block_display_block_stacks(void) {
    lot52_render("<body style='margin:0'><div id=p>"
        "<div id=b style='display:inline-block;display:block;width:20px;height:10px'>z</div>cd</div></body>", 320);
    TEST_ASSERT_EQUAL(WE_DISP_BLOCK, lot52_node("b")->display);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(11, find_text("cd")->y);
}

static void test_l56_z_index_paints_the_higher_absolute_later(void) {
    int red, blue;
    lot52_render("<body style='margin:0'><div id=h style='position:relative;width:30px;height:30px'>"
        "<div id=a style='position:absolute;left:0;top:0;width:10px;height:10px;background:#ff0000;z-index:2'></div>"
        "<div id=b style='position:absolute;left:0;top:0;width:10px;height:10px;background:#0000ff;z-index:1'></div>"
        "</div></body>", 320);
    red = lot56_rect_at(0xFF0000);
    blue = lot56_rect_at(0x0000FF);
    TEST_ASSERT_TRUE(red >= 0 && blue >= 0);
    TEST_ASSERT_TRUE(blue < red);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->z_index);
    TEST_ASSERT_EQUAL(1, lot52_node("b")->z_index);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l56_z_index_tie_keeps_source_order(void) {
    int red, blue;
    lot52_render("<body style='margin:0'><div style='position:relative;width:20px;height:20px'>"
        "<div id=a style='position:absolute;width:10px;height:10px;background:#ff0000;z-index:1'></div>"
        "<div id=b style='position:absolute;width:10px;height:10px;background:#0000ff;z-index:1'></div>"
        "</div></body>", 320);
    red = lot56_rect_at(0xFF0000);
    blue = lot56_rect_at(0x0000FF);
    TEST_ASSERT_TRUE(red >= 0 && blue > red);
}

static void test_l56_z_index_negative_stays_before_positive(void) {
    int red, blue;
    lot52_render("<body style='margin:0'><div style='position:relative;width:20px;height:20px'>"
        "<div id=a style='position:absolute;width:10px;height:10px;background:#ff0000;z-index:1'></div>"
        "<div id=b style='position:absolute;width:10px;height:10px;background:#0000ff;z-index:-1'></div>"
        "</div></body>", 320);
    red = lot56_rect_at(0xFF0000);
    blue = lot56_rect_at(0x0000FF);
    TEST_ASSERT_EQUAL(-1, lot52_node("b")->z_index);
    TEST_ASSERT_TRUE(blue < red);
}

static void test_l56_z_index_auto_restores_source_order(void) {
    int red, blue;
    lot52_render("<body style='margin:0'><div style='position:relative;width:20px;height:20px'>"
        "<div id=a style='position:absolute;width:10px;height:10px;background:#ff0000;z-index:2;z-index:auto'></div>"
        "<div id=b style='position:absolute;width:10px;height:10px;background:#0000ff;z-index:1'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->z_set);
    red = lot56_rect_at(0xFF0000);
    blue = lot56_rect_at(0x0000FF);
    TEST_ASSERT_TRUE(red < blue);
}

static void test_l56_z_index_out_of_range_keeps_the_previous(void) {
    int red, blue;
    lot52_render("<body style='margin:0'><div style='position:relative;width:20px;height:20px'>"
        "<div id=a style='position:absolute;width:10px;height:10px;background:#ff0000;z-index:2;z-index:9'></div>"
        "<div id=b style='position:absolute;width:10px;height:10px;background:#0000ff;z-index:1'></div>"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->z_index);
    TEST_ASSERT_TRUE(lot52_has_diag("z-index refused"));
    red = lot56_rect_at(0xFF0000);
    blue = lot56_rect_at(0x0000FF);
    TEST_ASSERT_TRUE(blue < red);
}

static void test_l56_z_index_on_static_does_not_reorder_flow(void) {
    int red, blue;
    lot52_render("<body style='margin:0'>"
        "<div id=a style='z-index:5;width:10px;height:10px;background:#ff0000'></div>"
        "<div id=b style='position:relative;z-index:-1;width:10px;height:10px;background:#0000ff'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->absolute);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->z_set);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    red = lot56_rect_at(0xFF0000);
    blue = lot56_rect_at(0x0000FF);
    TEST_ASSERT_TRUE(red < blue);
}

static void test_l56_z_index_bounds_are_accepted(void) {
    lot52_render("<body style='margin:0'><div style='position:relative;height:10px'>"
        "<div id=a style='position:absolute;z-index:-8;width:4px;height:4px;background:#ff0000'></div>"
        "<div id=b style='position:absolute;z-index:8;width:4px;height:4px;background:#0000ff'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(-8, lot52_node("a")->z_index);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->z_index);
    TEST_ASSERT_TRUE(lot56_rect_at(0xFF0000) < lot56_rect_at(0x0000FF));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot56b_run(void) {
    RUN_TEST(test_l56_pre_line_min_content_is_the_longest_line_word);
    RUN_TEST(test_l56_pre_line_wraps_and_keeps_the_newline);
    RUN_TEST(test_l56_pre_line_collapses_a_run_of_spaces);
    RUN_TEST(test_l56_pre_line_leading_newline_is_an_empty_line);
    RUN_TEST(test_l56_pre_line_inherits_through_a_span);
    RUN_TEST(test_l56_pre_line_resets_pre);
    RUN_TEST(test_l56_break_spaces_is_refused);
    RUN_TEST(test_l56_spacing_two_values_split_the_axes);
    RUN_TEST(test_l56_spacing_one_value_sets_both_axes);
    RUN_TEST(test_l56_spacing_horizontal_zero_keeps_rows_apart);
    RUN_TEST(test_l56_spacing_vertical_zero_keeps_columns_apart);
    RUN_TEST(test_l56_spacing_collapse_zeros_both_axes);
    RUN_TEST(test_l56_spacing_second_value_refused_keeps_both);
    RUN_TEST(test_l56_spacing_rowspan_uses_the_vertical_axis);
    RUN_TEST(test_l56_clear_left_moves_below_the_float);
    RUN_TEST(test_l56_no_clear_sits_beside_the_float);
    RUN_TEST(test_l56_clear_right_ignores_a_left_float);
    RUN_TEST(test_l56_clear_both_uses_the_taller_float);
    RUN_TEST(test_l56_clear_left_stops_at_the_left_float);
    RUN_TEST(test_l56_clear_right_stops_at_the_right_float);
    RUN_TEST(test_l56_second_float_clears_the_first);
    RUN_TEST(test_l56_invalid_clear_keeps_left);
    RUN_TEST(test_l56_clear_none_resets_left);
    RUN_TEST(test_l56_wrap_reverse_packs_lines_from_the_cross_end);
    RUN_TEST(test_l56_wrap_reverse_single_line_uses_the_cross_end);
    RUN_TEST(test_l56_wrap_reverse_row_gap_mirrors_between_lines);
    RUN_TEST(test_l56_wrap_reverse_justify_end_stays_on_the_main_axis);
    RUN_TEST(test_l56_wrap_reverse_align_end_packs_toward_the_start);
    RUN_TEST(test_l56_column_wrap_reverse_mirrors_x);
    RUN_TEST(test_l56_invalid_wrap_keeps_wrap_reverse);
    RUN_TEST(test_l56_nowrap_clears_wrap_reverse);
    RUN_TEST(test_l56_line_height_grows_one_line);
    RUN_TEST(test_l56_line_height_spaces_the_second_line);
    RUN_TEST(test_l56_line_height_unitless_is_a_multiple);
    RUN_TEST(test_l56_line_height_inherits);
    RUN_TEST(test_l56_line_height_normal_resets);
    RUN_TEST(test_l56_line_height_too_small_is_refused);
    RUN_TEST(test_l56_line_height_unitless_above_four_is_refused);
    RUN_TEST(test_l56_line_height_above_the_cap_is_refused);
    RUN_TEST(test_l56_line_height_with_pre_line);
    RUN_TEST(test_l56_line_height_br_uses_the_same_step);
    RUN_TEST(test_l56_text_indent_shifts_only_the_first_line);
    RUN_TEST(test_l56_text_indent_center_uses_the_shortened_line);
    RUN_TEST(test_l56_text_indent_max_content_includes_the_indent);
    RUN_TEST(test_l56_text_indent_inherits_and_zero_resets);
    RUN_TEST(test_l56_text_indent_zero_resets);
    RUN_TEST(test_l56_text_indent_percent_is_refused);
    RUN_TEST(test_l56_text_indent_negative_is_refused);
    RUN_TEST(test_l56_text_indent_after_a_break_starts_at_zero);
    RUN_TEST(test_l56_valign_middle_centers_the_line_in_the_row);
    RUN_TEST(test_l56_valign_bottom_pins_the_line_to_the_row);
    RUN_TEST(test_l56_valign_top_keeps_the_content_box);
    RUN_TEST(test_l56_valign_middle_on_a_rowspan);
    RUN_TEST(test_l56_valign_baseline_is_refused);
    RUN_TEST(test_l56_inline_block_sits_between_words);
    RUN_TEST(test_l56_inline_block_taller_than_the_line);
    RUN_TEST(test_l56_two_inline_blocks_pack_in_order);
    RUN_TEST(test_l56_inline_block_wraps_when_it_does_not_fit);
    RUN_TEST(test_l56_inline_block_shrinks_to_its_text);
    RUN_TEST(test_l56_inline_block_width_wraps_its_text);
    RUN_TEST(test_l56_inline_block_margin_pushes_the_border_box);
    RUN_TEST(test_l56_inline_block_display_block_stacks);
    RUN_TEST(test_l56_z_index_paints_the_higher_absolute_later);
    RUN_TEST(test_l56_z_index_tie_keeps_source_order);
    RUN_TEST(test_l56_z_index_negative_stays_before_positive);
    RUN_TEST(test_l56_z_index_auto_restores_source_order);
    RUN_TEST(test_l56_z_index_out_of_range_keeps_the_previous);
    RUN_TEST(test_l56_z_index_on_static_does_not_reorder_flow);
    RUN_TEST(test_l56_z_index_bounds_are_accepted);
}
