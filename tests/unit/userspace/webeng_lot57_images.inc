static void lot57_image(const char *html) {
    lot52_image(html);
    we_set_image_reader(0, 0);
}

static void test_l57_fit_fill(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='width:24px;height:24px;object-fit:fill'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 12, &painted));
    TEST_ASSERT_EQUAL(1, painted);
}

static void test_l57_fit_contain(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='width:24px;height:24px;object-fit:contain'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 12, &painted));
    TEST_ASSERT_EQUAL(1, painted);
}

static void test_l57_fit_cover(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='width:24px;height:24px;object-fit:cover'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 12, &painted));
    TEST_ASSERT_EQUAL(1, painted);
}

static void test_l57_fit_none(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='width:24px;height:24px;object-fit:none'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 12, &painted));
    TEST_ASSERT_EQUAL(1, painted);
}

static void test_l57_fit_scale_down(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='width:24px;height:24px;object-fit:scale-down'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 12, &painted));
    TEST_ASSERT_EQUAL(1, painted);
}

static void test_l57_intrinsic_natural(void) {
    lot57_image("<body style='margin:0'><img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(32, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_ratio_width(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;width:24px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(16, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_ratio_height(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;height:8px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(12, lot52_node("im")->min_content);
    TEST_ASSERT_EQUAL(8, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_ratio_max_width(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;max-width:24px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(16, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_ratio_min_width(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;min-width:60px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(40, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_ratio_min_height(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;min-height:20px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(32, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_ratio_max_height(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;max-height:8px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(8, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_percent_width(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;width:10%'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(21, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(21, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_explicit_both(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;width:12px;height:20px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(20, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_max_overrides_width(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;width:40px;max-width:24px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(16, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_min_beats_max(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;width:20px;min-width:40px;max-width:24px'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(26, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(26, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_scale_down_natural(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:scale-down;width:60px;height:32px'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(32, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(32, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_none_small(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:none;width:8px;height:8px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(8, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_contain_tall(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;width:12px;height:36px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(36, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_cover_wide(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:cover;width:60px;height:8px'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(8, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_zero_percent_height_indefinite(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:fill;width:24px;height:50%'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(16, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l57_contain_top(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:contain;object-position:left top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_contain_bottom(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:contain;object-position:left bottom'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 16, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_contain_center(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:contain;object-position:center'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 8, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_contain_25pct(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:contain;object-position:0% 25%'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 4, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_contain_bg_above(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:contain;object-position:left bottom'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_pixel(1, 1, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_contain_bg_below(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:contain;object-position:left top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_pixel(1, 23, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cover_left(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:cover;object-position:left top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cover_right(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:cover;object-position:right top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_none_left(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:none;object-position:left top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_none_right(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:none;object-position:right top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_none_center(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:none;object-position:center'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 4, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_none_quarter(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:none;object-position:25% top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0xffffffU, lot52_pixel(1, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_scale_down_top(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:scale-down;object-position:left top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 0, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_one_vertical_keyword(void) {
    int painted;
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' style='width:24px;height:24px;background:red;object-fit:contain;object-position:bottom'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(0U, lot52_pixel(0, 16, &painted));
    TEST_ASSERT_EQUAL(1, painted);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_invalid_fit_35(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;object-position:left top;width:24px;height:24px;object-fit:huge'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(WE_FIT_CONTAIN, lot52_node("im")->object_fit);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_y);
    TEST_ASSERT_TRUE(lot52_has_diag("object fit refused"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
}

static void test_l57_invalid_position_36(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;object-position:left top;width:24px;height:24px;object-position:101% 0%'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(WE_FIT_CONTAIN, lot52_node("im")->object_fit);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_y);
    TEST_ASSERT_TRUE(lot52_has_diag("object position refused"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
}

static void test_l57_invalid_position_37(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;object-position:left top;width:24px;height:24px;object-position:left right'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(WE_FIT_CONTAIN, lot52_node("im")->object_fit);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_y);
    TEST_ASSERT_TRUE(lot52_has_diag("object position refused"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
}

static void test_l57_invalid_position_38(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;object-position:left top;width:24px;height:24px;object-position:-1% top'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(WE_FIT_CONTAIN, lot52_node("im")->object_fit);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_y);
    TEST_ASSERT_TRUE(lot52_has_diag("object position refused"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
}

static void test_l57_invalid_position_39(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;object-position:left top;width:24px;height:24px;object-position:left top center'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(WE_FIT_CONTAIN, lot52_node("im")->object_fit);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_y);
    TEST_ASSERT_TRUE(lot52_has_diag("object position refused"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
}

static void test_l57_invalid_position_40(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;object-position:left top;width:24px;height:24px;object-position:3px 4px'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("im")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("im")->h);
    TEST_ASSERT_EQUAL(WE_FIT_CONTAIN, lot52_node("im")->object_fit);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_x);
    TEST_ASSERT_EQUAL(0, lot52_node("im")->object_y);
    TEST_ASSERT_TRUE(lot52_has_diag("object position refused"));
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
}

static void test_l57_refuse_width_cap(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;width:65px;height:8px'></body>");
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[fallback]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l57_refuse_height_cap(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;width:8px;height:65px'></body>");
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[fallback]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l57_refuse_ratio_cap(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;height:48px'></body>");
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[fallback]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l57_refuse_zero_width(void) {
    lot57_image("<body style='margin:0'><img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;width:0'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[fallback]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l57_refuse_zero_height(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;height:0'></body>");
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[fallback]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l57_refuse_minimum_cap(void) {
    lot57_image("<body style='margin:0'>"
        "<img id=im src='/images/mire-g1.png' alt='fallback' style='object-fit:contain;min-width:65px'></body>");
    TEST_ASSERT_EQUAL(0, g_doc.perf.images);
    TEST_ASSERT_EQUAL(1, g_doc.perf.image_refused);
    TEST_ASSERT_NOT_NULL(find_text("[fallback]"));
    TEST_ASSERT_TRUE(lot52_has_diag("image output exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}
