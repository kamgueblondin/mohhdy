/* Lot 63: a rounded overflow clip under a transform applies in the box's
 * own space, then the transform maps the clipped pieces. */
static int l63_painted_in(int x0, int y0, int x1, int y1) {
    int x, y, count = 0;
    for (y = y0; y < y1; y++) for (x = x0; x < x1; x++) if (l61_paint(x, y) != 0xFFFFFFFFu) count++;
    return count;
}

static void test_l63_clip_rounded_translated(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden;border-radius:10px;transform:translate(1px, 2px)'>"
                 "<div style='height:20px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nitems);
    l63_item_is(0, 6, 2, 10, 3);                      /* the clip's bands, moved by (1, 2) */
    l63_item_is(2, 1, 8, 20, 8);
    l63_item_is(4, 6, 19, 10, 3);
    TEST_ASSERT_EQUAL(0, l63_painted_in(1, 2, 4, 5));
    TEST_ASSERT_EQUAL(0, l61_diag_count("display budget"));
}

static void test_l63_clip_rounded_quarter_turn(void) {
    /* the left half, clipped round, turns about the centre to the top half */
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden;border-radius:10px;transform:rotate(90deg)'>"
                 "<div style='height:20px;width:10px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(5, (int)g_doc.nitems);
    l63_item_is(2, 6, 0, 8, 10);
    l63_item_is(0, 17, 5, 3, 5);
    l63_item_is(4, 0, 5, 3, 5);
    TEST_ASSERT_EQUAL(0, l63_painted_in(0, 0, 3, 3));
    TEST_ASSERT_EQUAL(0, l63_painted_in(0, 10, 20, 20));
}

static void test_l63_clip_rounded_rotated_and_scaled(void) {
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden;border-radius:10px;transform:rotate(45deg)'>"
                 "<div style='height:20px;background:#ff0000'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, (int)g_doc.nitems);         /* 5 clip bands, 8 row bands each */
    TEST_ASSERT_EQUAL(0, l63_painted_in(0, 0, 2, 2)); /* the stepped disc turns: its steps reach (2,2) */
    TEST_ASSERT_EQUAL(4, l63_painted_in(9, 9, 11, 11));
    TEST_ASSERT_EQUAL(0, l63_painted_in(18, 18, 21, 21));
    lot52_render(L62_BODY "<div style='width:20px;height:20px;overflow:hidden;border-radius:10px;transform:scale(2);transform-origin:0 0'>"
                 "<p style='margin:0'>HHH</p></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l61_count(OS_FB_WEB_TEXT));  /* glyph runs, cut by the bands, then doubled */
    TEST_ASSERT_EQUAL(0, l63_painted_in(0, 0, 4, 6));
    TEST_ASSERT_TRUE(l63_painted_in(10, 2, 14, 6) > 0);
}

static void lot63_clip_run(void) {
    RUN_TEST(test_l63_clip_rounded_translated);
    RUN_TEST(test_l63_clip_rounded_quarter_turn);
    RUN_TEST(test_l63_clip_rounded_rotated_and_scaled);
}
