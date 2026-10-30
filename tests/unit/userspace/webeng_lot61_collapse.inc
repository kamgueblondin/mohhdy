/* Lot 61: per-edge collapsed border resolution (CSS 2.1 17.6.2.1). */
#define L61_C_OPEN "<body style='margin:0'><table id=t style='width:40px;border-collapse:collapse"

static void test_l61_resolve_style_precedence_double_over_solid(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:2px solid #ff0000;height:6px'></td>"
                 "<td id=b style='border:2px double #0000ff;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("b")->border_style);
    l61_rect_at(19, 0, 2, 10, 0x0000FF);          /* shared line: double wins */
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(20, 5));
    l61_rect_at(0, 0, 2, 10, 0xFF0000);           /* a keeps its outer edge */
    l61_rect_at(38, 0, 2, 10, 0x0000FF);
}

static void test_l61_resolve_style_ranks_dashed_dotted_ridge_inset(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:2px dotted #ff0000;height:6px'></td>"
                 "<td id=b style='border:2px dashed #0000ff;height:6px'></td></tr><tr>"
                 "<td id=c style='border:2px inset #00ff00;height:6px'></td>"
                 "<td id=d style='border:2px ridge #ffff00;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(20, 4));   /* dashed > dotted */
    TEST_ASSERT_EQUAL(0xFFFF00, (int)l61_paint(20, 14));  /* ridge > inset */
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 8));    /* dotted > inset */
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(30, 8));   /* dashed > ridge */
}

static void test_l61_resolve_wider_edge_wins_over_style(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:3px dotted #ff0000;height:6px'></td>"
                 "<td id=b style='border:2px double #0000ff;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("t")->collapse_w);
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(20, 5));
    /* b's 2px outer edge is centred in the 3px band. */
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(37, 5));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(38, 5));
    TEST_ASSERT_TRUE(l61_paint(39, 5) != 0x0000FF);
}

static void test_l61_resolve_hidden_suppresses_the_shared_edge(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:2px solid #ff0000;height:6px'></td>"
                 "<td id=b style='border:2px solid #0000ff;border-left:2px hidden;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("b")->edge_style[3]);
    TEST_ASSERT_TRUE(l61_paint(19, 5) == 0xFFFFFFFFu);
    TEST_ASSERT_TRUE(l61_paint(20, 5) == 0xFFFFFFFFu);
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(0, 5));
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(39, 5));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_resolve_none_loses_to_any_visible_edge(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:2px solid #ff0000;border-right:4px none;height:6px'></td>"
                 "<td id=b style='border:1px solid #0000ff;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("t")->collapse_w);       /* none's 4px is ignored */
    TEST_ASSERT_EQUAL(0x0000FF, (int)l61_paint(20, 5));      /* b's 1px on the grid line */
    TEST_ASSERT_TRUE(l61_paint(19, 5) != 0xFF0000);
}

static void test_l61_resolve_origin_cell_over_row_over_table(void) {
    lot52_render(L61_C_OPEN ";border:3px solid #0000ff'><tr style='border:3px solid #00ff00'>"
                 "<td id=a style='border:3px solid #ff0000;height:6px'></td><td id=b style='height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("t")->border);           /* no own border box */
    TEST_ASSERT_EQUAL(3, lot52_node("t")->edge_w[0]);
    l61_box("t", 0, 0, 40, 12);
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_BORDER));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 0));       /* cell beats row and table */
    TEST_ASSERT_EQUAL(0x00FF00, (int)l61_paint(30, 0));      /* row beats table */
    TEST_ASSERT_EQUAL(0x00FF00, (int)l61_paint(39, 5));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(20, 5));      /* a's right edge */
}

static void test_l61_resolve_table_outer_border_merges_with_cells(void) {
    lot52_render(L61_C_OPEN ";border:4px solid #0000ff'><tr><td id=a style='border:2px solid #ff0000;height:6px'>A</td>"
                 "<td id=b style='height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("t")->collapse_w);
    l61_box("t", 0, 0, 40, 14);                              /* 4 + 6 + 4: content clears the outer border */
    l61_rect_at(0, 0, 40, 4, 0x0000FF);                      /* one merged top line */
    l61_rect_at(0, 10, 40, 4, 0x0000FF);
    l61_rect_at(0, 0, 4, 14, 0x0000FF);
    l61_rect_at(36, 0, 4, 14, 0x0000FF);
    l61_rect_at(19, 0, 2, 14, 0xFF0000);                     /* inner 2px centred in 4 */
    l61_text_at("A", 4, 5);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_resolve_row_group_boundary(void) {
    lot52_render(L61_C_OPEN "'><thead style='border-bottom:3px solid #00ff00'><tr><td id=a style='border:1px solid #ff0000;height:4px'></td></tr></thead>"
                 "<tbody><tr><td id=b style='border:1px solid #ff0000;height:4px'></td></tr></tbody></table></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("t")->collapse_w);
    l61_box("a", 0, 0, 40, 9);                               /* 2 + 4 + 3 */
    l61_box("b", 0, 6, 40, 9);                               /* 3 + 4 + 2 */
    l61_rect_at(0, 6, 40, 3, 0x00FF00);
    l61_rect_at(0, 1, 40, 1, 0xFF0000);
    l61_rect_at(0, 13, 40, 1, 0xFF0000);
}

static void test_l61_resolve_rowspan_has_no_inner_segment(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a rowspan=2 style='border:2px solid #ff0000'></td><td id=b style='border:1px solid #0000ff;height:6px'></td></tr>"
                 "<tr><td id=c style='border:1px solid #00ff00;height:6px'></td></tr></table></body>", 320);
    l61_box("c", 19, 7, 21, 9);
    TEST_ASSERT_TRUE(l61_paint(5, 6) == 0xFFFFFFFFu);        /* inside a */
    TEST_ASSERT_TRUE(l61_paint(5, 7) == 0xFFFFFFFFu);
    l61_rect_at(19, 8, 21, 1, 0x0000FF);                     /* b above c: upper wins */
    l61_rect_at(19, 0, 2, 16, 0xFF0000);                     /* a's right edge spans both rows */
    l61_rect_at(39, 0, 1, 9, 0x0000FF);
    l61_rect_at(39, 7, 1, 9, 0x00FF00);
}

static void test_l61_resolve_edge_lists_map_four_sides(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border-style:solid dashed;border-width:1px 2px 3px 4px;"
                 "border-color:#ff0000 #00ff00 #0000ff #ffff00;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->edge_style[0]);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->edge_style[1]);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->edge_style[2]);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->edge_style[3]);
    TEST_ASSERT_EQUAL(4, lot52_node("t")->collapse_w);
    l61_box("a", 0, 0, 40, 12);                              /* 3 + 6 + 3 */
    l61_rect_at(0, 2, 40, 1, 0xFF0000);
    l61_rect_at(0, 9, 40, 3, 0x0000FF);
    l61_rect_at(0, 0, 4, 12, 0xFFFF00);
    l61_rect_at(37, 0, 2, 12, 0x00FF00);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_resolve_one_edge_shorthand_keeps_others(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:1px solid #ff0000;border-bottom:3px #0000ff;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->edge_w[2]);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->edge_w[0]);
    l61_box("a", 0, 0, 40, 11);                              /* 2 + 6 + 3 */
    l61_rect_at(0, 1, 40, 1, 0xFF0000);
    l61_rect_at(0, 8, 40, 3, 0x0000FF);
    /* A later border shorthand resets every edge. */
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border-bottom:3px #0000ff;border:1px solid #ff0000;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->edge_w[2]);
    TEST_ASSERT_EQUAL(1, lot52_node("t")->collapse_w);
}

static void test_l61_resolve_invalid_edges_are_refused_atomically(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:40px;border-collapse:collapse'><tr><td id=a style='"
                 "border:2px solid #ff0000;border-top:9px wavy;border-left:1px solid red blue;border-style:solid bogus;"
                 "border-width:1px 2px 3px 4px 5px;border-color:#ff0000 nope;border-right:-1px solid;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(6, l61_diag_count("border edge refused"));
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->edge_w[0]);
    TEST_ASSERT_EQUAL(-1, lot52_node("a")->edge_w[1]);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->edge_style[0]);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->edge_color_set);
    TEST_ASSERT_EQUAL(2, lot52_node("t")->collapse_w);
    l61_rect_at(0, 0, 40, 2, 0xFF0000);
}

static void test_l61_resolve_edges_outside_collapse_are_diagnosed(void) {
    lot52_render("<body style='margin:0'><div id=x style='border:2px solid #ff0000;border-top:4px solid #0000ff;width:10px;height:10px'></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("border edge refused"));
    l61_box("x", 0, 0, 14, 14);                              /* uniform separate border */
    TEST_ASSERT_EQUAL(1, l61_count(OS_FB_WEB_BORDER));
    TEST_ASSERT_EQUAL(0xFF0000, (int)l61_paint(5, 0));
}

static void test_l61_border_style_none_and_hidden_draw_nothing(void) {
    lot52_render("<body style='margin:0'><div id=x style='border:2px none #ff0000;width:10px;height:10px'></div>"
                 "<div id=y style='border:2px solid #ff0000;border-style:hidden;width:10px;height:10px'></div>"
                 "<div id=z style='border-width:3px;border-color:#00ff00;width:10px;height:10px'></div></body>", 320);
    l61_box("x", 0, 0, 10, 10);
    l61_box("y", 0, 10, 10, 10);
    l61_box("z", 0, 20, 16, 16);
    TEST_ASSERT_EQUAL(1, l61_count(OS_FB_WEB_BORDER));
    TEST_ASSERT_EQUAL(0x00FF00, (int)l61_paint(0, 20));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_resolve_successive_layouts_and_restyle(void) {
    int pass;
    lot52_render(L61_C_OPEN ";border:4px solid #0000ff'><tr><td id=a style='border:2px solid #ff0000;height:6px'>A</td>"
                 "<td id=b style='height:6px'></td></tr></table></body>", 320);
    for (pass = 0; pass < 2; pass++) {
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
        TEST_ASSERT_EQUAL(4, lot52_node("t")->collapse_w);
        TEST_ASSERT_EQUAL(0, lot52_node("t")->border);
        l61_rect_at(0, 0, 40, 4, 0x0000FF);
        l61_rect_at(19, 0, 2, 14, 0xFF0000);
    }
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 320));
    TEST_ASSERT_EQUAL(4, lot52_node("t")->edge_w[3]);
    l61_rect_at(0, 0, 4, 14, 0x0000FF);
}


/* Content clears half of each resolved line: the line of width w sits at
 * B/2 - w/2 in the band, an odd pixel on the side after the grid line. */
static void test_l61_inset_odd_line_goes_after_the_grid_line(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:4px solid #ff0000;border-right:1px solid #ff0000'>A</td>"
                 "<td id=b style='border:4px solid #0000ff;border-left:1px solid #00ff00'>B</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("t")->collapse_w);
    l61_box("a", 0, 0, 22, 18);                              /* 4 + 10 + 4 */
    l61_box("b", 18, 0, 22, 18);
    l61_rect_at(20, 0, 1, 18, 0xFF0000);                     /* tie: left cell, at band + 2 */
    l61_text_at("A", 4, 5);
    l61_text_at("B", 21, 5);                                 /* right after the 1px line */
    TEST_ASSERT_EQUAL(2, lot52_node("a")->cin[1]);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->cin[3]);
}

static void test_l61_inset_full_band_line_is_cleared_on_both_sides(void) {
    lot52_render(L61_C_OPEN "'><tr><td id=a style='border:3px solid #ff0000'>A</td>"
                 "<td id=b style='border:3px solid #0000ff'>B</td></tr></table></body>", 320);
    l61_rect_at(19, 0, 3, 16, 0xFF0000);
    l61_text_at("A", 3, 4);
    l61_text_at("B", 22, 4);
    l61_box("a", 0, 0, 22, 16);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_column_borders_join_resolution(void) {
    lot52_render(L61_C_OPEN "'><col><col style='border:2px solid #ff0000'><tr><td id=a style='height:6px'></td><td id=b style='height:6px'></td></tr>"
                 "<tr><td id=c style='height:6px'></td><td id=d style='border-bottom:2px solid #0000ff;height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("t")->collapse_w);
    l61_rect_at(19, 0, 21, 2, 0xFF0000);                     /* column top */
    l61_rect_at(19, 0, 2, 16, 0xFF0000);                     /* column left, both rows */
    l61_rect_at(38, 0, 2, 16, 0xFF0000);
    l61_rect_at(19, 14, 21, 2, 0x0000FF);                    /* cell beats column on a tie */
    TEST_ASSERT_TRUE(l61_paint(5, 0) == 0xFFFFFFFFu);        /* first column has no border */
    TEST_ASSERT_TRUE(l61_paint(25, 7) == 0xFFFFFFFFu);       /* no line between rows */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l61_column_beats_column_group_beats_table(void) {
    lot52_render(L61_C_OPEN ";border:2px solid #00ff00'><colgroup span=2 style='border:2px solid #ff0000'></colgroup>"
                 "<col style='border:2px solid #0000ff'><tr><td id=a style='height:6px'>A</td><td id=b style='height:6px'></td>"
                 "<td id=c style='height:6px'></td></tr></table></body>", 320);
    l61_rect_at(0, 0, 28, 2, 0xFF0000);                      /* group over table */
    l61_rect_at(26, 0, 14, 2, 0x0000FF);
    l61_rect_at(26, 0, 2, 10, 0x0000FF);                     /* column over group */
    l61_rect_at(38, 0, 2, 10, 0x0000FF);
    l61_rect_at(0, 0, 2, 10, 0xFF0000);
    TEST_ASSERT_TRUE(l61_paint(14, 5) == 0xFFFFFFFFu);       /* no line inside the group */
    TEST_ASSERT_EQUAL(-1, l61_index(0x00FF00));
    l61_text_at("A", 2, 3);
}

static void test_l61_column_group_children_and_span(void) {
    lot52_render(L61_C_OPEN "'><colgroup style='border:2px solid #ff0000'><col span=2><col></colgroup><tr>"
                 "<td id=a style='height:6px'></td><td id=b style='height:6px'></td><td id=c style='height:6px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(2, lot52_node("t")->collapse_w);
    l61_rect_at(0, 0, 40, 2, 0xFF0000);
    l61_rect_at(0, 8, 40, 2, 0xFF0000);
    l61_rect_at(0, 0, 2, 10, 0xFF0000);
    l61_rect_at(38, 0, 2, 10, 0xFF0000);
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);
    /* A group with col children covers only them, not the next column. */
    lot52_render(L61_C_OPEN "'><colgroup style='border:2px solid #ff0000'><col></colgroup><tr>"
                 "<td id=a style='height:6px'></td><td id=b style='height:6px'></td></tr></table></body>", 320);
    l61_rect_at(0, 0, 21, 2, 0xFF0000);
    TEST_ASSERT_TRUE(l61_paint(30, 0) == 0xFFFFFFFFu);
    /* A <col> span past the 8 track limit is refused like colspan. */
    lot52_render(L61_C_OPEN "'><col span=9><tr><td>A</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("table span refused"));
}

static void test_l61_column_is_void_and_inert_when_separate(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:40px'><col style='border:2px solid #ff0000'>"
                 "<tr><td id=a>A</td></tr></table><p id=p>P</p></body>", 320);
    l61_box("a", 0, 0, 40, 10);
    TEST_ASSERT_EQUAL(-1, l61_index(0xFF0000));
    TEST_ASSERT_EQUAL(0, l61_diag_count("table child refused"));
    TEST_ASSERT_EQUAL(0, l61_diag_count("border edge refused"));
    TEST_ASSERT_EQUAL(lot52_node("t")->parent, lot52_node("p")->parent);   /* col is void */
    l61_text_at("P", 0, 19);
}

static void lot61_collapse_run(void) {
    RUN_TEST(test_l61_resolve_style_precedence_double_over_solid);
    RUN_TEST(test_l61_resolve_style_ranks_dashed_dotted_ridge_inset);
    RUN_TEST(test_l61_resolve_wider_edge_wins_over_style);
    RUN_TEST(test_l61_resolve_hidden_suppresses_the_shared_edge);
    RUN_TEST(test_l61_resolve_none_loses_to_any_visible_edge);
    RUN_TEST(test_l61_resolve_origin_cell_over_row_over_table);
    RUN_TEST(test_l61_resolve_table_outer_border_merges_with_cells);
    RUN_TEST(test_l61_resolve_row_group_boundary);
    RUN_TEST(test_l61_resolve_rowspan_has_no_inner_segment);
    RUN_TEST(test_l61_resolve_edge_lists_map_four_sides);
    RUN_TEST(test_l61_resolve_one_edge_shorthand_keeps_others);
    RUN_TEST(test_l61_resolve_invalid_edges_are_refused_atomically);
    RUN_TEST(test_l61_resolve_edges_outside_collapse_are_diagnosed);
    RUN_TEST(test_l61_border_style_none_and_hidden_draw_nothing);
    RUN_TEST(test_l61_resolve_successive_layouts_and_restyle);
    RUN_TEST(test_l61_inset_odd_line_goes_after_the_grid_line);
    RUN_TEST(test_l61_inset_full_band_line_is_cleared_on_both_sides);
    RUN_TEST(test_l61_column_borders_join_resolution);
    RUN_TEST(test_l61_column_beats_column_group_beats_table);
    RUN_TEST(test_l61_column_group_children_and_span);
    RUN_TEST(test_l61_column_is_void_and_inert_when_separate);
}
