/* Lot 62: list numbering (start, reversed, value) and marker styles. */
static void test_l62_list_numbering(void) {
    lot52_render("<body style='margin:0 0 0 40px'><ol start=3 reversed type=i><li>a</li><li value=10>b</li><li>c</li></ol>"
                 "<ol style='list-style-type:upper-alpha' start=27><li>e</li><li style='display:none'>x</li><li>f</li></ol>"
                 "<ol reversed><li>k</li><li>l</li></ol></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("iii."));
    TEST_ASSERT_NOT_NULL(find_text("x."));                 /* value resets, reversed goes on down */
    TEST_ASSERT_NOT_NULL(find_text("ix."));
    TEST_ASSERT_NOT_NULL(find_text("AA."));
    TEST_ASSERT_NOT_NULL(find_text("AB."));                /* hidden items do not count */
    TEST_ASSERT_NOT_NULL(find_text("2."));                 /* reversed starts at the count */
    TEST_ASSERT_NOT_NULL(find_text("1."));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_list_marker_styles(void) {
    lot52_render("<body style='margin:0 0 0 140px'><ol style='list-style-type:decimal-leading-zero' start=-1><li>g</li><li>h</li><li>i</li></ol>"
                 "<ol type=I start=3888><li>j</li></ol><ol type=a start=0><li>m</li></ol>"
                 "<ol style='list-style-type:lower-roman' start=4000><li>n</li></ol>"
                 "<ul style='list-style:circle outside'><li>d</li></ul><ul type=square><li>o</li></ul>"
                 "<ol style='list-style-type:lower-latin' start=52><li>p</li></ol></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("-01."));
    TEST_ASSERT_NOT_NULL(find_text("00."));
    TEST_ASSERT_NOT_NULL(find_text("01."));
    TEST_ASSERT_NOT_NULL(find_text("MMMDCCCLXXXVIII."));
    TEST_ASSERT_EQUAL(140 + 40 - 17 * 8, find_text("MMMDCCCLXXXVIII.")->x);  /* ol padding 40 */
    TEST_ASSERT_NOT_NULL(find_text("0."));                 /* alpha below 1 falls back to decimal */
    TEST_ASSERT_NOT_NULL(find_text("4000."));              /* roman above 3999 too */
    TEST_ASSERT_NOT_NULL(find_text("o"));
    TEST_ASSERT_NOT_NULL(find_text("#"));
    TEST_ASSERT_NOT_NULL(find_text("az."));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_list_refusals(void) {
    lot52_render("<body style='margin:0 0 0 40px'><ol start=1x><li>a</li></ol><ol start=10000><li>b</li></ol>"
                 "<ol type=q><li value=-><li>c</li></ol>"
                 "<ul style='list-style:inside square;list-style:square circle;list-style:url(x)'><li>d</li></ul></body>", 320);
    TEST_ASSERT_EQUAL(3, l61_diag_count("list number refused"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("list type refused"));
    TEST_ASSERT_EQUAL(3, l61_diag_count("list marker refused"));
    TEST_ASSERT_NOT_NULL(find_text("1."));                 /* a refused start keeps 1 */
    TEST_ASSERT_NOT_NULL(find_text("*"));                  /* ul keeps disc */
}

static void lot62_list_run(void) {
    RUN_TEST(test_l62_list_numbering);
    RUN_TEST(test_l62_list_marker_styles);
    RUN_TEST(test_l62_list_refusals);
}
