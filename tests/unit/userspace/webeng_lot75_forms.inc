/* Lot 75: reader-view engine forms, type=file, multipart/form-data.
 * Host tests only; file bytes come from a local reader (no DNS). */
static we_forms_t g_l75f;
static unsigned char g_l75_file[WE_FFILE_MAX + 8];
static int g_l75_file_n;
static char g_l75_path[64];

static int l75_reader(void *ctx, const char *path, unsigned char *dst, int cap) {
    int i;
    (void)ctx;
    if (!path || !dst || cap <= 0) return -1;
    for (i = 0; g_l75_path[i] && path[i] && g_l75_path[i] == path[i]; i++) {}
    if (g_l75_path[i] || path[i]) return -1;
    if (g_l75_file_n > cap) return cap + 1;   /* reports truncated / too large */
    for (i = 0; i < g_l75_file_n && i < cap; i++) dst[i] = g_l75_file[i];
    return g_l75_file_n;
}

static const char *g_l75_html;

static void l75_render(const char *html) {
    we_forms_init(&g_l75f);
    we_set_forms(&g_doc, &g_l75f);
    we_set_image_reader(l75_reader, 0);
    g_l75_html = html;
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}

static void l75_done(void) {
    we_set_forms(&g_doc, 0);
    we_set_image_reader(0, 0);
    g_l75_html = 0;
}

static int l75_node(const char *id) {
    int i = we_find_id(&g_doc, id);
    return i > 0 ? i : 0;
}

static int l75_ctl(const char *id) { return g_doc.nodes[l75_node(id)].form_ord - 1; }

static void l75_reflow(void) {
    TEST_ASSERT_EQUAL(0, we_render(&g_doc, g_l75_html, (int)strlen(g_l75_html), 480));
}

static int l75_click(const char *id) {
    int i = l75_node(id), x, y, rc, c;
    for (c = g_doc.nodes[i].first; g_doc.nodes[i].w <= 0 && c >= 0; c = g_doc.nodes[c].next)
        if (g_doc.nodes[c].is_text && g_doc.nodes[c].w > 0) { i = c; break; }
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    rc = we_form_click(&g_doc, x, y);
    l75_reflow();
    return rc;
}

static int l75_tag_is(int i, const char *tag) {
    return strcmp(g_doc.nodes[i].tag, tag) == 0;
}

/* Lot 75: multipart and type=file */
static void test_l75_multipart_text_only(void) {
    we_form_req_t q;
    int i, x, y;
    l75_render("<form method=POST enctype='multipart/form-data' action='/up'>"
               "<input name=a value='x y'><input type=submit name=s value=Go></form>");
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.post);
    TEST_ASSERT_EQUAL(1, q.multipart);
    TEST_ASSERT_TRUE(q.boundary[0] != 0);
    TEST_ASSERT_NOT_NULL(strstr(q.ctype, "multipart/form-data; boundary="));
    TEST_ASSERT_NOT_NULL(strstr(q.ctype, q.boundary));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "name=\"a\""));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "x y"));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "name=\"s\""));
    TEST_ASSERT_TRUE(q.body_len > 40);
    /* body ends with closing boundary */
    TEST_ASSERT_NOT_NULL(strstr(q.body, q.boundary));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "--"));
    l75_done();
}

static void test_l75_file_bind_and_multipart(void) {
    we_form_req_t q;
    int i, x, y, ctl;
    const char *html =
        "<form method=POST enctype=multipart/form-data action=/upload>"
        "<input id=f type=file name=doc>"
        "<input name=note value=hi>"
        "<input id=s type=submit value=Send></form>";
    /* prepare a small VFS file */
    g_l75_file_n = 11;
    memcpy(g_l75_file, "hello-file!", 11);
    strcpy(g_l75_path, "/tmp/lot75.txt");
    l75_render(html);
    TEST_ASSERT_EQUAL(WE_FORM_FILE, g_doc.nodes[l75_node("f")].form_type);
    ctl = l75_ctl("f");
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "http://evil/x"));   /* URL refused */
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/../etc/passwd")); /* .. refused */
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "relative.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/lot75.txt"));
    l75_reflow();
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.multipart);
    TEST_ASSERT_NOT_NULL(strstr(q.body, "filename=\"lot75.txt\""));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "hello-file!"));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "name=\"note\""));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "hi"));
    l75_done();
}

static void test_l75_file_size_refused(void) {
    we_form_req_t q;
    int i, x, y, ctl, k;
    const char *html =
        "<form method=POST enctype=multipart/form-data action=/up>"
        "<input id=f type=file name=big><input id=s type=submit></form>";
    for (k = 0; k < WE_FFILE_MAX + 1; k++) g_l75_file[k] = (unsigned char)('A' + (k % 26));
    g_l75_file_n = WE_FFILE_MAX + 1;   /* reader reports cap+1 */
    strcpy(g_l75_path, "/tmp/big.bin");
    l75_render(html);
    ctl = l75_ctl("f");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/big.bin"));
    l75_reflow();
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(-1, we_form_request(&g_doc, &q));   /* too large / truncated */
    /* exact max size is accepted */
    g_l75_file_n = WE_FFILE_MAX;
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "ABCDEFG"));
    TEST_ASSERT_TRUE(q.body_len > WE_FFILE_MAX);
    l75_done();
}

static void test_l75_file_without_multipart_skipped(void) {
    we_form_req_t q;
    int i, x, y, ctl;
    /* urlencoded form: file control is not sent */
    l75_render("<form method=POST action=/p><input id=f type=file name=f>"
               "<input name=a value=1><input id=s type=submit></form>");
    ctl = l75_ctl("f");
    strcpy(g_l75_path, "/tmp/x");
    g_l75_file_n = 3; memcpy(g_l75_file, "abc", 3);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/x"));
    l75_reflow();
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(0, q.multipart);
    TEST_ASSERT_EQUAL_STRING("a=1", q.body);
    l75_done();
}

static void lot75_forms_run(void) {
    RUN_TEST(test_l75_multipart_text_only);
    RUN_TEST(test_l75_file_bind_and_multipart);
    RUN_TEST(test_l75_file_size_refused);
    RUN_TEST(test_l75_file_without_multipart_skipped);
}
