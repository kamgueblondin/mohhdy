/* Lot 77: richer type=file UX (accept, multiple, clear/replace, paint).
 * Host tests only; bytes from a local image reader (no DNS). */
/* Flag values match userspace/webeng_forms.inc (not exported on we_node_t). */
enum { L77_FF_MULTIPLE = 16, L77_FF2_ACCEPT = 4194304 };
static we_forms_t g_l77f;
static unsigned char g_l77_file[WE_FFILE_MAX + 8];
static int g_l77_file_n;
static const char *g_l77_html;

static int l77_reader(void *ctx, const char *path, unsigned char *dst, int cap) {
    int i;
    (void)ctx;
    if (!path || path[0] != '/' || !dst || cap <= 0) return -1;
    if (g_l77_file_n > cap) return cap + 1;
    for (i = 0; i < g_l77_file_n && i < cap; i++) dst[i] = g_l77_file[i];
    return g_l77_file_n;
}

static void l77_render(const char *html) {
    we_forms_init(&g_l77f);
    we_set_forms(&g_doc, &g_l77f);
    we_set_image_reader(l77_reader, 0);
    g_l77_html = html;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}

static void l77_done(void) {
    we_set_forms(&g_doc, 0);
    we_set_image_reader(0, 0);
    g_l77_html = 0;
}

static void l77_reflow(void) {
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, g_l77_html, (int)strlen(g_l77_html), 480));
}

static int l77_node(const char *id) {
    int i = we_find_id(&g_doc, id);
    return i > 0 ? i : 0;
}


static int l77_ctl(const char *id) { return g_doc.nodes[l77_node(id)].form_ord - 1; }

static int l77_paint_has(const char *id, const char *want) {
    int c, wlen = (int)strlen(want);
    for (c = g_doc.nodes[l77_node(id)].first; c >= 0; c = g_doc.nodes[c].next) {
        if (g_doc.nodes[c].is_text && (int)g_doc.nodes[c].text_len == wlen) {
            const char *tx = g_doc.pool + g_doc.nodes[c].text_off;
            int k, ok = 1;
            for (k = 0; k < wlen; k++) if (tx[k] != want[k]) { ok = 0; break; }
            if (ok) return 1;
        }
    }
    return 0;
}

static void test_l77_accept_filters_extension(void) {
    int ctl;
    l77_render("<form method=POST enctype=multipart/form-data action=/u>"
               "<input id=f type=file name=doc accept='.txt,.png'>"
               "<input id=s type=submit></form>");
    ctl = l77_ctl("f");
    TEST_ASSERT_TRUE(g_doc.nodes[l77_node("f")].form_flags2 & L77_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/note.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/note.pdf"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/pic.PNG"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    {
        char out[64];
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, 0, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, "pic.PNG"));
    }
    l77_done();
}

static void test_l77_accept_mime_group(void) {
    int ctl;
    l77_render("<form method=POST enctype=multipart/form-data action=/u>"
               "<input id=f type=file name=doc accept='image/*'>"
               "<input id=s type=submit></form>");
    ctl = l77_ctl("f");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/a.jpg"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/a.txt"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77_done();
}

static void test_l77_multiple_append_and_paint(void) {
    int ctl;
    g_l77_file_n = 4; memcpy(g_l77_file, "abcd", 4);
    l77_render("<form method=POST enctype=multipart/form-data action=/u>"
               "<input id=f type=file name=doc multiple>"
               "<input id=s type=submit></form>");
    ctl = l77_ctl("f");
    TEST_ASSERT_TRUE(g_doc.nodes[l77_node("f")].form_flags & L77_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/a.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/b.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/c.txt"));
    TEST_ASSERT_EQUAL(3, we_form_file_count(&g_doc, ctl));
    l77_reflow();
    TEST_ASSERT_EQUAL(1, l77_paint_has("f", "a.txt +2"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/d.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/e.txt"));
    l77_done();
}

static void test_l77_multipart_emits_each_path(void) {
    we_form_req_t q;
    int ctl, i, x, y;
    g_l77_file_n = 3; memcpy(g_l77_file, "xyz", 3);
    l77_render("<form method=POST enctype=multipart/form-data action=/u>"
               "<input id=f type=file name=doc multiple>"
               "<input id=s type=submit value=Go></form>");
    ctl = l77_ctl("f");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/one.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/two.txt"));
    l77_reflow();
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "filename=\"one.txt\""));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "filename=\"two.txt\""));
    l77_done();
}

static void test_l77_clear_and_escape(void) {
    int ctl;
    l77_render("<form method=POST enctype=multipart/form-data action=/u>"
               "<input id=f type=file name=doc>"
               "<input id=s type=submit></form>");
    ctl = l77_ctl("f");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/z.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    g_l77f.focus = (int16_t)ctl;
    TEST_ASSERT_EQUAL(WE_FACT_CHANGED, we_form_key(&g_doc, WE_KEY_ESC));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77_reflow();
    TEST_ASSERT_EQUAL(1, l77_paint_has("f", "Browse..."));
    l77_done();
}

static void test_l77_replace_without_multiple(void) {
    int ctl;
    char out[64];
    l77_render("<form><input id=f type=file name=doc></form>");
    ctl = l77_ctl("f");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/first.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/second.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, 0, out, 64) > 0);
    TEST_ASSERT_NOT_NULL(strstr(out, "second.txt"));
    l77_done();
}

static void test_l77_doc_under_1mib(void) {
    TEST_ASSERT_TRUE(sizeof(we_doc_t) <= WE_DOCUMENT_MAX);
}

static void lot77_file_run(void) {
    RUN_TEST(test_l77_accept_filters_extension);
    RUN_TEST(test_l77_accept_mime_group);
    RUN_TEST(test_l77_multiple_append_and_paint);
    RUN_TEST(test_l77_multipart_emits_each_path);
    RUN_TEST(test_l77_clear_and_escape);
    RUN_TEST(test_l77_replace_without_multiple);
    RUN_TEST(test_l77_doc_under_1mib);
}
