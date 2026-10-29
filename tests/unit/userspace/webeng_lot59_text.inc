static void test_l59_text_anywhere_long(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:anywhere'>abcdefgh</div><div id=b s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_long(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;word-break:break-all'>abcdefgh</div><div id=b sty"
        "le='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_word_old(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:break-word'>abcdefgh</div><div id=b"
        " style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_exact(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:64px;overflow-wrap:anywhere'>abcdefgh</div><div id=b s"
        "tyle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(0, find_text("abcdefgh")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcdefgh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_exact(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:64px;word-break:break-all'>abcdefgh</div><div id=b sty"
        "le='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(0, find_text("abcdefgh")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcdefgh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_narrow(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:8px;overflow-wrap:anywhere'>abcdefgh</div><div id=b st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(8, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("a"));
    TEST_ASSERT_EQUAL(0, find_text("a")->x);
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_NOT_NULL(find_text("b"));
    TEST_ASSERT_EQUAL(0, find_text("b")->x);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("c"));
    TEST_ASSERT_EQUAL(0, find_text("c")->x);
    TEST_ASSERT_EQUAL(21, find_text("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("d"));
    TEST_ASSERT_EQUAL(0, find_text("d")->x);
    TEST_ASSERT_EQUAL(31, find_text("d")->y);
    TEST_ASSERT_NOT_NULL(find_text("e"));
    TEST_ASSERT_EQUAL(0, find_text("e")->x);
    TEST_ASSERT_EQUAL(41, find_text("e")->y);
    TEST_ASSERT_NOT_NULL(find_text("f"));
    TEST_ASSERT_EQUAL(0, find_text("f")->x);
    TEST_ASSERT_EQUAL(51, find_text("f")->y);
    TEST_ASSERT_NOT_NULL(find_text("g"));
    TEST_ASSERT_EQUAL(0, find_text("g")->x);
    TEST_ASSERT_EQUAL(61, find_text("g")->y);
    TEST_ASSERT_NOT_NULL(find_text("h"));
    TEST_ASSERT_EQUAL(0, find_text("h")->x);
    TEST_ASSERT_EQUAL(71, find_text("h")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_subglyph(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:4px;overflow-wrap:anywhere'>abcdefgh</div><div id=b st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(4, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(80, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(80, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(8, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("a"));
    TEST_ASSERT_EQUAL(0, find_text("a")->x);
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_NOT_NULL(find_text("b"));
    TEST_ASSERT_EQUAL(0, find_text("b")->x);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("c"));
    TEST_ASSERT_EQUAL(0, find_text("c")->x);
    TEST_ASSERT_EQUAL(21, find_text("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("d"));
    TEST_ASSERT_EQUAL(0, find_text("d")->x);
    TEST_ASSERT_EQUAL(31, find_text("d")->y);
    TEST_ASSERT_NOT_NULL(find_text("e"));
    TEST_ASSERT_EQUAL(0, find_text("e")->x);
    TEST_ASSERT_EQUAL(41, find_text("e")->y);
    TEST_ASSERT_NOT_NULL(find_text("f"));
    TEST_ASSERT_EQUAL(0, find_text("f")->x);
    TEST_ASSERT_EQUAL(51, find_text("f")->y);
    TEST_ASSERT_NOT_NULL(find_text("g"));
    TEST_ASSERT_EQUAL(0, find_text("g")->x);
    TEST_ASSERT_EQUAL(61, find_text("g")->y);
    TEST_ASSERT_NOT_NULL(find_text("h"));
    TEST_ASSERT_EQUAL(0, find_text("h")->x);
    TEST_ASSERT_EQUAL(71, find_text("h")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_nowrap_wins_anywhere(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:nowrap;overflow-wrap:anywhere'>abcdef"
        "gh</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(0, find_text("abcdefgh")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcdefgh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_pre_wins_break_all(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:pre;word-break:break-all'>abcdefgh</d"
        "iv><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(0, find_text("abcdefgh")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcdefgh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_reset_break_all(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;word-break:break-all;word-break:normal'>abcdefgh<"
        "/div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(0, find_text("abcdefgh")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcdefgh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_reset_anywhere(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:anywhere;overflow-wrap:normal'>abcd"
        "efgh</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(0, find_text("abcdefgh")->x);
    TEST_ASSERT_EQUAL(1, find_text("abcdefgh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_word_replaces_anywhere(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:anywhere;overflow-wrap:break-word'>"
        "abcdefgh</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_independent(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;word-break:break-all;overflow-wrap:normal'>abcdef"
        "gh</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_independent(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:anywhere;word-break:normal'>abcdefg"
        "h</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_partial(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:40px;word-break:break-all'>ab cdef</div><div id=b styl"
        "e='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("ab cd"));
    TEST_ASSERT_EQUAL(0, find_text("ab cd")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab cd")->y);
    TEST_ASSERT_NOT_NULL(find_text("ef"));
    TEST_ASSERT_EQUAL(0, find_text("ef")->x);
    TEST_ASSERT_EQUAL(11, find_text("ef")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_word_fits_next(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:40px;overflow-wrap:anywhere'>ab cdef</div><div id=b st"
        "yle='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("ab"));
    TEST_ASSERT_EQUAL(0, find_text("ab")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_NOT_NULL(find_text("cdef"));
    TEST_ASSERT_EQUAL(0, find_text("cdef")->x);
    TEST_ASSERT_EQUAL(11, find_text("cdef")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_space_exact(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:40px;word-break:break-all'>ab cd</div><div id=b style="
        "'height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("ab cd"));
    TEST_ASSERT_EQUAL(0, find_text("ab cd")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab cd")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_space_dropped(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;word-break:break-all'>abc de</div><div id=b style"
        "='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(48, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("de"));
    TEST_ASSERT_EQUAL(0, find_text("de")->x);
    TEST_ASSERT_EQUAL(11, find_text("de")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_preline(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:pre-line;overflow-wrap:anywhere'>ab\nc"
        "def</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("ab"));
    TEST_ASSERT_EQUAL(0, find_text("ab")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_NOT_NULL(find_text("cde"));
    TEST_ASSERT_EQUAL(0, find_text("cde")->x);
    TEST_ASSERT_EQUAL(11, find_text("cde")->y);
    TEST_ASSERT_NOT_NULL(find_text("f"));
    TEST_ASSERT_EQUAL(0, find_text("f")->x);
    TEST_ASSERT_EQUAL(21, find_text("f")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_preline(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:pre-line;word-break:break-all'>ab\ncde"
        "f</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("ab"));
    TEST_ASSERT_EQUAL(0, find_text("ab")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab")->y);
    TEST_ASSERT_NOT_NULL(find_text("cde"));
    TEST_ASSERT_EQUAL(0, find_text("cde")->x);
    TEST_ASSERT_EQUAL(11, find_text("cde")->y);
    TEST_ASSERT_NOT_NULL(find_text("f"));
    TEST_ASSERT_EQUAL(0, find_text("f")->x);
    TEST_ASSERT_EQUAL(21, find_text("f")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_line_height(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;line-height:20px;word-break:break-all'>abcdefgh</"
        "div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(21, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(41, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_line_height(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;line-height:20px;overflow-wrap:anywhere'>abcdefgh"
        "</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(60, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(60, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(21, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(41, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_min_content(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:8px;width:min-content;overflow-wrap:anywhere'>abcd</di"
        "v><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(4, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("a"));
    TEST_ASSERT_EQUAL(0, find_text("a")->x);
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_NOT_NULL(find_text("b"));
    TEST_ASSERT_EQUAL(0, find_text("b")->x);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("c"));
    TEST_ASSERT_EQUAL(0, find_text("c")->x);
    TEST_ASSERT_EQUAL(21, find_text("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("d"));
    TEST_ASSERT_EQUAL(0, find_text("d")->x);
    TEST_ASSERT_EQUAL(31, find_text("d")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_min_content(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:8px;width:min-content;word-break:break-all'>abcd</div>"
        "<div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(32, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(40, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(4, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("a"));
    TEST_ASSERT_EQUAL(0, find_text("a")->x);
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_NOT_NULL(find_text("b"));
    TEST_ASSERT_EQUAL(0, find_text("b")->x);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_NOT_NULL(find_text("c"));
    TEST_ASSERT_EQUAL(0, find_text("c")->x);
    TEST_ASSERT_EQUAL(21, find_text("c")->y);
    TEST_ASSERT_NOT_NULL(find_text("d"));
    TEST_ASSERT_EQUAL(0, find_text("d")->x);
    TEST_ASSERT_EQUAL(31, find_text("d")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_max_content(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:56px;width:max-content;word-break:break-all'>ab cdef</"
        "div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(10, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("ab cdef"));
    TEST_ASSERT_EQUAL(0, find_text("ab cdef")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab cdef")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_anywhere_fit_content(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;width:fit-content(24px);overflow-wrap:anywhere'>a"
        "bcdefgh</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(64, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(0, find_text("abc")->x);
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_EQUAL(0, find_text("def")->x);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(0, find_text("gh")->x);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l59_text_break_all_fit_content(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:40px;width:fit-content(40px);word-break:break-all'>ab "
        "cdef</div><div id=b style='height:5px'></div></body>", 320);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(8, lot52_node("a")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("a")->max_content);
    TEST_ASSERT_EQUAL(20, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_NOT_NULL(find_text("ab cd"));
    TEST_ASSERT_EQUAL(0, find_text("ab cd")->x);
    TEST_ASSERT_EQUAL(1, find_text("ab cd")->y);
    TEST_ASSERT_NOT_NULL(find_text("ef"));
    TEST_ASSERT_EQUAL(0, find_text("ef")->x);
    TEST_ASSERT_EQUAL(11, find_text("ef")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
