/* Lot 72: glyph pages in the web engine. The source (wf_html output) is in
 * display bytes with page selectors (include/os_charset.h); the engine
 * removes the selectors (no cell), keeps one page per text node, splits a
 * chunk where the page of its high bytes changes, and every TEXT item
 * carries its node's page in pad bits 8..10 next to the font scale. */
#define L72(body) "<style>body{margin:0}p{margin:0}</style><body>" body "</body>"

static const os_fb_web_item_t* l72_text_starting(const char* t) {
    uint32_t i;
    size_t n = strlen(t);
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && strncmp(g_doc.items[i].text, t, n) == 0) return &g_doc.items[i];
    return 0;
}

static void test_l72_greek_item_page(void) {
    static const char* p = L72("<p>\x8f\xc5\xeb\xeb\xe7\xed\xe9\xea\xdc \xe1\xe2\xe3</p>");
    const os_fb_web_item_t* it;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 400));
    it = find_text("\xc5\xeb\xeb\xe7\xed\xe9\xea\xdc \xe1\xe2\xe3");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL(2, (it->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(0, it->pad & 0xFF);      /* 8x8 cell: no scale */
    TEST_ASSERT_EQUAL(12 * 8, it->w);           /* one cell per letter, the selector none */
}

static void test_l72_cyrillic_heading_keeps_scale(void) {
    static const char* p = L72("<h1>\x90\xc0\xe3\xe1\xe1\xda\xd8\xd9</h1>");
    const os_fb_web_item_t* it;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 400));
    it = find_text("\xc0\xe3\xe1\xe1\xda\xd8\xd9");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL(3, (it->pad >> 8) & 7);
    TEST_ASSERT_TRUE((it->pad & 0xFF) > 4);     /* h1 scale still there */
}

static void test_l72_page_change_splits_node(void) {
    /* French then Russian then French in one paragraph */
    static const char* p = L72("<p>caf\xe9 \x90\xbc\xde\xe1\xda\xd2\xd0\x81 \xe9t\xe9</p>");
    const os_fb_web_item_t *a, *b, *c;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 400));
    a = l72_text_starting("caf\xe9");
    b = l72_text_starting("\xbc\xde\xe1\xda\xd2\xd0");
    c = l72_text_starting("\xe9t\xe9");
    if (!c) c = l72_text_starting(" \xe9t\xe9");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL(0, (a->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(3, (b->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(0, (c->pad >> 8) & 7);
    TEST_ASSERT_TRUE(b->x > a->x);              /* one line, in order */
    TEST_ASSERT_EQUAL(a->y, b->y);
    /* no selector byte reaches an item */
    {
        uint32_t i;
        int k;
        for (i = 0; i < g_doc.nitems; i++)
            for (k = 0; g_doc.items[i].text[k]; k++) TEST_ASSERT_TRUE(os_page_of_sel((unsigned char)g_doc.items[i].text[k]) < 0);
    }
}

static void test_l72_ascii_does_not_split(void) {
    /* digits and punctuation are in every page: one node */
    static const char* p = L72("<p>\x8f\xe1\xe2 12, \xe3!</p>");
    const os_fb_web_item_t* it;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 400));
    it = find_text("\xe1\xe2 12, \xe3!");
    TEST_ASSERT_NOT_NULL(it);
    TEST_ASSERT_EQUAL(2, (it->pad >> 8) & 7);
}

static void test_l72_numeric_references(void) {
    /* &#945; (alpha) and &#x416; (Zhe) arrive as references */
    static const char* p = L72("<p>&#945;&#946;</p><p>&#x416;</p><p>&#233;</p>");
    const os_fb_web_item_t *a, *z, *e;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 400));
    a = find_text("\xe1\xe2");
    z = find_text("\xb6");
    e = find_text("\xe9");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(z);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(2, (a->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(3, (z->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(0, (e->pad >> 8) & 7);
}

static void test_l72_whitespace_chunk_unchanged(void) {
    /* a selector alone between elements is not text */
    static const char* p = L72("<p>a</p>\x8f<p>b</p>");
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 400));
    TEST_ASSERT_NOT_NULL(find_text("a"));
    TEST_ASSERT_NOT_NULL(find_text("b"));
}

static void test_l72_named_references(void) {
    /* names from the shared HTML 4 table: Greek (case-sensitive), arrows,
     * math, Latin-1; an unknown name stays as written */
    static const char* p = L72("<p>&alpha;&Omega;</p><p>&rarr;&le;</p><p>&eacute;t&eacute;</p><p>&Alpha;</p><p>&bogus;x</p>");
    const os_fb_web_item_t *g, *s, *e, *A;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, p, (int)strlen(p), 400));
    g = find_text("\xe1\xd9");
    s = find_text("\x83\xa2");
    e = find_text("\xe9t\xe9");
    A = find_text("\xc1");
    TEST_ASSERT_NOT_NULL(g);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(A);
    TEST_ASSERT_EQUAL(2, (g->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(4, (s->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(0, (e->pad >> 8) & 7);
    TEST_ASSERT_EQUAL(2, (A->pad >> 8) & 7);
    TEST_ASSERT_NOT_NULL(find_text("&bogus;x"));
    /* a prefix of a name is not that name */
    {
        static const char* q = L72("<p>&rar;&alph;</p>");
        TEST_ASSERT_EQUAL(0, we_render(&g_doc, q, (int)strlen(q), 400));
        TEST_ASSERT_NOT_NULL(find_text("&rar;&alph;"));
    }
}

static void lot72_run(void) {
    RUN_TEST(test_l72_greek_item_page);
    RUN_TEST(test_l72_cyrillic_heading_keeps_scale);
    RUN_TEST(test_l72_page_change_splits_node);
    RUN_TEST(test_l72_ascii_does_not_split);
    RUN_TEST(test_l72_numeric_references);
    RUN_TEST(test_l72_named_references);
    RUN_TEST(test_l72_whitespace_chunk_unchanged);
}
