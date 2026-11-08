/* Lot 82: the per-document pool of min()/max()/clamp() track expressions. */
static void l82_pool_page(char *html, int size, int grids) {
    int k, at = 0;
    at += snprintf(html + at, size - at, "<body style='margin:0'>");
    for (k = 0; k < grids; k++)
        at += snprintf(html + at, size - at, "<div style='display:grid;width:200px;grid-template-columns:min(50%%, %dpx) max(10%%, 30px) 1fr'>"
                       "<div id=a%d style='height:2px'></div><div id=b%d style='height:2px'></div></div>", 20 + k, k, k);
    snprintf(html + at, size - at, "</body>");
}

static char l82_html[16384];

static void test_l82_pool_sixteen_grids_fit(void) {
    char id[8];
    l82_pool_page(l82_html, sizeof l82_html, 16);
    lot52_render(l82_html, 480);
    snprintf(id, sizeof id, "a15");
    TEST_ASSERT_EQUAL(35, lot52_node(id)->w);
    snprintf(id, sizeof id, "b15");
    TEST_ASSERT_EQUAL(35, lot52_node(id)->x);
    TEST_ASSERT_EQUAL(30, lot52_node(id)->w);
    TEST_ASSERT_EQUAL(32, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l82_pool_full_refuses_the_next_track_list(void) {
    l82_pool_page(l82_html, sizeof l82_html, 17);
    lot52_render(l82_html, 480);
    TEST_ASSERT_EQUAL(35, lot52_node("a15")->w);
    TEST_ASSERT_EQUAL(200, lot52_node("a16")->w);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l82_pool_restyle_reuses_the_pool(void) {
    int pass;
    l82_pool_page(l82_html, sizeof l82_html, 12);
    lot52_render(l82_html, 480);
    for (pass = 0; pass < 3; pass++) {
        TEST_ASSERT_EQUAL(0, we_style(&g_doc));
        TEST_ASSERT_TRUE(we_layout(&g_doc, 480) >= 0);
    }
    TEST_ASSERT_EQUAL(24, g_doc.ntrack_pool);
    TEST_ASSERT_EQUAL(31, lot52_node("a11")->w);
    TEST_ASSERT_EQUAL(31, lot52_node("b11")->x);
}

static void test_l82_pool_pooled_track_is_auto_without_a_base(void) {
    /* an indefinite row base: the pooled track sizes to its content */
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:40px;grid-template-columns:40px;grid-template-rows:max(50%, 3px) 2px'>"
                 "<div id=r0 style='height:9px'></div><div id=r1></div></div></body>", 480);
    TEST_ASSERT_EQUAL(9, lot52_node("r0")->h);
    TEST_ASSERT_EQUAL(9, lot52_node("r1")->y);
    TEST_ASSERT_EQUAL(11, lot52_node("g")->h);
}

static void test_l82_pool_subgrid_padding_moves_shared_baseline(void) {
    /* the row subgrid's padding-top is part of its edge item's baseline
     * contribution, so the parent's sibling aligns 10px down */
    lot52_render("<body style='margin:0'><div id=p style='display:grid;width:100px;grid-template-columns:50px 50px;grid-template-rows:auto auto'>"
                 "<div id=s style='display:grid;grid-column:1;grid-row:1 / 3;grid-template-rows:subgrid;padding-top:10px'>"
                 "<div id=a style='align-self:baseline'>A</div><div id=a2>B</div></div>"
                 "<div id=x style='grid-column:2;grid-row:1;align-self:baseline'>X</div></div></body>", 480);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("x")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("s")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void lot82_extra_run(void) {
    RUN_TEST(test_l82_pool_subgrid_padding_moves_shared_baseline);
    RUN_TEST(test_l82_pool_sixteen_grids_fit);
    RUN_TEST(test_l82_pool_full_refuses_the_next_track_list);
    RUN_TEST(test_l82_pool_restyle_reuses_the_pool);
    RUN_TEST(test_l82_pool_pooled_track_is_auto_without_a_base);
}
