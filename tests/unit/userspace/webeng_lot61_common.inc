/* Lot 61 helpers: geometry, paint and diagnostics of the one guest engine. */
static const os_fb_web_item_t *l61_rect(uint32_t color) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == color) return &g_doc.items[i];
    unity_test_fail("missing rectangle", __FILE__, __LINE__);
    return &g_doc.items[0];
}

static int l61_index(uint32_t color) {
    uint32_t i;
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_RECT && g_doc.items[i].color == color) return (int)i;
    return -1;
}

static void l61_box(const char *id, int x, int y, int w, int h) {
    const we_node_t *n = lot52_node(id);
    TEST_ASSERT_EQUAL(x, n->x);
    TEST_ASSERT_EQUAL(y, n->y);
    TEST_ASSERT_EQUAL(w, n->w);
    TEST_ASSERT_EQUAL(h, n->h);
}

static void l61_rect_is(uint32_t color, int x, int y, int w, int h) {
    const os_fb_web_item_t *it = l61_rect(color);
    TEST_ASSERT_EQUAL(x, it->x);
    TEST_ASSERT_EQUAL(y, it->y);
    TEST_ASSERT_EQUAL(w, it->w);
    TEST_ASSERT_EQUAL(h, it->h);
}

static void l61_text_at(const char *text, int x, int y) {
    const os_fb_web_item_t *it = find_text(text);
    TEST_ASSERT_NOT_NULL(it);
    if (!it) return;
    TEST_ASSERT_EQUAL(x, it->x);
    TEST_ASSERT_EQUAL(y, it->y);
}

static int l61_count(uint8_t kind) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.items[i].kind == kind) count++;
    return count;
}

static int l61_diag_count(const char *fragment) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], fragment)) count++;
    return count;
}
