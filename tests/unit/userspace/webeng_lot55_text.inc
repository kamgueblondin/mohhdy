/* Fixed-font word flow and bounded text publication; no new font engine. */
static void test_l55_text_nowrap_overflows_on_one_line(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:nowrap'>one two three</div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("one two three"));
    TEST_ASSERT_EQUAL(104, find_text("one two three")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_normal_resets_nowrap(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:nowrap;white-space:normal'>one two three</div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("one"));
    TEST_ASSERT_NOT_NULL(find_text("two"));
    TEST_ASSERT_NOT_NULL(find_text("three"));
    TEST_ASSERT_EQUAL(11, find_text("two")->y);
    TEST_ASSERT_EQUAL(21, find_text("three")->y);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_nowrap_inherits_through_span(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:nowrap'>one <span id=s>two three</span></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("s")->nowrap);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(32, find_text("two three")->x);
    TEST_ASSERT_EQUAL(1, find_text("two three")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_nowrap_br_still_breaks(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:nowrap'>one two<br>three four</div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("one two"));
    TEST_ASSERT_NOT_NULL(find_text("three four"));
    TEST_ASSERT_EQUAL(1, find_text("one two")->y);
    TEST_ASSERT_EQUAL(11, find_text("three four")->y);
    TEST_ASSERT_EQUAL(0, find_text("three four")->x);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
}

static void test_l55_text_invalid_white_space_keeps_nowrap(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;white-space:nowrap;white-space:pre'>one two</div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->nowrap);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("one two"));
    TEST_ASSERT_TRUE(lot52_has_diag("white space refused"));
}

static void test_l55_text_break_word_splits_long_word(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:break-word'>abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(1, find_text("abc")->y);
    TEST_ASSERT_EQUAL(11, find_text("def")->y);
    TEST_ASSERT_EQUAL(21, find_text("gh")->y);
    TEST_ASSERT_EQUAL(24, find_text("abc")->w);
    TEST_ASSERT_EQUAL(16, find_text("gh")->w);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
}

static void test_l55_text_break_word_keeps_normal_word_boundary(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:40px;overflow-wrap:break-word'>z abcdef</div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("z"));
    TEST_ASSERT_NOT_NULL(find_text("abcde"));
    TEST_ASSERT_NOT_NULL(find_text("f"));
    TEST_ASSERT_EQUAL(1, find_text("z")->y);
    TEST_ASSERT_EQUAL(11, find_text("abcde")->y);
    TEST_ASSERT_EQUAL(21, find_text("f")->y);
    TEST_ASSERT_EQUAL(0, find_text("abcde")->x);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
}

static void test_l55_text_break_word_exact_width_not_split(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:break-word'>abc</div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_EQUAL(24, find_text("abc")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_break_word_minimum_character_width(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:1px;overflow-wrap:break-word'>abc</div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->w);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("a"));
    TEST_ASSERT_NOT_NULL(find_text("b"));
    TEST_ASSERT_NOT_NULL(find_text("c"));
    TEST_ASSERT_EQUAL(1, find_text("a")->y);
    TEST_ASSERT_EQUAL(11, find_text("b")->y);
    TEST_ASSERT_EQUAL(21, find_text("c")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_nowrap_overrides_break_word(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:break-word;white-space:nowrap'>abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(64, find_text("abcdefgh")->w);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->nowrap);
    TEST_ASSERT_EQUAL(1, lot52_node("a")->break_word);
}

static void test_l55_text_invalid_wrap_preserves_break_word(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:break-word;overflow-wrap:anywhere'>abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(1, lot52_node("a")->break_word);
    TEST_ASSERT_TRUE(lot52_has_diag("word wrapping refused"));
}

static void test_l55_text_normal_wrap_replaces_break_word(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:break-word;overflow-wrap:normal'>abcdefgh</div></body>", 320);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_EQUAL(64, find_text("abcdefgh")->w);
    TEST_ASSERT_EQUAL(0, lot52_node("a")->break_word);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l55_text_break_word_inherits(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:24px;overflow-wrap:break-word'>"
        "<span id=s>abcdefgh</span></div></body>", 320);
    TEST_ASSERT_EQUAL(1, lot52_node("s")->break_word);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abc"));
    TEST_ASSERT_NOT_NULL(find_text("def"));
    TEST_ASSERT_NOT_NULL(find_text("gh"));
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_break_word_column_intrinsic_height_matches_paint(void) {
    lot52_render("<body style='margin:0'><div id=p style='display:flex;flex-direction:column;width:40px'>"
        "<div id=a style='flex-basis:auto;overflow-wrap:break-word'>z abcdef</div>"
        "<div id=b style='flex-basis:auto'>end</div></div></body>", 320);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(30, lot52_node("b")->y);
    TEST_ASSERT_EQUAL(40, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(11, find_text("abcde")->y);
    TEST_ASSERT_EQUAL(21, find_text("f")->y);
    TEST_ASSERT_EQUAL(31, find_text("end")->y);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_white_gap_between_inline_elements(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'>"
        "<span>one</span> <span>two</span></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(24, lot52_node("p")->min_content);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->max_content);
    TEST_ASSERT_EQUAL(0, find_text("one")->x);
    TEST_ASSERT_EQUAL(32, find_text("two")->x);
    TEST_ASSERT_EQUAL(1, find_text("two")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l55_text_collapsed_gap_does_not_multiply(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'>"
        "<span>one</span> \n\t  <b>two</b></div></body>", 320);
    TEST_ASSERT_EQUAL(56, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(32, find_text("two")->x);
    TEST_ASSERT_EQUAL(1, find_text("two")->y);
    TEST_ASSERT_EQUAL(1, find_text("two")->bold);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_inline_gap_wraps_at_word_boundary(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:40px'>"
        "<span>one</span> <span>two</span></div></body>", 320);
    TEST_ASSERT_EQUAL(20, lot52_node("p")->h);
    TEST_ASSERT_EQUAL(0, find_text("one")->x);
    TEST_ASSERT_EQUAL(0, find_text("two")->x);
    TEST_ASSERT_EQUAL(1, find_text("one")->y);
    TEST_ASSERT_EQUAL(11, find_text("two")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
}

static void test_l55_text_inline_without_gap_stays_adjacent(void) {
    lot52_render("<body style='margin:0'><div id=p style='width:max-content'>"
        "<span>one</span><span>two</span></div></body>", 320);
    TEST_ASSERT_EQUAL(48, lot52_node("p")->w);
    TEST_ASSERT_EQUAL(48, lot52_node("p")->min_content);
    TEST_ASSERT_EQUAL(24, find_text("two")->x);
    TEST_ASSERT_EQUAL(1, find_text("two")->y);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
}

static void test_l55_text_long_word_publishes_all_characters(void) {
    char html[800], word[201];
    unsigned i, total = 0;
    memset(word, 'x', 200); word[200] = 0;
    snprintf(html, sizeof(html), "<body style='margin:0'><div id=a>%s</div></body>", word);
    lot52_render(html, 320);
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_TEXT) {
        TEST_ASSERT_TRUE(strlen(g_doc.items[i].text) < OS_FB_WEB_TEXT_LEN);
        TEST_ASSERT_EQUAL((int)total * WE_CHAR_W, g_doc.items[i].x);
        TEST_ASSERT_EQUAL(1, g_doc.items[i].y);
        total += (unsigned)strlen(g_doc.items[i].text);
    }
    TEST_ASSERT_EQUAL(200, total);
    TEST_ASSERT_EQUAL(10, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(1, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_long_word_breaks_without_losing_characters(void) {
    char html[900], word[301];
    unsigned i, total = 0, rows = 0;
    int previous = -1;
    memset(word, 'x', 300); word[300] = 0;
    snprintf(html, sizeof(html), "<body style='margin:0'><div id=a style='width:800px;overflow-wrap:break-word'>%s</div></body>", word);
    lot52_render(html, 1000);
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_TEXT) {
        TEST_ASSERT_TRUE(g_doc.items[i].x + g_doc.items[i].w <= 800);
        TEST_ASSERT_TRUE(strlen(g_doc.items[i].text) < OS_FB_WEB_TEXT_LEN);
        total += (unsigned)strlen(g_doc.items[i].text);
        if (g_doc.items[i].y != previous) { rows++; previous = g_doc.items[i].y; }
    }
    TEST_ASSERT_EQUAL(300, total);
    TEST_ASSERT_EQUAL(3, rows);
    TEST_ASSERT_EQUAL(30, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(3, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_source_cap_is_reported(void) {
    char html[900], word[601];
    unsigned i, total = 0;
    memset(word, 'x', 600); word[600] = 0;
    snprintf(html, sizeof(html), "<body style='margin:0'><div>%s</div></body>", word);
    lot52_render(html, 320);
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == OS_FB_WEB_TEXT)
        total += (unsigned)strlen(g_doc.items[i].text);
    TEST_ASSERT_EQUAL(WE_TEXT_SOURCE_MAX - 1, total);
    TEST_ASSERT_TRUE(lot52_has_diag("text source exceeds limit"));
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
}

static void test_l55_text_nowrap_word_workspace_cap_is_reported(void) {
    char html[500];
    unsigned i;
    strcpy(html, "<body style='margin:0'><div id=a style='white-space:nowrap'>");
    for (i = 0; i < 70; i++) strcat(html, "x ");
    strcat(html, "</div></body>");
    lot52_render(html, 2000);
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_TRUE(lot52_has_diag("inline word capacity"));
    TEST_ASSERT_TRUE(g_doc.nitems <= OS_FB_WEB_ITEMS);
    TEST_ASSERT_EQUAL(0, g_doc.perf.items_dropped);
}

static void test_l55_text_long_link_has_hits_across_fragments(void) {
    char html[600], word[201], href[32];
    memset(word, 'x', 200); word[200] = 0;
    snprintf(html, sizeof(html), "<body style='margin:0'><div><a href='/next'>%s</a></div></body>", word);
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 8, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, strcmp(href, "/next"));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 800, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(5, we_link_at(&g_doc, 1599, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, we_link_at(&g_doc, 1600, 4, href, sizeof(href)));
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l55_text_break_word_relayout_reflows(void) {
    lot52_render("<body style='margin:0'><div id=a style='width:50%;overflow-wrap:break-word'>abcdefghijklmnop</div></body>", 64);
    TEST_ASSERT_EQUAL(40, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abcd"));
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, 128));
    TEST_ASSERT_EQUAL(20, lot52_node("a")->h);
    TEST_ASSERT_NOT_NULL(find_text("abcdefgh"));
    TEST_ASSERT_NOT_NULL(find_text("ijklmnop"));
    TEST_ASSERT_EQUAL(11, find_text("ijklmnop")->y);
    TEST_ASSERT_EQUAL(2, g_doc.perf.lines);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}
