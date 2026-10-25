/* UX5 SYS_HEAP_GROW core (kernel/user_heap.c): page by page, grow only. */
#include "../../framework/unity.h"
#include "kernel/user_heap.h"
#include "include/os_syscalls.h"

static uint32_t g_mapped[64];
static int g_n, g_fail_at = -1;

static int map_page(void *ctx, uint32_t va) {
    (void)ctx;
    if (g_n == g_fail_at) return -1;
    if (g_n < 64) g_mapped[g_n] = va;
    g_n++;
    return 0;
}

static void test_grow_maps_whole_pages(void) {
    uint32_t size = 0;
    g_n = 0; g_fail_at = -1;
    TEST_ASSERT_EQUAL(0, user_heap_grow(&size, 0, map_page, 0));   /* query */
    TEST_ASSERT_EQUAL(8192, user_heap_grow(&size, 5000, map_page, 0));
    TEST_ASSERT_EQUAL(2, g_n);
    TEST_ASSERT_EQUAL(OS_USER_HEAP_BASE, g_mapped[0]);
    TEST_ASSERT_EQUAL(OS_USER_HEAP_BASE + 4096U, g_mapped[1]);
    TEST_ASSERT_EQUAL(8192, user_heap_grow(&size, 100, map_page, 0)); /* never shrinks */
    TEST_ASSERT_EQUAL(2, g_n);
    TEST_ASSERT_EQUAL(12288, user_heap_grow(&size, 12288, map_page, 0));
    TEST_ASSERT_EQUAL(OS_USER_HEAP_BASE + 8192U, g_mapped[2]);
}

static void test_limits_and_failures(void) {
    uint32_t size = 0;
    g_n = 0; g_fail_at = 1;
    TEST_ASSERT_EQUAL(OS_HEAP_TOO_BIG, user_heap_grow(&size, OS_USER_HEAP_MAX + 1U, map_page, 0));
    TEST_ASSERT_EQUAL(0, (int)size);
    TEST_ASSERT_EQUAL(OS_HEAP_NO_MEMORY, user_heap_grow(&size, 3 * 4096, map_page, 0));
    TEST_ASSERT_EQUAL(4096, (int)size);   /* the page mapped before the failure stays */
    TEST_ASSERT_EQUAL(OS_HEAP_NO_MEMORY, user_heap_grow(0, 10, map_page, 0));
    TEST_ASSERT_EQUAL(OS_HEAP_NO_MEMORY, user_heap_grow(&size, 10, 0, 0));
}

int main(void) {
    unity_init();
    RUN_TEST(test_grow_maps_whole_pages);
    RUN_TEST(test_limits_and_failures);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
