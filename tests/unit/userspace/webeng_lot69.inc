/* Lot 69: per-element font-size and line-height with scaled glyphs. A
 * node of font_px renders at round(font_px / 4) quarters (4..16): its
 * cell is 2q px wide, its glyph 2q px high and its normal line 10q/4 px.
 * TEXT items carry the scale in pad for the kernel painter. */
#define L69(css, body) "<style>body{margin:0}p{margin:0}" css "</style><body>" body "</body>"

static void test_l69_parse_font_size(void) {
    int px = 0;
    TEST_ASSERT_EQUAL(0, we_parse_font_size("20px", 4, 16, &px)); TEST_ASSERT_EQUAL(20, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("2em", 3, 16, &px)); TEST_ASSERT_EQUAL(32, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("1.5em", 5, 20, &px)); TEST_ASSERT_EQUAL(30, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("150%", 4, 16, &px)); TEST_ASSERT_EQUAL(24, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("2rem", 4, 40, &px)); TEST_ASSERT_EQUAL(32, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("12pt", 4, 16, &px)); TEST_ASSERT_EQUAL(16, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size(" X-Large ", 9, 16, &px)); TEST_ASSERT_EQUAL(24, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("xxx-large", 9, 16, &px)); TEST_ASSERT_EQUAL(48, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("small", 5, 16, &px)); TEST_ASSERT_EQUAL(13, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("larger", 6, 20, &px)); TEST_ASSERT_EQUAL(24, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("smaller", 7, 24, &px)); TEST_ASSERT_EQUAL(20, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("999px", 5, 16, &px)); TEST_ASSERT_EQUAL(WE_FONT_PX_MAX, px);
    TEST_ASSERT_EQUAL(0, we_parse_font_size("1px", 3, 16, &px)); TEST_ASSERT_EQUAL(WE_FONT_PX_MIN, px);
    TEST_ASSERT_EQUAL(-1, we_parse_font_size("big", 3, 16, &px));
    TEST_ASSERT_EQUAL(-1, we_parse_font_size("12", 2, 16, &px));
    TEST_ASSERT_EQUAL(-1, we_parse_font_size("-2px", 4, 16, &px));
    TEST_ASSERT_EQUAL(-1, we_parse_font_size("0px", 3, 16, &px));
    TEST_ASSERT_EQUAL(-1, we_parse_font_size("1.2.3em", 7, 16, &px));
    TEST_ASSERT_EQUAL(-1, we_parse_font_size("", 0, 16, &px));
    TEST_ASSERT_EQUAL(-1, we_parse_font_size("calc(2em)", 9, 16, &px));
}

static void test_l69_font_q(void) {
    TEST_ASSERT_EQUAL(4, we_font_q(0));
    TEST_ASSERT_EQUAL(4, we_font_q(9));
    TEST_ASSERT_EQUAL(4, we_font_q(16));
    TEST_ASSERT_EQUAL(5, we_font_q(18));
    TEST_ASSERT_EQUAL(6, we_font_q(24));
    TEST_ASSERT_EQUAL(8, we_font_q(32));
    TEST_ASSERT_EQUAL(16, we_font_q(64));
}

static void test_l69_heading_defaults(void) {
    const os_fb_web_item_t *a, *b, *c, *p;
    lot52_render(L69("h1,h2,h3{margin:0}", "<h1>Big</h1><h2>Mid</h2><h3>Low</h3><p>txt</p>"), 400);
    a = find_text("Big"); b = find_text("Mid"); c = find_text("Low"); p = find_text("txt");
    TEST_ASSERT_NOT_NULL(a); TEST_ASSERT_NOT_NULL(b); TEST_ASSERT_NOT_NULL(c); TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL(8, a->pad); TEST_ASSERT_EQUAL(16, a->h); TEST_ASSERT_EQUAL(48, a->w);
    TEST_ASSERT_EQUAL(6, b->pad); TEST_ASSERT_EQUAL(12, b->h); TEST_ASSERT_EQUAL(36, b->w);
    TEST_ASSERT_EQUAL(5, c->pad); TEST_ASSERT_EQUAL(10, c->h);
    TEST_ASSERT_EQUAL(0, p->pad); TEST_ASSERT_EQUAL(8, p->h); TEST_ASSERT_EQUAL(24, p->w);
    TEST_ASSERT_EQUAL(1, a->y);
    TEST_ASSERT_EQUAL(1 + 20, b->y);          /* h1 line = 10 * 8 / 4 */
    TEST_ASSERT_EQUAL(1 + 20 + 15, c->y);     /* h2 line = 15 */
    TEST_ASSERT_EQUAL(1 + 20 + 15 + 13, p->y); /* h3 line = round(12.5) */
    TEST_ASSERT_EQUAL(1, a->bold);
}

static void test_l69_small_headings_clamp(void) {
    const os_fb_web_item_t *a, *b;
    lot52_render(L69("h5,h6{margin:0}", "<h5>five</h5><h6>six</h6><p><small>tiny</small></p>"), 400);
    a = find_text("five"); b = find_text("six");
    TEST_ASSERT_NOT_NULL(a); TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL(0, a->pad); TEST_ASSERT_EQUAL(8, a->h);   /* below 16 px: drawn at the 8 px cell */
    TEST_ASSERT_EQUAL(11, b->y);
    TEST_ASSERT_NOT_NULL(find_text("tiny"));
    TEST_ASSERT_EQUAL(0, find_text("tiny")->pad);
}

static void test_l69_px_em_inherit(void) {
    const os_fb_web_item_t *a, *b;
    lot52_render(L69("#d{font-size:2em}#s{font-size:1.5em}", "<div id=d>aa<span id=s>bb</span></div>"), 600);
    a = find_text("aa"); b = find_text("bb");
    TEST_ASSERT_NOT_NULL(a); TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL(8, a->pad);
    TEST_ASSERT_EQUAL(12, b->pad);            /* 48 px = 16 * 2 * 1.5 */
    TEST_ASSERT_EQUAL(24, b->h);
    TEST_ASSERT_EQUAL(a->x + 2 * 16, b->x);   /* "aa" at 16 px cells */
    TEST_ASSERT_EQUAL(48, (int)lot52_node("s")->font_px);
}

static void test_l69_em_does_not_compound_on_redeclare(void) {
    lot52_render(L69("#d{font-size:2em}#d{font-size:2em}", "<div id=d>x</div>"), 400);
    TEST_ASSERT_EQUAL(32, (int)lot52_node("d")->font_px);
}

static void test_l69_rem_keywords_percent(void) {
    lot52_render(L69("#a{font-size:3rem}#b{font-size:xx-large}#c{font-size:50%}#e{font-size:larger}",
                     "<div style='font-size:40px'><p id=a>a</p><p id=c>c</p></div><p id=b>b</p><p id=e>e</p>"), 400);
    TEST_ASSERT_EQUAL(48, (int)lot52_node("a")->font_px);
    TEST_ASSERT_EQUAL(32, (int)lot52_node("b")->font_px);
    TEST_ASSERT_EQUAL(20, (int)lot52_node("c")->font_px);
    TEST_ASSERT_EQUAL(19, (int)lot52_node("e")->font_px);
    TEST_ASSERT_EQUAL(12, find_text("a")->pad);
}

static void test_l69_refused_font_size(void) {
    lot52_render(L69("#a{font-size:huge}#b{font-size:-3px}", "<p id=a>a</p><p id=b>b</p>"), 400);
    TEST_ASSERT_EQUAL(2, l61_diag_count("font size refused"));
    TEST_ASSERT_EQUAL(16, (int)lot52_node("a")->font_px);
    TEST_ASSERT_EQUAL(0, find_text("a")->pad);
}

static void test_l69_inline_big_text_raises_line(void) {
    const os_fb_web_item_t *n2;
    lot52_render(L69("", "<p>ab <span style='font-size:32px'>CD</span><br>ef</p>"), 400);
    n2 = find_text("ef");
    TEST_ASSERT_NOT_NULL(n2);
    TEST_ASSERT_EQUAL(1 + 20, n2->y);        /* the line takes the 32 px span's 20 px */
    TEST_ASSERT_EQUAL(8, find_text("CD")->pad);
}

static void test_l69_wrap_uses_scaled_advance(void) {
    const os_fb_web_item_t *first;
    uint32_t i, lines = 0;
    int last_y = -100;
    lot52_render(L69("p{font-size:32px}", "<p>aaaa bbbb cccc dddd</p>"), 140);
    first = find_text("aaaa");
    TEST_ASSERT_NOT_NULL(first);
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind != OS_FB_WEB_TEXT) continue;
        TEST_ASSERT_TRUE(it->x + it->w <= 140);
        if (it->y != last_y) { lines++; last_y = it->y; }
    }
    TEST_ASSERT_EQUAL(4, (int)lines);        /* "aaaa bbbb" needs 9 cells of 16 px, 140 holds 8 */
}

static void test_l69_unitless_line_height_follows_font(void) {
    lot52_render(L69("p{font-size:32px;line-height:2}", "<p>a<br>b</p>"), 400);
    TEST_ASSERT_EQUAL(find_text("a")->y + 40, find_text("b")->y);
    lot52_render(L69("p{font-size:32px;line-height:30px}", "<p>a<br>b</p>"), 400);
    TEST_ASSERT_EQUAL(find_text("a")->y + 30, find_text("b")->y);
    lot52_render(L69("div{line-height:2}p{font-size:32px}", "<div><p>a<br>b</p></div>"), 400);
    TEST_ASSERT_EQUAL(find_text("a")->y + 40, find_text("b")->y);   /* the number inherits, not the px */
}

static void test_l69_letter_spacing_scaled(void) {
    const os_fb_web_item_t *x, *y;
    lot52_render(L69("p{font-size:32px;letter-spacing:4px}", "<p>xy</p>"), 400);
    x = find_text("x"); y = find_text("y");
    TEST_ASSERT_NOT_NULL(x); TEST_ASSERT_NOT_NULL(y);
    TEST_ASSERT_EQUAL(16, x->w);
    TEST_ASSERT_EQUAL(x->x + 20, y->x);
    TEST_ASSERT_EQUAL(8, y->pad);
}

static void test_l69_ellipsis_scaled(void) {
    uint32_t i;
    lot52_render(L69("p{font-size:32px;width:100px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}",
                     "<p>abcdefghijkl</p>"), 400);
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind != OS_FB_WEB_TEXT) continue;
        TEST_ASSERT_TRUE(it->w <= 100);
        TEST_ASSERT_EQUAL((int)strlen(it->text) * 16, (int)it->w);
    }
}

static void test_l69_clip_drops_cut_scaled_glyphs(void) {
    uint32_t i;
    int seen = 0;
    lot52_render(L69("div{width:40px;height:40px;overflow:hidden;font-size:32px}", "<div>abcdef</div>"), 400);
    for (i = 0; i < g_doc.nitems; i++) {
        const os_fb_web_item_t *it = &g_doc.items[i];
        if (it->kind == OS_FB_WEB_TEXT) { seen++; TEST_ASSERT_TRUE(it->x + it->w <= 40); TEST_ASSERT_EQUAL(16, it->h); }
    }
    TEST_ASSERT_TRUE(seen >= 1);
}

static void test_l69_link_hit_on_big_text(void) {
    char href[32];
    lot52_render(L69("a{font-size:48px}", "<p><a href='/x.html'>go</a></p>"), 400);
    TEST_ASSERT_TRUE(we_link_at(&g_doc, 10, 20, href, (int)sizeof(href)) > 0);
    TEST_ASSERT_EQUAL_STRING("/x.html", href);
}

static void test_l69_shadow_scaled(void) {
    uint32_t i, n = 0;
    lot52_render(L69("p{font-size:32px;text-shadow:2px 2px #f00}", "<p>s</p>"), 400);
    for (i = 0; i < g_doc.nitems; i++)
        if (g_doc.items[i].kind == OS_FB_WEB_TEXT && !strcmp(g_doc.items[i].text, "s")) {
            TEST_ASSERT_EQUAL(16, g_doc.items[i].h);
            TEST_ASSERT_EQUAL(8, g_doc.items[i].pad);
            n++;
        }
    TEST_ASSERT_EQUAL(2, (int)n);
}

static void lot69_run(void) {
    RUN_TEST(test_l69_parse_font_size);
    RUN_TEST(test_l69_font_q);
    RUN_TEST(test_l69_heading_defaults);
    RUN_TEST(test_l69_small_headings_clamp);
    RUN_TEST(test_l69_px_em_inherit);
    RUN_TEST(test_l69_em_does_not_compound_on_redeclare);
    RUN_TEST(test_l69_rem_keywords_percent);
    RUN_TEST(test_l69_refused_font_size);
    RUN_TEST(test_l69_inline_big_text_raises_line);
    RUN_TEST(test_l69_wrap_uses_scaled_advance);
    RUN_TEST(test_l69_unitless_line_height_follows_font);
    RUN_TEST(test_l69_letter_spacing_scaled);
    RUN_TEST(test_l69_ellipsis_scaled);
    RUN_TEST(test_l69_clip_drops_cut_scaled_glyphs);
    RUN_TEST(test_l69_link_hit_on_big_text);
    RUN_TEST(test_l69_shadow_scaled);
}
