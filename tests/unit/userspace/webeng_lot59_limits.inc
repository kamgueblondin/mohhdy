static void test_l59_limits_min_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:min-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_min_max_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_max_min_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:min-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_max_max_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_lower_does_not_shrink(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:80px;min-wid"
        "th:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_upper_does_not_grow(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;max-wid"
        "th:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_lower_wins(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:max-content;max-width:min-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_range_middle(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:40px;min-wid"
        "th:min-content;max-width:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_range_above(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:80px;min-wid"
        "th:min-content;max-width:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_range_below(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:16px;min-wid"
        "th:min-content;max-width:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_min_fit_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_max_fit_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_min_fit_limit(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content(40px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_max_fit_limit(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(40px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_min_fit_below_word(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content(8px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_max_fit_above_line(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(100px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_min_fit_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content(20%)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_max_fit_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(20%)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_logical_lower(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;inline-size:12px;m"
        "in-inline-size:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_logical_upper(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;inline-size:120px;"
        "max-inline-size:min-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_lower_px(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:max-content;min-width:20px'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_upper_px(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:min-content;max-width:80px'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_lower_auto(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:max-content;min-width:auto'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(12, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_upper_none(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:min-content;max-width:none'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_lower_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:max-content;min-width:calc(20px + 4px)'>AB CDEF</div><div id=b style='height:5px'></div></div></b"
        "ody>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_upper_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:min-content;max-width:calc(60px + 4px)'>AB CDEF</div><div id=b style='height:5px'></div></div></"
        "body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_lower_intrinsic(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:calc(40px + 40px);min-width:min-content'>AB CDEF</div><div id=b style='height:5px'></div></div></"
        "body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_upper_intrinsic(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:calc(20px + 4px);max-width:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></"
        "body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_lower_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:max-content;min-width:20%'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_replace_upper_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:min-content;max-width:40%'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_lower_fit_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content(calc(10% + 24px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_upper_fit_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(clamp(32px,20%,48px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_fit_zero_lower(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content(0px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_fit_zero_upper(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(0%)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_fit_negative_calc(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(calc(0px - 2px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_fit_typed_product(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content(calc(20px * 2))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:min-content;padding:2px;border:1px solid red'>AB CDEF</div><div id=b style='height:5px'></div></d"
        "iv></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(3, find_text("AB")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(41, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(14, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:max-content;padding:2px;border:1px solid red'>AB CDEF</div><div id=b style='height:5px'></div></"
        "div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(62, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(41, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_fit_limit(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(40px);padding:2px;border:1px solid red'>AB CDEF</div><div id=b style='height:5px'></"
        "div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(3, find_text("AB")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(41, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(14, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_fit_lower(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:12px;min-wid"
        "th:fit-content(40px);padding:2px;border:1px solid red'>AB CDEF</div><div id=b style='height:5px'></d"
        "iv></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(36, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(3, find_text("AB")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB")->y);
    TEST_ASSERT_EQUAL(36, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(41, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(14, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_box_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;box-sizing:border-"
        "box;width:12px;min-width:min-content;padding:2px;border:1px solid red'>AB CDEF</div><div id=b style="
        "'height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(38, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(3, find_text("AB")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(14, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_box_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;box-sizing:border-"
        "box;width:120px;max-width:max-content;padding:2px;border:1px solid red'>AB CDEF</div><div id=b style"
        "='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(62, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_box_fit(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;box-sizing:border-"
        "box;width:120px;max-width:fit-content(46px);padding:2px;border:1px solid red'>AB CDEF</div><div id=b"
        " style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(3, find_text("AB")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(14, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_border_box_lower(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;box-sizing:border-"
        "box;width:12px;min-width:fit-content(46px);padding:2px;border:1px solid red'>AB CDEF</div><div id=b "
        "style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(46, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(3, find_text("AB")->x);
    TEST_ASSERT_EQUAL(4, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(3, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(14, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_zero(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "0px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_word_floor(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "24px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_mid(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_line_cap(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "120px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_percentage(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "20%)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_percentage_resize(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:250px'><div id=a style='height:30px;width:fit-content("
        "20%)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(250, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_zero_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "0%)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_full_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "100%)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "calc(20px + 24px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_math_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "calc(10% + 20px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_min_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "min(48px,20%))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_max_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "max(20px,20%))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_clamp_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "clamp(32px,50%,48px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_division(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "calc(100px / 3))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(33, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_small_parent_overflow(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:24px'><div id=a style='height:30px;width:fit-content(4"
        "8px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_keyword_small_parent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:24px'><div id=a style='height:30px;width:fit-content'>"
        "AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_function_beyond_parent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:40px'><div id=a style='height:30px;width:fit-content(5"
        "6px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_keyword_uses_parent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:40px'><div id=a style='height:30px;width:fit-content'>"
        "AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_spaces(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:FIT-CONTENT("
        " 40PX )'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_united_zero(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "0)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_then_plain(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:72px'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(72, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_then_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:calc(40px + 40px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_then_auto(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:auto'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_then_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:min-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_then_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:max-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_then_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_logical_function(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;inline-size:fit-co"
        "ntent(40px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_logical_physical_order(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;inline-size:fit-co"
        "ntent(40px);width:fit-content(48px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_bounded_product(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "calc(2000px * 400))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_width_negative_result(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "calc(4px - 10px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_limits_atomic_width_0(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content()'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_1(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(auto)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(-1px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(2001px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_4(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(1em)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_5(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(1.5px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_6(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(40px,50px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_7(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(40px)tail'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_8(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(calc(2px * 3px))'>AB CDEF</div><div id=b style='height:5px'></div></div></bo"
        "dy>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_9(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(calc(2px / 0))'>AB CDEF</div><div id=b style='height:5px'></div></div></body"
        ">", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_10(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(calc(1px + 2))'>AB CDEF</div><div id=b style='height:5px'></div></div></body"
        ">", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_11(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(calc(calc(calc(calc(calc(1px))))))'>AB CDEF</div><div id=b style='height:5px"
        "'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_12(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(10vw)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_13(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(var(--x))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_14(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content(10px 20px)'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_width_15(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:fit-content("
        "40px);width:fit-content((40px))'>AB CDEF</div><div id=b style='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_limit_0(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;min-wi"
        "dth:fit-content(40px);min-width:fit-content(calc(2px * 3px))'>AB CDEF</div><div id=b style='height:5"
        "px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_limit_1(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-wi"
        "dth:fit-content(40px);max-width:fit-content(calc(2px * 3px))'>AB CDEF</div><div id=b style='height:5"
        "px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_limit_2(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;min-in"
        "line-size:fit-content(40px);min-inline-size:fit-content(calc(2px * 3px))'>AB CDEF</div><div id=b sty"
        "le='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("AB CDEF")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB CDEF")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}

static void test_l59_limits_atomic_limit_3(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:200px'><div id=a style='height:30px;width:120px;max-in"
        "line-size:fit-content(40px);max-inline-size:fit-content(calc(2px * 3px))'>AB CDEF</div><div id=b sty"
        "le='height:5px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, find_text("AB")->x);
    TEST_ASSERT_EQUAL(1, find_text("AB")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(200, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(35, lot52_node("p")->h);
    TEST_ASSERT_NOT_NULL(find_text("CDEF"));
    TEST_ASSERT_EQUAL(0, find_text("CDEF")->x);
    TEST_ASSERT_EQUAL(11, find_text("CDEF")->y);
    TEST_ASSERT_TRUE(lot52_has_diag("intrinsic size refused"));
}
