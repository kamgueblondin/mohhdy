static void test_l57_child_direct(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section>div{color:red;width:32px}</style><body><section><div id=a>A</div>"
        "</section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_child_not_descendant(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section>div{color:red;width:32px}</style><body><section><article>"
        "<div id=a>A</div></article></section></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_child_chain(void) {
    lot52_render("<style>body{margin:0}div{height:10px}body>section>article>div{color:red;width:32px}</style><body><section>"
        "<article><div id=a>A</div></article></section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_descendant_chain(void) {
    lot52_render("<style>body{margin:0}div{height:10px}body section article div{color:red;width:32px}</style><body><section>"
        "<article><div id=a>A</div></article></section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_mixed_chain(void) {
    lot52_render("<style>body{margin:0}div{height:10px}body section>article div{color:red;width:32px}</style><body><section>"
        "<article><main><div id=a>A</div></main></article></section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_universal_child(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section > *{color:red;width:32px}</style><body><section><div id=a>A</div>"
        "</section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_adjacent(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div+div{color:red;width:32px}</style><body><div id=a>A</div><div id=b>B"
        "</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_adjacent_ignores_text(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div + div{color:red;width:32px}</style><body><div id=a>A</div>noise"
        "<div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_adjacent_other_element_blocks(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div+div{color:red;width:32px}</style><body><div id=a>A</div><span>gap"
        "</span><div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_general_sibling(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div~div{color:red;width:32px}</style><body><div id=a>A</div><span>gap"
        "</span><div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_general_no_prior(void) {
    lot52_render("<style>body{margin:0}div{height:10px}span~div{color:red;width:32px}</style><body><div id=a>A</div><span>gap"
        "</span><div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_sibling_parent_scope(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section div~div{color:red;width:32px}</style><body><section><div id=a>A"
        "</div></section><div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_chain_backtracks_ancestors(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section>article div{color:red;width:32px}</style><body><section><article>"
        "<article><div id=a>A</div></article></article></section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_chain_backtracks_siblings(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section>div~div{color:red;width:32px}</style><body><section><div id=a>A"
        "</div><div></div><div id=b>B</div></section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_adjacent_child_chain(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section>div+div{color:red;width:32px}</style><body><section><div id=a>A"
        "</div><div id=b>B</div></section></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_multiple_classes(void) {
    lot52_render("<style>body{margin:0}div{height:10px}.hot.ready{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div><div id=b class=hot>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_three_classes(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div.hot.ready.on{color:red;width:32px}</style><body>"
        "<div id=a class='on ready hot'>A</div><div id=b class='hot ready'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_id_class_compound(void) {
    lot52_render("<style>body{margin:0}div{height:10px}#a.hot.ready{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div><div id=b class='hot ready'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_universal_class(void) {
    lot52_render("<style>body{margin:0}div{height:10px}*.hot{color:red;width:32px}</style><body><div id=a class=hot>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_first_child(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:first-child{color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_last_child(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:last-child{color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_only_child(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:only-child{color:red;width:32px}</style><body><section><div id=a>A"
        "</div></section><section><div id=b>B</div><div id=c>C</div></section></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_first_ignores_text(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:first-child{color:red;width:32px}</style><body>text<div id=a>A</div>"
        "<div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_last_hidden_counts(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:last-child{color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><span style='display:none'></span></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_odd(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(odd){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_even(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(even){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_integer(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(2){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_formula(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(2n + 1){color:red;width:32px}</style><body><div id=a>A"
        "</div><div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_negative(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(-n+2){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_all(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(n){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_zero(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(0n+2){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_negative_constant(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(2n-1){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_no_positive(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(-n-1){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_last(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-last-child(2){color:red;width:32px}</style><body><div id=a>A"
        "</div><div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_last_even(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-last-child(even){color:red;width:32px}</style><body><div id=a>A"
        "</div><div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_type(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-of-type(2){color:red;width:32px}</style><body><div id=a>A</div>"
        "<span>gap</span><div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_type_last(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-last-of-type(1){color:red;width:32px}</style><body><div id=a>A"
        "</div><span>gap</span><div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_type_odd(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-of-type(odd){color:red;width:32px}</style><body><div id=a>A</div>"
        "<span>gap</span><div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_plus_n(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(+n+2){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_nth_upper_bound(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(160){color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_root_matches_body(void) {
    lot52_render("<style>body{margin:0}div{height:10px}:root{color:red}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_id_presence(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[id]{color:red;width:32px}</style><body><div id=a>A</div><div id=b>B"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_id_equal(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[id='a']{color:red;width:32px}</style><body><div id=a>A</div>"
        "<div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_class_token(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class~='ready']{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div><div id=b class='already'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_prefix(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class^='hot']{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div><div id=b class='cold hot'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_suffix(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class$='ready']{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div><div id=b class='ready hot'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_contains(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class*='ot r']{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div><div id=b class='cold'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_insensitive(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class='HOT READY' i]{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div><div id=b class='hot'>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_sensitive(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class='HOT']{color:red;width:32px}</style><body>"
        "<div id=a class='hot'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_two_attrs(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[id=a][class~=hot]{color:red;width:32px}</style><body>"
        "<div id=a class=hot>A</div><div id=b class=hot>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_comma_value(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class='a,b'],#b{color:red;width:32px}</style><body>"
        "<div id=a class='a,b'>A</div><div id=b>B</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_empty_prefix(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class^='']{color:red;width:32px}</style><body><div id=a class=hot>A"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_value_spaces(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[class='hot ready']{color:red;width:32px}</style><body>"
        "<div id=a class='hot ready'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_comma_groups(void) {
    lot52_render("<style>body{margin:0}div{height:10px}section>div,article>div{color:red;width:32px}</style><body><section>"
        "<div id=a>A</div></section><article><div id=b>B</div></article><div id=c>C</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("b")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("b")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->h);
    TEST_ASSERT_NOT_NULL(find_text("B"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("B")->color);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("c")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("c")->h);
    TEST_ASSERT_NOT_NULL(find_text("C"));
    TEST_ASSERT_EQUAL(0, find_text("C")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_important_beats_inline(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div{color:red!important;width:32px!important}</style><body>"
        "<div id=a class='hot ready' style='color:blue;width:48px'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_inline_important_wins(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div{color:red!important;width:32px!important}</style><body>"
        "<div id=a class='hot ready' style='color:blue!important;width:48px!important'>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0x0000ffU, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0x0000ffU, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_important_specificity(void) {
    lot52_render("<style>"
        "body{margin:0}div{height:10px}div{color:blue!important;width:48px!important}#a{color:red!important;width:32px!important}"
        "</style><body><div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_important_source_order(void) {
    lot52_render("<style>"
        "body{margin:0}div{height:10px}div{color:blue!important;width:48px!important}div{color:red!important;width:32px!important}"
        "</style><body><div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_important_lower_specificity(void) {
    lot52_render("<style>body{margin:0}div{height:10px}#a{color:blue;width:48px}div{color:red!important;width:32px!important}"
        "</style><body><div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_important_whitespace(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div{color:red ! important ;width:32px ! important}</style><body>"
        "<div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_important_case(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div{color:red!IMPORTANT;width:32px!IMPORTANT}</style><body>"
        "<div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_normal_later_does_not_win(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div{color:red!important;width:32px!important;color:blue;width:48px}"
        "</style><body><div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_class_specificity(void) {
    lot52_render("<style>body{margin:0}div{height:10px}.hot.ready{color:red;width:32px}div.hot{color:blue;width:48px}</style>"
        "<body><div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_type_specificity(void) {
    lot52_render("<style>body{margin:0}div{height:10px}body>div{color:red;width:32px}div{color:blue;width:48px}</style><body>"
        "<div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_pseudo_specificity(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(1){color:red;width:32px}div{color:blue;width:48px}</style>"
        "<body><div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_attr_specificity(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div[id=a]{color:red;width:32px}div{color:blue;width:48px}</style><body>"
        "<div id=a class='hot ready' style=''>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0xff0000U, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0xff0000U, find_text("A")->color);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l57_bad_selector_66(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:hover{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_67(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div::before{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_68(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:not(.x){color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_69(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(161){color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_70(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(2.5n){color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_71(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(n+-1){color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_72(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(2n+){color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_73(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(n n){color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_74(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(oddly){color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_75(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div:nth-child(){color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_76(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div>{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_77(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div++div{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_78(void) {
    lot52_render("<style>body{margin:0}div{height:10px}div||||div{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_79(void) {
    lot52_render("<style>body{margin:0}div{height:10px}.a.b.c.d{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_80(void) {
    lot52_render("<style>body{margin:0}div{height:10px}body main section article div{color:red;width:32px}</style><body>"
        "<div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_81(void) {
    lot52_render("<style>body{margin:0}div{height:10px}[unknown=value]{color:red;width:32px}</style><body><div id=a>A</div>"
        "</body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_82(void) {
    lot52_render("<style>body{margin:0}div{height:10px}[id|=a]{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_83(void) {
    lot52_render("<style>body{margin:0}div{height:10px}[id=a s]{color:red;width:32px}</style><body><div id=a>A</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}

static void test_l57_bad_selector_84(void) {
    lot52_render("<style>body{margin:0}div{height:10px}[id=a][class=x][href=x]{color:red;width:32px}</style><body><div id=a>A"
        "</div></body>", 320);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->color);
    TEST_ASSERT_EQUAL(320, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("A"));
    TEST_ASSERT_EQUAL(0, find_text("A")->color);
    TEST_ASSERT_TRUE(lot52_has_diag("css selector unsupported"));
    TEST_ASSERT_EQUAL(2, g_doc.nrules);
}
