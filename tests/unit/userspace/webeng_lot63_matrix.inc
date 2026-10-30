/* Lot 63 J: transform: matrix(a, b, c, d, e, f). */
#define L63_M_BOX(tf) "<body style='margin:0'><div id=m style='width:20px;height:10px;background:#ff0000;transform:" tf ";transform-origin:0 0'></div></body>"

static void test_l63_matrix_maps_like_css(void) {
    lot52_render(L63_M_BOX("matrix(1,0,0,1,5,3)"), 320);
    TEST_ASSERT_EQUAL(WE_TF_MATRIX, lot52_node("m")->tf[0].kind);
    TEST_ASSERT_EQUAL(WE_TF_DATA, lot52_node("m")->tf[2].kind);
    TEST_ASSERT_EQUAL(3, lot52_node("m")->tf_count);
    l63_item_is(0, 5, 3, 20, 10);
    lot52_render(L63_M_BOX("matrix(0,1,-1,0,0,0)"), 320);          /* x' = -y, y' = x */
    l63_item_is(0, -10, 0, 10, 20);
    lot52_render(L63_M_BOX("MATRIX(2, 0, 0, .5, 0, 0)"), 320);
    l63_item_is(0, 0, 0, 40, 5);
    lot52_render(L63_M_BOX("matrix(1,0,1,1,0,0)"), 320);           /* c shears x by y */
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    l63_item_is(0, 1, 0, 20, 1);
    l63_item_is(7, 9, 8, 20, 2);
    lot52_render(L63_M_BOX("matrix(1,1,0,1,0,0)"), 320);           /* b shears y by x */
    TEST_ASSERT_EQUAL(8, (int)g_doc.nitems);
    lot52_render(L63_M_BOX("matrix(1,0,0,1,1.5,-1.5)"), 320);      /* half away from zero */
    l63_item_is(0, 2, -2, 20, 10);
    lot52_render(L63_M_BOX("matrix(1,0,0,1,0,0) translate(2px)"), 320);
    TEST_ASSERT_EQUAL(4, lot52_node("m")->tf_count);
    l63_item_is(0, 2, 0, 20, 10);
    lot52_render(L63_M_BOX("scale(2) matrix(1,0,0,1,3,0)"), 320);  /* the matrix after the scale */
    l63_item_is(0, 6, 0, 40, 20);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l63_matrix_refusals(void) {
    static const char *const bad[] = { "matrix(1,0,0,1,0)", "matrix(1,0,0,1,0,0,0)", "matrix(9,0,0,1,0,0)", "matrix(1,0,0,-9,0,0)",
        "matrix(1,0,0,1,5px,0)", "matrix(1,0,0,1,0,5%)", "matrix(1,0,0,1,4001,0)", "matrix(1,0,0,1,0,-4001)", "matrix(1,0,0,1,0,0",
        "matrix(1,0,0,1,0,0) matrix(1,0,0,1,0,0)", "translate(1px) translate(1px) matrix(1,0,0,1,0,0)", "matrix(1,0,0,1,,0)",
        "matrix(1,0,0,1,0,.)", "matrix(1deg,0,0,1,0,0)", "matrix(1,0,0,1,0,0) translate(1px) translate(1px)" };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        char html[256];
        snprintf(html, sizeof(html), "<body style='margin:0'><div id=m style='width:20px;height:10px;transform:translate(1px);transform:%s'></div></body>", bad[k]);
        lot52_render(html, 320);
        L63_CASE(1 == l61_diag_count("transform refused"), bad[k]);
        L63_CASE(1 == lot52_node("m")->tf_count && WE_TF_TRANSLATE == lot52_node("m")->tf[0].kind, bad[k]);
    }
    lot52_render(L63_M_BOX("matrix(1,0,0,1,4000,-4000)"), 320);
    TEST_ASSERT_EQUAL(4000, lot52_node("m")->tf[2].x);
    TEST_ASSERT_EQUAL(-4000, lot52_node("m")->tf[2].y);
    lot52_render(L63_M_BOX("translate(1px) matrix(8,0,0,-8,0,0)"), 320);
    TEST_ASSERT_EQUAL(8000, lot52_node("m")->tf[1].x);
    TEST_ASSERT_EQUAL(-8000, lot52_node("m")->tf[2].y);
}

static void lot63_matrix_run(void) {
    RUN_TEST(test_l63_matrix_maps_like_css);
    RUN_TEST(test_l63_matrix_refusals);
}
