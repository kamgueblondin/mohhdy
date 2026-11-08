/* Lot 83: hand-written tests: the shared expression pool for spacing and
 * tracks, its capacity, fixed math clamps, masonry next wrap and intrinsic
 * corner cases. */
static char l83_html[32768];

static void l83_spacing_page(int nodes, int base) {
    int k, at = 0;
    at += snprintf(l83_html + at, sizeof l83_html - at, "<body style='margin:0'><div style='width:200px'>");
    for (k = 0; k < nodes; k++)
        at += snprintf(l83_html + at, sizeof l83_html - at, "<div id=n%d style='padding-left:min(50%%, %dpx);height:1px'></div>", k, base + k);
    snprintf(l83_html + at, sizeof l83_html - at, "</div></body>");
}

static void test_l83_pool_identical_padding_shares_one_slot(void) {
    int k, at = 0;
    at += snprintf(l83_html + at, sizeof l83_html - at, "<body style='margin:0'><div style='width:200px'>");
    for (k = 0; k < 40; k++)
        at += snprintf(l83_html + at, sizeof l83_html - at, "<div id=n%d style='margin-left:min(10%%, 15px);height:1px'></div>", k);
    snprintf(l83_html + at, sizeof l83_html - at, "</div></body>");
    lot52_render(l83_html, 480);
    TEST_ASSERT_EQUAL(1, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(15, lot52_node("n0")->x);
    TEST_ASSERT_EQUAL(15, lot52_node("n39")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_pool_distinct_padding_takes_slots(void) {
    l83_spacing_page(10, 30);
    lot52_render(l83_html, 480);
    TEST_ASSERT_EQUAL(10, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(200, lot52_node("n9")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_pool_track_and_padding_share_a_slot(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:200px;grid-template-columns:min(50%, 40px) 1fr'>"
                 "<div id=a style='padding-left:min(50%, 40px)'><div id=b style='height:2px'></div></div><div id=c style='height:2px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(1, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(40, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_pool_fixed_math_and_linear_calc_use_no_slot(void) {
    lot52_render("<body style='margin:0'><div style='width:200px'><div id=a style='padding-left:min(6px, 9px);margin-left:max(1px, 3px)'>"
                 "<div id=b style='height:2px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(3, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(9, lot52_node("b")->x);
}

static void test_l83_pool_full_refuses_the_next_padding(void) {
    int at;
    l83_spacing_page(64, 10);
    at = (int)strlen(l83_html) - (int)strlen("</div></body>");
    snprintf(l83_html + at, sizeof l83_html - at, "<div id=x style='padding-left:3px;padding-left:min(50%%, 99px)'><div id=y style='height:1px'></div></div></div></body>");
    lot52_render(l83_html, 480);
    TEST_ASSERT_EQUAL(64, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(73, lot52_node("n63")->w - 127);
    TEST_ASSERT_EQUAL(3, lot52_node("y")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("padding refused"));
}

static void test_l83_pool_full_still_shares_an_existing_entry(void) {
    int at;
    l83_spacing_page(64, 10);
    at = (int)strlen(l83_html) - (int)strlen("</div></body>");
    snprintf(l83_html + at, sizeof l83_html - at, "<div id=x style='margin-left:min(50%%, 12px);height:1px'></div></div></body>");
    lot52_render(l83_html, 480);
    TEST_ASSERT_EQUAL(64, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(12, lot52_node("x")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_pool_full_refuses_the_next_margin(void) {
    int at;
    l83_spacing_page(64, 10);
    at = (int)strlen(l83_html) - (int)strlen("</div></body>");
    snprintf(l83_html + at, sizeof l83_html - at, "<div id=x style='margin-left:5px;margin-left:max(50%%, 1px);height:1px'></div></div></body>");
    lot52_render(l83_html, 480);
    TEST_ASSERT_EQUAL(5, lot52_node("x")->x);
    TEST_ASSERT_TRUE(g_doc.ndiag > 0);
}

static void test_l83_pool_restyle_does_not_grow(void) {
    int pass;
    l83_spacing_page(20, 30);
    lot52_render(l83_html, 480);
    for (pass = 0; pass < 4; pass++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_TRUE(we_layout(&g_doc, 480) >= 0);
    }
    TEST_ASSERT_EQUAL(20, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(49, lot52_node("n19")->w - 151);
}

static void test_l83_pool_relayout_new_width(void) {
    lot52_render("<body style='margin:0'><div id=a style='padding-left:min(10%, 30px)'><div id=b style='height:1px'></div></div></body>", 200);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_TRUE(we_layout(&g_doc, 400) >= 0);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_TRUE(we_layout(&g_doc, 100) >= 0);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->x);
}

static void test_l83_fixed_math_padding_negative_is_zero(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='padding-left:min(-4px, 2px)'><div id=b style='height:1px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_fixed_math_margin_negative_kept(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='margin-left:min(-4px, 2px);height:1px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(-4, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(104, lot52_node("a")->w);
}

static void test_l83_fixed_math_margin_clamped_to_range(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='margin-top:calc(1500px + 1500px);height:1px'></div><div id=b style='margin-top:calc(-1500px - 1500px);height:1px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(2000, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(1, lot52_node("b")->y);
}

static void test_l83_pct_padding_negative_math_is_zero(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='padding-left:calc(10% - 30px)'><div id=b style='height:1px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
}

static void test_l83_plain_negative_padding_refused(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='padding-left:6px;padding-left:-3px'><div id=b style='height:1px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(6, lot52_node("b")->x);
    TEST_ASSERT_TRUE(lot52_has_diag("padding refused"));
}

static void test_l83_math_with_fr_refused_everywhere(void) {
    lot52_render("<body style='margin:0'><div style='width:100px'><div id=a style='margin-right:min(10%, 1fr);height:1px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_TRUE(lot52_has_diag("math length refused"));
}

static void test_l83_masonry_next_wraps_a_span(void) {
    /* three rows, next: span 2 at cursor 2 does not fit and wraps to row 1 */
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:200px;grid-template-columns:masonry;masonry-auto-flow:next;grid-template-rows:5px 5px 5px'>"
                 "<div id=c0 style='width:10px;height:5px'></div><div id=c1 style='width:10px;height:5px'></div><div id=c2 style='width:30px;height:5px;grid-row:span 2'></div><div id=c3 style='width:7px;height:5px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(0, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("c1")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c2")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c3")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c3")->y);
}

static void test_l83_masonry_next_after_explicit_start(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:200px;grid-template-columns:masonry;masonry-auto-flow:next;grid-template-rows:5px 5px 5px'>"
                 "<div id=c0 style='width:10px;height:5px;grid-row:2'></div><div id=c1 style='width:20px;height:5px'></div><div id=c2 style='width:4px;height:5px'></div></div></body>", 480);
    /* c0 takes row 2, so next continues at row 3, then wraps to row 1 */
    TEST_ASSERT_EQUAL(5, lot52_node("c0")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("c1")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c2")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("c2")->x);
}

static void test_l83_masonry_next_restyle_back_to_pack(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:200px;grid-template-columns:masonry;masonry-auto-flow:next;grid-template-rows:5px 5px'>"
                 "<div id=c0 style='width:50px;height:5px'></div><div id=c1 style='width:5px;height:5px'></div><div id=c2 style='width:5px;height:5px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(1, lot52_node("g")->masonry_next);
}

static void test_l83_masonry_pack_ignores_cursor(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:200px;grid-template-columns:masonry;grid-template-rows:5px 5px'>"
                 "<div id=c0 style='width:50px;height:5px'></div><div id=c1 style='width:5px;height:5px'></div><div id=c2 style='width:5px;height:5px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(5, lot52_node("c2")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("c2")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("g")->masonry_next);
}

static void test_l83_masonry_flow_on_non_masonry_grid_is_inert(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:50px 50px;masonry-auto-flow:next'>"
                 "<div id=a style='height:5px'></div><div id=b style='height:5px'></div><div id=c style='height:5px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(5, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_masonry_intrinsic_no_items(void) {
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=g style='float:left;display:grid;grid-template-columns:masonry;grid-template-rows:auto auto'></div></div></body>", 480);
    /* the empty float keeps the engine's empty-float fallback width */
    TEST_ASSERT_EQUAL(8, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_masonry_intrinsic_no_row_tracks(void) {
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=g style='float:left;display:grid;grid-template-columns:masonry'>"
                 "<div id=c0 style='width:30px;height:2px'></div><div id=c1 style='width:20px;height:2px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
}

static void test_l83_masonry_intrinsic_span_covers_rows(void) {
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=g style='float:left;display:grid;grid-template-columns:masonry;grid-template-rows:auto auto'>"
                 "<div id=c0 style='width:30px;height:2px'></div><div id=c1 style='width:20px;height:2px;grid-row:span 2'></div><div id=c2 style='width:5px;height:2px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("c1")->x);
    TEST_ASSERT_EQUAL(55, lot52_node("g")->w);
}

static void test_l83_masonry_intrinsic_span_clamped_to_rows(void) {
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=g style='float:left;display:grid;grid-template-columns:masonry;grid-template-rows:auto auto'>"
                 "<div id=c0 style='width:30px;height:2px'></div><div id=c1 style='width:20px;height:2px;grid-row:span 5'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(50, lot52_node("g")->w);
}

static void test_l83_masonry_intrinsic_text_items(void) {
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=g style='float:left;display:grid;grid-template-columns:masonry;grid-template-rows:auto'>"
                 "<div id=c0>ab</div><div id=c1>cd</div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(lot52_node("c0")->w + lot52_node("c1")->w, lot52_node("g")->w);
    TEST_ASSERT_TRUE(lot52_node("c0")->w > 0);
}

static void test_l83_masonry_rows_intrinsic_unchanged(void) {
    /* masonry rows (lot 81) keep the track-based intrinsic width */
    lot52_render("<body style='margin:0'><div style='width:300px'><div id=g style='float:left;display:grid;grid-template-columns:30px 40px;grid-template-rows:masonry'>"
                 "<div id=c0 style='height:2px'></div></div></div></body>", 480);
    TEST_ASSERT_EQUAL(70, lot52_node("g")->w);
}

static void test_l83_subgrid_auto_margin_no_diag_restyle(void) {
    int pass;
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:50px 50px'><div id=s style='display:grid;grid-column:1 / 3;grid-template-columns:subgrid;margin:0 auto'>"
                 "<div id=a style='height:5px'></div><div id=b style='height:5px'></div></div></div></body>", 480);
    for (pass = 0; pass < 2; pass++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_TRUE(we_layout(&g_doc, 480) >= 0);
    }
    TEST_ASSERT_EQUAL(0, lot52_node("s")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l83_non_subgrid_auto_margin_still_centres(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:200px;grid-template-columns:100px 100px'><div id=s style='width:40px;margin:0 auto;height:2px'></div></div></body>", 480);
    TEST_ASSERT_EQUAL(30, lot52_node("s")->x);
}

static void test_l83_document_still_fits(void) {
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
    TEST_ASSERT_TRUE(sizeof(we_doc_t) < 1024u * 1024u);
    TEST_ASSERT_EQUAL(64, WE_TRACK_POOL_MAX);
}

static void lot83_extra_run(void) {
    RUN_TEST(test_l83_pool_identical_padding_shares_one_slot);
    RUN_TEST(test_l83_pool_distinct_padding_takes_slots);
    RUN_TEST(test_l83_pool_track_and_padding_share_a_slot);
    RUN_TEST(test_l83_pool_fixed_math_and_linear_calc_use_no_slot);
    RUN_TEST(test_l83_pool_full_refuses_the_next_padding);
    RUN_TEST(test_l83_pool_full_still_shares_an_existing_entry);
    RUN_TEST(test_l83_pool_full_refuses_the_next_margin);
    RUN_TEST(test_l83_pool_restyle_does_not_grow);
    RUN_TEST(test_l83_pool_relayout_new_width);
    RUN_TEST(test_l83_fixed_math_padding_negative_is_zero);
    RUN_TEST(test_l83_fixed_math_margin_negative_kept);
    RUN_TEST(test_l83_fixed_math_margin_clamped_to_range);
    RUN_TEST(test_l83_pct_padding_negative_math_is_zero);
    RUN_TEST(test_l83_plain_negative_padding_refused);
    RUN_TEST(test_l83_math_with_fr_refused_everywhere);
    RUN_TEST(test_l83_masonry_next_wraps_a_span);
    RUN_TEST(test_l83_masonry_next_after_explicit_start);
    RUN_TEST(test_l83_masonry_next_restyle_back_to_pack);
    RUN_TEST(test_l83_masonry_pack_ignores_cursor);
    RUN_TEST(test_l83_masonry_flow_on_non_masonry_grid_is_inert);
    RUN_TEST(test_l83_masonry_intrinsic_no_items);
    RUN_TEST(test_l83_masonry_intrinsic_no_row_tracks);
    RUN_TEST(test_l83_masonry_intrinsic_span_covers_rows);
    RUN_TEST(test_l83_masonry_intrinsic_span_clamped_to_rows);
    RUN_TEST(test_l83_masonry_intrinsic_text_items);
    RUN_TEST(test_l83_masonry_rows_intrinsic_unchanged);
    RUN_TEST(test_l83_subgrid_auto_margin_no_diag_restyle);
    RUN_TEST(test_l83_non_subgrid_auto_margin_still_centres);
    RUN_TEST(test_l83_document_still_fits);
}
