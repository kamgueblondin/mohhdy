static void test_l59_flex_basis_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:min-content'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:0;fle"
        "x-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(32, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:max-content'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:0;fle"
        "x-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_fit(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:fit-content(40px)'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width"
        ":0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(40, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_keyword(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:fit-content'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:0;fle"
        "x-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_min_grow(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:min-content'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:0;fle"
        "x-grow:1'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(88, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(32, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_max_grow(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:max-content'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:0;fle"
        "x-grow:1'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(64, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_fit_grow(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:fit-content(40px)'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width"
        ":0;flex-grow:1'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(40, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_grow_self(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:1;flex-basis:min-content'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:0;fle"
        "x-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(80, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_grow_capped_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:1;flex-basis:20px;max-width:min-content'>AB CDEF</div><div id=b style='height:20px;width:40px;"
        "min-width:0;flex-grow:1'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(88, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(32, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_grow_capped_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:1;flex-basis:20px;max-width:max-content'>AB CDEF</div><div id=b style='height:20px;width:40px;"
        "min-width:0;flex-grow:1'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(50, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_grow_capped_fit(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:1;flex-basis:20px;max-width:fit-content(40px)'>AB CDEF</div><div id=b style='height:20px;width"
        ":40px;min-width:0;flex-grow:1'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(40, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_grow_min_wins_cap(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:1;flex-basis:20px;min-width:max-content;max-width:min-content'>AB CDEF</div><div id=b style='h"
        "eight:20px;width:40px;min-width:0;flex-grow:1'>X</div></div><div id=c style='height:5px'></div></bod"
        "y>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(64, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_shrink_floor_min(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:40px;'><div id=a style='height:30px;flex-"
        "grow:0;flex-basis:80px;min-width:min-content'>AB CDEF</div><div id=b style='height:20px;width:40px;m"
        "in-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(32, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_shrink_floor_max(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:64px;'><div id=a style='height:30px;flex-"
        "grow:0;flex-basis:80px;min-width:max-content'>AB CDEF</div><div id=b style='height:20px;width:40px;m"
        "in-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_shrink_floor_fit(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:64px;'><div id=a style='height:30px;flex-"
        "grow:0;flex-basis:80px;min-width:fit-content(40px)'>AB CDEF</div><div id=b style='height:20px;width:"
        "40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(42, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(42, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(22, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(42, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_shrink_fit_larger_floor(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:64px;'><div id=a style='height:30px;flex-"
        "grow:0;flex-basis:80px;min-width:fit-content(48px)'>AB CDEF</div><div id=b style='height:20px;width:"
        "40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(16, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(48, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_px_replaces(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:min-content;flex-basis:44px'>AB CDEF</div><div id=b style='height:20px;width:40px"
        ";min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(44, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_math_replaces(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:max-content;flex-basis:calc(20px + 20px)'>AB CDEF</div><div id=b style='height:20"
        "px;width:40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(40, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_auto_replaces(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;width:48px;flex-basis:min-content;flex-basis:auto'>AB CDEF</div><div id=b style='height:20px"
        ";width:40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(48, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_content_replaces(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;width:48px;flex-basis:min-content;flex-basis:content'>AB CDEF</div><div id=b style='height:2"
        "0px;width:40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_intrinsic_replaces_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:calc(20px + 20px);flex-basis:min-content'>AB CDEF</div><div id=b style='height:20"
        "px;width:40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(32, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_intrinsic_replaces_width(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;width:100px;flex-basis:max-content'>AB CDEF</div><div id=b style='height:20px;width:40px;min"
        "-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_fit_percent(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:fit-content(40%)'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:"
        "0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(48, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(48, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_fit_math(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:fit-content(calc(20% + 20px))'>AB CDEF</div><div id=b style='height:20px;width:40"
        "px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(44, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(44, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(44, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_low_bounded(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:fit-content(0px)'>AB CDEF</div><div id=b style='height:20px;width:40px;min-width:"
        "0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(32, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_high_bounded(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:fit-content(2000px)'>AB CDEF</div><div id=b style='height:20px;width:40px;min-wid"
        "th:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_upper_limit(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:max-content;max-width:min-content'>AB CDEF</div><div id=b style='height:20px;widt"
        "h:40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(32, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_basis_lower_limit(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:0;flex-basis:min-content;min-width:max-content'>AB CDEF</div><div id=b style='height:20px;widt"
        "h:40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_logical_grow_cap(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:120px;'><div id=a style='height:30px;flex"
        "-grow:1;flex-basis:20px;max-inline-size:fit-content(40px)'>AB CDEF</div><div id=b style='height:20px"
        ";width:40px;min-width:0;flex-grow:1'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(40, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_flex_logical_shrink_floor(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;width:64px;'><div id=a style='height:30px;flex-"
        "grow:0;flex-basis:80px;min-inline-size:max-content'>AB CDEF</div><div id=b style='height:20px;width:"
        "40px;min-width:0;flex-grow:0'>X</div></div><div id=c style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(8, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("X"));
    TEST_ASSERT_EQUAL(56, find_text("X")->x);
    TEST_ASSERT_EQUAL(1, find_text("X")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
