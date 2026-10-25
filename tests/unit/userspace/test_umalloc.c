/* UX5 user heap allocator (userspace/umalloc.c) over a host arena. */
#include "../../framework/unity.h"
#include "umalloc.h"
#include <string.h>

static unsigned char g_arena[1 << 20] __attribute__((aligned(16)));
static unsigned g_limit;
static int g_calls;

static int grow(unsigned want) {
    unsigned got = (want + 4095U) & ~4095U;
    g_calls++;
    if (got > g_limit) return -1;
    return (int)got;
}

static void reset(unsigned limit) { g_limit = limit; g_calls = 0; umalloc_init(g_arena, grow); }

static void test_alloc_grows_on_demand(void) {
    char *a, *b;
    reset(sizeof(g_arena));
    TEST_ASSERT_EQUAL(0, (int)umalloc_mapped());
    a = (char *)umalloc(100);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL(0, ((unsigned long)a) & 15UL);
    TEST_ASSERT_EQUAL(4096, (int)umalloc_mapped());
    b = (char *)umalloc(10000);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT(b >= a + 100);
    TEST_ASSERT(umalloc_mapped() >= 10100U);
    memset(a, 'a', 100); memset(b, 'b', 10000);
    TEST_ASSERT_EQUAL('a', a[99]);
    TEST_ASSERT(umalloc_in_use() >= 10100U);
    TEST_ASSERT_NULL(umalloc(0));
}

static void test_free_reuses_and_coalesces(void) {
    char *a, *b, *c, *d;
    reset(sizeof(g_arena));
    a = (char *)umalloc(64); b = (char *)umalloc(64); c = (char *)umalloc(64);
    ufree(a); ufree(b);                       /* a+b merge into one hole */
    d = (char *)umalloc(120);
    TEST_ASSERT_EQUAL_PTR(a, d);
    ufree(d); ufree(c);                       /* tail free: heap back to empty */
    TEST_ASSERT_EQUAL(0, (int)umalloc_in_use());
    TEST_ASSERT_EQUAL_PTR(a, umalloc(16));
}

static void test_realloc_in_place_and_copy(void) {
    char *a, *b, *a2, *a3;
    reset(sizeof(g_arena));
    a = (char *)umalloc(32);
    strcpy(a, "bonjour");
    a2 = (char *)urealloc(a, 50000);          /* last block: grows in place */
    TEST_ASSERT_EQUAL_PTR(a, a2);
    TEST_ASSERT_EQUAL_STRING("bonjour", a2);
    b = (char *)umalloc(16);
    a3 = (char *)urealloc(a2, 60000);         /* not last: moves, keeps the bytes */
    TEST_ASSERT_NOT_NULL(a3);
    TEST_ASSERT(a3 != a2);
    TEST_ASSERT_EQUAL_STRING("bonjour", a3);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_PTR(a3, urealloc(a3, 10));  /* shrink request: same block */
}

static void test_out_of_memory_is_reported(void) {
    char *a;
    reset(8192);
    a = (char *)umalloc(4000);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NULL(umalloc(9000));          /* grow refused */
    TEST_ASSERT_NULL(urealloc(a, 9000));
    TEST_ASSERT_NOT_NULL(umalloc(1000));      /* still usable */
    umalloc_init(0, grow);
    TEST_ASSERT_NULL(umalloc(10));            /* no heap */
}

int main(void) {
    unity_init();
    RUN_TEST(test_alloc_grows_on_demand);
    RUN_TEST(test_free_reuses_and_coalesces);
    RUN_TEST(test_realloc_in_place_and_copy);
    RUN_TEST(test_out_of_memory_is_reported);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
