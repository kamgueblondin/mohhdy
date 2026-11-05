/* Lot 74 systematic proofs round 2: scrollbars, accent, sticky+forms, fragments */

static void test_l74_scroll_0(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_1(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_2(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_3(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_4(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_5(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_6(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_7(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_8(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_9(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_10(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_11(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_12(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_13(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_14(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_15(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_16(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_17(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_18(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_19(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_20(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_21(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_22(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_23(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_24(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_25(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_26(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_27(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_28(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_29(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_30(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_31(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_32(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_33(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_34(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_35(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_36(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_37(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_38(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_39(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_40(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_41(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_42(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_43(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_44(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_45(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_46(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:36px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_47(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:40px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_48(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:28px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_scroll_49(void) {
    int x,y,bars=0,j;
    l74_open("<style>#b{overflow:scroll;height:32px;width:100px}#b p{height:18px;margin:0}</style>"
             "<div id=b><p id=a>a</p><p>b</p><p>c</p><p>d</p><p>e</p></div>");
    for (j=0;j<(int)g_doc.nitems;j++)
        if (g_doc.item_owner[j]==l74_node("b") && g_doc.items[j].w==6) bars++;
    TEST_ASSERT_TRUE(bars>=2);
    x=g_doc.nodes[l74_node("b")].x+g_doc.nodes[l74_node("b")].w-3;
    y=g_doc.nodes[l74_node("b")].y+g_doc.nodes[l74_node("b")].h-2;
    TEST_ASSERT_TRUE(we_box_scroll_click(&g_doc,x,y)>=1);
    l74_done();
}

static void test_l74_acc_0(void) {
    l74_open("<style>input{accent-color:#100000}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100000, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_1(void) {
    l74_open("<style>input{accent-color:#100100}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100100, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_2(void) {
    l74_open("<style>input{accent-color:#100200}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100200, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_3(void) {
    l74_open("<style>input{accent-color:#100300}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100300, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_4(void) {
    l74_open("<style>input{accent-color:#100400}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100400, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_5(void) {
    l74_open("<style>input{accent-color:#100500}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100500, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_6(void) {
    l74_open("<style>input{accent-color:#100600}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100600, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_7(void) {
    l74_open("<style>input{accent-color:#100700}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100700, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_8(void) {
    l74_open("<style>input{accent-color:#100800}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100800, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_9(void) {
    l74_open("<style>input{accent-color:#100900}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100900, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_10(void) {
    l74_open("<style>input{accent-color:#100a00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100a00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_11(void) {
    l74_open("<style>input{accent-color:#100b00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100b00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_12(void) {
    l74_open("<style>input{accent-color:#100c00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100c00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_13(void) {
    l74_open("<style>input{accent-color:#100d00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100d00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_14(void) {
    l74_open("<style>input{accent-color:#100e00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100e00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_15(void) {
    l74_open("<style>input{accent-color:#100f00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x100f00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_16(void) {
    l74_open("<style>input{accent-color:#101000}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101000, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_17(void) {
    l74_open("<style>input{accent-color:#101100}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101100, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_18(void) {
    l74_open("<style>input{accent-color:#101200}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101200, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_19(void) {
    l74_open("<style>input{accent-color:#101300}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101300, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_20(void) {
    l74_open("<style>input{accent-color:#101400}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101400, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_21(void) {
    l74_open("<style>input{accent-color:#101500}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101500, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_22(void) {
    l74_open("<style>input{accent-color:#101600}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101600, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_23(void) {
    l74_open("<style>input{accent-color:#101700}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101700, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_24(void) {
    l74_open("<style>input{accent-color:#101800}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101800, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_25(void) {
    l74_open("<style>input{accent-color:#101900}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101900, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_26(void) {
    l74_open("<style>input{accent-color:#101a00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101a00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_27(void) {
    l74_open("<style>input{accent-color:#101b00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101b00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_28(void) {
    l74_open("<style>input{accent-color:#101c00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101c00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_29(void) {
    l74_open("<style>input{accent-color:#101d00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101d00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_30(void) {
    l74_open("<style>input{accent-color:#101e00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101e00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_31(void) {
    l74_open("<style>input{accent-color:#101f00}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x101f00, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_32(void) {
    l74_open("<style>input{accent-color:#102000}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102000, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_33(void) {
    l74_open("<style>input{accent-color:#102100}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102100, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_34(void) {
    l74_open("<style>input{accent-color:#102200}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102200, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_35(void) {
    l74_open("<style>input{accent-color:#102300}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102300, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_36(void) {
    l74_open("<style>input{accent-color:#102400}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102400, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_37(void) {
    l74_open("<style>input{accent-color:#102500}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102500, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_38(void) {
    l74_open("<style>input{accent-color:#102600}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102600, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_acc_39(void) {
    l74_open("<style>input{accent-color:#102700}</style><form><input id=c type=checkbox></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("c")].accent_set);
    TEST_ASSERT_EQUAL(0x102700, g_doc.nodes[l74_node("c")].accent & 0xffffff);
    l74_done();
}

static void test_l74_frag_0(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t0>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t0"));
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t0>t</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("t0"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_1(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t1>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t1"));
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t1>t</p></body>", 320);
    TEST_ASSERT_EQUAL(2, l73c("t1"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_2(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t2>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t2"));
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t2>t</p></body>", 320);
    TEST_ASSERT_EQUAL(3, l73c("t2"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_3(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t3>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t3"));
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t3>t</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l73c("t3"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_4(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t4>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t4"));
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t4>t</p></body>", 320);
    TEST_ASSERT_EQUAL(5, l73c("t4"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_5(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t5>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t5"));
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t5>t</p></body>", 320);
    TEST_ASSERT_EQUAL(6, l73c("t5"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_6(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t6>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t6"));
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t6>t</p></body>", 320);
    TEST_ASSERT_EQUAL(7, l73c("t6"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_7(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t7>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t7"));
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t7>t</p></body>", 320);
    TEST_ASSERT_EQUAL(8, l73c("t7"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_8(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t8>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t8"));
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t8>t</p></body>", 320);
    TEST_ASSERT_EQUAL(9, l73c("t8"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_9(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t9>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t9"));
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t9>t</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("t9"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_10(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t10>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t10"));
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t10>t</p></body>", 320);
    TEST_ASSERT_EQUAL(2, l73c("t10"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_11(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t11>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t11"));
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t11>t</p></body>", 320);
    TEST_ASSERT_EQUAL(3, l73c("t11"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_12(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t12>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t12"));
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t12>t</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l73c("t12"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_13(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t13>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t13"));
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t13>t</p></body>", 320);
    TEST_ASSERT_EQUAL(5, l73c("t13"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_14(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t14>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t14"));
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t14>t</p></body>", 320);
    TEST_ASSERT_EQUAL(6, l73c("t14"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_15(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t15>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t15"));
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t15>t</p></body>", 320);
    TEST_ASSERT_EQUAL(7, l73c("t15"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_16(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t16>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t16"));
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t16>t</p></body>", 320);
    TEST_ASSERT_EQUAL(8, l73c("t16"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_17(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t17>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t17"));
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t17>t</p></body>", 320);
    TEST_ASSERT_EQUAL(9, l73c("t17"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_18(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t18>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t18"));
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t18>t</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("t18"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_19(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t19>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t19"));
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t19>t</p></body>", 320);
    TEST_ASSERT_EQUAL(2, l73c("t19"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_20(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t20>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t20"));
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t20>t</p></body>", 320);
    TEST_ASSERT_EQUAL(3, l73c("t20"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_21(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t21>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t21"));
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t21>t</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l73c("t21"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_22(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t22>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t22"));
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t22>t</p></body>", 320);
    TEST_ASSERT_EQUAL(5, l73c("t22"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_23(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t23>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t23"));
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t23>t</p></body>", 320);
    TEST_ASSERT_EQUAL(6, l73c("t23"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_24(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t24>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t24"));
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t24>t</p></body>", 320);
    TEST_ASSERT_EQUAL(7, l73c("t24"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_25(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t25>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t25"));
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t25>t</p></body>", 320);
    TEST_ASSERT_EQUAL(8, l73c("t25"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_26(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t26>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t26"));
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t26>t</p></body>", 320);
    TEST_ASSERT_EQUAL(9, l73c("t26"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_27(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t27>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t27"));
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t27>t</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("t27"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_28(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t28>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t28"));
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t28>t</p></body>", 320);
    TEST_ASSERT_EQUAL(2, l73c("t28"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_29(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t29>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t29"));
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t29>t</p></body>", 320);
    TEST_ASSERT_EQUAL(3, l73c("t29"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_30(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t30>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t30"));
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t30>t</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l73c("t30"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_31(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t31>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t31"));
    lot52_render("<style>:target{color:#000005}</style>" L64_BODY
                 "<p id=t31>t</p></body>", 320);
    TEST_ASSERT_EQUAL(5, l73c("t31"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_32(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t32>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t32"));
    lot52_render("<style>:target{color:#000006}</style>" L64_BODY
                 "<p id=t32>t</p></body>", 320);
    TEST_ASSERT_EQUAL(6, l73c("t32"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_33(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t33>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t33"));
    lot52_render("<style>:target{color:#000007}</style>" L64_BODY
                 "<p id=t33>t</p></body>", 320);
    TEST_ASSERT_EQUAL(7, l73c("t33"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_34(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t34>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t34"));
    lot52_render("<style>:target{color:#000008}</style>" L64_BODY
                 "<p id=t34>t</p></body>", 320);
    TEST_ASSERT_EQUAL(8, l73c("t34"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_35(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t35>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t35"));
    lot52_render("<style>:target{color:#000009}</style>" L64_BODY
                 "<p id=t35>t</p></body>", 320);
    TEST_ASSERT_EQUAL(9, l73c("t35"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_36(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t36>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t36"));
    lot52_render("<style>:target{color:#000001}</style>" L64_BODY
                 "<p id=t36>t</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("t36"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_37(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t37>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t37"));
    lot52_render("<style>:target{color:#000002}</style>" L64_BODY
                 "<p id=t37>t</p></body>", 320);
    TEST_ASSERT_EQUAL(2, l73c("t37"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_38(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t38>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t38"));
    lot52_render("<style>:target{color:#000003}</style>" L64_BODY
                 "<p id=t38>t</p></body>", 320);
    TEST_ASSERT_EQUAL(3, l73c("t38"));
    we_set_forms(&g_doc,0);
}

static void test_l74_frag_39(void) {
    static we_forms_t f;
    we_forms_init(&f); we_set_forms(&g_doc,&f);
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t39>t</p></body>", 320);
    TEST_ASSERT_EQUAL(0, we_set_target(&g_doc, "t39"));
    lot52_render("<style>:target{color:#000004}</style>" L64_BODY
                 "<p id=t39>t</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l73c("t39"));
    we_set_forms(&g_doc,0);
}

static void test_l74_app_0(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v0></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_1(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v1></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_2(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v2></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_3(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v3></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_4(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v4></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_5(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v5></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_6(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v6></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_7(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v7></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_8(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v8></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_9(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v9></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_10(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v10></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_11(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v11></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_12(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v12></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_13(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v13></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_14(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v14></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_15(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v15></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_16(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v16></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_17(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v17></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_18(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v18></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_19(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v19></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_20(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v20></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_21(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v21></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_22(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v22></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_23(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v23></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_24(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v24></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_25(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v25></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_26(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v26></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_27(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v27></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_28(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v28></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void test_l74_app_29(void) {
    l74_open("<style>#n{appearance:none}</style><form><input id=n type=text value=v29></form>");
    TEST_ASSERT_EQUAL(1, g_doc.nodes[l74_node("n")].appearance);
    TEST_ASSERT_EQUAL(0, g_doc.nodes[l74_node("n")].border);
    l74_done();
}

static void lot74_sys2_tests(void) {
    RUN_TEST(test_l74_scroll_0);
    RUN_TEST(test_l74_scroll_1);
    RUN_TEST(test_l74_scroll_2);
    RUN_TEST(test_l74_scroll_3);
    RUN_TEST(test_l74_scroll_4);
    RUN_TEST(test_l74_scroll_5);
    RUN_TEST(test_l74_scroll_6);
    RUN_TEST(test_l74_scroll_7);
    RUN_TEST(test_l74_scroll_8);
    RUN_TEST(test_l74_scroll_9);
    RUN_TEST(test_l74_scroll_10);
    RUN_TEST(test_l74_scroll_11);
    RUN_TEST(test_l74_scroll_12);
    RUN_TEST(test_l74_scroll_13);
    RUN_TEST(test_l74_scroll_14);
    RUN_TEST(test_l74_scroll_15);
    RUN_TEST(test_l74_scroll_16);
    RUN_TEST(test_l74_scroll_17);
    RUN_TEST(test_l74_scroll_18);
    RUN_TEST(test_l74_scroll_19);
    RUN_TEST(test_l74_scroll_20);
    RUN_TEST(test_l74_scroll_21);
    RUN_TEST(test_l74_scroll_22);
    RUN_TEST(test_l74_scroll_23);
    RUN_TEST(test_l74_scroll_24);
    RUN_TEST(test_l74_scroll_25);
    RUN_TEST(test_l74_scroll_26);
    RUN_TEST(test_l74_scroll_27);
    RUN_TEST(test_l74_scroll_28);
    RUN_TEST(test_l74_scroll_29);
    RUN_TEST(test_l74_scroll_30);
    RUN_TEST(test_l74_scroll_31);
    RUN_TEST(test_l74_scroll_32);
    RUN_TEST(test_l74_scroll_33);
    RUN_TEST(test_l74_scroll_34);
    RUN_TEST(test_l74_scroll_35);
    RUN_TEST(test_l74_scroll_36);
    RUN_TEST(test_l74_scroll_37);
    RUN_TEST(test_l74_scroll_38);
    RUN_TEST(test_l74_scroll_39);
    RUN_TEST(test_l74_scroll_40);
    RUN_TEST(test_l74_scroll_41);
    RUN_TEST(test_l74_scroll_42);
    RUN_TEST(test_l74_scroll_43);
    RUN_TEST(test_l74_scroll_44);
    RUN_TEST(test_l74_scroll_45);
    RUN_TEST(test_l74_scroll_46);
    RUN_TEST(test_l74_scroll_47);
    RUN_TEST(test_l74_scroll_48);
    RUN_TEST(test_l74_scroll_49);
    RUN_TEST(test_l74_acc_0);
    RUN_TEST(test_l74_acc_1);
    RUN_TEST(test_l74_acc_2);
    RUN_TEST(test_l74_acc_3);
    RUN_TEST(test_l74_acc_4);
    RUN_TEST(test_l74_acc_5);
    RUN_TEST(test_l74_acc_6);
    RUN_TEST(test_l74_acc_7);
    RUN_TEST(test_l74_acc_8);
    RUN_TEST(test_l74_acc_9);
    RUN_TEST(test_l74_acc_10);
    RUN_TEST(test_l74_acc_11);
    RUN_TEST(test_l74_acc_12);
    RUN_TEST(test_l74_acc_13);
    RUN_TEST(test_l74_acc_14);
    RUN_TEST(test_l74_acc_15);
    RUN_TEST(test_l74_acc_16);
    RUN_TEST(test_l74_acc_17);
    RUN_TEST(test_l74_acc_18);
    RUN_TEST(test_l74_acc_19);
    RUN_TEST(test_l74_acc_20);
    RUN_TEST(test_l74_acc_21);
    RUN_TEST(test_l74_acc_22);
    RUN_TEST(test_l74_acc_23);
    RUN_TEST(test_l74_acc_24);
    RUN_TEST(test_l74_acc_25);
    RUN_TEST(test_l74_acc_26);
    RUN_TEST(test_l74_acc_27);
    RUN_TEST(test_l74_acc_28);
    RUN_TEST(test_l74_acc_29);
    RUN_TEST(test_l74_acc_30);
    RUN_TEST(test_l74_acc_31);
    RUN_TEST(test_l74_acc_32);
    RUN_TEST(test_l74_acc_33);
    RUN_TEST(test_l74_acc_34);
    RUN_TEST(test_l74_acc_35);
    RUN_TEST(test_l74_acc_36);
    RUN_TEST(test_l74_acc_37);
    RUN_TEST(test_l74_acc_38);
    RUN_TEST(test_l74_acc_39);
    RUN_TEST(test_l74_frag_0);
    RUN_TEST(test_l74_frag_1);
    RUN_TEST(test_l74_frag_2);
    RUN_TEST(test_l74_frag_3);
    RUN_TEST(test_l74_frag_4);
    RUN_TEST(test_l74_frag_5);
    RUN_TEST(test_l74_frag_6);
    RUN_TEST(test_l74_frag_7);
    RUN_TEST(test_l74_frag_8);
    RUN_TEST(test_l74_frag_9);
    RUN_TEST(test_l74_frag_10);
    RUN_TEST(test_l74_frag_11);
    RUN_TEST(test_l74_frag_12);
    RUN_TEST(test_l74_frag_13);
    RUN_TEST(test_l74_frag_14);
    RUN_TEST(test_l74_frag_15);
    RUN_TEST(test_l74_frag_16);
    RUN_TEST(test_l74_frag_17);
    RUN_TEST(test_l74_frag_18);
    RUN_TEST(test_l74_frag_19);
    RUN_TEST(test_l74_frag_20);
    RUN_TEST(test_l74_frag_21);
    RUN_TEST(test_l74_frag_22);
    RUN_TEST(test_l74_frag_23);
    RUN_TEST(test_l74_frag_24);
    RUN_TEST(test_l74_frag_25);
    RUN_TEST(test_l74_frag_26);
    RUN_TEST(test_l74_frag_27);
    RUN_TEST(test_l74_frag_28);
    RUN_TEST(test_l74_frag_29);
    RUN_TEST(test_l74_frag_30);
    RUN_TEST(test_l74_frag_31);
    RUN_TEST(test_l74_frag_32);
    RUN_TEST(test_l74_frag_33);
    RUN_TEST(test_l74_frag_34);
    RUN_TEST(test_l74_frag_35);
    RUN_TEST(test_l74_frag_36);
    RUN_TEST(test_l74_frag_37);
    RUN_TEST(test_l74_frag_38);
    RUN_TEST(test_l74_frag_39);
    RUN_TEST(test_l74_app_0);
    RUN_TEST(test_l74_app_1);
    RUN_TEST(test_l74_app_2);
    RUN_TEST(test_l74_app_3);
    RUN_TEST(test_l74_app_4);
    RUN_TEST(test_l74_app_5);
    RUN_TEST(test_l74_app_6);
    RUN_TEST(test_l74_app_7);
    RUN_TEST(test_l74_app_8);
    RUN_TEST(test_l74_app_9);
    RUN_TEST(test_l74_app_10);
    RUN_TEST(test_l74_app_11);
    RUN_TEST(test_l74_app_12);
    RUN_TEST(test_l74_app_13);
    RUN_TEST(test_l74_app_14);
    RUN_TEST(test_l74_app_15);
    RUN_TEST(test_l74_app_16);
    RUN_TEST(test_l74_app_17);
    RUN_TEST(test_l74_app_18);
    RUN_TEST(test_l74_app_19);
    RUN_TEST(test_l74_app_20);
    RUN_TEST(test_l74_app_21);
    RUN_TEST(test_l74_app_22);
    RUN_TEST(test_l74_app_23);
    RUN_TEST(test_l74_app_24);
    RUN_TEST(test_l74_app_25);
    RUN_TEST(test_l74_app_26);
    RUN_TEST(test_l74_app_27);
    RUN_TEST(test_l74_app_28);
    RUN_TEST(test_l74_app_29);
}
