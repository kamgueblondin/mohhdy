/* Lot 63: radial-gradient centre positions and explicit sizes. Each case
 * renders a 40x20 box; the first ring band gives the outer ellipse. */
static const os_fb_web_item_t *l63_radial(const char *gradient) {
    static char html[256];
    snprintf(html, sizeof(html), "<body style='margin:0'><div id=a style='width:40px;height:20px;background:radial-gradient(%s, #ff0000, #0000ff)'></div></body>", gradient);
    lot52_render(html, 320);
    return g_doc.nitems > 1 ? &g_doc.items[1] : 0;
}

#define L63_BAND(g, bx, by, bw, bh) do { const os_fb_web_item_t *it = l63_radial(g); \
    L63_CASE(it && it->x == (bx) && it->y == (by) && it->w == (bw) && it->h == (bh), g); } while (0)

static void test_l63_radial_positions(void) {
    L63_BAND("circle closest-side at 10px 50%", 5, 0, 10, 3);      /* r 10 around (10,10) */
    L63_BAND("ellipse closest-side at 30% 25%", 7, 0, 10, 1);      /* (12,5): rx 12, ry 5 */
    L63_BAND("circle 6px at left top", 0, 0, 6, 2);                /* cut to the box */
    L63_BAND("20px 4px at right bottom", 30, 16, 10, 1);           /* ellipse 20x4 at (40,20) */
    L63_BAND("at 10px", 0, 0, 40, 20);                             /* one value: y centre */
    TEST_ASSERT_EQUAL(10, lot52_node("a")->grad_at[0]);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->grad_at[1]);
    TEST_ASSERT_EQUAL(2, lot52_node("a")->grad_at_pct);
    L63_BAND("circle 8px at bottom left", 0, 12, 4, 2);            /* keyword y first: (0,20) */
    TEST_ASSERT_EQUAL(0, l61_diag_count("refused"));
}

static void test_l63_radial_extents_off_centre(void) {
    L63_BAND("farthest-side at 10px 5px", 0, 0, 40, 10);           /* ellipse 30 x 15 */
    L63_BAND("closest-corner circle at 10px 5px", 0, 0, 20, 1);    /* sqrt(100 + 25) = 11 */
    L63_BAND("circle at 10px 5px", 0, 0, 40, 20);                  /* farthest corner: 33 */
    L63_BAND("ellipse farthest-corner at 10px 5px", 0, 0, 40, 20); /* 30 x 15 times sqrt 2 */
    L63_BAND("ellipse farthest-side at 50px 5px", 1, 0, 39, 10);   /* a centre outside: 50 x 15 */
    L63_BAND("circle farthest-side at 10px 5px", 0, 0, 40, 15);    /* the larger distance: 30 */
    L63_BAND("ellipse closest-corner at 10px 15px", 3, 8, 14, 2);  /* 10 x 5 times sqrt 2: 14 x 7 */
}

static void test_l63_radial_sizes(void) {
    L63_BAND("50% 50% at center", 9, 0, 22, 3);                    /* the inscribed ellipse */
    L63_BAND("ellipse 20px 10px", 9, 0, 22, 3);
    L63_BAND("circle 10px", 15, 0, 10, 3);
    L63_BAND("10px", 15, 0, 10, 3);                                /* one length: a circle */
    TEST_ASSERT_EQUAL(1, lot52_node("a")->grad_shape & 1);
    L63_BAND("8px 50%", 16, 0, 8, 3);                              /* two sizes: an ellipse, y in percent */
    TEST_ASSERT_EQUAL(0, lot52_node("a")->grad_shape & 1);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->grad_at_pct & 12);
}

static void test_l63_radial_refusals(void) {
    static const char *const bad[] = { "circle 10px 20px", "circle 50%", "ellipse 10px", "circle circle", "closest-side 10px",
        "10px closest-side", "20px 10px 5px", "at", "at left left", "circle -5px", "closest-side farthest-side", "at center center center" };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        (void)l63_radial(bad[k]);
        L63_CASE(1 == l61_diag_count("gradient refused"), bad[k]);
        L63_CASE(0 == lot52_node("a")->grad_dir, bad[k]);
    }
}

static void lot63_radial_run(void) {
    RUN_TEST(test_l63_radial_positions);
    RUN_TEST(test_l63_radial_extents_off_centre);
    RUN_TEST(test_l63_radial_sizes);
    RUN_TEST(test_l63_radial_refusals);
}
