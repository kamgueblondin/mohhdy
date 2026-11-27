/* Lot 128 (US-031): flex-grow, flex-shrink and the flex shorthand take
 * fractional factors (CSS Flexbox 7.3, a non-negative <number>): 0.5, 1.25,
 * .75 are kept as an integer part and hundredths, further decimals cut. A
 * line whose unfrozen factors sum below 1 grows (or shrinks) by that
 * fraction of its initial free space (or overflow) only (Flexbox 9.7 step
 * 4b), the rest stays free (or overflows) without a diagnostic. A math
 * function still gives an integer (Lot 118). Values follow
 * docs/lot128_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

#define EQ_MSG(e, a, m) do { int e_ = (int)(e), a_ = (int)(a); \
    if (e_ != a_) printf("  %s: expected %d, got %d\n", (m), e_, a_); \
    TEST_ASSERT_EQUAL(e_, a_); } while (0)

static we_doc_t g_doc;
static we_forms_t g_forms;
static we_node_t g_none;

static void lays(const char *html) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 640, 480));
}

static const we_node_t *el(const char *id) {
    int i = we_find_id(&g_doc, id);
    g_none.h = -1; g_none.w = -1; g_none.x = -1; g_none.y = -1;
    g_none.flex_grow = -2; g_none.flex_shrink = -2; g_none.grow_frac = 255; g_none.shrink_frac = 255;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

static int diag_count(const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) c++;
    return c;
}

#define GROW(id, W, F) do { EQ_MSG(W, el(id)->flex_grow, id " grow"); EQ_MSG(F, el(id)->grow_frac, id " grow hundredths"); } while (0)
#define SHRINK(id, W, F) do { EQ_MSG(W, el(id)->flex_shrink, id " shrink"); EQ_MSG(F, el(id)->shrink_frac, id " shrink hundredths"); } while (0)
#define WX(id, X, W) do { EQ_MSG(X, el(id)->x - el("r")->x, id " x"); EQ_MSG(W, el(id)->w, id " w"); } while (0)

/* --- the longhands: which numbers are factors --- */
static void test_l128_parse(void) {
    lays("<div style='display:flex'>"
         "<p id=a style='flex-grow:0.5;flex-shrink:1.5'>a</p>"
         "<p id=b style='flex-grow:.25;flex-shrink:2.125'>b</p>"
         "<p id=c style='flex-grow:3.7;flex-shrink:0.05'>c</p>"
         "<p id=d style='flex-grow:1000.99;flex-shrink:0.999'>d</p>"
         "<p id=e style='flex-grow:0.5;flex-grow:2;flex-shrink:0.5;flex-shrink:4'>e</p>"
         "<p id=f style='flex-grow:0.5;flex-grow:calc(1 / 2);flex-shrink:7.5;flex-shrink:calc(3)'>f</p>"
         "<p id=g style='flex-grow:0.0;flex-shrink:00.10'>g</p>"
         "<p id=h>h</p></div>");
    GROW("a", 0, 50); SHRINK("a", 1, 50);
    GROW("b", 0, 25); SHRINK("b", 2, 12);
    GROW("c", 3, 70); SHRINK("c", 0, 5);
    GROW("d", 1000, 99); SHRINK("d", 0, 99);
    GROW("e", 2, 0); SHRINK("e", 4, 0);          /* an integer clears the hundredths */
    GROW("f", 1, 0); SHRINK("f", 3, 0);          /* so does a math function, rounded */
    GROW("g", 0, 0); SHRINK("g", 0, 10);
    GROW("h", -1, 0); SHRINK("h", -1, 0);
    EQ_MSG(0, diag_count("refused"), "accepted");
}

/* a second style pass (a container query read another size) starts
 * from no factor: 0.5 / 2.5 apply in a 624 px container, not in 284 */
static void test_l128_restyle(void) {
    lays("<style>.c{container-type:inline-size}@container (min-width: 500px){#a{flex-grow:0.5;flex-shrink:2.5}}</style>"
         "<div class=c><div style='display:flex'><p id=a>a</p></div></div>");
    GROW("a", 0, 50); SHRINK("a", 2, 50);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 300, 480));
    GROW("a", -1, 0); SHRINK("a", -1, 0);
}

/* refused: the earlier factor and its hundredths stay */
static void test_l128_refused(void) {
    static const char *const bad[] = {
        "1.", ".", "1001.5", "-0.5", "+0.5", "1.5.5", "1.5e1", "0.5x", "a.5", "1,5", "..5", "0.-5"
    };
    unsigned k;
    for (k = 0; k < sizeof(bad) / sizeof(bad[0]); k++) {
        char html[256];
        snprintf(html, sizeof(html), "<div style='display:flex'><p id=a style='flex-grow:2.5;flex-shrink:3.25;flex-grow:%s;flex-shrink:%s'>a</p></div>", bad[k], bad[k]);
        lays(html);
        EQ_MSG(2, el("a")->flex_grow, bad[k]); EQ_MSG(50, el("a")->grow_frac, bad[k]);
        EQ_MSG(3, el("a")->flex_shrink, bad[k]); EQ_MSG(25, el("a")->shrink_frac, bad[k]);
        EQ_MSG(1, diag_count("flex-grow refused"), bad[k]);
        EQ_MSG(1, diag_count("flex-shrink refused"), bad[k]);
        EQ_MSG(2, (int)g_doc.ndiag, bad[k]);
    }
}

/* --- the flex shorthand --- */
static void test_l128_shorthand(void) {
    lays("<div style='display:flex'>"
         "<p id=a style='flex:0.5'>a</p>"
         "<p id=b style='flex:1.5 0.25'>b</p>"
         "<p id=c style='flex:2.75 3.5 10px'>c</p>"
         "<p id=d style='flex:.5 auto'>d</p>"
         "<p id=e style='flex:0.5 0.5;flex:none'>e</p>"
         "<p id=f style='flex:1.5 2.5;flex:initial'>f</p>"
         "<p id=g style='flex:1.5 2.5;flex:3'>g</p>"
         "<p id=h style='flex:1.5 2.5;flex:auto'>h</p>"
         "<p id=i style='flex:20px 0.5'>i</p></div>");
    GROW("a", 0, 50); SHRINK("a", 1, 0); EQ_MSG(0, el("a")->flex_basis, "a basis 0");
    GROW("b", 1, 50); SHRINK("b", 0, 25);
    GROW("c", 2, 75); SHRINK("c", 3, 50); EQ_MSG(10, el("c")->flex_basis, "c basis");
    GROW("d", 0, 50); SHRINK("d", 1, 0); EQ_MSG(-1, el("d")->flex_basis, "d basis auto");
    GROW("e", 0, 0); SHRINK("e", 0, 0);
    GROW("f", 0, 0); SHRINK("f", 1, 0);
    GROW("g", 3, 0); SHRINK("g", 1, 0);
    GROW("h", 1, 0); SHRINK("h", 1, 0);
    GROW("i", 0, 50); SHRINK("i", 1, 0); EQ_MSG(20, el("i")->flex_basis, "i basis");
    EQ_MSG(0, diag_count("refused"), "accepted");
    /* refused as a whole: the earlier values stay */
    lays("<div style='display:flex'><p id=a style='flex:2.5 3.25 4px;flex:1. 2'>a</p>"
         "<p id=b style='flex:2.5 3.25 4px;flex:0.5 1.5 2.5'>b</p>"
         "<p id=c style='flex:2.5 3.25 4px;flex:0.5px'>c</p></div>");
    GROW("a", 2, 50); SHRINK("a", 3, 25);
    GROW("b", 2, 50); SHRINK("b", 3, 25);
    GROW("c", 2, 50); SHRINK("c", 3, 25);
    EQ_MSG(3, diag_count("flex refused"), "shorthand refused");
}

/* --- growing --- */
static void test_l128_grow(void) {
    /* weighted by hundredths, rounding left to right */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex-grow:0.5;width:0px'></i><i id=b style='flex-grow:1.5;width:0px'></i></div>");
    WX("a", 0, 25); WX("b", 25, 75);
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:1 0 0'></i><i id=b style='flex:0.5 0 0'></i></div>");
    WX("a", 0, 67); WX("b", 67, 33);
    /* a sum of exactly 1 uses all the free space */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:0.5 0 0'></i><i id=b style='flex:0.5 0 0'></i></div>");
    WX("a", 0, 50); WX("b", 50, 50);
    /* below 1: that fraction of the free space only */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex-grow:0.25;flex-basis:0'></i></div>");
    WX("a", 0, 25);
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:0.3'></i><i id=b style='flex:.3'></i></div>");
    WX("a", 0, 30); WX("b", 30, 30);
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:0.07 0 0'></i><i id=b style='flex:0.02 0 0'></i><i id=c style='flex:0.01 0 0'></i></div>");
    WX("a", 0, 7); WX("b", 7, 2); WX("c", 9, 1);
    /* on top of a basis, the initial free space counts */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:0.5 1 20px'></i><i id=b style='flex:0 1 40px'></i></div>");
    WX("a", 0, 40); WX("b", 40, 40);
    /* a clamped item freezes; the others share initial free space times
     * their own sum, less what they took (25, not 50) */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:0.5 0 0;max-width:10px'></i><i id=b style='flex:0.25 0 0'></i></div>");
    WX("a", 0, 10); WX("b", 10, 25);
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:0.5 0 0;max-width:10px'></i><i id=b style='flex:0.75 0 0'></i></div>");
    WX("a", 0, 10); WX("b", 10, 75);
    /* a clamp that leaves the rest above 1 */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='flex:1 0 0;max-width:10px'></i><i id=b style='flex:0.5 0 0'></i><i id=c style='flex:0.5 0 0'></i></div>");
    WX("a", 0, 10); WX("b", 10, 45); WX("c", 55, 45);
    /* tiny shares: 8 * 0.9 is 7, 2 each, the last pixel left to right */
    lays("<div id=r style='display:flex;width:8px'><i id=a style='flex:0.3 0 0'></i><i id=b style='flex:0.3 0 0'></i><i id=c style='flex:0.3 0 0'></i></div>");
    WX("a", 0, 3); WX("b", 3, 2); WX("c", 5, 2);
    /* the rounding pixel skips an item at its maximum (a at 2) */
    lays("<div id=r style='display:flex;width:10px'><i id=a style='flex:1 0 0;max-width:2px'></i><i id=b style='flex:0.3 0 0'></i><i id=c style='flex:0.3 0 0'></i><i id=d style='flex:0.3 0 0'></i></div>");
    WX("a", 0, 2); WX("b", 2, 3); WX("c", 5, 3); WX("d", 8, 2);
    /* the rounding pixel skips an item that does not grow (a, factor 0) */
    lays("<div id=r style='display:flex;width:8px'><i id=a style='flex:0 0 0'></i><i id=b style='flex:0.3 0 0'></i><i id=c style='flex:0.3 0 0'></i><i id=d style='flex:0.3 0 0'></i></div>");
    WX("a", 0, 0); WX("b", 0, 3); WX("c", 3, 2); WX("d", 5, 2);
    /* a column grows the same way */
    lays("<div id=r style='display:flex;flex-direction:column;height:100px'><i id=a style='flex:0.5 0 0;min-height:0'></i><i id=b style='flex:0.2 0 0;min-height:0'></i></div>");
    EQ_MSG(50, el("a")->h, "column a"); EQ_MSG(20, el("b")->h, "column b");
    EQ_MSG(50, el("b")->y - el("r")->y, "column b y");
    EQ_MSG(0, diag_count("flex"), "no flex diagnostic");
}

/* --- shrinking --- */
static void test_l128_shrink(void) {
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.5'></i><i id=b style='width:80px;flex-shrink:1.5'></i></div>");
    WX("a", 0, 65); WX("b", 65, 35);
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.5'></i><i id=b style='width:80px;flex-shrink:0.5'></i></div>");
    WX("a", 0, 50); WX("b", 50, 50);
    /* below 1: only that fraction of the overflow, no diagnostic */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.25'></i><i id=b style='width:80px;flex-shrink:0.25'></i></div>");
    WX("a", 0, 65); WX("b", 65, 65);
    EQ_MSG(0, diag_count("flex minimum prevents fit"), "intended overflow");
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0'></i><i id=b style='width:80px;flex-shrink:0.5'></i></div>");
    WX("a", 0, 80); WX("b", 80, 50);
    EQ_MSG(0, diag_count("flex minimum prevents fit"), "intended overflow 2");
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.01'></i><i id=b style='width:80px;flex-shrink:0'></i></div>");
    WX("a", 0, 80); WX("b", 80, 80);
    EQ_MSG(0, diag_count("flex minimum prevents fit"), "under a pixel");
    /* a frozen minimum: the rest shrink by initial overflow times their
     * own sum, less what they gave */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.25;min-width:75px'></i><i id=b style='width:80px;flex-shrink:0.25'></i></div>");
    WX("a", 0, 75); WX("b", 75, 65);
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.5;min-width:78px'></i><i id=b style='width:80px;flex-shrink:0.25'></i></div>");
    WX("a", 0, 78); WX("b", 78, 65);
    EQ_MSG(0, diag_count("flex minimum prevents fit"), "frozen, intended");
    /* a minimum inside the fraction still reports the miss */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.75;min-width:80px'></i><i id=b style='width:80px;flex-shrink:0.25;min-width:80px'></i></div>");
    WX("a", 0, 80); WX("b", 80, 80);
    EQ_MSG(1, diag_count("flex minimum prevents fit"), "minimum blocks");
    /* the fraction shrinks a pixel, the minimums stop it: still reported */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0.25;min-width:79px'></i><i id=b style='width:80px;flex-shrink:0.25;min-width:79px'></i></div>");
    WX("a", 0, 79); WX("b", 79, 79);
    EQ_MSG(1, diag_count("flex minimum prevents fit"), "minimum blocks after a pass");
    /* a factor of 1 freezes at its minimum, the other (0.5) then takes
     * half the initial overflow in all, 30, not half of what is left */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:1;min-width:70px'></i><i id=b style='width:80px;flex-shrink:0.5'></i></div>");
    WX("a", 0, 70); WX("b", 70, 50);
    EQ_MSG(0, diag_count("flex minimum prevents fit"), "frozen at 1, intended");
    /* a large factor freezes at its minimum having given 50; the remaining
     * overflow (10) is below half the initial one: it is all taken */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:10;min-width:30px'></i><i id=b style='width:80px;flex-shrink:0.5'></i></div>");
    WX("a", 0, 30); WX("b", 30, 70);
    EQ_MSG(0, diag_count("flex minimum prevents fit"), "fits");
    /* shares follow basis * factor, the fraction only the factors: b
     * freezes at its minimum and a, having given 60, restarts from its
     * basis and gives a tenth of the initial 120 only (CSS Flexbox 9.7
     * step 4) */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:200px;flex-shrink:0.1'></i><i id=b style='width:20px;flex-shrink:0.8;min-width:19px'></i></div>");
    WX("a", 0, 188); WX("b", 188, 19);
    EQ_MSG(0, diag_count("flex minimum prevents fit"), "restart, intended");
    /* the restart leaves a frozen item at its minimum (b stays 19 even
     * though its share of the new 9 px would be 0) */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:1000px;flex-shrink:0.01'></i><i id=b style='width:20px;flex-shrink:0.05;min-width:19px'></i></div>");
    WX("a", 0, 991); WX("b", 991, 19);
    /* a fraction spent exactly (cap less what was given is 0) stops
     * there: no restart that would round the shares again */
    lays("<div id=r style='display:flex;width:28px'><i id=a style='flex:0.30 0.23 67px'></i><i id=b style='flex:0.88 0.29 30px'></i></div>");
    WX("a", 0, 44); WX("b", 44, 18);
    /* after a restart the remaining overflow counts what the restarted
     * items gave back: a later pass takes the right last pixel */
    lays("<div id=r style='display:flex;width:101px'><i id=a style='flex:0.14 0.42 93px'></i><i id=b style='flex:2.3 0.1 297px'></i><i id=c style='flex:0 0.8 73px'></i><i id=d style='flex:0.29 0.36 215px'></i><i id=e style='flex:0.62 0.49 34px;max-width:197px'></i></div>");
    WX("a", 0, 0); WX("b", 0, 219); WX("c", 219, 0); WX("d", 219, 12); WX("e", 231, 0);
    /* a 0 basis never violates its minimum: its factor stays in the sum */
    lays("<div id=r style='display:flex;width:50px'><i id=a style='flex:0 0.5 0'></i><i id=b style='flex:0 0.5 100px'></i></div>");
    WX("a", 0, 0); WX("b", 0, 50);
    lays("<div id=r style='display:flex;width:50px'><i id=a style='flex:0 0.25 0'></i><i id=b style='flex:0 0.5 100px'></i></div>");
    WX("a", 0, 0); WX("b", 0, 63);   /* 0.75 * 50 is 37 */
    /* a 0 basis held by min-width is frozen from the start */
    lays("<div id=r style='display:flex;width:50px'><i id=a style='flex:0 0.5 0;min-width:10px'></i><i id=b style='flex:0 0.5 100px'></i></div>");
    WX("a", 0, 10); WX("b", 10, 70);
    /* tiny overflow: 2 * 0.9 is 1 pixel, from the first item only */
    lays("<div id=r style='display:flex;width:28px'><i id=a style='width:10px;flex-shrink:0.3'></i><i id=b style='width:10px;flex-shrink:0.3'></i><i id=c style='width:10px;flex-shrink:0.3'></i></div>");
    WX("a", 0, 9); WX("b", 9, 10); WX("c", 19, 10);
    /* the integer base: unchanged */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:80px;flex-shrink:0'></i><i id=b style='width:80px;flex-shrink:0'></i></div>");
    WX("a", 0, 80); WX("b", 80, 80);
    EQ_MSG(1, diag_count("flex minimum prevents fit"), "inflexible");
    /* a weight sum above 32 bits */
    lays("<div id=r style='display:flex;width:100px'><i id=a style='width:16000px;flex-shrink:1000.99'></i><i id=b style='width:16000px;flex-shrink:1000.99'></i><i id=c style='width:16000px;flex-shrink:1000.99'></i></div>");
    WX("a", 0, 33); WX("b", 33, 33); WX("c", 66, 34);
}

int main(void) {
    unity_init();
    RUN_TEST(test_l128_parse);
    RUN_TEST(test_l128_restyle);
    RUN_TEST(test_l128_refused);
    RUN_TEST(test_l128_shorthand);
    RUN_TEST(test_l128_grow);
    RUN_TEST(test_l128_shrink);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
