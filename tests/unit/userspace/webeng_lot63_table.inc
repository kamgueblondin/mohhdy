/* Lot 63 H: every grid line of a collapsed table has its own band, the
 * widest resolved segment along it; the tracks on both sides overlap by
 * exactly that band. */
#define L63_T_OPEN "<body style='margin:0'><table id=t style='width:40px;border-collapse:collapse'>"

/* a 1px segment is centred in the 3px band set by another row */
static void test_l63_table_narrow_segment_in_wide_band(void) {
    lot52_render(L63_T_OPEN "<tr><td id=a style='border-right:3px solid #ff0000;height:4px'></td><td id=b style='height:4px'></td></tr>"
                 "<tr><td id=c style='border-right:1px solid #0000ff;height:4px'>x</td><td id=d style='height:4px'>y</td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(4, (int)g_doc.nitems);
    l61_box("a", 0, 0, 22, 4);                               /* 40 + 3 shared */
    l61_box("b", 19, 0, 21, 4);
    l61_box("d", 19, 4, 21, 4);
    l61_rect_is(0xFF0000, 19, 0, 3, 4);
    l61_rect_is(0x0000FF, 20, 4, 1, 4);                      /* band + 3/2 - 1/2 */
    TEST_ASSERT_EQUAL(2, lot52_node("c")->cin[1]);
    TEST_ASSERT_EQUAL(2, lot52_node("d")->cin[3]);
    l61_text_at("y", 21, 5);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->cin[0]);           /* rows 0 and 1 meet on a 0px line */
}

/* a hidden line has no band: the cells abut */
static void test_l63_table_hidden_line_has_no_band(void) {
    lot52_render(L63_T_OPEN "<tr><td id=a style='border:2px solid #ff0000;border-right:2px hidden;height:4px'></td>"
                 "<td id=b style='border:2px solid #0000ff;height:4px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(6, (int)g_doc.nitems);
    l61_box("a", 0, 0, 20, 8);
    l61_box("b", 20, 0, 20, 8);
    TEST_ASSERT_TRUE(l61_paint(19, 4) == 0xFFFFFFFFu);
    TEST_ASSERT_TRUE(l61_paint(20, 4) == 0xFFFFFFFFu);
    l61_rect_at(38, 0, 2, 8, 0x0000FF);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->cin[1]);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->cin[3]);
}

/* rows: bands 0, 4, 1, 0 */
static void test_l63_table_row_lines(void) {
    lot52_render(L63_T_OPEN "<tr><td id=a style='border-bottom:4px solid #ff0000;height:4px'></td></tr>"
                 "<tr><td id=b style='border-bottom:1px solid #0000ff;height:4px'></td></tr>"
                 "<tr><td id=c style='height:4px'></td></tr></table></body>", 320);
    TEST_ASSERT_EQUAL(2, (int)g_doc.nitems);
    l61_box("t", 0, 0, 40, 17);
    l61_box("a", 0, 0, 40, 8);
    l61_box("b", 0, 4, 40, 9);                               /* 4 + 4 + 1 */
    l61_box("c", 0, 12, 40, 5);
    l61_rect_is(0xFF0000, 0, 4, 40, 4);
    l61_rect_is(0x0000FF, 0, 12, 40, 1);
    /* the measured height of a nested table subtracts the same bands */
    lot52_render("<body style='margin:0'><table id=o><tr><td id=oc style='padding:0'><table style='width:40px;border-collapse:collapse'>"
                 "<tr><td style='border-bottom:4px solid #ff0000;height:4px'></td></tr><tr><td style='border-bottom:1px solid #0000ff;height:4px'></td></tr>"
                 "<tr><td style='height:4px'></td></tr></table></td><td style='padding:0;vertical-align:bottom'>x</td></tr></table></body>", 320);
    l61_box("o", 0, 0, 320, 17);
    l61_text_at("x", 160, 8);
}

/* shrink-to-fit sums the columns minus each inner band */
static void test_l63_table_intrinsic_bands(void) {
    lot52_render("<body style='margin:0'><div id=w style='display:inline-block'><table id=t style='border-collapse:collapse'><tr>"
                 "<td id=a style='border:3px solid #ff0000;width:10px'></td><td id=b style='width:10px'></td>"
                 "<td id=c style='border-right:1px solid #0000ff;width:10px'></td></tr></table></div></body>", 320);
    l61_box("w", 0, 0, 37, 6);                               /* 16 + 13 + 11 - 3 */
    l61_box("a", 0, 0, 16, 6);
    l61_box("b", 13, 0, 13, 3);
    l61_box("c", 26, 0, 11, 3);
    l61_rect_at(36, 0, 1, 6, 0x0000FF);
}

static void lot63_table_run(void) {
    RUN_TEST(test_l63_table_narrow_segment_in_wide_band);
    RUN_TEST(test_l63_table_hidden_line_has_no_band);
    RUN_TEST(test_l63_table_row_lines);
    RUN_TEST(test_l63_table_intrinsic_bands);
}
