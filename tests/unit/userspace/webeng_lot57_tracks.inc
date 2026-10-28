static void test_l57_floor_small(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(20px,1fr) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_floor_frozen(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(80px,1fr) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_weighted_floor(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(20px,1fr) 3fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_weighted_frozen(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(60px,1fr) 3fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_two_floors(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(80px,1fr) minmax(20px,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_both_floors_overflow(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(80px,1fr) minmax(80px,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid width overflow"));
}

static void test_l57_zero_floor(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:101px;grid-template-columns:minmax(0,1fr) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(51, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(51, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_fraction_eight(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:90px;grid-template-columns:minmax(0,8fr) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_floor_max_lower(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(60px,20px) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_floor_cap_empty(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(20px,60px) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cap_zero(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:fit-content(0) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cap_empty(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:fit-content(40px) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_auto_minimum(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(30px,auto) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_repeat_minimum(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:repeat(2,minmax(70px,1fr));'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid width overflow"));
}

static void test_l57_repeat_pair(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:repeat(1,20px 1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_mixed_repeat(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:20px repeat(1,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(100, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_space_function(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax( 20px , 1fr ) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cap_space(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:fit-content( 40px ) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_track_upper_bound(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(2000px,1fr) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(2000, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2000, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid width overflow"));
}

static void test_l57_min_wins_zero(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(40px,0px) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_two_auto_floors(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(20px,auto) minmax(30px,auto);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cap_no_stretch(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(0,60px) 1fr;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cap_long_word(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:fit-content(24px) 1fr;'><div id=a>abc def"
        "</div><div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(96, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_cap_short_word(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:fit-content(64px) 1fr;'><div id=a>abc</div>"
        "<div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(96, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_bounded_auto_content(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(16px,40px) 1fr;'><div id=a>abc def"
        "</div><div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_auto_word_floor(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:minmax(40px,auto) 1fr;'><div id=a>abc def"
        "</div><div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(56, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(64, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_fraction_floor_ignores_content(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-columns:minmax(16px,1fr) 1fr;'>"
        "<div id=a>abc def</div><div id=b style='height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_bad_track_27(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:minmax(auto,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_28(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:minmax(-1px,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_29(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:minmax(2001px,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_30(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:minmax(10px,9fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_31(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:minmax(10%,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_32(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:fit-content(10%);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_33(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:fit-content(-1px);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_34(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:repeat(9,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_35(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:repeat(2,repeat(2,1fr));'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_36(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:repeat(0,1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_37(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:minmax(10px,1fr)junk;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_38(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:repeat(2,1fr)junk;'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_39(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:repeat(5,1fr 1fr);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_40(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:minmax(10px);'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_bad_track_41(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:30px 1fr;grid-template-columns:fit-content();'>"
        "<div id=a style='height:10px;background:red'></div><div id=b style='height:10px;background:blue'></div></div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->grid_cols);
    TEST_ASSERT_TRUE(lot52_has_diag("grid tracks refused"));
}

static void test_l57_row_floor(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;height:120px;grid-template-rows:minmax(80px,1fr) 1fr;align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_row_weighted(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;height:120px;grid-template-rows:minmax(0,1fr) 3fr;align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(90, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_row_cap(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-template-rows:minmax(20px,60px) auto;align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_row_fit_content(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-template-rows:fit-content(5px) auto;align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(5, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_row_indefinite_fraction(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-template-rows:minmax(20px,1fr) 1fr;align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_row_min_overflow(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;height:120px;grid-template-rows:minmax(80px,1fr) minmax(80px,1fr);align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_row_fixed_max_min_wins(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;grid-template-rows:minmax(30px,10px) auto;align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}

static void test_l57_row_repeat(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:1fr;height:101px;grid-template-rows:repeat(2,minmax(0,1fr));align-items:stretch;'>"
        "<div id=a>A</div><div id=b>B</div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(51, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(51, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(2, g_doc.perf.grid_lines);
}
