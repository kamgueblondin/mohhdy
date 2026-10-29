static void test_l59_nested_nested_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'><div id=a style='display:block;width:min-"
        "content'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_nested_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:min-content'><div id=a style='display:block;width:max-"
        "content'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_nested_fit(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'><div id=a style='display:block;width:fit-"
        "content(40px)'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_inline_fit(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'><div id=a style='display:inline-block;wid"
        "th:fit-content(40px)'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_inline_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'><div id=a style='display:inline-block;wid"
        "th:min-content'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_inline_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:min-content'><div id=a style='display:inline-block;wid"
        "th:max-content'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_nested_lower(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:min-content'><div id=a style='display:block;min-width:"
        "max-content'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB CDEF"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_nested_upper(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'><div id=a style='display:block;max-width:"
        "min-content'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(32, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_nested_fit_lower(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:min-content'><div id=a style='display:block;min-width:"
        "fit-content(40px)'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_nested_nested_fit_upper(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'><div id=a style='display:block;max-width:"
        "fit-content(40px)'>AB CDEF</div></div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("AB"));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
