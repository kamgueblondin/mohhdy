/* Lot 6 (US-031, US-043, US-045): guest web engine, parse + style + layout
 * + display list, inspector and diagnostics. */
#include "../../framework/unity.h"
#include "../../../userspace/webeng.h"
#include <string.h>

static we_doc_t g_doc;

static const char* PAGE =
    "<!DOCTYPE html><html><head><title>Demo page</title>"
    "<style>body{margin:0} #hero{background:#ff0000;width:200px;height:40px;padding:4px}"
    " .note{color:#00ff00} div p{font-weight:bold} .hide{display:none} p.c{text-align:center}"
    " @media (max-width:10px){p{color:red}}</style></head>"
    "<body><div id=hero>Hi</div>"
    "<div><p class=note>green bold</p></div>"
    "<p class=hide>hidden</p>"
    "<p>one &amp; two<br>three</p>"
    "<ul><li>a<li>b</ul>"
    "<script>alert(1)</script>"
    "<p class=c style='color:#0000ff'>mid</p>"
    "</body></html>";

static const os_fb_web_item_t* find_text(const char* t) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && strcmp(g_doc.items[i].text, t) == 0) return &g_doc.items[i];
    return 0;
}

static void test_parse_tree_and_title(void) {
    int hero, li;
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, PAGE, (int)strlen(PAGE)));
    TEST_ASSERT_EQUAL(0, strcmp(g_doc.title, "Demo page"));
    TEST_ASSERT_EQUAL(1U, g_doc.perf.scripts_skipped);
    hero = we_find_id(&g_doc, "hero");
    TEST_ASSERT_TRUE(hero > 0);
    TEST_ASSERT_EQUAL(0, strcmp(g_doc.nodes[g_doc.nodes[hero].parent].tag, "body"));
    /* implied </li>: the two li are siblings */
    for (li = 0; li < (int)g_doc.nnodes; li++) if (strcmp(g_doc.nodes[li].tag, "li") == 0) break;
    TEST_ASSERT_TRUE(li < (int)g_doc.nnodes);
    TEST_ASSERT_TRUE(g_doc.nodes[li].next > 0);
    TEST_ASSERT_EQUAL(0, strcmp(g_doc.nodes[g_doc.nodes[li].next].tag, "li"));
    TEST_ASSERT_TRUE(g_doc.css_len > 20);
}

static void test_cascade(void) {
    int hero, i, note = -1, hide = -1, c = -1;
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, PAGE, (int)strlen(PAGE)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_TRUE(g_doc.nrules >= 5);
    hero = we_find_id(&g_doc, "hero");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[hero].has_bg);
    TEST_ASSERT_EQUAL(0xFF0000U, g_doc.nodes[hero].bg);
    TEST_ASSERT_EQUAL(200, g_doc.nodes[hero].width);
    for (i = 0; i < (int)g_doc.nnodes; i++) {
        if (strcmp(g_doc.nodes[i].cls, "note") == 0) note = i;
        if (strcmp(g_doc.nodes[i].cls, "hide") == 0) hide = i;
        if (strcmp(g_doc.nodes[i].cls, "c") == 0) c = i;
    }
    TEST_ASSERT_EQUAL(0x00FF00U, g_doc.nodes[note].color);
    TEST_ASSERT_EQUAL(1, g_doc.nodes[note].bold);           /* descendant div p */
    TEST_ASSERT_EQUAL(WE_DISP_NONE, g_doc.nodes[hide].display);
    TEST_ASSERT_EQUAL(0x0000FFU, g_doc.nodes[c].color);     /* inline style wins */
    TEST_ASSERT_EQUAL(WE_ALIGN_CENTER, g_doc.nodes[c].align);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[c].bold);               /* not inside a div */
}

static void test_layout_and_display_list(void) {
    const os_fb_web_item_t *hi, *green, *one, *three, *mid;
    int hero;
    uint32_t i;
    int saw_red = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, PAGE, (int)strlen(PAGE), 400));
    hero = we_find_id(&g_doc, "hero");
    /* body margin 0: hero at 0,0, 200 + 2*4 padding wide, 40 + 8 high */
    TEST_ASSERT_EQUAL(0, g_doc.nodes[hero].x);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[hero].y);
    TEST_ASSERT_EQUAL(208, g_doc.nodes[hero].w);
    TEST_ASSERT_EQUAL(48, g_doc.nodes[hero].h);
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == 0xFF0000U &&
            g_doc.items[i].w == 208 && g_doc.items[i].h == 48) saw_red = 1;
    TEST_ASSERT_EQUAL(1, saw_red);
    hi = find_text("Hi");
    TEST_ASSERT_NOT_NULL(hi);
    TEST_ASSERT_EQUAL(4, hi->x);
    TEST_ASSERT_EQUAL(5, hi->y);
    green = find_text("green bold");
    TEST_ASSERT_NOT_NULL(green);
    TEST_ASSERT_EQUAL(1, green->bold);
    TEST_ASSERT_EQUAL(0x00FF00U, green->color);
    TEST_ASSERT_TRUE(green->y > 48);
    TEST_ASSERT_NULL(find_text("hidden"));
    TEST_ASSERT_NULL(find_text("alert(1)"));
    one = find_text("one & two");
    three = find_text("three");
    TEST_ASSERT_NOT_NULL(one);
    TEST_ASSERT_NOT_NULL(three);
    TEST_ASSERT_EQUAL(one->x, three->x);
    TEST_ASSERT_EQUAL(one->y + WE_LINE_H, three->y);   /* <br> */
    mid = find_text("mid");
    TEST_ASSERT_NOT_NULL(mid);
    TEST_ASSERT_EQUAL((400 - 3 * 8) / 2, mid->x);      /* centered */
    TEST_ASSERT_EQUAL(0x0000FFU, mid->color);
    TEST_ASSERT_TRUE(g_doc.perf.page_h > (uint32_t)mid->y);
    TEST_ASSERT_TRUE(g_doc.perf.lines >= 6);
    TEST_ASSERT_EQUAL(g_doc.nitems, g_doc.perf.items);
}

static void test_wrapping_and_borders(void) {
    static const char* p = "<body style='margin:0'><div style='width:80px;border:2px solid #000;padding:2px'>aaa bbb ccc ddd</div></body>";
    const os_fb_web_item_t* a;
    uint32_t i, borders = 0;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 300));
    /* content width 80 px = 10 chars: "aaa bbb" then "ccc ddd" */
    a = find_text("aaa bbb");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL(4, a->x);
    TEST_ASSERT_NOT_NULL(find_text("ccc ddd"));
    TEST_ASSERT_EQUAL(a->y + WE_LINE_H, find_text("ccc ddd")->y);
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_BORDER) { borders++; TEST_ASSERT_EQUAL(2, g_doc.items[i].bold); TEST_ASSERT_EQUAL(88, g_doc.items[i].w); }
    TEST_ASSERT_EQUAL(1U, borders);
}

static void test_percent_width_and_img_alt(void) {
    static const char* p = "<body style='margin:0'><div id=x style='width:50%;background:#123'>t <img alt='logo'> u</div></body>";
    int x;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 300));
    x = we_find_id(&g_doc, "x");
    TEST_ASSERT_EQUAL(150, g_doc.nodes[x].w);
    TEST_ASSERT_EQUAL(0x112233U, g_doc.nodes[x].bg);
    TEST_ASSERT_NOT_NULL(find_text("[logo]"));
    TEST_ASSERT_NOT_NULL(find_text("t"));
    TEST_ASSERT_NOT_NULL(find_text("u"));
}

static char g_out[4096];
static void cap(void* ctx, const char* s) { (void)ctx; strncat(g_out, s, sizeof(g_out) - strlen(g_out) - 1); }

static void test_inspector_and_diagnostics(void) {
    static const char* p = "<body><div id=a class='k'>x</span></div><script>1</script><style>p>q{x:y} div{zz:1}</style></body>";
    int a;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 300));
    TEST_ASSERT_TRUE(g_doc.ndiag >= 3);
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[0], "unmatched close tag"));
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[1], "script ignored"));
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[2], "css selector unsupported"));
    TEST_ASSERT_EQUAL(1U, g_doc.perf.unknown_props);
    a = we_find_id(&g_doc, "a");
    g_out[0] = 0;
    we_inspect(&g_doc, a, cap, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "inspect div#a display block box 8 8 284x10"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "color #000000"));
    g_out[0] = 0;
    we_dump_dom(&g_doc, cap, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "div#a class=k block"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "\"x\""));
    g_out[0] = 0;
    we_inspect(&g_doc, 999, cap, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "inspect none"));
}

static void test_limits(void) {
    static char big[8192];
    int i, n = 0;
    for (i = 0; i < 300 && n < (int)sizeof(big) - 16; i++) { memcpy(big + n, "<p>w</p>", 8); n += 8; }
    big[n] = 0;
    TEST_ASSERT_TRUE(we_parse(&g_doc, big, n) < 0);   /* node table full, reported */
    TEST_ASSERT_NOT_NULL(strstr(g_doc.diag[g_doc.ndiag - 1], "too many nodes"));
    n = 0;
    for (i = 0; i < 70; i++) { memcpy(big + n, "<b>q</b> ", 9); n += 9; }
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, big, n, 100));
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
    TEST_ASSERT_TRUE(g_doc.perf.items_dropped == 0 || g_doc.nitems == OS_FB_WEB_ITEMS);
    TEST_ASSERT_EQUAL(-1, we_layout(&g_doc, 10));
}

/* UX4: href kept by the parser, link hit test on laid-out text */
static void test_link_at(void) {
    static const char html[] = "<p>avant <a href=\"b.html\">le lien</a></p><div style=\"height:40px\">vide</div>";
    char href[32];
    uint32_t i;
    int x = -1, y = -1;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)sizeof(html) - 1, 400));
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && strstr(g_doc.items[i].text, "le lien")) { x = g_doc.items[i].x + 4; y = g_doc.items[i].y + 2; }
    TEST_ASSERT(x >= 0);
    TEST_ASSERT_EQUAL(6, we_link_at(&g_doc, x, y, href, (int)sizeof(href)));
    TEST_ASSERT_EQUAL_STRING("b.html", href);
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 2, y, href, (int)sizeof(href)));     /* "avant" */
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 2, y + 20, href, (int)sizeof(href)));
}

/* Lot 49 (US-031, partial): flex row/column and a fixed grid. No JavaScript. */
static void test_flex_and_grid(void) {
    static const char flex[] =
        "<html><body style='margin:0'>"
        "<div id=row style='display:flex;gap:10px'>"
        "<div id=a style='width:40px;height:20px'></div>"
        "<div id=b style='width:40px;height:20px'></div></div>"
        "<div id=col style='display:flex;flex-direction:column;gap:4px'>"
        "<div id=c style='height:10px'></div>"
        "<div id=d style='height:10px'></div></div>"
        "<div id=g style='display:grid;grid-template-columns:repeat(2, 1fr);gap:8px'>"
        "<div id=e style='height:10px'></div><div id=f style='height:10px'></div>"
        "<div id=h style='height:10px'></div></div>"
        "</body></html>";
    int row, a, b, col, c, d, g, e, f, h, cell;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, flex, (int)sizeof(flex) - 1, 90));
    row = we_find_id(&g_doc, "row");
    a = we_find_id(&g_doc, "a");
    b = we_find_id(&g_doc, "b");
    TEST_ASSERT_EQUAL(WE_DISP_FLEX, g_doc.nodes[row].display);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[a].x);
    TEST_ASSERT_EQUAL(40, g_doc.nodes[a].w);
    TEST_ASSERT_EQUAL(20, g_doc.nodes[a].h);
    TEST_ASSERT_EQUAL(50, g_doc.nodes[b].x);          /* (90-10)/2 + gap */
    TEST_ASSERT_EQUAL(g_doc.nodes[a].y, g_doc.nodes[b].y);
    col = we_find_id(&g_doc, "col");
    c = we_find_id(&g_doc, "c");
    d = we_find_id(&g_doc, "d");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[col].flex_dir);
    TEST_ASSERT_EQUAL(g_doc.nodes[c].x, g_doc.nodes[d].x);
    TEST_ASSERT_EQUAL(g_doc.nodes[c].y + 10 + 4, g_doc.nodes[d].y);
    g = we_find_id(&g_doc, "g");
    e = we_find_id(&g_doc, "e");
    f = we_find_id(&g_doc, "f");
    h = we_find_id(&g_doc, "h");
    TEST_ASSERT_EQUAL(WE_DISP_GRID, g_doc.nodes[g].display);
    TEST_ASSERT_EQUAL(2, g_doc.nodes[g].grid_cols);
    cell = (90 - 8) / 2;
    TEST_ASSERT_EQUAL(0, g_doc.nodes[e].x);
    TEST_ASSERT_EQUAL(cell + 8, g_doc.nodes[f].x);
    TEST_ASSERT_EQUAL(g_doc.nodes[e].y, g_doc.nodes[f].y);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[h].x);
    TEST_ASSERT_EQUAL(g_doc.nodes[e].y + 10 + 8, g_doc.nodes[h].y);
}

#include "webeng_lot52.inc"

int main(void) {
    unity_init();
    RUN_TEST(test_parse_tree_and_title);
    RUN_TEST(test_cascade);
    RUN_TEST(test_layout_and_display_list);
    RUN_TEST(test_wrapping_and_borders);
    RUN_TEST(test_percent_width_and_img_alt);
    RUN_TEST(test_inspector_and_diagnostics);
    RUN_TEST(test_limits);
    RUN_TEST(test_link_at);
    RUN_TEST(test_flex_and_grid);
    RUN_TEST(test_l52_flex_grow_zero_keeps_basis);
    RUN_TEST(test_l52_flex_weighted_grow);
    RUN_TEST(test_l52_flex_wrap_crosses_container);
    RUN_TEST(test_l52_flex_wrap_exact_fit);
    RUN_TEST(test_l52_wrap_then_grow_per_line);
    RUN_TEST(test_l52_flex_all_zero_leaves_free_space);
    RUN_TEST(test_l52_flex_fractional_grow_refused);
    RUN_TEST(test_l52_flex_grow_overflow_refused);
    RUN_TEST(test_l52_grid_explicit_rows);
    RUN_TEST(test_l52_grid_fraction_rows_definite_height);
    RUN_TEST(test_l52_grid_mixed_rows);
    RUN_TEST(test_l52_grid_rejects_nine_columns);
    RUN_TEST(test_l52_grid_rejects_nine_rows);
    RUN_TEST(test_l52_float_left_text_beside);
    RUN_TEST(test_l52_float_right_text_beside);
    RUN_TEST(test_l52_float_expires_and_text_uses_full_width);
    RUN_TEST(test_l52_float_full_width_waits_for_bottom);
    RUN_TEST(test_l52_float_margins_do_not_collapse);
    RUN_TEST(test_l52_table_rows_and_columns);
    RUN_TEST(test_l52_table_column_widths_shared_across_rows);
    RUN_TEST(test_l52_table_spans_are_refused);
    RUN_TEST(test_l52_table_oversized_columns_are_bounded);
    RUN_TEST(test_l52_image_initrd_pixels);
    RUN_TEST(test_l52_image_too_complex_is_atomic);
    RUN_TEST(test_l52_image_network_path_never_read);
    RUN_TEST(test_l52_image_large_source_refused);
    RUN_TEST(test_l52_image_bad_decode_keeps_alt);
    RUN_TEST(test_l52_min_width_expands_content_box);
    RUN_TEST(test_l52_max_width_limits_auto_box);
    RUN_TEST(test_l52_min_beats_conflicting_max);
    RUN_TEST(test_l52_relative_moves_paint_and_descendants);
    RUN_TEST(test_l52_relative_clamps_to_containing_box);
    RUN_TEST(test_l52_disc_list_marker);
    RUN_TEST(test_l52_decimal_list_marker);
    RUN_TEST(test_l52_legacy_auto_equal_columns);
    RUN_TEST(test_l52_legacy_column_with_margins);
    RUN_TEST(test_l52_column_grow_keeps_zero_fixed);
    RUN_TEST(test_l52_column_wrap_moves_next_column);
    RUN_TEST(test_l52_column_wrap_requires_definite_height);
    RUN_TEST(test_l52_column_wrap_reports_horizontal_limit);
    RUN_TEST(test_l52_unset_grow_does_not_steal_explicit_grow);
    RUN_TEST(test_l52_flex_hidden_item_has_no_gap);
    RUN_TEST(test_l52_flex_gap_zero_overrides_rule);
    RUN_TEST(test_l52_flex_inline_zero_wins_cascade);
    RUN_TEST(test_l52_grow_auto_basis_is_zero);
    RUN_TEST(test_l52_grow_percent_basis_uses_container);
    RUN_TEST(test_l52_grow_accounts_for_box_extras);
    RUN_TEST(test_l52_grow_cap_redistributes_unused_share);
    RUN_TEST(test_l52_all_grow_caps_leave_space);
    RUN_TEST(test_l52_grow_rounding_is_deterministic);
    RUN_TEST(test_l52_negative_free_space_does_not_shrink);
    RUN_TEST(test_l52_nested_flex_keeps_outer_workspace);
    RUN_TEST(test_l52_wrap_uses_tallest_line_item);
    RUN_TEST(test_l52_wrap_oversized_first_item_terminates);
    RUN_TEST(test_l52_wrap_accounts_for_margins);
    RUN_TEST(test_l52_grid_eight_columns_and_rows);
    RUN_TEST(test_l52_grid_auto_fraction_rows_use_content);
    RUN_TEST(test_l52_grid_implicit_rows_after_template);
    RUN_TEST(test_l52_grid_empty_explicit_rows_keep_space);
    RUN_TEST(test_l52_grid_zero_pixel_row_is_not_auto);
    RUN_TEST(test_l52_grid_fixed_rows_larger_than_height);
    RUN_TEST(test_l52_grid_rejects_zero_tracks);
    RUN_TEST(test_l52_grid_malformed_repeat_does_not_replace_valid);
    RUN_TEST(test_l52_grid_rejects_fraction_weight_other_than_one);
    RUN_TEST(test_l52_grid_rejects_fixed_column_syntax);
    RUN_TEST(test_l52_grid_track_integer_overflow_refused);
    RUN_TEST(test_l52_grid_remainder_stays_inside_container);
    RUN_TEST(test_l52_grid_padding_defines_track_origin);
    RUN_TEST(test_l52_grid_hidden_item_does_not_take_cell);
    RUN_TEST(test_l52_grid_nested_tracks_keep_outer_indexes);
    RUN_TEST(test_l52_grid_repeat_accepts_spaces_and_case);
    RUN_TEST(test_l52_grid_row_cascade_uses_inline_value);
    RUN_TEST(test_l52_grid_layout_twice_resets_counters);
    RUN_TEST(test_l52_two_opposing_floats_leave_middle);
    RUN_TEST(test_l52_two_left_floats_share_line);
    RUN_TEST(test_l52_two_right_floats_share_line);
    RUN_TEST(test_l52_second_float_waits_for_first_bottom);
    RUN_TEST(test_l52_float_scope_does_not_leak_to_next_block);
    RUN_TEST(test_l52_float_inside_inline_wrapper);
    RUN_TEST(test_l52_hidden_float_does_not_exclude_text);
    RUN_TEST(test_l52_oversized_float_is_bounded);
    RUN_TEST(test_l52_float_center_aligns_in_remaining_space);
    RUN_TEST(test_l52_float_right_aligns_in_remaining_space);
    RUN_TEST(test_l52_float_link_hit_uses_excluded_position);
    RUN_TEST(test_l52_float_limit_reports_refusal);
    RUN_TEST(test_l52_table_sections_share_columns);
    RUN_TEST(test_l52_table_implied_cell_and_row_end);
    RUN_TEST(test_l52_table_tall_cell_sets_next_row);
    RUN_TEST(test_l52_table_row_background_behind_cells);
    RUN_TEST(test_l52_table_span_one_is_supported);
    RUN_TEST(test_l52_table_rowspan_two_refuses_whole_table);
    RUN_TEST(test_l52_table_nine_columns_refused);
    RUN_TEST(test_l52_table_eight_columns_are_inside);
    RUN_TEST(test_l52_table_missing_cell_leaves_empty_column);
    RUN_TEST(test_l52_table_nested_inside_cell);
    RUN_TEST(test_l52_table_percent_columns_use_table_width);
    RUN_TEST(test_l52_table_text_wrap_changes_row_height);
    RUN_TEST(test_l52_image_no_reader_keeps_alt);
    RUN_TEST(test_l52_image_read_failure_keeps_alt);
    RUN_TEST(test_l52_image_parent_traversal_never_read);
    RUN_TEST(test_l52_image_encoded_traversal_never_read);
    RUN_TEST(test_l52_image_protocol_relative_never_read);
    RUN_TEST(test_l52_image_relative_path_diagnosed);
    RUN_TEST(test_l52_hidden_image_does_not_call_decoder);
    RUN_TEST(test_l52_image_output_cap_diagnosed);
    RUN_TEST(test_l52_image_zero_height_diagnosed);
    RUN_TEST(test_l52_image_scaled_jpeg_uses_shared_decoder);
    RUN_TEST(test_l52_progressive_image_is_not_second_jpeg_engine);
    RUN_TEST(test_l52_two_images_have_independent_paint);
    RUN_TEST(test_l52_image_in_table_cell_uses_cell_origin);
    RUN_TEST(test_l52_image_block_natural_width);
    RUN_TEST(test_l52_image_budget_exhaustion_does_not_corrupt_prior_items);
    RUN_TEST(test_l52_relayout_image_resets_geometry_and_counters);
    RUN_TEST(test_l52_min_width_percentage_uses_parent_content);
    RUN_TEST(test_l52_max_width_percentage_limits_auto);
    RUN_TEST(test_l52_negative_min_width_refused);
    RUN_TEST(test_l52_negative_percentage_constraint_refused);
    RUN_TEST(test_l52_width_constraints_reset_to_none);
    RUN_TEST(test_l52_grow_respects_min_width_before_distribution);
    RUN_TEST(test_l52_relative_negative_offsets_clamp_at_origin);
    RUN_TEST(test_l52_relative_keeps_normal_flow_height);
    RUN_TEST(test_l52_relative_auto_height_does_not_escape_unknown_bottom);
    RUN_TEST(test_l52_relative_link_hit_moves_with_paint);
    RUN_TEST(test_l52_relative_position_static_overrides_rule);
    RUN_TEST(test_l52_absolute_position_is_diagnosed);
    RUN_TEST(test_l52_list_none_does_not_paint_marker);
    RUN_TEST(test_l52_list_square_marker_inherits_color);
    RUN_TEST(test_l52_hidden_list_item_does_not_advance_number);
    RUN_TEST(test_l52_nested_list_counter_is_local);
    RUN_TEST(test_l52_list_counter_reaches_two_digits);
    RUN_TEST(test_l52_image_source_dimensions_over_cap);
    RUN_TEST(test_l52_image_max_source_size_is_accepted);
    RUN_TEST(test_l52_image_max_dimensions_can_be_one_rectangle);
    RUN_TEST(test_l52_table_all_fixed_widths_share_remaining_space);
    RUN_TEST(test_l52_table_rounding_does_not_expand_fixed_column);
    RUN_TEST(test_l52_layout_clamps_viewport_before_narrowing);
    RUN_TEST(test_l52_deep_padding_never_wraps_coordinates);
    RUN_TEST(test_l52_grow_percentage_cap_redistributes);
    RUN_TEST(test_l52_grow_minimum_wins_lower_maximum);
    RUN_TEST(test_l52_largest_grow_coefficient_stays_bounded);
    RUN_TEST(test_l52_wrap_origin_includes_parent_border_and_padding);
    RUN_TEST(test_l52_empty_flex_has_no_phantom_line);
    RUN_TEST(test_l52_grid_max_width_changes_both_columns);
    RUN_TEST(test_l52_grid_ninth_pixel_track_refuses_whole_declaration);
    RUN_TEST(test_l52_float_auto_width_uses_bounded_half_container);
    RUN_TEST(test_l52_float_max_width_constrains_exclusion);
    RUN_TEST(test_l52_float_none_overrides_stylesheet);
    RUN_TEST(test_l52_float_br_uses_exclusion_on_next_line);
    RUN_TEST(test_l52_float_after_queued_words_starts_after_line);
    RUN_TEST(test_l52_nested_float_context_uses_available_box);
    RUN_TEST(test_l52_table_minimum_request_is_shared_by_column);
    RUN_TEST(test_l52_table_padding_sets_cell_origin);
    RUN_TEST(test_l52_css_table_display_uses_same_layout);
    RUN_TEST(test_l52_hidden_table_row_is_not_painted);
    RUN_TEST(test_l52_hidden_table_cell_does_not_take_column);
    RUN_TEST(test_l52_empty_table_row_explicit_height_is_kept);
    RUN_TEST(test_l52_zero_colspan_is_refused_not_treated_as_one);
    RUN_TEST(test_l52_non_numeric_colspan_is_diagnosed);
    RUN_TEST(test_l52_header_font_weight_can_be_overridden);
    RUN_TEST(test_l52_link_in_table_uses_row_and_column_geometry);
    RUN_TEST(test_l52_image_missing_src_uses_default_alt);
    RUN_TEST(test_l52_image_in_float_shares_word_exclusion);
    RUN_TEST(test_l52_relative_container_moves_image_pixels);
    RUN_TEST(test_l52_second_image_failure_keeps_first_image);
    RUN_TEST(test_l52_image_anchor_hit_uses_image_box);
    RUN_TEST(test_l52_image_reader_context_survives_parse_reset);
    RUN_TEST(test_l52_relative_nested_offsets_compose_once);
    RUN_TEST(test_l52_relative_oversized_box_does_not_shift_to_negative_x);
    RUN_TEST(test_l52_relative_border_is_translated_with_background);
    RUN_TEST(test_l52_relative_relayout_does_not_accumulate_offsets);
    RUN_TEST(test_l52_adjacent_ordered_lists_restart_number);
    RUN_TEST(test_l52_single_list_item_can_override_inherited_marker);
    RUN_TEST(test_l52_non_li_child_does_not_increment_list_number);
    RUN_TEST(test_l52_multiline_list_item_has_one_marker);
    RUN_TEST(test_l52_marker_display_budget_remains_bounded);
    RUN_TEST(test_l52_long_inline_style_keeps_flex_properties);
    RUN_TEST(test_l52_overlong_src_is_not_read_as_a_prefix);
    RUN_TEST(test_l52_overlong_style_is_refused_atomically);
    RUN_TEST(test_l52_multiple_full_css_blocks_stay_in_css_buffer);
    RUN_TEST(test_l52_float_percent_width_uses_parent_before_exclusion);
    RUN_TEST(test_l52_float_zero_height_does_not_exclude_words);
    RUN_TEST(test_l52_relative_table_moves_rows_and_cells_together);
    RUN_TEST(test_l52_flex_in_table_cell_keeps_column_bounds);
    RUN_TEST(test_l52_grid_in_wrapped_flex_keeps_row_offsets);
    RUN_TEST(test_l52_list_in_table_has_one_marker_per_cell_item);
    RUN_TEST(test_l52_relayout_float_does_not_keep_old_exclusions);
    RUN_TEST(test_l52_relayout_table_recomputes_column_widths);
    RUN_TEST(test_l52_relayout_wrap_recomputes_line_partition);
    RUN_TEST(test_l52_relative_list_moves_marker_and_words);
    RUN_TEST(test_l52_empty_table_rows_keep_definite_heights);
    RUN_TEST(test_l52_empty_table_has_no_phantom_row);
    RUN_TEST(test_l52_relative_later_child_clamps_to_parent_top);
    RUN_TEST(test_l52_column_grow_auto_basis_is_zero);
    RUN_TEST(test_l52_overlong_word_clears_float_before_overflow);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
