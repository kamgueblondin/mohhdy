/* Cross-feature scenes verify the shared sizing, paint and hit-test paths. */
static void test_l55_scene_intrinsic_grid_fraction(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:max-content;grid-template-columns:20px 1fr'>"
        "<div id=a>A</div><div id=b>three</div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->min_content);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->max_content);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_intrinsic_grid_minimum_auto_track(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:min-content;grid-template-columns:auto 20px'>"
        "<div id=a>one seven</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->min_content);
    TEST_ASSERT_EQUAL(92, lot52_node("g")->max_content);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_auto_tracks_shrink_to_word_floors(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:100px;grid-template-columns:auto auto'>"
        "<div id=a>one seven</div><div id=b>two seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_auto_floor_overflow_diagnosed(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:60px;grid-template-columns:auto auto'>"
        "<div id=a>seven</div><div id=b>seven</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x + lot52_node("b")->w);
    TEST_ASSERT_TRUE(lot52_has_diag("grid width overflow"));
}

static void test_l55_scene_intrinsic_table_maximum(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:max-content'><tr>"
        "<td id=a>one seven</td><td id=b>two</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(96, lot52_node("t")->w);
    TEST_ASSERT_EQUAL(64, lot52_node("t")->min_content);
    TEST_ASSERT_EQUAL(96, lot52_node("t")->max_content);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(72, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_intrinsic_table_minimum(void) {
    lot52_render("<body style='margin:0'><table id=t style='width:min-content'><tr>"
        "<td id=a>one seven</td><td id=b>two</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(64, lot52_node("t")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(20, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(11, find_text("seven")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_content_basis_inside_flex(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;width:200px;align-items:center'>"
        "<div id=g style='display:grid;flex-basis:content;grid-template-columns:20px 1fr'>"
        "<div id=a>A</div><div id=b>three</div></div>"
        "<div id=c style='flex-basis:content'>two</div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("g")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(1, g_doc.perf.grid_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_table_auto_basis_inside_column(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:100px'>"
        "<table id=t style='flex-basis:auto'><tr><td>A</td><td>B</td></tr>"
        "<tr><td id=a colspan=2 style='height:20px'>C</td></tr></table>"
        "<div id=b style='flex-basis:auto'>tail</div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("t")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(1, g_doc.perf.flex_lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_float_intrinsic_height_in_centered_row(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;width:140px;align-items:center'>"
        "<div id=a style='flex-basis:100px'><div id=f style='float:left;width:20px;height:20px;background:red'></div>word</div>"
        "<div id=b style='flex-basis:40px;height:30px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("f")->y);
    TEST_ASSERT_EQUAL(20, find_text("word")->x);
    TEST_ASSERT_EQUAL(6, find_text("word")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_image_auto_height_in_column(void) {
    int painted;
    lot52_image("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:60px'>"
        "<div id=a style='flex-basis:auto'><img id=im src='/images/mire-g1.png' alt='fallback'></div>"
        "<div id=b style='flex-basis:auto'>tail</div></div></body>");
    TEST_ASSERT_EQUAL(32, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(0xffffff, lot52_pixel(47, 31, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_two_images_share_one_decode_workspace(void) {
    int painted;
    lot52_image("<body style='margin:0'><div style='display:flex;align-items:center;width:40px'>"
        "<div id=a style='flex-basis:20px'><img id=x src='/images/mire-g1.png' style='width:8px;height:4px'></div>"
        "<div id=b style='flex-basis:20px'><img id=y src='/images/mire-g2-i.png' style='width:8px;height:4px'></div>"
        "</div></body>");
    TEST_ASSERT_EQUAL(8, lot52_node("x")->w);
    TEST_ASSERT_EQUAL(4, lot52_node("x")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("y")->x);
    TEST_ASSERT_EQUAL(8, lot52_node("y")->w);
    TEST_ASSERT_EQUAL(4, lot52_node("y")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_TRUE(lot52_read_calls >= 2 && lot52_read_calls <= 4);
    lot52_pixel(20, 0, &painted); TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_failed_image_measurement_keeps_alt(void) {
    lot52_image("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:100px'>"
        "<div id=a style='flex-basis:auto'><img src='/images/missing.png' alt='missing'></div>"
        "<div id=b style='flex-basis:auto'>tail</div></div></body>");
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("[missing]"));
    TEST_ASSERT_EQUAL(1, lot52_read_calls);
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_TRUE(lot52_has_diag("image read failed"));
}

static void test_l55_scene_percentage_height_uses_grid_area(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-rows:50px 50px'>"
        "<div id=a style='height:50%'>A</div><div id=b style='height:100%'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->height_pct);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_area_percentage_minimum_and_alignment(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-rows:50px 50px;align-items:end'>"
        "<div id=a style='height:10px;min-height:50%'>A</div><div id=b style='height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(26, find_text("A")->y);
    TEST_ASSERT_EQUAL(91, find_text("B")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_span_includes_distributed_column_space(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-columns:20px 20px;justify-content:space-between'>"
        "<div id=a style='grid-column:1 / span 2;height:10px'>A</div>"
        "<div id=b style='grid-column:1;grid-row:2;height:10px'>B</div>"
        "<div id=c style='grid-column:2;grid-row:2;height:10px'>C</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_span_includes_distributed_row_space(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-columns:50px 50px;grid-template-rows:20px 20px;align-content:space-between;align-items:stretch'>"
        "<div id=a style='grid-column:1;grid-row:1 / span 2'>A</div>"
        "<div id=b style='grid-column:2;grid-row:1'>B</div>"
        "<div id=c style='grid-column:2;grid-row:2'>C</div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_fixed_rows_do_not_stretch(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-rows:10px 20px;align-content:stretch;align-items:stretch'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(11, find_text("B")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_only_auto_rows_stretch(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;height:100px;grid-template-rows:auto 20px;align-content:stretch;align-items:stretch'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, find_text("A")->y);
    TEST_ASSERT_EQUAL(81, find_text("B")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_clipped_centered_link(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='display:grid;width:80px;grid-template-columns:40px 40px;grid-template-rows:20px;align-items:center'>"
        "<div id=a style='height:10px;overflow:hidden;white-space:nowrap'><a href='/next'>abcdefgh</a></div>"
        "<div id=b style='height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_NOT_NULL(find_text("abcde"));
    TEST_ASSERT_EQUAL(6, find_text("abcde")->y);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 32, 8, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 42, 8, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_percentage_height_and_border_box_grid(void) {
    lot52_render("<body style='margin:0'><div style='height:200px'>"
        "<div id=g style='display:grid;height:50%;box-sizing:border-box;padding:5px;grid-template-rows:1fr 1fr;align-items:stretch'>"
        "<div id=a>A</div><div id=b>B</div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(45, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("g")->height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_relative_link_in_assigned_flex_height(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div style='display:flex;width:100px;height:60px;align-items:stretch'>"
        "<div id=a style='flex-basis:100px'><div id=b style='position:relative;top:20px;width:40px;height:10px'>"
        "<a href='/next'>one</a></div></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(21, find_text("one")->y);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 8, 24, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_list_markers_follow_grid_alignment(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:100px;grid-template-rows:40px;align-items:end'>"
        "<ul id=u style='margin:0;padding:0'><li id=a style='height:10px'>one</li></ul></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("u")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(31, find_text("one")->y);
    TEST_ASSERT_NOT_NULL(find_text("*"));
    TEST_ASSERT_EQUAL(31, find_text("*")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_two_documents_keep_independent_hit_owners(void) {
    static we_doc_t first;
    const char *a = "<body style='margin:0'><div style='width:24px;height:10px;overflow:hidden'>"
                    "<a href='/first'>one two</a></div></body>";
    const char *b = "<body style='margin:0'><div><div><div><a href='/second'>second</a></div></div></div></body>";
    char href[32];
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(0, we_render(&first, a, (int)strlen(a), 100));
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, b, (int)strlen(b), 100));
    TEST_ASSERT_EQUAL(6, we_link_at(&first, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/first"));
    TEST_ASSERT_EQUAL(7, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/second"));
    TEST_ASSERT_EQUAL(0, we_layout(&first, 200));
    TEST_ASSERT_EQUAL(7, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/second"));
}

static void test_l55_scene_restyle_does_not_grow_css_pool(void) {
    unsigned i, pool, rules;
    lot52_render("<style>.x{display:grid;grid-template-columns:20px 1fr;grid-template-rows:40px;height:40px;align-items:end}</style>"
        "<body style='margin:0'><div class=x><div id=a>A</div><div id=b>B</div></div></body>", 100);
    pool = g_doc.pool_used;
    rules = g_doc.nrules;
    for (i = 0; i < 30; i++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 100));
        TEST_ASSERT_EQUAL(pool, g_doc.pool_used);
        TEST_ASSERT_EQUAL(rules, g_doc.nrules);
        TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
        TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
        TEST_ASSERT_EQUAL(30, lot52_node("a")->y);
    }
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_repeated_documents_reset_all_layout_flags(void) {
    unsigned i;
    const char *complex = "<body style='margin:0'><div id=a style='display:grid;width:100px;height:40px;"
        "grid-template-columns:20px 1fr;align-items:end;overflow:hidden;visibility:hidden'>"
        "<div id=b style='height:50%;box-sizing:border-box;padding:2px'>hidden</div></div></body>";
    const char *plain = "<body style='margin:0'><div id=a><div id=b>shown</div></div></body>";
    for (i = 0; i < 12; i++) {
        lot52_render(complex, 100);
        TEST_ASSERT_EQUAL(0, g_doc.nitems);
        TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
        lot52_render(plain, 100);
        TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
        TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
        TEST_ASSERT_EQUAL(0, lot52_node("b")->height_percent);
        TEST_ASSERT_EQUAL(0, lot52_node("b")->border_box);
        TEST_ASSERT_EQUAL(0, lot52_node("b")->visibility);
        TEST_ASSERT_NOT_NULL(find_text("shown"));
        TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    }
}

static void test_l55_scene_deep_measurement_reuses_cache(void) {
    char html[4096];
    unsigned i;
    strcpy(html, "<style>.x{display:flex;flex-basis:auto}</style><body style='margin:0'>");
    for (i = 0; i < 16; i++) strcat(html, "<div class=x>");
    strcat(html, "<div id=leaf style='flex-basis:auto'>one</div>");
    for (i = 0; i < 16; i++) strcat(html, "</div>");
    strcat(html, "</body>");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(24, lot52_node("leaf")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("leaf")->h);
    TEST_ASSERT_EQUAL(10, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("one"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l55_scene_caller_document_canaries_survive_bounds(void) {
    static struct { uint32_t before; we_doc_t doc; uint32_t after; } guarded;
    char html[5000];
    unsigned i;
    guarded.before = 0x12345678U;
    guarded.after = 0x87654321U;
    strcpy(html, "<style>.x{grid-column:span 8;grid-row:span 8;height:1px}</style>"
        "<div style='display:grid;grid-template-columns:repeat(8,1fr)'>");
    for (i = 0; i < 80; i++) strcat(html, "<div class=x></div>");
    strcat(html, "</div>");
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(0, we_render(&guarded.doc, html, (int)strlen(html), 320));
    TEST_ASSERT_EQUAL(0x12345678U, guarded.before);
    TEST_ASSERT_EQUAL(0x87654321U, guarded.after);
    TEST_ASSERT_TRUE(guarded.doc.nnodes <= WE_MAX_NODES);
    TEST_ASSERT_TRUE(guarded.doc.nitems <= OS_FB_WEB_ITEMS);
    TEST_ASSERT_TRUE(guarded.doc.ndiag <= WE_MAX_DIAG);
    TEST_ASSERT_TRUE(guarded.doc.diag_dropped > 0);
}

static void test_l55_scene_largest_intrinsic_grid_spans_stay_bounded(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:max-content;grid-template-columns:repeat(8,auto)'>"
        "<div id=a style='grid-column:span 8;min-width:2000px;height:10px'>A</div>"
        "<div id=b style='grid-column:8;grid-row:2;height:10px'>B</div></div></body>", 320);
    TEST_ASSERT_TRUE(lot52_node("g")->w <= WE_LAYOUT_MAX);
    TEST_ASSERT_EQUAL(2000, lot52_node("a")->w);
    TEST_ASSERT_TRUE(lot52_node("b")->x >= 0);
    TEST_ASSERT_TRUE(lot52_node("b")->x <= WE_LAYOUT_MAX);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grid_col_span);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l55_scene_relayout_changes_grid_wrapping_and_clipped_hits(void) {
    char href[32];
    lot52_render("<body style='margin:0'><div id=g style='display:grid;grid-template-columns:20px 1fr'>"
        "<div style='height:10px'>A</div><div id=a style='height:10px;overflow:hidden;white-space:nowrap'>"
        "<a href='/next'>abcdefgh</a></div></div></body>", 64);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_NOT_NULL(find_text("abcde"));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 62, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 128));
    TEST_ASSERT_EQUAL(108, lot52_node("a")->w);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 62, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_border_box_table_inside_weighted_grid(void) {
    lot52_render("<body style='margin:0'><div style='display:grid;width:120px;grid-template-columns:1fr 2fr'>"
        "<div id=a style='height:20px'>A</div>"
        "<table id=t style='box-sizing:border-box;padding:2px;border:1px solid red'><tr>"
        "<td id=b colspan=2>B</td><td id=c>C</td></tr></table></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("t")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("t")->w);
    TEST_ASSERT_EQUAL(43, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(93, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(24, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.table_rows);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_grid_area_percentages_relayout_without_stale_cache(void) {
    int parent;
    lot52_render("<body style='margin:0'><div id=p style='height:100px'><div id=g style='display:grid;height:100%;grid-template-rows:1fr 1fr'>"
        "<div id=a style='height:50%'>A</div><div id=b style='height:100%'>B</div></div></div></body>", 100);
    TEST_ASSERT_EQUAL(25, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->h);
    parent = we_find_id(&g_doc, "p");
    TEST_ASSERT_TRUE(parent >= 0);
    g_doc.nodes[parent].height = 200;
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 100));
    TEST_ASSERT_EQUAL(50, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("g")->height_pct);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_percentage_float_measurement_matches_paint(void) {
    lot52_render("<body style='margin:0'><div id=r style='display:flex;width:140px;align-items:center'>"
        "<div id=a style='flex-basis:100px'><div id=f style='float:left;width:50%'>ab cd ef</div>end</div>"
        "<div id=b style='flex-basis:40px;height:40px'>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("r")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("f")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("f")->h);
    TEST_ASSERT_EQUAL(50, find_text("end")->x);
    TEST_ASSERT_EQUAL(11, find_text("end")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.floats);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_scene_image_output_failure_measures_alt_height(void) {
    lot52_image("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:100px'>"
        "<div id=a style='flex-basis:auto'><img src='/images/mire-g1.png' alt='large' style='height:65px'></div>"
        "<div id=b style='flex-basis:auto'>tail</div></div></body>");
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("[large]"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
}
