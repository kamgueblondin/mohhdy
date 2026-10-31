/* Lot 64 round 2: the dir attribute, background-position with edge
 * offsets (three and four values) and its -x / -y longhands, column-span
 * all and forced column breaks. */

static void test_l64_dir_attribute(void) {
    lot52_render(L64_BODY "<p id=a dir=rtl style='width:40px;text-align:start'>ab</p><p id=b dir=RTL style='direction:ltr'>cd</p>"
                 "<div dir=rtl style='width:40px'><p>ef</p><span dir=ltr>g</span></div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("ab", 24, 9);                                 /* start is the right in rtl */
    l61_text_at("cd", 0, 35);                                 /* the style wins over the hint */
    l61_text_at("ef", 24, 61);                                /* inherited */
    l61_text_at("g", 32, 79);                                 /* an inline keeps the line of its block */
    TEST_ASSERT_EQUAL(1, lot52_node("a")->dir_rtl);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->dir_rtl);
    lot52_render(L64_BODY "<div style='direction:rtl'><p id=a dir=auto>e</p><p id=b dir=up>f</p><p id=c dir=ltr>g</p></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("dir auto taken as ltr"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("dir attribute refused"));
    TEST_ASSERT_EQUAL(0, lot52_node("a")->dir_rtl);
    TEST_ASSERT_EQUAL(1, lot52_node("b")->dir_rtl);           /* refused: inherits */
    TEST_ASSERT_EQUAL(0, lot52_node("c")->dir_rtl);
}

#define L64_POS(decl) L63_BGI("width:20px;height:10px", "background-size:6px 4px;background-repeat:no-repeat;" decl)

/* the 6x4 tile has its top left corner at (x, y) */
static void l64_tile_at(int x, int y) {
    TEST_ASSERT_EQUAL((long)l63_ramp[0], l63_at(x, y));
    TEST_ASSERT_EQUAL((long)l63_ramp[5], l63_at(x + 5, y + 3));
    TEST_ASSERT_EQUAL(-1, l63_at(x - 1, y));
    TEST_ASSERT_EQUAL(-1, l63_at(x, y - 1));
    TEST_ASSERT_EQUAL(-1, l63_at(x + 6, y + 3));
    TEST_ASSERT_EQUAL(-1, l63_at(x + 5, y + 4));
}

static void test_l64_bgpos_edge_offsets(void) {
    l63_bgi(L64_POS("background-position:right 3px bottom 2px"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l64_tile_at(11, 4);
    l63_bgi(L64_POS("background-position:bottom 2px right 3px"));
    l64_tile_at(11, 4);
    l63_bgi(L64_POS("background-position:left 2px top 1px"));
    l64_tile_at(2, 1);
    l63_bgi(L64_POS("background-position:bottom 10% right"));  /* 90% of the free 6 rows */
    l64_tile_at(14, 5);
    l63_bgi(L64_POS("background-position:center top 2px"));
    l64_tile_at(7, 2);
    l63_bgi(L64_POS("background-position:right 25% center"));
    l64_tile_at(10, 3);
    l63_bgi(L63_BGI("width:20px;height:10px", "background:url(/images/mire-ga16.png) no-repeat left 4px bottom 1px / 6px 4px"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l64_tile_at(4, 5);
}

static void test_l64_bgpos_longhands(void) {
    l63_bgi(L64_POS("background-position:left 2px top 1px;background-position-x:right 1px"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l64_tile_at(13, 1);                                       /* y kept */
    l63_bgi(L64_POS("background-position:right 3px bottom 2px;background-position-x:10%;background-position-y:bottom 25%"));
    l64_tile_at(1, 4);                                        /* the far-edge flags drop */
    l63_bgi(L64_POS("background-position-y:5px;background-position-x:center"));
    l64_tile_at(7, 5);
    l63_bgi(L64_POS("background-position-x:left 1px, center"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l64_tile_at(1, 0);
}

static void test_l64_bgpos_refusals(void) {
    static const char *const bad[] = {
        "background-position:left 2px left 3px", "background-position:center 1px top", "background-position:left top 1px 2px",
        "background-position:left 1px 2px top", "background-position:right 2px top 1px bottom", "background-position:1px 2px 3px",
        "background-position:right 2px top 1px 4px", "background-position-x:top", "background-position-y:right 2px",
        "background-position-x:1px 2px", "background-position-y:bottom 1px 2px", "background-position-x:left right", 0 };
    char html[300];
    int k;
    for (k = 0; bad[k]; k++) {
        int n = snprintf(html, sizeof(html), "<body style='margin:0'><div style='width:20px;height:10px;background-image:url(/images/mire-ga16.png);"
                         "background-size:6px 4px;background-repeat:no-repeat;background-position:right 3px bottom 2px;%s'></div></body>", bad[k]);
        TEST_ASSERT_TRUE(n > 0 && n < (int)sizeof(html));
        l63_bgi(html);
        TEST_ASSERT_EQUAL(1, l61_diag_count("background position refused"));
        l64_tile_at(11, 4);                                   /* unchanged */
    }
}

static void test_l64_columns_span_all(void) {
    lot52_render(L64_BODY "<div id=m style='column-count:2;width:40px;column-rule:1px solid #ff0000'><div>a</div><div>b</div><div>c</div>"
                 "<div id=s style='column-span:all;background:#00ff00'>long span</div><div>d</div><div style='break-before:column'>e</div>"
                 "<div>f</div></div><p id=after style='margin:0'>z</p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("a", 0, 1);                                   /* first segment balanced: a b | c */
    l61_text_at("b", 0, 11);
    l61_text_at("c", 24, 1);
    l64_box("s", 0, 20, 40, 20);                              /* the spanner below, full width */
    l61_rect_at(0, 20, 40, 20, 0x00FF00);
    l61_text_at("long", 0, 21);
    l61_text_at("d", 0, 41);                                  /* next segment below the spanner */
    l61_text_at("e", 24, 41);                                 /* forced into the second column */
    l61_text_at("f", 24, 51);
    l61_rect_at(20, 0, 1, 20, 0xFF0000);                      /* one rule per segment */
    l61_rect_at(20, 40, 1, 20, 0xFF0000);
    TEST_ASSERT_EQUAL(0x00FF00, l63_at(20, 30));                 /* no rule across the spanner */
    l64_box("m", 0, 0, 40, 60);
    l61_text_at("z", 0, 61);
}

static void test_l64_columns_span_widths_and_breaks(void) {
    lot52_render(L64_BODY "<div style='column-count:3;width:56px'><div style='break-after:column'>a</div><div>b</div><div>c</div><div>d</div>"
                 "<div id=s style='column-span:all;width:16px;margin-top:2px'>x</div><span>p q r</span></div>"
                 "<div id=t style='column-span:all;width:50%'>t</div></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    l61_text_at("a", 0, 1);                                   /* alone in its column */
    l61_text_at("b", 21, 1);
    l61_text_at("c", 21, 11);
    l61_text_at("d", 42, 1);
    l64_box("s", 0, 22, 16, 10);                              /* its own width, its margin kept */
    l61_text_at("p", 0, 33);                                  /* inline lines balanced after it */
    l61_text_at("q", 21, 33);
    l61_text_at("r", 42, 33);
    l64_box("t", 0, 42, 160, 10);                             /* outside a multi-column box: a block */
    lot52_render(L64_BODY "<div style='column-count:2;width:40px;height:50px;column-fill:auto'><div id=u style='column-span:all;width:50%'>u</div>"
                 "<div>v</div><div>w</div></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("column span width on the column"));
    l64_box("u", 0, 0, 8, 10);                                /* 50% of the 16 px column */
    l61_text_at("v", 0, 11);                                  /* balanced: no fill with a spanner */
    l61_text_at("w", 24, 11);
    lot52_render(L64_BODY "<div id=a style='break-before:page;break-after:column;break-after:left;break-inside:avoid-page;"
                 "break-before:avoid-column;break-inside:avoid'>v</div></body>", 320);
    TEST_ASSERT_EQUAL(3, l61_diag_count("break refused"));
    TEST_ASSERT_EQUAL(2, lot52_node("a")->col_break);
}

static void lot64_more_run(void) {
    RUN_TEST(test_l64_dir_attribute);
    RUN_TEST(test_l64_bgpos_edge_offsets);
    RUN_TEST(test_l64_bgpos_longhands);
    RUN_TEST(test_l64_bgpos_refusals);
    RUN_TEST(test_l64_columns_span_all);
    RUN_TEST(test_l64_columns_span_widths_and_breaks);
}
