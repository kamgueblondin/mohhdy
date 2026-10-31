/* Lot 64 C: translucent radial-gradient stops paint disjoint rings. */

/* every pixel of (x, y, w, h) covered by exactly one RECT of node owner */
static int l64_cover_once(int owner, int x, int y, int w, int h) {
    int px, py;
    uint32_t i;
    for (py = y; py < y + h; py++)
        for (px = x; px < x + w; px++) {
            int hits = 0;
            for (i = 0; i < g_doc.nitems; i++) {
                const os_fb_web_item_t *it = &g_doc.items[i];
                if (it->kind == OS_FB_WEB_RECT && g_doc.item_owner[i] == owner &&
                    px >= it->x && px < it->x + it->w && py >= it->y && py < it->y + it->h) hits++;
            }
            if (hits != 1) return 0;
        }
    return 1;
}

static int l64_owned(int owner) {
    uint32_t i;
    int count = 0;
    for (i = 0; i < g_doc.nitems; i++) if (g_doc.item_owner[i] == owner && g_doc.items[i].kind == OS_FB_WEB_RECT) count++;
    return count;
}

static void test_l64_radial_translucent_rings(void) {
    int g;
    lot52_render("<body style='margin:0;background:#ffffff'><div id=g style='width:40px;height:30px;"
                 "background:radial-gradient(rgba(255,0,0,0.5),rgba(0,0,255,0.5))'></div></body>", 320);
    g = (int)(lot52_node("g") - g_doc.nodes);
    TEST_ASSERT_EQUAL(0, l61_diag_count("radial alpha painted opaque"));
    TEST_ASSERT_TRUE(l64_cover_once(g, 0, 0, 40, 30));
    TEST_ASSERT_EQUAL(39, l64_owned(g));
    l61_rect_at(11, 12, 18, 6, 0xEA7F94);                     /* inner ring: 5/6 red over white */
    l61_rect_at(2, 12, 9, 6, 0xBF7FBF);                       /* middle ring: half and half */
    l61_rect_at(29, 12, 9, 6, 0xBF7FBF);
    l61_rect_at(0, 12, 2, 6, 0x957FE9);                       /* outer ring */
    l61_rect_at(0, 0, 40, 1, 0x957FE9);
    /* the clip box outside the outermost ellipse takes the last stop */
    lot52_render("<body style='margin:0;background:#ffffff'><div id=g style='width:40px;height:30px;"
                 "background:radial-gradient(circle closest-side at 10px 10px,rgba(255,0,0,0.5),#0000ff)'></div></body>", 320);
    g = (int)(lot52_node("g") - g_doc.nodes);
    TEST_ASSERT_TRUE(l64_cover_once(g, 0, 0, 40, 30));
    l61_rect_at(15, 0, 25, 3, 0x0000FF);
    l61_rect_at(0, 20, 40, 10, 0x0000FF);
    /* a translucent stop over a coloured parent composes with it */
    lot52_render("<body style='margin:0'><div style='background:#00ff00;width:60px'><div id=g style='width:40px;height:30px;"
                 "background:radial-gradient(rgba(255,0,0,0.5),rgba(255,0,0,0.5))'></div></div></body>", 320);
    g = (int)(lot52_node("g") - g_doc.nodes);
    TEST_ASSERT_TRUE(l64_cover_once(g, 0, 0, 40, 30));
    l61_rect_at(11, 12, 18, 6, 0x807F00);
    l61_rect_at(0, 0, 40, 1, 0x807F00);
}

static void test_l64_radial_translucent_budget(void) {
    char html[1200];
    int k, at, g;
    /* opaque stops keep the nested rings and their smaller reservation */
    lot52_render("<body style='margin:0'><div id=g style='width:40px;height:30px;"
                 "background:radial-gradient(#ff0000,#0000ff)'></div></body>", 320);
    g = (int)(lot52_node("g") - g_doc.nodes);
    TEST_ASSERT_FALSE(l64_cover_once(g, 15, 13, 2, 2));       /* overlapping */
    TEST_ASSERT_EQUAL(0, l61_diag_count("radial alpha painted opaque"));
    /* no room for the disjoint rings: nested rings painted without alpha */
    at = snprintf(html, sizeof(html), "<body style='margin:0'><div style='width:8px'>");
    for (k = 0; k < 80; k++) at += snprintf(html + at, sizeof(html) - (size_t)at, "a ");
    snprintf(html + at, sizeof(html) - (size_t)at, "</div><div id=g style='width:40px;height:30px;"
             "background:radial-gradient(rgba(255,0,0,0.5),rgba(0,0,255,0.5))'></div></body>");
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, l61_diag_count("radial alpha painted opaque"));
    l61_rect_at(0, 800, 40, 30, 0x0000FF);
    l61_rect_at(15, 808, 10, 2, 0xD5002A);
    TEST_ASSERT_EQUAL(OS_FB_WEB_ITEMS - 4, (int)g_doc.nitems);
}

static void lot64_radial_run(void) {
    RUN_TEST(test_l64_radial_translucent_rings);
    RUN_TEST(test_l64_radial_translucent_budget);
}
