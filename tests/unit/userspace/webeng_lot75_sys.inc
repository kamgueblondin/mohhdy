
static void test_l75_sys_1(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (1 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n1>item 1</div>",
            c & 0xffffffu, 1%40, (1*2)%40, 1%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n1")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n1")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n1")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n1")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n1")->scroll_m_set);
    TEST_ASSERT_EQUAL(1%40, lot52_node("n1")->scroll_m[0]);
    TEST_ASSERT_EQUAL((1*2)%40, lot52_node("n1")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n1")->scroll_p_set);
    TEST_ASSERT_EQUAL(1%15, lot52_node("n1")->scroll_p[0]);
}

static void test_l75_sys_2(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (2 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n2>item 2</div>",
            c & 0xffffffu, 2%40, (2*2)%40, 2%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n2")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n2")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n2")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n2")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n2")->scroll_m_set);
    TEST_ASSERT_EQUAL(2%40, lot52_node("n2")->scroll_m[0]);
    TEST_ASSERT_EQUAL((2*2)%40, lot52_node("n2")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n2")->scroll_p_set);
    TEST_ASSERT_EQUAL(2%15, lot52_node("n2")->scroll_p[0]);
}

static void test_l75_sys_3(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (3 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n3>item 3</div>",
            c & 0xffffffu, 3%40, (3*2)%40, 3%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n3")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n3")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n3")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n3")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n3")->scroll_m_set);
    TEST_ASSERT_EQUAL(3%40, lot52_node("n3")->scroll_m[0]);
    TEST_ASSERT_EQUAL((3*2)%40, lot52_node("n3")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n3")->scroll_p_set);
    TEST_ASSERT_EQUAL(3%15, lot52_node("n3")->scroll_p[0]);
}

static void test_l75_sys_4(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (4 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n4>item 4</div>",
            c & 0xffffffu, 4%40, (4*2)%40, 4%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n4")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n4")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n4")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n4")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n4")->scroll_m_set);
    TEST_ASSERT_EQUAL(4%40, lot52_node("n4")->scroll_m[0]);
    TEST_ASSERT_EQUAL((4*2)%40, lot52_node("n4")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n4")->scroll_p_set);
    TEST_ASSERT_EQUAL(4%15, lot52_node("n4")->scroll_p[0]);
}

static void test_l75_sys_5(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (5 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n5>item 5</div>",
            c & 0xffffffu, 5%40, (5*2)%40, 5%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n5")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n5")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n5")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n5")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n5")->scroll_m_set);
    TEST_ASSERT_EQUAL(5%40, lot52_node("n5")->scroll_m[0]);
    TEST_ASSERT_EQUAL((5*2)%40, lot52_node("n5")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n5")->scroll_p_set);
    TEST_ASSERT_EQUAL(5%15, lot52_node("n5")->scroll_p[0]);
}

static void test_l75_sys_6(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (6 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n6>item 6</div>",
            c & 0xffffffu, 6%40, (6*2)%40, 6%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n6")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n6")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n6")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n6")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n6")->scroll_m_set);
    TEST_ASSERT_EQUAL(6%40, lot52_node("n6")->scroll_m[0]);
    TEST_ASSERT_EQUAL((6*2)%40, lot52_node("n6")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n6")->scroll_p_set);
    TEST_ASSERT_EQUAL(6%15, lot52_node("n6")->scroll_p[0]);
}

static void test_l75_sys_7(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (7 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n7>item 7</div>",
            c & 0xffffffu, 7%40, (7*2)%40, 7%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n7")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n7")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n7")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n7")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n7")->scroll_m_set);
    TEST_ASSERT_EQUAL(7%40, lot52_node("n7")->scroll_m[0]);
    TEST_ASSERT_EQUAL((7*2)%40, lot52_node("n7")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n7")->scroll_p_set);
    TEST_ASSERT_EQUAL(7%15, lot52_node("n7")->scroll_p[0]);
}

static void test_l75_sys_8(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (8 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n8>item 8</div>",
            c & 0xffffffu, 8%40, (8*2)%40, 8%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n8")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n8")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n8")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n8")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n8")->scroll_m_set);
    TEST_ASSERT_EQUAL(8%40, lot52_node("n8")->scroll_m[0]);
    TEST_ASSERT_EQUAL((8*2)%40, lot52_node("n8")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n8")->scroll_p_set);
    TEST_ASSERT_EQUAL(8%15, lot52_node("n8")->scroll_p[0]);
}

static void test_l75_sys_9(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (9 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n9>item 9</div>",
            c & 0xffffffu, 9%40, (9*2)%40, 9%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n9")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n9")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n9")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n9")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n9")->scroll_m_set);
    TEST_ASSERT_EQUAL(9%40, lot52_node("n9")->scroll_m[0]);
    TEST_ASSERT_EQUAL((9*2)%40, lot52_node("n9")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n9")->scroll_p_set);
    TEST_ASSERT_EQUAL(9%15, lot52_node("n9")->scroll_p[0]);
}

static void test_l75_sys_10(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (10 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n10>item 10</div>",
            c & 0xffffffu, 10%40, (10*2)%40, 10%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n10")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n10")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n10")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n10")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n10")->scroll_m_set);
    TEST_ASSERT_EQUAL(10%40, lot52_node("n10")->scroll_m[0]);
    TEST_ASSERT_EQUAL((10*2)%40, lot52_node("n10")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n10")->scroll_p_set);
    TEST_ASSERT_EQUAL(10%15, lot52_node("n10")->scroll_p[0]);
}

static void test_l75_sys_11(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (11 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n11>item 11</div>",
            c & 0xffffffu, 11%40, (11*2)%40, 11%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n11")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n11")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n11")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n11")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n11")->scroll_m_set);
    TEST_ASSERT_EQUAL(11%40, lot52_node("n11")->scroll_m[0]);
    TEST_ASSERT_EQUAL((11*2)%40, lot52_node("n11")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n11")->scroll_p_set);
    TEST_ASSERT_EQUAL(11%15, lot52_node("n11")->scroll_p[0]);
}

static void test_l75_sys_12(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (12 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n12>item 12</div>",
            c & 0xffffffu, 12%40, (12*2)%40, 12%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n12")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n12")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n12")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n12")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n12")->scroll_m_set);
    TEST_ASSERT_EQUAL(12%40, lot52_node("n12")->scroll_m[0]);
    TEST_ASSERT_EQUAL((12*2)%40, lot52_node("n12")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n12")->scroll_p_set);
    TEST_ASSERT_EQUAL(12%15, lot52_node("n12")->scroll_p[0]);
}

static void test_l75_sys_13(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (13 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n13>item 13</div>",
            c & 0xffffffu, 13%40, (13*2)%40, 13%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n13")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n13")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n13")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n13")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n13")->scroll_m_set);
    TEST_ASSERT_EQUAL(13%40, lot52_node("n13")->scroll_m[0]);
    TEST_ASSERT_EQUAL((13*2)%40, lot52_node("n13")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n13")->scroll_p_set);
    TEST_ASSERT_EQUAL(13%15, lot52_node("n13")->scroll_p[0]);
}

static void test_l75_sys_14(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (14 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n14>item 14</div>",
            c & 0xffffffu, 14%40, (14*2)%40, 14%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n14")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n14")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n14")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n14")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n14")->scroll_m_set);
    TEST_ASSERT_EQUAL(14%40, lot52_node("n14")->scroll_m[0]);
    TEST_ASSERT_EQUAL((14*2)%40, lot52_node("n14")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n14")->scroll_p_set);
    TEST_ASSERT_EQUAL(14%15, lot52_node("n14")->scroll_p[0]);
}

static void test_l75_sys_15(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (15 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n15>item 15</div>",
            c & 0xffffffu, 15%40, (15*2)%40, 15%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n15")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n15")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n15")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n15")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n15")->scroll_m_set);
    TEST_ASSERT_EQUAL(15%40, lot52_node("n15")->scroll_m[0]);
    TEST_ASSERT_EQUAL((15*2)%40, lot52_node("n15")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n15")->scroll_p_set);
    TEST_ASSERT_EQUAL(15%15, lot52_node("n15")->scroll_p[0]);
}

static void test_l75_sys_16(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (16 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n16>item 16</div>",
            c & 0xffffffu, 16%40, (16*2)%40, 16%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n16")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n16")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n16")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n16")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n16")->scroll_m_set);
    TEST_ASSERT_EQUAL(16%40, lot52_node("n16")->scroll_m[0]);
    TEST_ASSERT_EQUAL((16*2)%40, lot52_node("n16")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n16")->scroll_p_set);
    TEST_ASSERT_EQUAL(16%15, lot52_node("n16")->scroll_p[0]);
}

static void test_l75_sys_17(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (17 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n17>item 17</div>",
            c & 0xffffffu, 17%40, (17*2)%40, 17%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n17")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n17")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n17")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n17")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n17")->scroll_m_set);
    TEST_ASSERT_EQUAL(17%40, lot52_node("n17")->scroll_m[0]);
    TEST_ASSERT_EQUAL((17*2)%40, lot52_node("n17")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n17")->scroll_p_set);
    TEST_ASSERT_EQUAL(17%15, lot52_node("n17")->scroll_p[0]);
}

static void test_l75_sys_18(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (18 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n18>item 18</div>",
            c & 0xffffffu, 18%40, (18*2)%40, 18%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n18")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n18")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n18")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n18")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n18")->scroll_m_set);
    TEST_ASSERT_EQUAL(18%40, lot52_node("n18")->scroll_m[0]);
    TEST_ASSERT_EQUAL((18*2)%40, lot52_node("n18")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n18")->scroll_p_set);
    TEST_ASSERT_EQUAL(18%15, lot52_node("n18")->scroll_p[0]);
}

static void test_l75_sys_19(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (19 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n19>item 19</div>",
            c & 0xffffffu, 19%40, (19*2)%40, 19%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n19")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n19")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n19")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n19")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n19")->scroll_m_set);
    TEST_ASSERT_EQUAL(19%40, lot52_node("n19")->scroll_m[0]);
    TEST_ASSERT_EQUAL((19*2)%40, lot52_node("n19")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n19")->scroll_p_set);
    TEST_ASSERT_EQUAL(19%15, lot52_node("n19")->scroll_p[0]);
}

static void test_l75_sys_20(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (20 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n20>item 20</div>",
            c & 0xffffffu, 20%40, (20*2)%40, 20%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n20")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n20")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n20")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n20")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n20")->scroll_m_set);
    TEST_ASSERT_EQUAL(20%40, lot52_node("n20")->scroll_m[0]);
    TEST_ASSERT_EQUAL((20*2)%40, lot52_node("n20")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n20")->scroll_p_set);
    TEST_ASSERT_EQUAL(20%15, lot52_node("n20")->scroll_p[0]);
}

static void test_l75_sys_21(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (21 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n21>item 21</div>",
            c & 0xffffffu, 21%40, (21*2)%40, 21%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n21")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n21")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n21")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n21")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n21")->scroll_m_set);
    TEST_ASSERT_EQUAL(21%40, lot52_node("n21")->scroll_m[0]);
    TEST_ASSERT_EQUAL((21*2)%40, lot52_node("n21")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n21")->scroll_p_set);
    TEST_ASSERT_EQUAL(21%15, lot52_node("n21")->scroll_p[0]);
}

static void test_l75_sys_22(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (22 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n22>item 22</div>",
            c & 0xffffffu, 22%40, (22*2)%40, 22%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n22")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n22")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n22")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n22")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n22")->scroll_m_set);
    TEST_ASSERT_EQUAL(22%40, lot52_node("n22")->scroll_m[0]);
    TEST_ASSERT_EQUAL((22*2)%40, lot52_node("n22")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n22")->scroll_p_set);
    TEST_ASSERT_EQUAL(22%15, lot52_node("n22")->scroll_p[0]);
}

static void test_l75_sys_23(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (23 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n23>item 23</div>",
            c & 0xffffffu, 23%40, (23*2)%40, 23%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n23")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n23")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n23")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n23")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n23")->scroll_m_set);
    TEST_ASSERT_EQUAL(23%40, lot52_node("n23")->scroll_m[0]);
    TEST_ASSERT_EQUAL((23*2)%40, lot52_node("n23")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n23")->scroll_p_set);
    TEST_ASSERT_EQUAL(23%15, lot52_node("n23")->scroll_p[0]);
}

static void test_l75_sys_24(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (24 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n24>item 24</div>",
            c & 0xffffffu, 24%40, (24*2)%40, 24%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n24")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n24")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n24")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n24")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n24")->scroll_m_set);
    TEST_ASSERT_EQUAL(24%40, lot52_node("n24")->scroll_m[0]);
    TEST_ASSERT_EQUAL((24*2)%40, lot52_node("n24")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n24")->scroll_p_set);
    TEST_ASSERT_EQUAL(24%15, lot52_node("n24")->scroll_p[0]);
}

static void test_l75_sys_25(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (25 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n25>item 25</div>",
            c & 0xffffffu, 25%40, (25*2)%40, 25%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n25")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n25")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n25")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n25")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n25")->scroll_m_set);
    TEST_ASSERT_EQUAL(25%40, lot52_node("n25")->scroll_m[0]);
    TEST_ASSERT_EQUAL((25*2)%40, lot52_node("n25")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n25")->scroll_p_set);
    TEST_ASSERT_EQUAL(25%15, lot52_node("n25")->scroll_p[0]);
}

static void test_l75_sys_26(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (26 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n26>item 26</div>",
            c & 0xffffffu, 26%40, (26*2)%40, 26%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n26")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n26")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n26")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n26")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n26")->scroll_m_set);
    TEST_ASSERT_EQUAL(26%40, lot52_node("n26")->scroll_m[0]);
    TEST_ASSERT_EQUAL((26*2)%40, lot52_node("n26")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n26")->scroll_p_set);
    TEST_ASSERT_EQUAL(26%15, lot52_node("n26")->scroll_p[0]);
}

static void test_l75_sys_27(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (27 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n27>item 27</div>",
            c & 0xffffffu, 27%40, (27*2)%40, 27%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n27")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n27")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n27")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n27")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n27")->scroll_m_set);
    TEST_ASSERT_EQUAL(27%40, lot52_node("n27")->scroll_m[0]);
    TEST_ASSERT_EQUAL((27*2)%40, lot52_node("n27")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n27")->scroll_p_set);
    TEST_ASSERT_EQUAL(27%15, lot52_node("n27")->scroll_p[0]);
}

static void test_l75_sys_28(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (28 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n28>item 28</div>",
            c & 0xffffffu, 28%40, (28*2)%40, 28%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n28")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n28")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n28")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n28")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n28")->scroll_m_set);
    TEST_ASSERT_EQUAL(28%40, lot52_node("n28")->scroll_m[0]);
    TEST_ASSERT_EQUAL((28*2)%40, lot52_node("n28")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n28")->scroll_p_set);
    TEST_ASSERT_EQUAL(28%15, lot52_node("n28")->scroll_p[0]);
}

static void test_l75_sys_29(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (29 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n29>item 29</div>",
            c & 0xffffffu, 29%40, (29*2)%40, 29%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n29")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n29")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n29")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n29")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n29")->scroll_m_set);
    TEST_ASSERT_EQUAL(29%40, lot52_node("n29")->scroll_m[0]);
    TEST_ASSERT_EQUAL((29*2)%40, lot52_node("n29")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n29")->scroll_p_set);
    TEST_ASSERT_EQUAL(29%15, lot52_node("n29")->scroll_p[0]);
}

static void test_l75_sys_30(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (30 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n30>item 30</div>",
            c & 0xffffffu, 30%40, (30*2)%40, 30%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n30")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n30")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n30")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n30")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n30")->scroll_m_set);
    TEST_ASSERT_EQUAL(30%40, lot52_node("n30")->scroll_m[0]);
    TEST_ASSERT_EQUAL((30*2)%40, lot52_node("n30")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n30")->scroll_p_set);
    TEST_ASSERT_EQUAL(30%15, lot52_node("n30")->scroll_p[0]);
}

static void test_l75_sys_31(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (31 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n31>item 31</div>",
            c & 0xffffffu, 31%40, (31*2)%40, 31%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n31")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n31")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n31")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n31")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n31")->scroll_m_set);
    TEST_ASSERT_EQUAL(31%40, lot52_node("n31")->scroll_m[0]);
    TEST_ASSERT_EQUAL((31*2)%40, lot52_node("n31")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n31")->scroll_p_set);
    TEST_ASSERT_EQUAL(31%15, lot52_node("n31")->scroll_p[0]);
}

static void test_l75_sys_32(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (32 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n32>item 32</div>",
            c & 0xffffffu, 32%40, (32*2)%40, 32%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n32")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n32")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n32")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n32")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n32")->scroll_m_set);
    TEST_ASSERT_EQUAL(32%40, lot52_node("n32")->scroll_m[0]);
    TEST_ASSERT_EQUAL((32*2)%40, lot52_node("n32")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n32")->scroll_p_set);
    TEST_ASSERT_EQUAL(32%15, lot52_node("n32")->scroll_p[0]);
}

static void test_l75_sys_33(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (33 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n33>item 33</div>",
            c & 0xffffffu, 33%40, (33*2)%40, 33%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n33")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n33")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n33")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n33")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n33")->scroll_m_set);
    TEST_ASSERT_EQUAL(33%40, lot52_node("n33")->scroll_m[0]);
    TEST_ASSERT_EQUAL((33*2)%40, lot52_node("n33")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n33")->scroll_p_set);
    TEST_ASSERT_EQUAL(33%15, lot52_node("n33")->scroll_p[0]);
}

static void test_l75_sys_34(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (34 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n34>item 34</div>",
            c & 0xffffffu, 34%40, (34*2)%40, 34%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n34")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n34")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n34")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n34")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n34")->scroll_m_set);
    TEST_ASSERT_EQUAL(34%40, lot52_node("n34")->scroll_m[0]);
    TEST_ASSERT_EQUAL((34*2)%40, lot52_node("n34")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n34")->scroll_p_set);
    TEST_ASSERT_EQUAL(34%15, lot52_node("n34")->scroll_p[0]);
}

static void test_l75_sys_35(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (35 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n35>item 35</div>",
            c & 0xffffffu, 35%40, (35*2)%40, 35%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n35")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n35")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n35")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n35")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n35")->scroll_m_set);
    TEST_ASSERT_EQUAL(35%40, lot52_node("n35")->scroll_m[0]);
    TEST_ASSERT_EQUAL((35*2)%40, lot52_node("n35")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n35")->scroll_p_set);
    TEST_ASSERT_EQUAL(35%15, lot52_node("n35")->scroll_p[0]);
}

static void test_l75_sys_36(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (36 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n36>item 36</div>",
            c & 0xffffffu, 36%40, (36*2)%40, 36%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n36")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n36")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n36")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n36")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n36")->scroll_m_set);
    TEST_ASSERT_EQUAL(36%40, lot52_node("n36")->scroll_m[0]);
    TEST_ASSERT_EQUAL((36*2)%40, lot52_node("n36")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n36")->scroll_p_set);
    TEST_ASSERT_EQUAL(36%15, lot52_node("n36")->scroll_p[0]);
}

static void test_l75_sys_37(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (37 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n37>item 37</div>",
            c & 0xffffffu, 37%40, (37*2)%40, 37%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n37")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n37")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n37")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n37")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n37")->scroll_m_set);
    TEST_ASSERT_EQUAL(37%40, lot52_node("n37")->scroll_m[0]);
    TEST_ASSERT_EQUAL((37*2)%40, lot52_node("n37")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n37")->scroll_p_set);
    TEST_ASSERT_EQUAL(37%15, lot52_node("n37")->scroll_p[0]);
}

static void test_l75_sys_38(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (38 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n38>item 38</div>",
            c & 0xffffffu, 38%40, (38*2)%40, 38%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n38")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n38")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n38")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n38")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n38")->scroll_m_set);
    TEST_ASSERT_EQUAL(38%40, lot52_node("n38")->scroll_m[0]);
    TEST_ASSERT_EQUAL((38*2)%40, lot52_node("n38")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n38")->scroll_p_set);
    TEST_ASSERT_EQUAL(38%15, lot52_node("n38")->scroll_p[0]);
}

static void test_l75_sys_39(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (39 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n39>item 39</div>",
            c & 0xffffffu, 39%40, (39*2)%40, 39%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n39")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n39")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n39")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n39")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n39")->scroll_m_set);
    TEST_ASSERT_EQUAL(39%40, lot52_node("n39")->scroll_m[0]);
    TEST_ASSERT_EQUAL((39*2)%40, lot52_node("n39")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n39")->scroll_p_set);
    TEST_ASSERT_EQUAL(39%15, lot52_node("n39")->scroll_p[0]);
}

static void test_l75_sys_40(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (40 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n40>item 40</div>",
            c & 0xffffffu, 40%40, (40*2)%40, 40%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n40")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n40")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n40")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n40")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n40")->scroll_m_set);
    TEST_ASSERT_EQUAL(40%40, lot52_node("n40")->scroll_m[0]);
    TEST_ASSERT_EQUAL((40*2)%40, lot52_node("n40")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n40")->scroll_p_set);
    TEST_ASSERT_EQUAL(40%15, lot52_node("n40")->scroll_p[0]);
}

static void test_l75_sys_41(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (41 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n41>item 41</div>",
            c & 0xffffffu, 41%40, (41*2)%40, 41%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n41")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n41")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n41")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n41")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n41")->scroll_m_set);
    TEST_ASSERT_EQUAL(41%40, lot52_node("n41")->scroll_m[0]);
    TEST_ASSERT_EQUAL((41*2)%40, lot52_node("n41")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n41")->scroll_p_set);
    TEST_ASSERT_EQUAL(41%15, lot52_node("n41")->scroll_p[0]);
}

static void test_l75_sys_42(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (42 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n42>item 42</div>",
            c & 0xffffffu, 42%40, (42*2)%40, 42%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n42")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n42")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n42")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n42")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n42")->scroll_m_set);
    TEST_ASSERT_EQUAL(42%40, lot52_node("n42")->scroll_m[0]);
    TEST_ASSERT_EQUAL((42*2)%40, lot52_node("n42")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n42")->scroll_p_set);
    TEST_ASSERT_EQUAL(42%15, lot52_node("n42")->scroll_p[0]);
}

static void test_l75_sys_43(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (43 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n43>item 43</div>",
            c & 0xffffffu, 43%40, (43*2)%40, 43%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n43")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n43")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n43")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n43")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n43")->scroll_m_set);
    TEST_ASSERT_EQUAL(43%40, lot52_node("n43")->scroll_m[0]);
    TEST_ASSERT_EQUAL((43*2)%40, lot52_node("n43")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n43")->scroll_p_set);
    TEST_ASSERT_EQUAL(43%15, lot52_node("n43")->scroll_p[0]);
}

static void test_l75_sys_44(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (44 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n44>item 44</div>",
            c & 0xffffffu, 44%40, (44*2)%40, 44%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n44")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n44")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n44")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n44")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n44")->scroll_m_set);
    TEST_ASSERT_EQUAL(44%40, lot52_node("n44")->scroll_m[0]);
    TEST_ASSERT_EQUAL((44*2)%40, lot52_node("n44")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n44")->scroll_p_set);
    TEST_ASSERT_EQUAL(44%15, lot52_node("n44")->scroll_p[0]);
}

static void test_l75_sys_45(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (45 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n45>item 45</div>",
            c & 0xffffffu, 45%40, (45*2)%40, 45%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n45")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n45")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n45")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n45")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n45")->scroll_m_set);
    TEST_ASSERT_EQUAL(45%40, lot52_node("n45")->scroll_m[0]);
    TEST_ASSERT_EQUAL((45*2)%40, lot52_node("n45")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n45")->scroll_p_set);
    TEST_ASSERT_EQUAL(45%15, lot52_node("n45")->scroll_p[0]);
}

static void test_l75_sys_46(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (46 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n46>item 46</div>",
            c & 0xffffffu, 46%40, (46*2)%40, 46%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n46")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n46")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n46")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n46")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n46")->scroll_m_set);
    TEST_ASSERT_EQUAL(46%40, lot52_node("n46")->scroll_m[0]);
    TEST_ASSERT_EQUAL((46*2)%40, lot52_node("n46")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n46")->scroll_p_set);
    TEST_ASSERT_EQUAL(46%15, lot52_node("n46")->scroll_p[0]);
}

static void test_l75_sys_47(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (47 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n47>item 47</div>",
            c & 0xffffffu, 47%40, (47*2)%40, 47%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n47")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n47")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n47")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n47")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n47")->scroll_m_set);
    TEST_ASSERT_EQUAL(47%40, lot52_node("n47")->scroll_m[0]);
    TEST_ASSERT_EQUAL((47*2)%40, lot52_node("n47")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n47")->scroll_p_set);
    TEST_ASSERT_EQUAL(47%15, lot52_node("n47")->scroll_p[0]);
}

static void test_l75_sys_48(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (48 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n48>item 48</div>",
            c & 0xffffffu, 48%40, (48*2)%40, 48%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n48")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n48")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n48")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n48")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n48")->scroll_m_set);
    TEST_ASSERT_EQUAL(48%40, lot52_node("n48")->scroll_m[0]);
    TEST_ASSERT_EQUAL((48*2)%40, lot52_node("n48")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n48")->scroll_p_set);
    TEST_ASSERT_EQUAL(48%15, lot52_node("n48")->scroll_p[0]);
}

static void test_l75_sys_49(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (49 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n49>item 49</div>",
            c & 0xffffffu, 49%40, (49*2)%40, 49%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n49")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n49")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n49")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n49")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n49")->scroll_m_set);
    TEST_ASSERT_EQUAL(49%40, lot52_node("n49")->scroll_m[0]);
    TEST_ASSERT_EQUAL((49*2)%40, lot52_node("n49")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n49")->scroll_p_set);
    TEST_ASSERT_EQUAL(49%15, lot52_node("n49")->scroll_p[0]);
}

static void test_l75_sys_50(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (50 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n50>item 50</div>",
            c & 0xffffffu, 50%40, (50*2)%40, 50%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n50")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n50")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n50")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n50")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n50")->scroll_m_set);
    TEST_ASSERT_EQUAL(50%40, lot52_node("n50")->scroll_m[0]);
    TEST_ASSERT_EQUAL((50*2)%40, lot52_node("n50")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n50")->scroll_p_set);
    TEST_ASSERT_EQUAL(50%15, lot52_node("n50")->scroll_p[0]);
}

static void test_l75_sys_51(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (51 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n51>item 51</div>",
            c & 0xffffffu, 51%40, (51*2)%40, 51%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n51")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n51")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n51")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n51")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n51")->scroll_m_set);
    TEST_ASSERT_EQUAL(51%40, lot52_node("n51")->scroll_m[0]);
    TEST_ASSERT_EQUAL((51*2)%40, lot52_node("n51")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n51")->scroll_p_set);
    TEST_ASSERT_EQUAL(51%15, lot52_node("n51")->scroll_p[0]);
}

static void test_l75_sys_52(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (52 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n52>item 52</div>",
            c & 0xffffffu, 52%40, (52*2)%40, 52%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n52")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n52")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n52")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n52")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n52")->scroll_m_set);
    TEST_ASSERT_EQUAL(52%40, lot52_node("n52")->scroll_m[0]);
    TEST_ASSERT_EQUAL((52*2)%40, lot52_node("n52")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n52")->scroll_p_set);
    TEST_ASSERT_EQUAL(52%15, lot52_node("n52")->scroll_p[0]);
}

static void test_l75_sys_53(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (53 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n53>item 53</div>",
            c & 0xffffffu, 53%40, (53*2)%40, 53%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n53")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n53")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n53")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n53")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n53")->scroll_m_set);
    TEST_ASSERT_EQUAL(53%40, lot52_node("n53")->scroll_m[0]);
    TEST_ASSERT_EQUAL((53*2)%40, lot52_node("n53")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n53")->scroll_p_set);
    TEST_ASSERT_EQUAL(53%15, lot52_node("n53")->scroll_p[0]);
}

static void test_l75_sys_54(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (54 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n54>item 54</div>",
            c & 0xffffffu, 54%40, (54*2)%40, 54%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n54")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n54")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n54")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n54")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n54")->scroll_m_set);
    TEST_ASSERT_EQUAL(54%40, lot52_node("n54")->scroll_m[0]);
    TEST_ASSERT_EQUAL((54*2)%40, lot52_node("n54")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n54")->scroll_p_set);
    TEST_ASSERT_EQUAL(54%15, lot52_node("n54")->scroll_p[0]);
}

static void test_l75_sys_55(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (55 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n55>item 55</div>",
            c & 0xffffffu, 55%40, (55*2)%40, 55%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n55")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n55")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n55")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n55")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n55")->scroll_m_set);
    TEST_ASSERT_EQUAL(55%40, lot52_node("n55")->scroll_m[0]);
    TEST_ASSERT_EQUAL((55*2)%40, lot52_node("n55")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n55")->scroll_p_set);
    TEST_ASSERT_EQUAL(55%15, lot52_node("n55")->scroll_p[0]);
}

static void test_l75_sys_56(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (56 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n56>item 56</div>",
            c & 0xffffffu, 56%40, (56*2)%40, 56%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n56")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n56")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n56")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n56")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n56")->scroll_m_set);
    TEST_ASSERT_EQUAL(56%40, lot52_node("n56")->scroll_m[0]);
    TEST_ASSERT_EQUAL((56*2)%40, lot52_node("n56")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n56")->scroll_p_set);
    TEST_ASSERT_EQUAL(56%15, lot52_node("n56")->scroll_p[0]);
}

static void test_l75_sys_57(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (57 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n57>item 57</div>",
            c & 0xffffffu, 57%40, (57*2)%40, 57%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n57")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n57")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n57")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n57")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n57")->scroll_m_set);
    TEST_ASSERT_EQUAL(57%40, lot52_node("n57")->scroll_m[0]);
    TEST_ASSERT_EQUAL((57*2)%40, lot52_node("n57")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n57")->scroll_p_set);
    TEST_ASSERT_EQUAL(57%15, lot52_node("n57")->scroll_p[0]);
}

static void test_l75_sys_58(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (58 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n58>item 58</div>",
            c & 0xffffffu, 58%40, (58*2)%40, 58%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n58")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n58")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n58")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n58")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n58")->scroll_m_set);
    TEST_ASSERT_EQUAL(58%40, lot52_node("n58")->scroll_m[0]);
    TEST_ASSERT_EQUAL((58*2)%40, lot52_node("n58")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n58")->scroll_p_set);
    TEST_ASSERT_EQUAL(58%15, lot52_node("n58")->scroll_p[0]);
}

static void test_l75_sys_59(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (59 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n59>item 59</div>",
            c & 0xffffffu, 59%40, (59*2)%40, 59%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n59")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n59")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n59")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n59")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n59")->scroll_m_set);
    TEST_ASSERT_EQUAL(59%40, lot52_node("n59")->scroll_m[0]);
    TEST_ASSERT_EQUAL((59*2)%40, lot52_node("n59")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n59")->scroll_p_set);
    TEST_ASSERT_EQUAL(59%15, lot52_node("n59")->scroll_p[0]);
}

static void test_l75_sys_60(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (60 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n60>item 60</div>",
            c & 0xffffffu, 60%40, (60*2)%40, 60%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n60")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n60")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n60")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n60")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n60")->scroll_m_set);
    TEST_ASSERT_EQUAL(60%40, lot52_node("n60")->scroll_m[0]);
    TEST_ASSERT_EQUAL((60*2)%40, lot52_node("n60")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n60")->scroll_p_set);
    TEST_ASSERT_EQUAL(60%15, lot52_node("n60")->scroll_p[0]);
}

static void test_l75_sys_61(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (61 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n61>item 61</div>",
            c & 0xffffffu, 61%40, (61*2)%40, 61%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n61")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n61")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n61")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n61")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n61")->scroll_m_set);
    TEST_ASSERT_EQUAL(61%40, lot52_node("n61")->scroll_m[0]);
    TEST_ASSERT_EQUAL((61*2)%40, lot52_node("n61")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n61")->scroll_p_set);
    TEST_ASSERT_EQUAL(61%15, lot52_node("n61")->scroll_p[0]);
}

static void test_l75_sys_62(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (62 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n62>item 62</div>",
            c & 0xffffffu, 62%40, (62*2)%40, 62%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n62")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n62")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n62")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n62")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n62")->scroll_m_set);
    TEST_ASSERT_EQUAL(62%40, lot52_node("n62")->scroll_m[0]);
    TEST_ASSERT_EQUAL((62*2)%40, lot52_node("n62")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n62")->scroll_p_set);
    TEST_ASSERT_EQUAL(62%15, lot52_node("n62")->scroll_p[0]);
}

static void test_l75_sys_63(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (63 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n63>item 63</div>",
            c & 0xffffffu, 63%40, (63*2)%40, 63%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n63")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n63")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n63")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n63")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n63")->scroll_m_set);
    TEST_ASSERT_EQUAL(63%40, lot52_node("n63")->scroll_m[0]);
    TEST_ASSERT_EQUAL((63*2)%40, lot52_node("n63")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n63")->scroll_p_set);
    TEST_ASSERT_EQUAL(63%15, lot52_node("n63")->scroll_p[0]);
}

static void test_l75_sys_64(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (64 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n64>item 64</div>",
            c & 0xffffffu, 64%40, (64*2)%40, 64%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n64")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n64")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n64")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n64")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n64")->scroll_m_set);
    TEST_ASSERT_EQUAL(64%40, lot52_node("n64")->scroll_m[0]);
    TEST_ASSERT_EQUAL((64*2)%40, lot52_node("n64")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n64")->scroll_p_set);
    TEST_ASSERT_EQUAL(64%15, lot52_node("n64")->scroll_p[0]);
}

static void test_l75_sys_65(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (65 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n65>item 65</div>",
            c & 0xffffffu, 65%40, (65*2)%40, 65%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n65")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n65")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n65")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n65")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n65")->scroll_m_set);
    TEST_ASSERT_EQUAL(65%40, lot52_node("n65")->scroll_m[0]);
    TEST_ASSERT_EQUAL((65*2)%40, lot52_node("n65")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n65")->scroll_p_set);
    TEST_ASSERT_EQUAL(65%15, lot52_node("n65")->scroll_p[0]);
}

static void test_l75_sys_66(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (66 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n66>item 66</div>",
            c & 0xffffffu, 66%40, (66*2)%40, 66%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n66")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n66")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n66")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n66")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n66")->scroll_m_set);
    TEST_ASSERT_EQUAL(66%40, lot52_node("n66")->scroll_m[0]);
    TEST_ASSERT_EQUAL((66*2)%40, lot52_node("n66")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n66")->scroll_p_set);
    TEST_ASSERT_EQUAL(66%15, lot52_node("n66")->scroll_p[0]);
}

static void test_l75_sys_67(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (67 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n67>item 67</div>",
            c & 0xffffffu, 67%40, (67*2)%40, 67%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n67")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n67")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n67")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n67")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n67")->scroll_m_set);
    TEST_ASSERT_EQUAL(67%40, lot52_node("n67")->scroll_m[0]);
    TEST_ASSERT_EQUAL((67*2)%40, lot52_node("n67")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n67")->scroll_p_set);
    TEST_ASSERT_EQUAL(67%15, lot52_node("n67")->scroll_p[0]);
}

static void test_l75_sys_68(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (68 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n68>item 68</div>",
            c & 0xffffffu, 68%40, (68*2)%40, 68%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n68")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n68")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n68")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n68")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n68")->scroll_m_set);
    TEST_ASSERT_EQUAL(68%40, lot52_node("n68")->scroll_m[0]);
    TEST_ASSERT_EQUAL((68*2)%40, lot52_node("n68")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n68")->scroll_p_set);
    TEST_ASSERT_EQUAL(68%15, lot52_node("n68")->scroll_p[0]);
}

static void test_l75_sys_69(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (69 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n69>item 69</div>",
            c & 0xffffffu, 69%40, (69*2)%40, 69%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n69")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n69")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n69")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n69")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n69")->scroll_m_set);
    TEST_ASSERT_EQUAL(69%40, lot52_node("n69")->scroll_m[0]);
    TEST_ASSERT_EQUAL((69*2)%40, lot52_node("n69")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n69")->scroll_p_set);
    TEST_ASSERT_EQUAL(69%15, lot52_node("n69")->scroll_p[0]);
}

static void test_l75_sys_70(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (70 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n70>item 70</div>",
            c & 0xffffffu, 70%40, (70*2)%40, 70%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n70")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n70")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n70")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n70")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n70")->scroll_m_set);
    TEST_ASSERT_EQUAL(70%40, lot52_node("n70")->scroll_m[0]);
    TEST_ASSERT_EQUAL((70*2)%40, lot52_node("n70")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n70")->scroll_p_set);
    TEST_ASSERT_EQUAL(70%15, lot52_node("n70")->scroll_p[0]);
}

static void test_l75_sys_71(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (71 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n71>item 71</div>",
            c & 0xffffffu, 71%40, (71*2)%40, 71%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n71")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n71")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n71")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n71")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n71")->scroll_m_set);
    TEST_ASSERT_EQUAL(71%40, lot52_node("n71")->scroll_m[0]);
    TEST_ASSERT_EQUAL((71*2)%40, lot52_node("n71")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n71")->scroll_p_set);
    TEST_ASSERT_EQUAL(71%15, lot52_node("n71")->scroll_p[0]);
}

static void test_l75_sys_72(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (72 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n72>item 72</div>",
            c & 0xffffffu, 72%40, (72*2)%40, 72%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n72")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n72")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n72")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n72")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n72")->scroll_m_set);
    TEST_ASSERT_EQUAL(72%40, lot52_node("n72")->scroll_m[0]);
    TEST_ASSERT_EQUAL((72*2)%40, lot52_node("n72")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n72")->scroll_p_set);
    TEST_ASSERT_EQUAL(72%15, lot52_node("n72")->scroll_p[0]);
}

static void test_l75_sys_73(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (73 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n73>item 73</div>",
            c & 0xffffffu, 73%40, (73*2)%40, 73%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n73")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n73")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n73")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n73")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n73")->scroll_m_set);
    TEST_ASSERT_EQUAL(73%40, lot52_node("n73")->scroll_m[0]);
    TEST_ASSERT_EQUAL((73*2)%40, lot52_node("n73")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n73")->scroll_p_set);
    TEST_ASSERT_EQUAL(73%15, lot52_node("n73")->scroll_p[0]);
}

static void test_l75_sys_74(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (74 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n74>item 74</div>",
            c & 0xffffffu, 74%40, (74*2)%40, 74%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n74")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n74")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n74")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n74")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n74")->scroll_m_set);
    TEST_ASSERT_EQUAL(74%40, lot52_node("n74")->scroll_m[0]);
    TEST_ASSERT_EQUAL((74*2)%40, lot52_node("n74")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n74")->scroll_p_set);
    TEST_ASSERT_EQUAL(74%15, lot52_node("n74")->scroll_p[0]);
}

static void test_l75_sys_75(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (75 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n75>item 75</div>",
            c & 0xffffffu, 75%40, (75*2)%40, 75%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n75")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n75")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n75")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n75")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n75")->scroll_m_set);
    TEST_ASSERT_EQUAL(75%40, lot52_node("n75")->scroll_m[0]);
    TEST_ASSERT_EQUAL((75*2)%40, lot52_node("n75")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n75")->scroll_p_set);
    TEST_ASSERT_EQUAL(75%15, lot52_node("n75")->scroll_p[0]);
}

static void test_l75_sys_76(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (76 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n76>item 76</div>",
            c & 0xffffffu, 76%40, (76*2)%40, 76%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n76")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n76")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n76")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n76")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n76")->scroll_m_set);
    TEST_ASSERT_EQUAL(76%40, lot52_node("n76")->scroll_m[0]);
    TEST_ASSERT_EQUAL((76*2)%40, lot52_node("n76")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n76")->scroll_p_set);
    TEST_ASSERT_EQUAL(76%15, lot52_node("n76")->scroll_p[0]);
}

static void test_l75_sys_77(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (77 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n77>item 77</div>",
            c & 0xffffffu, 77%40, (77*2)%40, 77%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n77")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n77")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n77")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n77")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n77")->scroll_m_set);
    TEST_ASSERT_EQUAL(77%40, lot52_node("n77")->scroll_m[0]);
    TEST_ASSERT_EQUAL((77*2)%40, lot52_node("n77")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n77")->scroll_p_set);
    TEST_ASSERT_EQUAL(77%15, lot52_node("n77")->scroll_p[0]);
}

static void test_l75_sys_78(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (78 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n78>item 78</div>",
            c & 0xffffffu, 78%40, (78*2)%40, 78%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n78")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n78")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n78")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n78")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n78")->scroll_m_set);
    TEST_ASSERT_EQUAL(78%40, lot52_node("n78")->scroll_m[0]);
    TEST_ASSERT_EQUAL((78*2)%40, lot52_node("n78")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n78")->scroll_p_set);
    TEST_ASSERT_EQUAL(78%15, lot52_node("n78")->scroll_p[0]);
}

static void test_l75_sys_79(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (79 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n79>item 79</div>",
            c & 0xffffffu, 79%40, (79*2)%40, 79%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n79")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n79")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n79")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n79")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n79")->scroll_m_set);
    TEST_ASSERT_EQUAL(79%40, lot52_node("n79")->scroll_m[0]);
    TEST_ASSERT_EQUAL((79*2)%40, lot52_node("n79")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n79")->scroll_p_set);
    TEST_ASSERT_EQUAL(79%15, lot52_node("n79")->scroll_p[0]);
}

static void test_l75_sys_80(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (80 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n80>item 80</div>",
            c & 0xffffffu, 80%40, (80*2)%40, 80%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n80")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n80")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n80")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n80")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n80")->scroll_m_set);
    TEST_ASSERT_EQUAL(80%40, lot52_node("n80")->scroll_m[0]);
    TEST_ASSERT_EQUAL((80*2)%40, lot52_node("n80")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n80")->scroll_p_set);
    TEST_ASSERT_EQUAL(80%15, lot52_node("n80")->scroll_p[0]);
}

static void test_l75_sys_81(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (81 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n81>item 81</div>",
            c & 0xffffffu, 81%40, (81*2)%40, 81%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n81")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n81")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n81")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n81")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n81")->scroll_m_set);
    TEST_ASSERT_EQUAL(81%40, lot52_node("n81")->scroll_m[0]);
    TEST_ASSERT_EQUAL((81*2)%40, lot52_node("n81")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n81")->scroll_p_set);
    TEST_ASSERT_EQUAL(81%15, lot52_node("n81")->scroll_p[0]);
}

static void test_l75_sys_82(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (82 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n82>item 82</div>",
            c & 0xffffffu, 82%40, (82*2)%40, 82%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n82")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n82")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n82")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n82")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n82")->scroll_m_set);
    TEST_ASSERT_EQUAL(82%40, lot52_node("n82")->scroll_m[0]);
    TEST_ASSERT_EQUAL((82*2)%40, lot52_node("n82")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n82")->scroll_p_set);
    TEST_ASSERT_EQUAL(82%15, lot52_node("n82")->scroll_p[0]);
}

static void test_l75_sys_83(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (83 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n83>item 83</div>",
            c & 0xffffffu, 83%40, (83*2)%40, 83%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n83")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n83")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n83")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n83")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n83")->scroll_m_set);
    TEST_ASSERT_EQUAL(83%40, lot52_node("n83")->scroll_m[0]);
    TEST_ASSERT_EQUAL((83*2)%40, lot52_node("n83")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n83")->scroll_p_set);
    TEST_ASSERT_EQUAL(83%15, lot52_node("n83")->scroll_p[0]);
}

static void test_l75_sys_84(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (84 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n84>item 84</div>",
            c & 0xffffffu, 84%40, (84*2)%40, 84%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n84")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n84")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n84")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n84")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n84")->scroll_m_set);
    TEST_ASSERT_EQUAL(84%40, lot52_node("n84")->scroll_m[0]);
    TEST_ASSERT_EQUAL((84*2)%40, lot52_node("n84")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n84")->scroll_p_set);
    TEST_ASSERT_EQUAL(84%15, lot52_node("n84")->scroll_p[0]);
}

static void test_l75_sys_85(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (85 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n85>item 85</div>",
            c & 0xffffffu, 85%40, (85*2)%40, 85%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n85")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n85")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n85")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n85")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n85")->scroll_m_set);
    TEST_ASSERT_EQUAL(85%40, lot52_node("n85")->scroll_m[0]);
    TEST_ASSERT_EQUAL((85*2)%40, lot52_node("n85")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n85")->scroll_p_set);
    TEST_ASSERT_EQUAL(85%15, lot52_node("n85")->scroll_p[0]);
}

static void test_l75_sys_86(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (86 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n86>item 86</div>",
            c & 0xffffffu, 86%40, (86*2)%40, 86%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n86")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n86")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n86")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n86")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n86")->scroll_m_set);
    TEST_ASSERT_EQUAL(86%40, lot52_node("n86")->scroll_m[0]);
    TEST_ASSERT_EQUAL((86*2)%40, lot52_node("n86")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n86")->scroll_p_set);
    TEST_ASSERT_EQUAL(86%15, lot52_node("n86")->scroll_p[0]);
}

static void test_l75_sys_87(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (87 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n87>item 87</div>",
            c & 0xffffffu, 87%40, (87*2)%40, 87%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n87")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n87")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n87")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n87")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n87")->scroll_m_set);
    TEST_ASSERT_EQUAL(87%40, lot52_node("n87")->scroll_m[0]);
    TEST_ASSERT_EQUAL((87*2)%40, lot52_node("n87")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n87")->scroll_p_set);
    TEST_ASSERT_EQUAL(87%15, lot52_node("n87")->scroll_p[0]);
}

static void test_l75_sys_88(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (88 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n88>item 88</div>",
            c & 0xffffffu, 88%40, (88*2)%40, 88%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n88")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n88")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n88")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n88")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n88")->scroll_m_set);
    TEST_ASSERT_EQUAL(88%40, lot52_node("n88")->scroll_m[0]);
    TEST_ASSERT_EQUAL((88*2)%40, lot52_node("n88")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n88")->scroll_p_set);
    TEST_ASSERT_EQUAL(88%15, lot52_node("n88")->scroll_p[0]);
}

static void test_l75_sys_89(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (89 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n89>item 89</div>",
            c & 0xffffffu, 89%40, (89*2)%40, 89%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n89")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n89")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n89")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n89")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n89")->scroll_m_set);
    TEST_ASSERT_EQUAL(89%40, lot52_node("n89")->scroll_m[0]);
    TEST_ASSERT_EQUAL((89*2)%40, lot52_node("n89")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n89")->scroll_p_set);
    TEST_ASSERT_EQUAL(89%15, lot52_node("n89")->scroll_p[0]);
}

static void test_l75_sys_90(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (90 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n90>item 90</div>",
            c & 0xffffffu, 90%40, (90*2)%40, 90%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n90")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n90")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n90")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n90")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n90")->scroll_m_set);
    TEST_ASSERT_EQUAL(90%40, lot52_node("n90")->scroll_m[0]);
    TEST_ASSERT_EQUAL((90*2)%40, lot52_node("n90")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n90")->scroll_p_set);
    TEST_ASSERT_EQUAL(90%15, lot52_node("n90")->scroll_p[0]);
}

static void test_l75_sys_91(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (91 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n91>item 91</div>",
            c & 0xffffffu, 91%40, (91*2)%40, 91%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n91")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n91")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n91")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n91")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n91")->scroll_m_set);
    TEST_ASSERT_EQUAL(91%40, lot52_node("n91")->scroll_m[0]);
    TEST_ASSERT_EQUAL((91*2)%40, lot52_node("n91")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n91")->scroll_p_set);
    TEST_ASSERT_EQUAL(91%15, lot52_node("n91")->scroll_p[0]);
}

static void test_l75_sys_92(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (92 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n92>item 92</div>",
            c & 0xffffffu, 92%40, (92*2)%40, 92%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n92")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n92")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n92")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n92")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n92")->scroll_m_set);
    TEST_ASSERT_EQUAL(92%40, lot52_node("n92")->scroll_m[0]);
    TEST_ASSERT_EQUAL((92*2)%40, lot52_node("n92")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n92")->scroll_p_set);
    TEST_ASSERT_EQUAL(92%15, lot52_node("n92")->scroll_p[0]);
}

static void test_l75_sys_93(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (93 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n93>item 93</div>",
            c & 0xffffffu, 93%40, (93*2)%40, 93%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n93")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n93")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n93")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n93")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n93")->scroll_m_set);
    TEST_ASSERT_EQUAL(93%40, lot52_node("n93")->scroll_m[0]);
    TEST_ASSERT_EQUAL((93*2)%40, lot52_node("n93")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n93")->scroll_p_set);
    TEST_ASSERT_EQUAL(93%15, lot52_node("n93")->scroll_p[0]);
}

static void test_l75_sys_94(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (94 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n94>item 94</div>",
            c & 0xffffffu, 94%40, (94*2)%40, 94%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n94")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n94")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n94")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n94")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n94")->scroll_m_set);
    TEST_ASSERT_EQUAL(94%40, lot52_node("n94")->scroll_m[0]);
    TEST_ASSERT_EQUAL((94*2)%40, lot52_node("n94")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n94")->scroll_p_set);
    TEST_ASSERT_EQUAL(94%15, lot52_node("n94")->scroll_p[0]);
}

static void test_l75_sys_95(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (95 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n95>item 95</div>",
            c & 0xffffffu, 95%40, (95*2)%40, 95%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n95")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n95")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n95")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n95")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n95")->scroll_m_set);
    TEST_ASSERT_EQUAL(95%40, lot52_node("n95")->scroll_m[0]);
    TEST_ASSERT_EQUAL((95*2)%40, lot52_node("n95")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n95")->scroll_p_set);
    TEST_ASSERT_EQUAL(95%15, lot52_node("n95")->scroll_p[0]);
}

static void test_l75_sys_96(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (96 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n96>item 96</div>",
            c & 0xffffffu, 96%40, (96*2)%40, 96%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n96")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n96")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n96")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n96")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n96")->scroll_m_set);
    TEST_ASSERT_EQUAL(96%40, lot52_node("n96")->scroll_m[0]);
    TEST_ASSERT_EQUAL((96*2)%40, lot52_node("n96")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n96")->scroll_p_set);
    TEST_ASSERT_EQUAL(96%15, lot52_node("n96")->scroll_p[0]);
}

static void test_l75_sys_97(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (97 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n97>item 97</div>",
            c & 0xffffffu, 97%40, (97*2)%40, 97%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n97")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n97")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n97")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n97")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n97")->scroll_m_set);
    TEST_ASSERT_EQUAL(97%40, lot52_node("n97")->scroll_m[0]);
    TEST_ASSERT_EQUAL((97*2)%40, lot52_node("n97")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n97")->scroll_p_set);
    TEST_ASSERT_EQUAL(97%15, lot52_node("n97")->scroll_p[0]);
}

static void test_l75_sys_98(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (98 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n98>item 98</div>",
            c & 0xffffffu, 98%40, (98*2)%40, 98%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n98")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n98")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n98")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n98")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n98")->scroll_m_set);
    TEST_ASSERT_EQUAL(98%40, lot52_node("n98")->scroll_m[0]);
    TEST_ASSERT_EQUAL((98*2)%40, lot52_node("n98")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n98")->scroll_p_set);
    TEST_ASSERT_EQUAL(98%15, lot52_node("n98")->scroll_p[0]);
}

static void test_l75_sys_99(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (99 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n99>item 99</div>",
            c & 0xffffffu, 99%40, (99*2)%40, 99%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n99")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n99")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n99")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n99")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n99")->scroll_m_set);
    TEST_ASSERT_EQUAL(99%40, lot52_node("n99")->scroll_m[0]);
    TEST_ASSERT_EQUAL((99*2)%40, lot52_node("n99")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n99")->scroll_p_set);
    TEST_ASSERT_EQUAL(99%15, lot52_node("n99")->scroll_p[0]);
}

static void test_l75_sys_100(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (100 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n100>item 100</div>",
            c & 0xffffffu, 100%40, (100*2)%40, 100%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n100")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n100")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n100")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n100")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n100")->scroll_m_set);
    TEST_ASSERT_EQUAL(100%40, lot52_node("n100")->scroll_m[0]);
    TEST_ASSERT_EQUAL((100*2)%40, lot52_node("n100")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n100")->scroll_p_set);
    TEST_ASSERT_EQUAL(100%15, lot52_node("n100")->scroll_p[0]);
}

static void test_l75_sys_101(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (101 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n101>item 101</div>",
            c & 0xffffffu, 101%40, (101*2)%40, 101%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n101")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n101")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n101")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n101")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n101")->scroll_m_set);
    TEST_ASSERT_EQUAL(101%40, lot52_node("n101")->scroll_m[0]);
    TEST_ASSERT_EQUAL((101*2)%40, lot52_node("n101")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n101")->scroll_p_set);
    TEST_ASSERT_EQUAL(101%15, lot52_node("n101")->scroll_p[0]);
}

static void test_l75_sys_102(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (102 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n102>item 102</div>",
            c & 0xffffffu, 102%40, (102*2)%40, 102%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n102")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n102")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n102")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n102")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n102")->scroll_m_set);
    TEST_ASSERT_EQUAL(102%40, lot52_node("n102")->scroll_m[0]);
    TEST_ASSERT_EQUAL((102*2)%40, lot52_node("n102")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n102")->scroll_p_set);
    TEST_ASSERT_EQUAL(102%15, lot52_node("n102")->scroll_p[0]);
}

static void test_l75_sys_103(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (103 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n103>item 103</div>",
            c & 0xffffffu, 103%40, (103*2)%40, 103%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n103")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n103")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n103")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n103")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n103")->scroll_m_set);
    TEST_ASSERT_EQUAL(103%40, lot52_node("n103")->scroll_m[0]);
    TEST_ASSERT_EQUAL((103*2)%40, lot52_node("n103")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n103")->scroll_p_set);
    TEST_ASSERT_EQUAL(103%15, lot52_node("n103")->scroll_p[0]);
}

static void test_l75_sys_104(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (104 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n104>item 104</div>",
            c & 0xffffffu, 104%40, (104*2)%40, 104%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n104")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n104")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n104")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n104")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n104")->scroll_m_set);
    TEST_ASSERT_EQUAL(104%40, lot52_node("n104")->scroll_m[0]);
    TEST_ASSERT_EQUAL((104*2)%40, lot52_node("n104")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n104")->scroll_p_set);
    TEST_ASSERT_EQUAL(104%15, lot52_node("n104")->scroll_p[0]);
}

static void test_l75_sys_105(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (105 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n105>item 105</div>",
            c & 0xffffffu, 105%40, (105*2)%40, 105%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n105")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n105")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n105")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n105")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n105")->scroll_m_set);
    TEST_ASSERT_EQUAL(105%40, lot52_node("n105")->scroll_m[0]);
    TEST_ASSERT_EQUAL((105*2)%40, lot52_node("n105")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n105")->scroll_p_set);
    TEST_ASSERT_EQUAL(105%15, lot52_node("n105")->scroll_p[0]);
}

static void test_l75_sys_106(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (106 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n106>item 106</div>",
            c & 0xffffffu, 106%40, (106*2)%40, 106%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n106")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n106")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n106")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n106")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n106")->scroll_m_set);
    TEST_ASSERT_EQUAL(106%40, lot52_node("n106")->scroll_m[0]);
    TEST_ASSERT_EQUAL((106*2)%40, lot52_node("n106")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n106")->scroll_p_set);
    TEST_ASSERT_EQUAL(106%15, lot52_node("n106")->scroll_p[0]);
}

static void test_l75_sys_107(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (107 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n107>item 107</div>",
            c & 0xffffffu, 107%40, (107*2)%40, 107%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n107")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n107")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n107")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n107")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n107")->scroll_m_set);
    TEST_ASSERT_EQUAL(107%40, lot52_node("n107")->scroll_m[0]);
    TEST_ASSERT_EQUAL((107*2)%40, lot52_node("n107")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n107")->scroll_p_set);
    TEST_ASSERT_EQUAL(107%15, lot52_node("n107")->scroll_p[0]);
}

static void test_l75_sys_108(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (108 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n108>item 108</div>",
            c & 0xffffffu, 108%40, (108*2)%40, 108%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n108")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n108")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n108")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n108")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n108")->scroll_m_set);
    TEST_ASSERT_EQUAL(108%40, lot52_node("n108")->scroll_m[0]);
    TEST_ASSERT_EQUAL((108*2)%40, lot52_node("n108")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n108")->scroll_p_set);
    TEST_ASSERT_EQUAL(108%15, lot52_node("n108")->scroll_p[0]);
}

static void test_l75_sys_109(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (109 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n109>item 109</div>",
            c & 0xffffffu, 109%40, (109*2)%40, 109%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n109")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n109")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n109")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n109")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n109")->scroll_m_set);
    TEST_ASSERT_EQUAL(109%40, lot52_node("n109")->scroll_m[0]);
    TEST_ASSERT_EQUAL((109*2)%40, lot52_node("n109")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n109")->scroll_p_set);
    TEST_ASSERT_EQUAL(109%15, lot52_node("n109")->scroll_p[0]);
}

static void test_l75_sys_110(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (110 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n110>item 110</div>",
            c & 0xffffffu, 110%40, (110*2)%40, 110%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n110")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n110")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n110")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n110")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n110")->scroll_m_set);
    TEST_ASSERT_EQUAL(110%40, lot52_node("n110")->scroll_m[0]);
    TEST_ASSERT_EQUAL((110*2)%40, lot52_node("n110")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n110")->scroll_p_set);
    TEST_ASSERT_EQUAL(110%15, lot52_node("n110")->scroll_p[0]);
}

static void test_l75_sys_111(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (111 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n111>item 111</div>",
            c & 0xffffffu, 111%40, (111*2)%40, 111%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n111")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n111")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n111")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n111")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n111")->scroll_m_set);
    TEST_ASSERT_EQUAL(111%40, lot52_node("n111")->scroll_m[0]);
    TEST_ASSERT_EQUAL((111*2)%40, lot52_node("n111")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n111")->scroll_p_set);
    TEST_ASSERT_EQUAL(111%15, lot52_node("n111")->scroll_p[0]);
}

static void test_l75_sys_112(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (112 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n112>item 112</div>",
            c & 0xffffffu, 112%40, (112*2)%40, 112%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n112")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n112")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n112")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n112")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n112")->scroll_m_set);
    TEST_ASSERT_EQUAL(112%40, lot52_node("n112")->scroll_m[0]);
    TEST_ASSERT_EQUAL((112*2)%40, lot52_node("n112")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n112")->scroll_p_set);
    TEST_ASSERT_EQUAL(112%15, lot52_node("n112")->scroll_p[0]);
}

static void test_l75_sys_113(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (113 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n113>item 113</div>",
            c & 0xffffffu, 113%40, (113*2)%40, 113%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n113")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n113")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n113")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n113")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n113")->scroll_m_set);
    TEST_ASSERT_EQUAL(113%40, lot52_node("n113")->scroll_m[0]);
    TEST_ASSERT_EQUAL((113*2)%40, lot52_node("n113")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n113")->scroll_p_set);
    TEST_ASSERT_EQUAL(113%15, lot52_node("n113")->scroll_p[0]);
}

static void test_l75_sys_114(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (114 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n114>item 114</div>",
            c & 0xffffffu, 114%40, (114*2)%40, 114%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n114")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n114")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n114")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n114")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n114")->scroll_m_set);
    TEST_ASSERT_EQUAL(114%40, lot52_node("n114")->scroll_m[0]);
    TEST_ASSERT_EQUAL((114*2)%40, lot52_node("n114")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n114")->scroll_p_set);
    TEST_ASSERT_EQUAL(114%15, lot52_node("n114")->scroll_p[0]);
}

static void test_l75_sys_115(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (115 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n115>item 115</div>",
            c & 0xffffffu, 115%40, (115*2)%40, 115%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n115")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n115")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n115")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n115")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n115")->scroll_m_set);
    TEST_ASSERT_EQUAL(115%40, lot52_node("n115")->scroll_m[0]);
    TEST_ASSERT_EQUAL((115*2)%40, lot52_node("n115")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n115")->scroll_p_set);
    TEST_ASSERT_EQUAL(115%15, lot52_node("n115")->scroll_p[0]);
}

static void test_l75_sys_116(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (116 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n116>item 116</div>",
            c & 0xffffffu, 116%40, (116*2)%40, 116%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n116")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n116")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n116")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n116")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n116")->scroll_m_set);
    TEST_ASSERT_EQUAL(116%40, lot52_node("n116")->scroll_m[0]);
    TEST_ASSERT_EQUAL((116*2)%40, lot52_node("n116")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n116")->scroll_p_set);
    TEST_ASSERT_EQUAL(116%15, lot52_node("n116")->scroll_p[0]);
}

static void test_l75_sys_117(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (117 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n117>item 117</div>",
            c & 0xffffffu, 117%40, (117*2)%40, 117%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n117")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n117")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n117")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n117")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n117")->scroll_m_set);
    TEST_ASSERT_EQUAL(117%40, lot52_node("n117")->scroll_m[0]);
    TEST_ASSERT_EQUAL((117*2)%40, lot52_node("n117")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n117")->scroll_p_set);
    TEST_ASSERT_EQUAL(117%15, lot52_node("n117")->scroll_p[0]);
}

static void test_l75_sys_118(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (118 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n118>item 118</div>",
            c & 0xffffffu, 118%40, (118*2)%40, 118%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n118")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n118")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n118")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n118")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n118")->scroll_m_set);
    TEST_ASSERT_EQUAL(118%40, lot52_node("n118")->scroll_m[0]);
    TEST_ASSERT_EQUAL((118*2)%40, lot52_node("n118")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n118")->scroll_p_set);
    TEST_ASSERT_EQUAL(118%15, lot52_node("n118")->scroll_p[0]);
}

static void test_l75_sys_119(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (119 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n119>item 119</div>",
            c & 0xffffffu, 119%40, (119*2)%40, 119%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n119")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n119")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n119")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n119")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n119")->scroll_m_set);
    TEST_ASSERT_EQUAL(119%40, lot52_node("n119")->scroll_m[0]);
    TEST_ASSERT_EQUAL((119*2)%40, lot52_node("n119")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n119")->scroll_p_set);
    TEST_ASSERT_EQUAL(119%15, lot52_node("n119")->scroll_p[0]);
}

static void test_l75_sys_120(void) {
    char html[500];
    unsigned c = (unsigned)(0xabcdefu ^ (120 * 0x010101u));
    sprintf(html, "<style>.b{caret-color:#%06x;user-select:none;pointer-events:none;"
                  "scroll-margin:%dpx %dpx;scroll-padding:%dpx}</style>"
                  "<div class=b id=n120>item 120</div>",
            c & 0xffffffu, 120%40, (120*2)%40, 120%15);
    lot52_render(html, 400);
    TEST_ASSERT_EQUAL(1, lot52_node("n120")->caret_set);
    TEST_ASSERT_EQUAL(c & 0xffffffu, lot52_node("n120")->caret);
    TEST_ASSERT_EQUAL(1, lot52_node("n120")->user_select);
    TEST_ASSERT_EQUAL(1, lot52_node("n120")->pointer_events);
    TEST_ASSERT_EQUAL(1, lot52_node("n120")->scroll_m_set);
    TEST_ASSERT_EQUAL(120%40, lot52_node("n120")->scroll_m[0]);
    TEST_ASSERT_EQUAL((120*2)%40, lot52_node("n120")->scroll_m[1]);
    TEST_ASSERT_EQUAL(1, lot52_node("n120")->scroll_p_set);
    TEST_ASSERT_EQUAL(120%15, lot52_node("n120")->scroll_p[0]);
}

static void test_l75_sys_empty_file_part(void) {
    we_form_req_t q;
    int i, x, y;
    l75_render("<form method=POST enctype=multipart/form-data action=/u>"
               "<input id=f type=file name=doc><input type=submit></form>");
    /* no file bound: empty filename part still present */
    for (i = 1; i < (int)g_doc.nnodes; i++)
        if (g_doc.nodes[i].form_type == WE_FORM_SUBMIT) break;
    x = g_doc.nodes[i].x + 2; y = g_doc.nodes[i].y + 2;
    TEST_ASSERT_EQUAL(WE_FACT_SUBMIT, we_form_click(&g_doc, x, y));
    TEST_ASSERT_EQUAL(0, we_form_request(&g_doc, &q));
    TEST_ASSERT_EQUAL(1, q.multipart);
    TEST_ASSERT_NOT_NULL(strstr(q.body, "name=\"doc\""));
    TEST_ASSERT_NOT_NULL(strstr(q.body, "filename=\"\""));
    l75_done();
}
static void lot75_sys_run(void) {
    RUN_TEST(test_l75_sys_1);
    RUN_TEST(test_l75_sys_2);
    RUN_TEST(test_l75_sys_3);
    RUN_TEST(test_l75_sys_4);
    RUN_TEST(test_l75_sys_5);
    RUN_TEST(test_l75_sys_6);
    RUN_TEST(test_l75_sys_7);
    RUN_TEST(test_l75_sys_8);
    RUN_TEST(test_l75_sys_9);
    RUN_TEST(test_l75_sys_10);
    RUN_TEST(test_l75_sys_11);
    RUN_TEST(test_l75_sys_12);
    RUN_TEST(test_l75_sys_13);
    RUN_TEST(test_l75_sys_14);
    RUN_TEST(test_l75_sys_15);
    RUN_TEST(test_l75_sys_16);
    RUN_TEST(test_l75_sys_17);
    RUN_TEST(test_l75_sys_18);
    RUN_TEST(test_l75_sys_19);
    RUN_TEST(test_l75_sys_20);
    RUN_TEST(test_l75_sys_21);
    RUN_TEST(test_l75_sys_22);
    RUN_TEST(test_l75_sys_23);
    RUN_TEST(test_l75_sys_24);
    RUN_TEST(test_l75_sys_25);
    RUN_TEST(test_l75_sys_26);
    RUN_TEST(test_l75_sys_27);
    RUN_TEST(test_l75_sys_28);
    RUN_TEST(test_l75_sys_29);
    RUN_TEST(test_l75_sys_30);
    RUN_TEST(test_l75_sys_31);
    RUN_TEST(test_l75_sys_32);
    RUN_TEST(test_l75_sys_33);
    RUN_TEST(test_l75_sys_34);
    RUN_TEST(test_l75_sys_35);
    RUN_TEST(test_l75_sys_36);
    RUN_TEST(test_l75_sys_37);
    RUN_TEST(test_l75_sys_38);
    RUN_TEST(test_l75_sys_39);
    RUN_TEST(test_l75_sys_40);
    RUN_TEST(test_l75_sys_41);
    RUN_TEST(test_l75_sys_42);
    RUN_TEST(test_l75_sys_43);
    RUN_TEST(test_l75_sys_44);
    RUN_TEST(test_l75_sys_45);
    RUN_TEST(test_l75_sys_46);
    RUN_TEST(test_l75_sys_47);
    RUN_TEST(test_l75_sys_48);
    RUN_TEST(test_l75_sys_49);
    RUN_TEST(test_l75_sys_50);
    RUN_TEST(test_l75_sys_51);
    RUN_TEST(test_l75_sys_52);
    RUN_TEST(test_l75_sys_53);
    RUN_TEST(test_l75_sys_54);
    RUN_TEST(test_l75_sys_55);
    RUN_TEST(test_l75_sys_56);
    RUN_TEST(test_l75_sys_57);
    RUN_TEST(test_l75_sys_58);
    RUN_TEST(test_l75_sys_59);
    RUN_TEST(test_l75_sys_60);
    RUN_TEST(test_l75_sys_61);
    RUN_TEST(test_l75_sys_62);
    RUN_TEST(test_l75_sys_63);
    RUN_TEST(test_l75_sys_64);
    RUN_TEST(test_l75_sys_65);
    RUN_TEST(test_l75_sys_66);
    RUN_TEST(test_l75_sys_67);
    RUN_TEST(test_l75_sys_68);
    RUN_TEST(test_l75_sys_69);
    RUN_TEST(test_l75_sys_70);
    RUN_TEST(test_l75_sys_71);
    RUN_TEST(test_l75_sys_72);
    RUN_TEST(test_l75_sys_73);
    RUN_TEST(test_l75_sys_74);
    RUN_TEST(test_l75_sys_75);
    RUN_TEST(test_l75_sys_76);
    RUN_TEST(test_l75_sys_77);
    RUN_TEST(test_l75_sys_78);
    RUN_TEST(test_l75_sys_79);
    RUN_TEST(test_l75_sys_80);
    RUN_TEST(test_l75_sys_81);
    RUN_TEST(test_l75_sys_82);
    RUN_TEST(test_l75_sys_83);
    RUN_TEST(test_l75_sys_84);
    RUN_TEST(test_l75_sys_85);
    RUN_TEST(test_l75_sys_86);
    RUN_TEST(test_l75_sys_87);
    RUN_TEST(test_l75_sys_88);
    RUN_TEST(test_l75_sys_89);
    RUN_TEST(test_l75_sys_90);
    RUN_TEST(test_l75_sys_91);
    RUN_TEST(test_l75_sys_92);
    RUN_TEST(test_l75_sys_93);
    RUN_TEST(test_l75_sys_94);
    RUN_TEST(test_l75_sys_95);
    RUN_TEST(test_l75_sys_96);
    RUN_TEST(test_l75_sys_97);
    RUN_TEST(test_l75_sys_98);
    RUN_TEST(test_l75_sys_99);
    RUN_TEST(test_l75_sys_100);
    RUN_TEST(test_l75_sys_101);
    RUN_TEST(test_l75_sys_102);
    RUN_TEST(test_l75_sys_103);
    RUN_TEST(test_l75_sys_104);
    RUN_TEST(test_l75_sys_105);
    RUN_TEST(test_l75_sys_106);
    RUN_TEST(test_l75_sys_107);
    RUN_TEST(test_l75_sys_108);
    RUN_TEST(test_l75_sys_109);
    RUN_TEST(test_l75_sys_110);
    RUN_TEST(test_l75_sys_111);
    RUN_TEST(test_l75_sys_112);
    RUN_TEST(test_l75_sys_113);
    RUN_TEST(test_l75_sys_114);
    RUN_TEST(test_l75_sys_115);
    RUN_TEST(test_l75_sys_116);
    RUN_TEST(test_l75_sys_117);
    RUN_TEST(test_l75_sys_118);
    RUN_TEST(test_l75_sys_119);
    RUN_TEST(test_l75_sys_120);
    RUN_TEST(test_l75_sys_empty_file_part);
}
