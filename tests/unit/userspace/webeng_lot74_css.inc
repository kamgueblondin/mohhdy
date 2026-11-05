/* Lot 74: cascade layers (@layer): order statements, nested and
 * anonymous layers, !important reversed, refusals */

static void test_l74_layer_order_statement(void) {
    /* b then a: a is later, it wins whatever the specificity */
    lot52_render("<style>@layer b, a;@layer a{p{color:#000001}}@layer b{body p#a{color:#000002}}"
                 "@layer a{#b{color:#000003}}#b{color:#000004}"
                 "@layer b{p#c{color:#000005}}p+p+p{color:#000006}</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(4, l73c("b"));          /* unlayered beats every layer */
    TEST_ASSERT_EQUAL(6, l73c("c"));
    TEST_ASSERT_EQUAL(0, l61_diag_count("css at-rule skipped"));
    TEST_ASSERT_EQUAL(2, (int)g_doc.nlayers);
    TEST_ASSERT_EQUAL_STRING("b", g_doc.layer_name[0]);
    TEST_ASSERT_EQUAL(0, g_doc.layer_rank[0]);
    TEST_ASSERT_EQUAL(1, g_doc.layer_rank[1]);
    /* same layer: specificity, then order */
    lot52_render("<style>@layer x{#a{color:#000001}p{color:#000002}p{color:#000003}}</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(3, l73c("b"));
}

static void test_l74_layer_important_reversed(void) {
    lot52_render("<style>@layer x, y;"
                 "@layer x{#a{color:#000001!important}}@layer y{#a{color:#000002!important}}#a{color:#000003!important}"
                 "@layer y{#b{color:#000004!important}}#b{color:#000005!important}"
                 "#c{color:#000006!important}@layer y{p{color:#000007}}"
                 "@layer x{#d{color:#000008}}@layer y{#d{color:#000009!important}}#d{color:#00000a}</style>"
                 L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));          /* first layer wins for !important */
    TEST_ASSERT_EQUAL(4, l73c("b"));          /* a layer beats unlayered */
    TEST_ASSERT_EQUAL(6, l73c("c"));
    TEST_ASSERT_EQUAL(9, l73c("d"));          /* important over normal */
}

static void test_l74_layer_nested_and_anonymous(void) {
    lot52_render("<style>@layer a{#a{color:#000001}@layer b{#a{color:#000002}#b{color:#000003}}}"
                 "@layer a.b{#c{color:#000004}}@layer a{#c{color:#000005}}"
                 "@layer a.c{#b{color:#000006}}"
                 "@layer{#d{color:#000007}}@layer{#d{color:#000008}}@layer z{#d{color:#000009}}"
                 "@layer{#e{color:#00000a}}@layer z{#e{color:#00000b}}"
                 "@media (min-width:100px){@layer a{#f{color:#00000c}}}@layer a.b{#f{color:#00000d}}</style>"
                 L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));          /* own rules of a beat a.b */
    TEST_ASSERT_EQUAL(6, l73c("b"));          /* a.c after a.b */
    TEST_ASSERT_EQUAL(5, l73c("c"));
    TEST_ASSERT_EQUAL(9, l73c("d"));          /* z created after both anonymous */
    TEST_ASSERT_EQUAL(0xa, l73c("e"));        /* that anonymous layer comes after z */
    TEST_ASSERT_EQUAL(0xc, l73c("f"));        /* @media keeps the layer */
    /* a, a.b, a.c, anon, anon, z, anon: ranks a.b 0, a.c 1, a 2 */
    TEST_ASSERT_EQUAL(7, (int)g_doc.nlayers);
    TEST_ASSERT_EQUAL(2, g_doc.layer_rank[0]);
    TEST_ASSERT_EQUAL(0, g_doc.layer_rank[1]);
    TEST_ASSERT_EQUAL(1, g_doc.layer_rank[2]);
    TEST_ASSERT_EQUAL(0, g_doc.layer_parent[1]);
    TEST_ASSERT_EQUAL(-1, g_doc.layer_parent[3]);
}

static void test_l74_layer_refusals(void) {
    char css[1400];
    int k, n;
    lot52_render("<style>@layer a b{#a{color:#000001}}@layer a.{#b{color:#000002}}@layer .a{#c{color:#000003}}"
                 "@layer x,{#d{color:#000004}}@layer x,;@layer ;@layer a..b;@layer q,r{#e{color:#000005}}"
                 "@layer abcdefghijklmnopqrstuvwxyz{#f{color:#000006}}@layer ok{#g{color:#000007}}</style>"
                 L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(0, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(0, l73c("c"));
    TEST_ASSERT_EQUAL(0, l73c("d"));
    TEST_ASSERT_EQUAL(0, l73c("e"));
    TEST_ASSERT_EQUAL(0, l73c("f"));
    TEST_ASSERT_EQUAL(7, l73c("g"));
    TEST_ASSERT_EQUAL(8, l61_diag_count("css at-rule skipped"));   /* the ninth past the diag table */
    TEST_ASSERT_EQUAL(1, (int)g_doc.diag_dropped);
    TEST_ASSERT_EQUAL(1, (int)g_doc.nlayers);                      /* a refused prelude declares nothing */
    TEST_ASSERT_EQUAL_STRING("ok", g_doc.layer_name[0]);
    /* a full table: the rules stay in the current layer */
    n = snprintf(css, sizeof(css), "<style>");
    for (k = 0; k < WE_LAYERS_MAX; k++) n += snprintf(css + n, sizeof(css) - (size_t)n, "@layer l%d{}", k);
    n += snprintf(css + n, sizeof(css) - (size_t)n, "@layer over{#a{color:#000001}}#a{color:#000002}@layer l0{#b{color:#000003}}"
                  "@layer o1,o2;@layer over{#c{color:#000004}}@layer l1{#c{color:#000005}}</style>" L64_BODY L73_IDS);
    lot52_render(css, 320);
    TEST_ASSERT_EQUAL(WE_LAYERS_MAX, (int)g_doc.nlayers);
    TEST_ASSERT_EQUAL(3, l61_diag_count("css layer table full"));
    TEST_ASSERT_EQUAL(2, l73c("a"));          /* unlayered, the later rule */
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));          /* unlayered beats l1 */
}

static void lot74_css_run(void) {
    RUN_TEST(test_l74_layer_order_statement);
    RUN_TEST(test_l74_layer_important_reversed);
    RUN_TEST(test_l74_layer_nested_and_anonymous);
    RUN_TEST(test_l74_layer_refusals);
}
