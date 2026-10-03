/* kernel/mem/vmm_share.c: kernel page tables added after a task directory
 * was created (VBE framebuffer at 0xFD000000, PDE 0x3F4..0x3F7) are copied
 * into it; private (user) tables and tables it already has are kept. The
 * QEMU check is qemu-osui-gui (IRQ0 cursor redraw with any CR3 loaded). */
#include "../../framework/unity.h"
#include <stdint.h>
#include <string.h>
#include "../../../kernel/mem/vmm.h"
#include "../../../kernel/mem/vmm_share.c"

static page_table_t* kernel_tables[ENTRIES_PER_TABLE];
static page_table_t* task_tables[ENTRIES_PER_TABLE];
static page_directory_t kernel_pd, task_pd;
static vmm_directory_t kdir, tdir;
static page_table_t t_low, t_fb0, t_fb1, t_user, t_own;

static void setup(void) {
    memset(kernel_tables, 0, sizeof(kernel_tables));
    memset(task_tables, 0, sizeof(task_tables));
    memset(&kernel_pd, 0, sizeof(kernel_pd));
    memset(&task_pd, 0, sizeof(task_pd));
    memset(&kdir, 0, sizeof(kdir));
    memset(&tdir, 0, sizeof(tdir));
    kdir.tables = kernel_tables; kdir.physical_dir = &kernel_pd;
    tdir.tables = task_tables; tdir.physical_dir = &task_pd;
    /* Task created when the kernel only had its low identity table. */
    kernel_tables[0] = &t_low; kernel_pd.tablesPhysical[0] = 0x1003U;
    task_tables[0] = &t_low; task_pd.tablesPhysical[0] = 0x1003U;
    /* Desktop start: the LFB tables appear in the kernel directory only. */
    kernel_tables[0x3F4] = &t_fb0; kernel_pd.tablesPhysical[0x3F4] = 0x2007U;
    kernel_tables[0x3F5] = &t_fb1; kernel_pd.tablesPhysical[0x3F5] = 0x3007U;
}

static void test_missing_kernel_tables_are_shared(void) {
    setup();
    TEST_ASSERT_TRUE(task_tables[0x3F4] == 0);
    TEST_ASSERT_EQUAL(2U, vmm_share_kernel_tables(&tdir, &kdir, 0x3F4U, 4U));
    TEST_ASSERT_TRUE(task_tables[0x3F4] == &t_fb0);
    TEST_ASSERT_TRUE(task_tables[0x3F5] == &t_fb1);
    TEST_ASSERT_EQUAL(0x2007U, task_pd.tablesPhysical[0x3F4]);
    TEST_ASSERT_EQUAL(0x3007U, task_pd.tablesPhysical[0x3F5]);
    TEST_ASSERT_TRUE(task_tables[0x3F6] == 0);
    TEST_ASSERT_EQUAL(0U, task_pd.tablesPhysical[0x3F6]);
    /* Idempotent. */
    TEST_ASSERT_EQUAL(0U, vmm_share_kernel_tables(&tdir, &kdir, 0x3F4U, 4U));
}

static void test_private_and_existing_tables_kept(void) {
    setup();
    /* A user-private table at the same index is never replaced. */
    task_tables[0x3F4] = &t_user; task_pd.tablesPhysical[0x3F4] = 0x4007U;
    tdir.private_table_mask[0x3F4 / 32U] |= 1U << (0x3F4 % 32U);
    /* A table the task already has (not private) is kept too. */
    task_tables[0x3F5] = &t_own; task_pd.tablesPhysical[0x3F5] = 0x5007U;
    TEST_ASSERT_EQUAL(0U, vmm_share_kernel_tables(&tdir, &kdir, 0x3F4U, 2U));
    TEST_ASSERT_TRUE(task_tables[0x3F4] == &t_user);
    TEST_ASSERT_TRUE(task_tables[0x3F5] == &t_own);
    TEST_ASSERT_EQUAL(0x4007U, task_pd.tablesPhysical[0x3F4]);
}

static void test_bounds_and_bad_arguments(void) {
    setup();
    TEST_ASSERT_EQUAL(0U, vmm_share_kernel_tables(0, &kdir, 0U, 1024U));
    TEST_ASSERT_EQUAL(0U, vmm_share_kernel_tables(&tdir, 0, 0U, 1024U));
    TEST_ASSERT_EQUAL(0U, vmm_share_kernel_tables(&kdir, &kdir, 0U, 1024U));
    TEST_ASSERT_EQUAL(0U, vmm_share_kernel_tables(&tdir, &kdir, 1024U, 1U));
    /* count clamped to the end of the directory. */
    kernel_tables[1023] = &t_fb0; kernel_pd.tablesPhysical[1023] = 0x6003U;
    TEST_ASSERT_EQUAL(1U, vmm_share_kernel_tables(&tdir, &kdir, 1023U, 0xFFFFFFFFU));
    TEST_ASSERT_TRUE(task_tables[1023] == &t_fb0);
    /* Whole range: the two LFB tables remain to share. */
    TEST_ASSERT_EQUAL(2U, vmm_share_kernel_tables(&tdir, &kdir, 0U, ENTRIES_PER_TABLE));
}

int main(void) {
    unity_init();
    RUN_TEST(test_missing_kernel_tables_are_shared);
    RUN_TEST(test_private_and_existing_tables_kept);
    RUN_TEST(test_bounds_and_bad_arguments);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
