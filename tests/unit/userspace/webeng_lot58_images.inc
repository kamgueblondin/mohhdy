static void lot58_image(const char *html) {
    lot52_image(html);
    we_set_image_reader(0, 0);
}

static void test_l58_images_image_math_width(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='width:calc(10px + 14px)'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(16, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(16, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_math_height(void) {
    lot58_image("<body style='margin:0'><img id=a src='/images/mire-g1.png' alt='fallback' style='height:calc(4px + 4px)'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_math_both(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='width:calc(20px + 4px);height:calc(20px + 4px)'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_ratio_square(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='width:24px;aspect-ratio:1'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_ratio_wide(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='width:24px;aspect-ratio:3 / 1'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_ratio_height(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='height:8px;aspect-ratio:3 / 1'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_ratio_both_fixed(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='width:24px;height:12px;aspect-ratio:1'></body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(12, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_ratio_contain(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='width:24px;aspect-ratio:1;object-fit:contain'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_ratio_math(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='width:calc(10px + 14px);aspect-ratio:1;object-fit:cover'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l58_images_image_logical(void) {
    lot58_image("<body style='margin:0'>"
        "<img id=a src='/images/mire-g1.png' alt='fallback' style='inline-size:24px;block-size:24px;object-fit:contain'>"
        "</body>");
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(24, g_doc.perf.page_h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.images);
    TEST_ASSERT_EQUAL(0, g_doc.perf.image_refused);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
