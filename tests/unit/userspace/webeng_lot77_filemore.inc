/* Lot 77: systematic type=file accept/multiple edge proofs. */
enum { L77M_FF_MULTIPLE = 16, L77M_FF2_ACCEPT = 4194304 };
static we_forms_t g_l77m;
static unsigned char g_l77m_file[64];
static int g_l77m_file_n;
static const char *g_l77m_html;
static int l77m_reader(void *ctx, const char *path, unsigned char *dst, int cap) {
    int i; (void)ctx; if (!path || path[0] != '/' || !dst || cap <= 0) return -1;
    if (g_l77m_file_n > cap) return cap + 1;
    for (i = 0; i < g_l77m_file_n && i < cap; i++) dst[i] = g_l77m_file[i];
    return g_l77m_file_n;
}
static void l77m_render(const char *html) {
    we_forms_init(&g_l77m); we_set_forms(&g_doc, &g_l77m); we_set_image_reader(l77m_reader, 0);
    g_l77m_html = html; TEST_ASSERT_EQUAL(0, we_render(&g_doc, html, (int)strlen(html), 480));
}
static void l77m_done(void) { we_set_forms(&g_doc, 0); we_set_image_reader(0, 0); g_l77m_html = 0; }
static int l77m_node(const char *id) { int i = we_find_id(&g_doc, id); return i > 0 ? i : 0; }
static int l77m_ctl(const char *id) { return g_doc.nodes[l77m_node(id)].form_ord - 1; }

static void test_l77m_accept_1(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a1>"
               "<input id=f1 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f1");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f1")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok1.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad1.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_2(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a2>"
               "<input id=f2 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f2");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f2")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok2.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad2.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_3(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a3>"
               "<input id=f3 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f3");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f3")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok3.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad3.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_4(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a4>"
               "<input id=f4 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f4");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f4")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok4.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad4.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_5(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a5>"
               "<input id=f5 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f5");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f5")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok5.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad5.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_6(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a6>"
               "<input id=f6 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f6");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f6")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok6.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad6.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_7(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a7>"
               "<input id=f7 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f7");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f7")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok7.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad7.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_8(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a8>"
               "<input id=f8 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f8");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f8")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok8.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad8.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_9(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a9>"
               "<input id=f9 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f9");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f9")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok9.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad9.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_10(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a10>"
               "<input id=f10 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f10");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f10")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok10.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad10.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_11(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a11>"
               "<input id=f11 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f11");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f11")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok11.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad11.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_12(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a12>"
               "<input id=f12 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f12");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f12")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok12.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad12.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_13(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a13>"
               "<input id=f13 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f13");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f13")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok13.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad13.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_14(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a14>"
               "<input id=f14 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f14");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f14")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok14.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad14.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_15(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a15>"
               "<input id=f15 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f15");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f15")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok15.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad15.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_16(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a16>"
               "<input id=f16 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f16");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f16")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok16.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad16.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_17(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a17>"
               "<input id=f17 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f17");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f17")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok17.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad17.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_18(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a18>"
               "<input id=f18 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f18");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f18")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok18.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad18.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_19(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a19>"
               "<input id=f19 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f19");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f19")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok19.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad19.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_20(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a20>"
               "<input id=f20 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f20");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f20")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok20.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad20.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_21(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a21>"
               "<input id=f21 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f21");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f21")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok21.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad21.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_22(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a22>"
               "<input id=f22 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f22");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f22")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok22.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad22.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_23(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a23>"
               "<input id=f23 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f23");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f23")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok23.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad23.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_24(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a24>"
               "<input id=f24 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f24");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f24")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok24.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad24.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_25(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a25>"
               "<input id=f25 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f25");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f25")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok25.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad25.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_26(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a26>"
               "<input id=f26 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f26");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f26")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok26.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad26.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_27(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a27>"
               "<input id=f27 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f27");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f27")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok27.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad27.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_28(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a28>"
               "<input id=f28 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f28");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f28")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok28.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad28.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_29(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a29>"
               "<input id=f29 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f29");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f29")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok29.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad29.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_30(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a30>"
               "<input id=f30 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f30");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f30")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok30.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad30.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_31(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a31>"
               "<input id=f31 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f31");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f31")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok31.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad31.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_32(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a32>"
               "<input id=f32 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f32");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f32")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok32.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad32.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_33(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a33>"
               "<input id=f33 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f33");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f33")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok33.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad33.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_34(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a34>"
               "<input id=f34 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f34");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f34")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok34.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad34.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_35(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a35>"
               "<input id=f35 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f35");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f35")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok35.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad35.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_36(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a36>"
               "<input id=f36 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f36");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f36")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok36.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad36.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_37(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a37>"
               "<input id=f37 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f37");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f37")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok37.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad37.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_38(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a38>"
               "<input id=f38 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f38");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f38")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok38.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad38.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_39(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a39>"
               "<input id=f39 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f39");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f39")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok39.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad39.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_40(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a40>"
               "<input id=f40 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f40");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f40")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok40.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad40.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_41(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a41>"
               "<input id=f41 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f41");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f41")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok41.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad41.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_42(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a42>"
               "<input id=f42 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f42");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f42")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok42.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad42.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_43(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a43>"
               "<input id=f43 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f43");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f43")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok43.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad43.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_44(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a44>"
               "<input id=f44 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f44");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f44")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok44.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad44.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_45(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a45>"
               "<input id=f45 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f45");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f45")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok45.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad45.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_46(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a46>"
               "<input id=f46 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f46");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f46")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok46.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad46.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_47(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a47>"
               "<input id=f47 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f47");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f47")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok47.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad47.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_48(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a48>"
               "<input id=f48 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f48");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f48")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok48.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad48.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_49(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a49>"
               "<input id=f49 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f49");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f49")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok49.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad49.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_50(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a50>"
               "<input id=f50 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f50");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f50")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok50.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad50.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_51(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a51>"
               "<input id=f51 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f51");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f51")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok51.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad51.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_52(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a52>"
               "<input id=f52 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f52");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f52")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok52.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad52.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_53(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a53>"
               "<input id=f53 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f53");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f53")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok53.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad53.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_54(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a54>"
               "<input id=f54 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f54");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f54")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok54.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad54.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_55(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a55>"
               "<input id=f55 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f55");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f55")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok55.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad55.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_56(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a56>"
               "<input id=f56 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f56");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f56")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok56.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad56.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_57(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a57>"
               "<input id=f57 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f57");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f57")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok57.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad57.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_58(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a58>"
               "<input id=f58 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f58");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f58")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok58.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad58.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_59(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a59>"
               "<input id=f59 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f59");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f59")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok59.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad59.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_60(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a60>"
               "<input id=f60 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f60");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f60")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok60.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad60.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_61(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a61>"
               "<input id=f61 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f61");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f61")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok61.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad61.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_62(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a62>"
               "<input id=f62 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f62");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f62")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok62.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad62.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_63(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a63>"
               "<input id=f63 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f63");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f63")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok63.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad63.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_64(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a64>"
               "<input id=f64 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f64");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f64")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok64.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad64.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_65(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a65>"
               "<input id=f65 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f65");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f65")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok65.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad65.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_66(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a66>"
               "<input id=f66 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f66");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f66")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok66.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad66.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_67(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a67>"
               "<input id=f67 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f67");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f67")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok67.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad67.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_68(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a68>"
               "<input id=f68 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f68");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f68")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok68.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad68.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_69(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a69>"
               "<input id=f69 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f69");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f69")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok69.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad69.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_70(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a70>"
               "<input id=f70 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f70");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f70")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok70.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad70.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_71(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a71>"
               "<input id=f71 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f71");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f71")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok71.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad71.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_72(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a72>"
               "<input id=f72 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f72");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f72")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok72.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad72.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_73(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a73>"
               "<input id=f73 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f73");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f73")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok73.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad73.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_74(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a74>"
               "<input id=f74 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f74");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f74")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok74.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad74.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_75(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a75>"
               "<input id=f75 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f75");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f75")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok75.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad75.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_76(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a76>"
               "<input id=f76 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f76");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f76")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok76.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad76.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_77(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a77>"
               "<input id=f77 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f77");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f77")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok77.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad77.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_78(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a78>"
               "<input id=f78 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f78");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f78")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok78.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad78.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_79(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a79>"
               "<input id=f79 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f79");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f79")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok79.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad79.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_80(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a80>"
               "<input id=f80 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f80");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f80")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok80.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad80.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_81(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a81>"
               "<input id=f81 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f81");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f81")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok81.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad81.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_82(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a82>"
               "<input id=f82 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f82");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f82")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok82.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad82.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_83(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a83>"
               "<input id=f83 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f83");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f83")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok83.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad83.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_84(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a84>"
               "<input id=f84 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f84");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f84")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok84.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad84.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_85(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a85>"
               "<input id=f85 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f85");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f85")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok85.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad85.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_86(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a86>"
               "<input id=f86 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f86");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f86")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok86.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad86.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_87(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a87>"
               "<input id=f87 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f87");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f87")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok87.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad87.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_88(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a88>"
               "<input id=f88 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f88");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f88")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok88.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad88.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_89(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a89>"
               "<input id=f89 type=file name=d accept='.csv'><input type=submit></form>");
    ctl = l77m_ctl("f89");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f89")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok89.csv"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad89.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_90(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a90>"
               "<input id=f90 type=file name=d accept='.md'><input type=submit></form>");
    ctl = l77m_ctl("f90");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f90")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok90.md"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad90.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_91(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a91>"
               "<input id=f91 type=file name=d accept='.html'><input type=submit></form>");
    ctl = l77m_ctl("f91");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f91")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok91.html"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad91.exe"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_92(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a92>"
               "<input id=f92 type=file name=d accept='.css'><input type=submit></form>");
    ctl = l77m_ctl("f92");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f92")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok92.css"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad92.zip"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_93(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a93>"
               "<input id=f93 type=file name=d accept='.js'><input type=submit></form>");
    ctl = l77m_ctl("f93");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f93")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok93.js"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad93.doc"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_94(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a94>"
               "<input id=f94 type=file name=d accept='.webp'><input type=submit></form>");
    ctl = l77m_ctl("f94");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f94")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok94.webp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad94.bin"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_95(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a95>"
               "<input id=f95 type=file name=d accept='.bmp'><input type=submit></form>");
    ctl = l77m_ctl("f95");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f95")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok95.bmp"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad95.dat"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_96(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a96>"
               "<input id=f96 type=file name=d accept='.txt'><input type=submit></form>");
    ctl = l77m_ctl("f96");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f96")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok96.txt"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad96.o"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_97(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a97>"
               "<input id=f97 type=file name=d accept='.png'><input type=submit></form>");
    ctl = l77m_ctl("f97");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f97")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok97.png"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad97.a"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_98(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a98>"
               "<input id=f98 type=file name=d accept='.jpg'><input type=submit></form>");
    ctl = l77m_ctl("f98");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f98")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok98.jpg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad98.so"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_99(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a99>"
               "<input id=f99 type=file name=d accept='.jpeg'><input type=submit></form>");
    ctl = l77m_ctl("f99");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f99")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok99.jpeg"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad99.tar"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_accept_100(void) {
    int ctl;
    l77m_render("<form method=POST enctype=multipart/form-data action=/a100>"
               "<input id=f100 type=file name=d accept='.gif'><input type=submit></form>");
    ctl = l77m_ctl("f100");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f100")].form_flags2 & L77M_FF2_ACCEPT);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/ok100.gif"));
    TEST_ASSERT_EQUAL(1, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/bad100.pdf"));
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_1(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m1>"
               "<input id=f1 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f1");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f1")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m1_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m1_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m1_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m1_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m1_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_2(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m2>"
               "<input id=f2 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f2");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f2")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m2_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m2_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m2_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m2_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m2_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_3(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m3>"
               "<input id=f3 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f3");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f3")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m3_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m3_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m3_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m3_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m3_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_4(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m4>"
               "<input id=f4 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f4");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f4")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m4_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m4_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m4_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m4_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m4_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_5(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m5>"
               "<input id=f5 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f5");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f5")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m5_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m5_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m5_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m5_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m5_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_6(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m6>"
               "<input id=f6 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f6");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f6")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m6_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m6_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m6_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m6_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m6_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_7(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m7>"
               "<input id=f7 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f7");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f7")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m7_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m7_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m7_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m7_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m7_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_8(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m8>"
               "<input id=f8 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f8");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f8")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m8_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m8_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m8_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m8_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m8_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_9(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m9>"
               "<input id=f9 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f9");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f9")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m9_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m9_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m9_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m9_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m9_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_10(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m10>"
               "<input id=f10 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f10");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f10")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m10_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m10_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m10_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m10_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m10_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_11(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m11>"
               "<input id=f11 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f11");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f11")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m11_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m11_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m11_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m11_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m11_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_12(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m12>"
               "<input id=f12 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f12");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f12")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m12_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m12_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m12_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m12_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m12_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_13(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m13>"
               "<input id=f13 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f13");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f13")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m13_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m13_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m13_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m13_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m13_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_14(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m14>"
               "<input id=f14 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f14");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f14")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m14_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m14_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m14_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m14_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m14_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_15(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m15>"
               "<input id=f15 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f15");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f15")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m15_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m15_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m15_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m15_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m15_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_16(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m16>"
               "<input id=f16 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f16");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f16")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m16_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m16_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m16_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m16_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m16_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_17(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m17>"
               "<input id=f17 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f17");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f17")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m17_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m17_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m17_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m17_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m17_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_18(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m18>"
               "<input id=f18 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f18");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f18")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m18_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m18_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m18_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m18_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m18_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_19(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m19>"
               "<input id=f19 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f19");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f19")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m19_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m19_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m19_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m19_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m19_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_20(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m20>"
               "<input id=f20 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f20");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f20")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m20_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m20_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m20_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m20_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m20_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_21(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m21>"
               "<input id=f21 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f21");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f21")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m21_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m21_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m21_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m21_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m21_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_22(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m22>"
               "<input id=f22 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f22");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f22")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m22_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m22_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m22_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m22_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m22_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_23(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m23>"
               "<input id=f23 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f23");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f23")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m23_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m23_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m23_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m23_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m23_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_24(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m24>"
               "<input id=f24 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f24");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f24")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m24_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m24_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m24_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m24_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m24_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_25(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m25>"
               "<input id=f25 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f25");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f25")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m25_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m25_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m25_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m25_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m25_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_26(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m26>"
               "<input id=f26 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f26");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f26")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m26_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m26_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m26_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m26_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m26_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_27(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m27>"
               "<input id=f27 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f27");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f27")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m27_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m27_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m27_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m27_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m27_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_28(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m28>"
               "<input id=f28 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f28");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f28")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m28_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m28_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m28_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m28_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m28_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_29(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m29>"
               "<input id=f29 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f29");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f29")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m29_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m29_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m29_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m29_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m29_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_30(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m30>"
               "<input id=f30 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f30");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f30")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m30_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m30_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m30_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m30_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m30_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_31(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m31>"
               "<input id=f31 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f31");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f31")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m31_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m31_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m31_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m31_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m31_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_32(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m32>"
               "<input id=f32 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f32");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f32")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m32_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m32_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m32_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m32_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m32_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_33(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m33>"
               "<input id=f33 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f33");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f33")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m33_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m33_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m33_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m33_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m33_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_34(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m34>"
               "<input id=f34 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f34");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f34")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m34_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m34_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m34_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m34_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m34_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_35(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m35>"
               "<input id=f35 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f35");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f35")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m35_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m35_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m35_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m35_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m35_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_36(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m36>"
               "<input id=f36 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f36");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f36")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m36_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m36_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m36_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m36_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m36_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_37(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m37>"
               "<input id=f37 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f37");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f37")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m37_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m37_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m37_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m37_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m37_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_38(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m38>"
               "<input id=f38 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f38");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f38")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m38_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m38_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m38_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m38_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m38_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_39(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m39>"
               "<input id=f39 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f39");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f39")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m39_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m39_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m39_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m39_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m39_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_40(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m40>"
               "<input id=f40 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f40");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f40")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m40_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m40_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m40_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m40_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m40_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_41(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m41>"
               "<input id=f41 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f41");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f41")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m41_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m41_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m41_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m41_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m41_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_42(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m42>"
               "<input id=f42 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f42");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f42")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m42_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m42_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m42_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m42_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m42_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_43(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m43>"
               "<input id=f43 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f43");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f43")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m43_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m43_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m43_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m43_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m43_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_44(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m44>"
               "<input id=f44 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f44");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f44")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m44_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m44_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m44_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m44_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m44_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_45(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m45>"
               "<input id=f45 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f45");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f45")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m45_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m45_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m45_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m45_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m45_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_46(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m46>"
               "<input id=f46 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f46");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f46")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m46_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m46_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m46_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m46_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m46_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_47(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m47>"
               "<input id=f47 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f47");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f47")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m47_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m47_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m47_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m47_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m47_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_48(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m48>"
               "<input id=f48 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f48");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f48")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m48_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m48_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m48_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m48_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m48_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_49(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m49>"
               "<input id=f49 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f49");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f49")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m49_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m49_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m49_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m49_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m49_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_50(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m50>"
               "<input id=f50 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f50");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f50")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m50_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m50_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m50_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m50_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m50_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_51(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m51>"
               "<input id=f51 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f51");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f51")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m51_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m51_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m51_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m51_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m51_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_52(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m52>"
               "<input id=f52 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f52");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f52")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m52_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m52_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m52_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m52_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m52_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_53(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m53>"
               "<input id=f53 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f53");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f53")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m53_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m53_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m53_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m53_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m53_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_54(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m54>"
               "<input id=f54 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f54");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f54")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m54_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m54_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m54_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m54_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m54_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_55(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m55>"
               "<input id=f55 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f55");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f55")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m55_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m55_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m55_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m55_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m55_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_56(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m56>"
               "<input id=f56 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f56");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f56")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m56_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m56_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m56_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m56_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m56_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_57(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m57>"
               "<input id=f57 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f57");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f57")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m57_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m57_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m57_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m57_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m57_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_58(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m58>"
               "<input id=f58 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f58");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f58")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m58_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m58_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m58_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m58_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m58_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_59(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m59>"
               "<input id=f59 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f59");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f59")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m59_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m59_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m59_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m59_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m59_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_60(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m60>"
               "<input id=f60 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f60");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f60")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m60_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m60_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m60_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m60_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m60_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_61(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m61>"
               "<input id=f61 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f61");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f61")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m61_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m61_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m61_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m61_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m61_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_62(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m62>"
               "<input id=f62 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f62");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f62")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m62_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m62_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m62_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m62_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m62_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_63(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m63>"
               "<input id=f63 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f63");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f63")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m63_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m63_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m63_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m63_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m63_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_64(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m64>"
               "<input id=f64 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f64");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f64")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m64_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m64_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m64_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m64_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m64_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_65(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m65>"
               "<input id=f65 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f65");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f65")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m65_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m65_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m65_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m65_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m65_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_66(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m66>"
               "<input id=f66 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f66");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f66")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m66_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m66_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m66_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m66_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m66_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_67(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m67>"
               "<input id=f67 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f67");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f67")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m67_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m67_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m67_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m67_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m67_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_68(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m68>"
               "<input id=f68 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f68");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f68")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m68_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m68_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m68_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m68_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m68_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_69(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m69>"
               "<input id=f69 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f69");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f69")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m69_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m69_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m69_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m69_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m69_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_70(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m70>"
               "<input id=f70 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f70");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f70")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m70_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m70_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m70_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m70_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m70_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_71(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m71>"
               "<input id=f71 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f71");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f71")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m71_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m71_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m71_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m71_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m71_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_72(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m72>"
               "<input id=f72 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f72");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f72")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m72_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m72_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m72_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m72_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m72_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_73(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m73>"
               "<input id=f73 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f73");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f73")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m73_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m73_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m73_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m73_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m73_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_74(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m74>"
               "<input id=f74 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f74");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f74")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m74_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m74_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m74_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m74_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m74_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_75(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m75>"
               "<input id=f75 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f75");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f75")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m75_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m75_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m75_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m75_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m75_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_76(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m76>"
               "<input id=f76 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f76");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f76")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m76_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m76_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m76_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m76_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m76_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_77(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m77>"
               "<input id=f77 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f77");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f77")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m77_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m77_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m77_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m77_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m77_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_78(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m78>"
               "<input id=f78 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f78");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f78")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m78_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m78_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m78_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m78_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m78_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_79(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m79>"
               "<input id=f79 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f79");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f79")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m79_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m79_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m79_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m79_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m79_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_multi_80(void) {
    int ctl, k; char out[64];
    l77m_render("<form method=POST enctype=multipart/form-data action=/m80>"
               "<input id=f80 type=file name=d multiple><input type=submit></form>");
    ctl = l77m_ctl("f80");
    TEST_ASSERT_TRUE(g_doc.nodes[l77m_node("f80")].form_flags & L77M_FF_MULTIPLE);
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m80_1.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m80_2.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m80_3.txt"));
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/m80_4.txt"));
    TEST_ASSERT_EQUAL(4, we_form_file_count(&g_doc, ctl));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/m80_5.txt"));
    for (k = 0; k < 4; k++) {
        TEST_ASSERT_TRUE(we_form_file_at(&g_doc, ctl, k, out, 64) > 0);
        TEST_ASSERT_NOT_NULL(strstr(out, ".txt"));
    }
    TEST_ASSERT_EQUAL(0, we_form_clear_file(&g_doc, ctl));
    TEST_ASSERT_EQUAL(0, we_form_file_count(&g_doc, ctl));
    l77m_done();
}

static void test_l77m_textstar_1(void) {
    int ctl;
    l77m_render("<form><input id=f1 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f1");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t1.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t1.png"));
    l77m_done();
}

static void test_l77m_textstar_2(void) {
    int ctl;
    l77m_render("<form><input id=f2 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f2");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t2.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t2.png"));
    l77m_done();
}

static void test_l77m_textstar_3(void) {
    int ctl;
    l77m_render("<form><input id=f3 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f3");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t3.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t3.png"));
    l77m_done();
}

static void test_l77m_textstar_4(void) {
    int ctl;
    l77m_render("<form><input id=f4 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f4");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t4.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t4.png"));
    l77m_done();
}

static void test_l77m_textstar_5(void) {
    int ctl;
    l77m_render("<form><input id=f5 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f5");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t5.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t5.png"));
    l77m_done();
}

static void test_l77m_textstar_6(void) {
    int ctl;
    l77m_render("<form><input id=f6 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f6");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t6.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t6.png"));
    l77m_done();
}

static void test_l77m_textstar_7(void) {
    int ctl;
    l77m_render("<form><input id=f7 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f7");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t7.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t7.png"));
    l77m_done();
}

static void test_l77m_textstar_8(void) {
    int ctl;
    l77m_render("<form><input id=f8 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f8");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t8.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t8.png"));
    l77m_done();
}

static void test_l77m_textstar_9(void) {
    int ctl;
    l77m_render("<form><input id=f9 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f9");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t9.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t9.png"));
    l77m_done();
}

static void test_l77m_textstar_10(void) {
    int ctl;
    l77m_render("<form><input id=f10 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f10");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t10.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t10.png"));
    l77m_done();
}

static void test_l77m_textstar_11(void) {
    int ctl;
    l77m_render("<form><input id=f11 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f11");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t11.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t11.png"));
    l77m_done();
}

static void test_l77m_textstar_12(void) {
    int ctl;
    l77m_render("<form><input id=f12 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f12");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t12.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t12.png"));
    l77m_done();
}

static void test_l77m_textstar_13(void) {
    int ctl;
    l77m_render("<form><input id=f13 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f13");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t13.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t13.png"));
    l77m_done();
}

static void test_l77m_textstar_14(void) {
    int ctl;
    l77m_render("<form><input id=f14 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f14");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t14.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t14.png"));
    l77m_done();
}

static void test_l77m_textstar_15(void) {
    int ctl;
    l77m_render("<form><input id=f15 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f15");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t15.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t15.png"));
    l77m_done();
}

static void test_l77m_textstar_16(void) {
    int ctl;
    l77m_render("<form><input id=f16 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f16");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t16.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t16.png"));
    l77m_done();
}

static void test_l77m_textstar_17(void) {
    int ctl;
    l77m_render("<form><input id=f17 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f17");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t17.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t17.png"));
    l77m_done();
}

static void test_l77m_textstar_18(void) {
    int ctl;
    l77m_render("<form><input id=f18 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f18");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t18.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t18.png"));
    l77m_done();
}

static void test_l77m_textstar_19(void) {
    int ctl;
    l77m_render("<form><input id=f19 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f19");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t19.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t19.png"));
    l77m_done();
}

static void test_l77m_textstar_20(void) {
    int ctl;
    l77m_render("<form><input id=f20 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f20");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t20.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t20.png"));
    l77m_done();
}

static void test_l77m_textstar_21(void) {
    int ctl;
    l77m_render("<form><input id=f21 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f21");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t21.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t21.png"));
    l77m_done();
}

static void test_l77m_textstar_22(void) {
    int ctl;
    l77m_render("<form><input id=f22 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f22");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t22.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t22.png"));
    l77m_done();
}

static void test_l77m_textstar_23(void) {
    int ctl;
    l77m_render("<form><input id=f23 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f23");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t23.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t23.png"));
    l77m_done();
}

static void test_l77m_textstar_24(void) {
    int ctl;
    l77m_render("<form><input id=f24 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f24");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t24.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t24.png"));
    l77m_done();
}

static void test_l77m_textstar_25(void) {
    int ctl;
    l77m_render("<form><input id=f25 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f25");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t25.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t25.png"));
    l77m_done();
}

static void test_l77m_textstar_26(void) {
    int ctl;
    l77m_render("<form><input id=f26 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f26");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t26.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t26.png"));
    l77m_done();
}

static void test_l77m_textstar_27(void) {
    int ctl;
    l77m_render("<form><input id=f27 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f27");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t27.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t27.png"));
    l77m_done();
}

static void test_l77m_textstar_28(void) {
    int ctl;
    l77m_render("<form><input id=f28 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f28");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t28.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t28.png"));
    l77m_done();
}

static void test_l77m_textstar_29(void) {
    int ctl;
    l77m_render("<form><input id=f29 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f29");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t29.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t29.png"));
    l77m_done();
}

static void test_l77m_textstar_30(void) {
    int ctl;
    l77m_render("<form><input id=f30 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f30");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t30.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t30.png"));
    l77m_done();
}

static void test_l77m_textstar_31(void) {
    int ctl;
    l77m_render("<form><input id=f31 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f31");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t31.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t31.png"));
    l77m_done();
}

static void test_l77m_textstar_32(void) {
    int ctl;
    l77m_render("<form><input id=f32 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f32");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t32.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t32.png"));
    l77m_done();
}

static void test_l77m_textstar_33(void) {
    int ctl;
    l77m_render("<form><input id=f33 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f33");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t33.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t33.png"));
    l77m_done();
}

static void test_l77m_textstar_34(void) {
    int ctl;
    l77m_render("<form><input id=f34 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f34");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t34.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t34.png"));
    l77m_done();
}

static void test_l77m_textstar_35(void) {
    int ctl;
    l77m_render("<form><input id=f35 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f35");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t35.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t35.png"));
    l77m_done();
}

static void test_l77m_textstar_36(void) {
    int ctl;
    l77m_render("<form><input id=f36 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f36");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t36.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t36.png"));
    l77m_done();
}

static void test_l77m_textstar_37(void) {
    int ctl;
    l77m_render("<form><input id=f37 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f37");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t37.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t37.png"));
    l77m_done();
}

static void test_l77m_textstar_38(void) {
    int ctl;
    l77m_render("<form><input id=f38 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f38");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t38.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t38.png"));
    l77m_done();
}

static void test_l77m_textstar_39(void) {
    int ctl;
    l77m_render("<form><input id=f39 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f39");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t39.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t39.png"));
    l77m_done();
}

static void test_l77m_textstar_40(void) {
    int ctl;
    l77m_render("<form><input id=f40 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f40");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t40.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t40.png"));
    l77m_done();
}

static void test_l77m_textstar_41(void) {
    int ctl;
    l77m_render("<form><input id=f41 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f41");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t41.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t41.png"));
    l77m_done();
}

static void test_l77m_textstar_42(void) {
    int ctl;
    l77m_render("<form><input id=f42 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f42");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t42.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t42.png"));
    l77m_done();
}

static void test_l77m_textstar_43(void) {
    int ctl;
    l77m_render("<form><input id=f43 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f43");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t43.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t43.png"));
    l77m_done();
}

static void test_l77m_textstar_44(void) {
    int ctl;
    l77m_render("<form><input id=f44 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f44");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t44.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t44.png"));
    l77m_done();
}

static void test_l77m_textstar_45(void) {
    int ctl;
    l77m_render("<form><input id=f45 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f45");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t45.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t45.png"));
    l77m_done();
}

static void test_l77m_textstar_46(void) {
    int ctl;
    l77m_render("<form><input id=f46 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f46");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t46.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t46.png"));
    l77m_done();
}

static void test_l77m_textstar_47(void) {
    int ctl;
    l77m_render("<form><input id=f47 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f47");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t47.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t47.png"));
    l77m_done();
}

static void test_l77m_textstar_48(void) {
    int ctl;
    l77m_render("<form><input id=f48 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f48");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t48.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t48.png"));
    l77m_done();
}

static void test_l77m_textstar_49(void) {
    int ctl;
    l77m_render("<form><input id=f49 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f49");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t49.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t49.png"));
    l77m_done();
}

static void test_l77m_textstar_50(void) {
    int ctl;
    l77m_render("<form><input id=f50 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f50");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t50.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t50.png"));
    l77m_done();
}

static void test_l77m_textstar_51(void) {
    int ctl;
    l77m_render("<form><input id=f51 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f51");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t51.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t51.png"));
    l77m_done();
}

static void test_l77m_textstar_52(void) {
    int ctl;
    l77m_render("<form><input id=f52 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f52");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t52.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t52.png"));
    l77m_done();
}

static void test_l77m_textstar_53(void) {
    int ctl;
    l77m_render("<form><input id=f53 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f53");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t53.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t53.png"));
    l77m_done();
}

static void test_l77m_textstar_54(void) {
    int ctl;
    l77m_render("<form><input id=f54 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f54");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t54.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t54.png"));
    l77m_done();
}

static void test_l77m_textstar_55(void) {
    int ctl;
    l77m_render("<form><input id=f55 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f55");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t55.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t55.png"));
    l77m_done();
}

static void test_l77m_textstar_56(void) {
    int ctl;
    l77m_render("<form><input id=f56 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f56");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t56.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t56.png"));
    l77m_done();
}

static void test_l77m_textstar_57(void) {
    int ctl;
    l77m_render("<form><input id=f57 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f57");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t57.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t57.png"));
    l77m_done();
}

static void test_l77m_textstar_58(void) {
    int ctl;
    l77m_render("<form><input id=f58 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f58");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t58.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t58.png"));
    l77m_done();
}

static void test_l77m_textstar_59(void) {
    int ctl;
    l77m_render("<form><input id=f59 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f59");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t59.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t59.png"));
    l77m_done();
}

static void test_l77m_textstar_60(void) {
    int ctl;
    l77m_render("<form><input id=f60 type=file name=d accept='text/*'></form>");
    ctl = l77m_ctl("f60");
    TEST_ASSERT_EQUAL(0, we_form_set_file(&g_doc, ctl, "/tmp/t60.txt"));
    TEST_ASSERT_EQUAL(-1, we_form_set_file(&g_doc, ctl, "/tmp/t60.png"));
    l77m_done();
}

static void lot77_filemore_run(void) {
    RUN_TEST(test_l77m_accept_1);
    RUN_TEST(test_l77m_accept_2);
    RUN_TEST(test_l77m_accept_3);
    RUN_TEST(test_l77m_accept_4);
    RUN_TEST(test_l77m_accept_5);
    RUN_TEST(test_l77m_accept_6);
    RUN_TEST(test_l77m_accept_7);
    RUN_TEST(test_l77m_accept_8);
    RUN_TEST(test_l77m_accept_9);
    RUN_TEST(test_l77m_accept_10);
    RUN_TEST(test_l77m_accept_11);
    RUN_TEST(test_l77m_accept_12);
    RUN_TEST(test_l77m_accept_13);
    RUN_TEST(test_l77m_accept_14);
    RUN_TEST(test_l77m_accept_15);
    RUN_TEST(test_l77m_accept_16);
    RUN_TEST(test_l77m_accept_17);
    RUN_TEST(test_l77m_accept_18);
    RUN_TEST(test_l77m_accept_19);
    RUN_TEST(test_l77m_accept_20);
    RUN_TEST(test_l77m_accept_21);
    RUN_TEST(test_l77m_accept_22);
    RUN_TEST(test_l77m_accept_23);
    RUN_TEST(test_l77m_accept_24);
    RUN_TEST(test_l77m_accept_25);
    RUN_TEST(test_l77m_accept_26);
    RUN_TEST(test_l77m_accept_27);
    RUN_TEST(test_l77m_accept_28);
    RUN_TEST(test_l77m_accept_29);
    RUN_TEST(test_l77m_accept_30);
    RUN_TEST(test_l77m_accept_31);
    RUN_TEST(test_l77m_accept_32);
    RUN_TEST(test_l77m_accept_33);
    RUN_TEST(test_l77m_accept_34);
    RUN_TEST(test_l77m_accept_35);
    RUN_TEST(test_l77m_accept_36);
    RUN_TEST(test_l77m_accept_37);
    RUN_TEST(test_l77m_accept_38);
    RUN_TEST(test_l77m_accept_39);
    RUN_TEST(test_l77m_accept_40);
    RUN_TEST(test_l77m_accept_41);
    RUN_TEST(test_l77m_accept_42);
    RUN_TEST(test_l77m_accept_43);
    RUN_TEST(test_l77m_accept_44);
    RUN_TEST(test_l77m_accept_45);
    RUN_TEST(test_l77m_accept_46);
    RUN_TEST(test_l77m_accept_47);
    RUN_TEST(test_l77m_accept_48);
    RUN_TEST(test_l77m_accept_49);
    RUN_TEST(test_l77m_accept_50);
    RUN_TEST(test_l77m_accept_51);
    RUN_TEST(test_l77m_accept_52);
    RUN_TEST(test_l77m_accept_53);
    RUN_TEST(test_l77m_accept_54);
    RUN_TEST(test_l77m_accept_55);
    RUN_TEST(test_l77m_accept_56);
    RUN_TEST(test_l77m_accept_57);
    RUN_TEST(test_l77m_accept_58);
    RUN_TEST(test_l77m_accept_59);
    RUN_TEST(test_l77m_accept_60);
    RUN_TEST(test_l77m_accept_61);
    RUN_TEST(test_l77m_accept_62);
    RUN_TEST(test_l77m_accept_63);
    RUN_TEST(test_l77m_accept_64);
    RUN_TEST(test_l77m_accept_65);
    RUN_TEST(test_l77m_accept_66);
    RUN_TEST(test_l77m_accept_67);
    RUN_TEST(test_l77m_accept_68);
    RUN_TEST(test_l77m_accept_69);
    RUN_TEST(test_l77m_accept_70);
    RUN_TEST(test_l77m_accept_71);
    RUN_TEST(test_l77m_accept_72);
    RUN_TEST(test_l77m_accept_73);
    RUN_TEST(test_l77m_accept_74);
    RUN_TEST(test_l77m_accept_75);
    RUN_TEST(test_l77m_accept_76);
    RUN_TEST(test_l77m_accept_77);
    RUN_TEST(test_l77m_accept_78);
    RUN_TEST(test_l77m_accept_79);
    RUN_TEST(test_l77m_accept_80);
    RUN_TEST(test_l77m_accept_81);
    RUN_TEST(test_l77m_accept_82);
    RUN_TEST(test_l77m_accept_83);
    RUN_TEST(test_l77m_accept_84);
    RUN_TEST(test_l77m_accept_85);
    RUN_TEST(test_l77m_accept_86);
    RUN_TEST(test_l77m_accept_87);
    RUN_TEST(test_l77m_accept_88);
    RUN_TEST(test_l77m_accept_89);
    RUN_TEST(test_l77m_accept_90);
    RUN_TEST(test_l77m_accept_91);
    RUN_TEST(test_l77m_accept_92);
    RUN_TEST(test_l77m_accept_93);
    RUN_TEST(test_l77m_accept_94);
    RUN_TEST(test_l77m_accept_95);
    RUN_TEST(test_l77m_accept_96);
    RUN_TEST(test_l77m_accept_97);
    RUN_TEST(test_l77m_accept_98);
    RUN_TEST(test_l77m_accept_99);
    RUN_TEST(test_l77m_accept_100);
    RUN_TEST(test_l77m_multi_1);
    RUN_TEST(test_l77m_multi_2);
    RUN_TEST(test_l77m_multi_3);
    RUN_TEST(test_l77m_multi_4);
    RUN_TEST(test_l77m_multi_5);
    RUN_TEST(test_l77m_multi_6);
    RUN_TEST(test_l77m_multi_7);
    RUN_TEST(test_l77m_multi_8);
    RUN_TEST(test_l77m_multi_9);
    RUN_TEST(test_l77m_multi_10);
    RUN_TEST(test_l77m_multi_11);
    RUN_TEST(test_l77m_multi_12);
    RUN_TEST(test_l77m_multi_13);
    RUN_TEST(test_l77m_multi_14);
    RUN_TEST(test_l77m_multi_15);
    RUN_TEST(test_l77m_multi_16);
    RUN_TEST(test_l77m_multi_17);
    RUN_TEST(test_l77m_multi_18);
    RUN_TEST(test_l77m_multi_19);
    RUN_TEST(test_l77m_multi_20);
    RUN_TEST(test_l77m_multi_21);
    RUN_TEST(test_l77m_multi_22);
    RUN_TEST(test_l77m_multi_23);
    RUN_TEST(test_l77m_multi_24);
    RUN_TEST(test_l77m_multi_25);
    RUN_TEST(test_l77m_multi_26);
    RUN_TEST(test_l77m_multi_27);
    RUN_TEST(test_l77m_multi_28);
    RUN_TEST(test_l77m_multi_29);
    RUN_TEST(test_l77m_multi_30);
    RUN_TEST(test_l77m_multi_31);
    RUN_TEST(test_l77m_multi_32);
    RUN_TEST(test_l77m_multi_33);
    RUN_TEST(test_l77m_multi_34);
    RUN_TEST(test_l77m_multi_35);
    RUN_TEST(test_l77m_multi_36);
    RUN_TEST(test_l77m_multi_37);
    RUN_TEST(test_l77m_multi_38);
    RUN_TEST(test_l77m_multi_39);
    RUN_TEST(test_l77m_multi_40);
    RUN_TEST(test_l77m_multi_41);
    RUN_TEST(test_l77m_multi_42);
    RUN_TEST(test_l77m_multi_43);
    RUN_TEST(test_l77m_multi_44);
    RUN_TEST(test_l77m_multi_45);
    RUN_TEST(test_l77m_multi_46);
    RUN_TEST(test_l77m_multi_47);
    RUN_TEST(test_l77m_multi_48);
    RUN_TEST(test_l77m_multi_49);
    RUN_TEST(test_l77m_multi_50);
    RUN_TEST(test_l77m_multi_51);
    RUN_TEST(test_l77m_multi_52);
    RUN_TEST(test_l77m_multi_53);
    RUN_TEST(test_l77m_multi_54);
    RUN_TEST(test_l77m_multi_55);
    RUN_TEST(test_l77m_multi_56);
    RUN_TEST(test_l77m_multi_57);
    RUN_TEST(test_l77m_multi_58);
    RUN_TEST(test_l77m_multi_59);
    RUN_TEST(test_l77m_multi_60);
    RUN_TEST(test_l77m_multi_61);
    RUN_TEST(test_l77m_multi_62);
    RUN_TEST(test_l77m_multi_63);
    RUN_TEST(test_l77m_multi_64);
    RUN_TEST(test_l77m_multi_65);
    RUN_TEST(test_l77m_multi_66);
    RUN_TEST(test_l77m_multi_67);
    RUN_TEST(test_l77m_multi_68);
    RUN_TEST(test_l77m_multi_69);
    RUN_TEST(test_l77m_multi_70);
    RUN_TEST(test_l77m_multi_71);
    RUN_TEST(test_l77m_multi_72);
    RUN_TEST(test_l77m_multi_73);
    RUN_TEST(test_l77m_multi_74);
    RUN_TEST(test_l77m_multi_75);
    RUN_TEST(test_l77m_multi_76);
    RUN_TEST(test_l77m_multi_77);
    RUN_TEST(test_l77m_multi_78);
    RUN_TEST(test_l77m_multi_79);
    RUN_TEST(test_l77m_multi_80);
    RUN_TEST(test_l77m_textstar_1);
    RUN_TEST(test_l77m_textstar_2);
    RUN_TEST(test_l77m_textstar_3);
    RUN_TEST(test_l77m_textstar_4);
    RUN_TEST(test_l77m_textstar_5);
    RUN_TEST(test_l77m_textstar_6);
    RUN_TEST(test_l77m_textstar_7);
    RUN_TEST(test_l77m_textstar_8);
    RUN_TEST(test_l77m_textstar_9);
    RUN_TEST(test_l77m_textstar_10);
    RUN_TEST(test_l77m_textstar_11);
    RUN_TEST(test_l77m_textstar_12);
    RUN_TEST(test_l77m_textstar_13);
    RUN_TEST(test_l77m_textstar_14);
    RUN_TEST(test_l77m_textstar_15);
    RUN_TEST(test_l77m_textstar_16);
    RUN_TEST(test_l77m_textstar_17);
    RUN_TEST(test_l77m_textstar_18);
    RUN_TEST(test_l77m_textstar_19);
    RUN_TEST(test_l77m_textstar_20);
    RUN_TEST(test_l77m_textstar_21);
    RUN_TEST(test_l77m_textstar_22);
    RUN_TEST(test_l77m_textstar_23);
    RUN_TEST(test_l77m_textstar_24);
    RUN_TEST(test_l77m_textstar_25);
    RUN_TEST(test_l77m_textstar_26);
    RUN_TEST(test_l77m_textstar_27);
    RUN_TEST(test_l77m_textstar_28);
    RUN_TEST(test_l77m_textstar_29);
    RUN_TEST(test_l77m_textstar_30);
    RUN_TEST(test_l77m_textstar_31);
    RUN_TEST(test_l77m_textstar_32);
    RUN_TEST(test_l77m_textstar_33);
    RUN_TEST(test_l77m_textstar_34);
    RUN_TEST(test_l77m_textstar_35);
    RUN_TEST(test_l77m_textstar_36);
    RUN_TEST(test_l77m_textstar_37);
    RUN_TEST(test_l77m_textstar_38);
    RUN_TEST(test_l77m_textstar_39);
    RUN_TEST(test_l77m_textstar_40);
    RUN_TEST(test_l77m_textstar_41);
    RUN_TEST(test_l77m_textstar_42);
    RUN_TEST(test_l77m_textstar_43);
    RUN_TEST(test_l77m_textstar_44);
    RUN_TEST(test_l77m_textstar_45);
    RUN_TEST(test_l77m_textstar_46);
    RUN_TEST(test_l77m_textstar_47);
    RUN_TEST(test_l77m_textstar_48);
    RUN_TEST(test_l77m_textstar_49);
    RUN_TEST(test_l77m_textstar_50);
    RUN_TEST(test_l77m_textstar_51);
    RUN_TEST(test_l77m_textstar_52);
    RUN_TEST(test_l77m_textstar_53);
    RUN_TEST(test_l77m_textstar_54);
    RUN_TEST(test_l77m_textstar_55);
    RUN_TEST(test_l77m_textstar_56);
    RUN_TEST(test_l77m_textstar_57);
    RUN_TEST(test_l77m_textstar_58);
    RUN_TEST(test_l77m_textstar_59);
    RUN_TEST(test_l77m_textstar_60);
}
