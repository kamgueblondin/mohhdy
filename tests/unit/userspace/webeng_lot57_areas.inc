static void test_l57_named_rectangles(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_named_vertical(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head side\" \"body side\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_named_reversed(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"side body\" \"head head\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_named_dot(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head .\" \"body side\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_named_dot_run(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head ...\" \"body side\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_bad_areas_5(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"head head\" \"head side\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_6(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"head . head\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_7(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"head side\" \"body\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_8(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"head side\" \"side body\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_9(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"123 body\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_10(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"a b c d e f g h i\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_11(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"a b c d\" \"e f g h\" \"i . . .\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_12(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:head body;'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_13(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"a\"junk;'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_14(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"a .\" \" . a\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_15(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_bad_areas_16(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head head\" \"body side\";grid-template-areas:\"a\" \"a\" \"a\" \"a\" \"a\" \"a\" \"a\" \"a\" \"a\";'>"
        "<div id=a style='grid-area:head;height:10px'></div><div id=b style='grid-area:body;height:10px'></div>"
        "<div id=c style='grid-area:side;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(50, lot52_node("c")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid areas refused"));
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_count);
}

static void test_l57_area_auto_clears(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head .\" \". .\";'>"
        "<div id=a style='height:10px;grid-area:head;grid-area:auto;'></div>"
        "<div style='height:10px;grid-column:1;grid-row:1;'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_area_numeric_clears(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head .\" \". .\";'>"
        "<div id=a style='height:10px;grid-area:head;grid-column:2;grid-row:2;'></div>"
        "<div style='height:10px;grid-column:1;grid-row:1;'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_area_name_unknown(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head .\" \". .\";'>"
        "<div id=a style='height:10px;grid-area:head;grid-area:missing;'></div>"
        "<div style='height:10px;grid-column:1;grid-row:1;'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(70, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid area name unknown"));
}

static void test_l57_area_bad_keeps(void) {
    lot52_render("<body style='margin:0'>"
        "<div id=g style='display:grid;width:120px;grid-template-columns:50px 70px;grid-template-rows:10px 20px;grid-template-areas:\"head .\" \". .\";'>"
        "<div id=a style='height:10px;grid-area:head;grid-area:9bad;'></div>"
        "<div style='height:10px;grid-column:1;grid-row:1;display:none;'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(50, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_TRUE(lot52_has_diag("grid area name refused"));
}

static void test_l57_areas_establish_track_count(void) {
    lot52_render("<body style='margin:0'><div id=g style='display:grid;width:120px;grid-template-areas:\"a b c\" \"d d d\"'>"
        "<div id=a style='grid-area:c;height:10px'></div><div id=b style='grid-area:d;height:10px'></div></div></body>", 320);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->x);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->x);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(120, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_EQUAL(2, lot52_node("g")->area_rows);
    TEST_ASSERT_EQUAL(3, lot52_node("g")->area_cols);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
