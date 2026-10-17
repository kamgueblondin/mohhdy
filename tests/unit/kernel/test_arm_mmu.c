/* Lot 8 (US-076): ARMv7-A section descriptors and user pointer rule of the
 * ARM port (arch/arm/arm_mmu.c), checked on the host. */
#include "../../framework/unity.h"
#include "../../../arch/arm/arm_mmu.h"

#define EQ32(e, a) TEST_ASSERT_TRUE((uint32_t)(e) == (uint32_t)(a))

void setUp(void) {}
void tearDown(void) {}

void test_kernel_section_is_privileged_only(void) {
    uint32_t d = arm_section_desc(ARM_KERNEL_BASE, ARM_AP_PRIV_ONLY, ARM_MEM_NORMAL, 0);
    EQ32(0x2U, d & 0x3U);                    /* section */
    EQ32(ARM_KERNEL_BASE, d & 0xFFF00000U);
    EQ32(1U, (d >> 10) & 3U);                /* AP = PL1 only */
    EQ32(0U, (d >> 15) & 1U);                /* APX clear */
    EQ32(0U, (d >> 4) & 1U);                 /* executable */
    EQ32(1U, (d >> 12) & 7U);                /* TEX 001 */
    EQ32(3U, (d >> 2) & 3U);                 /* C B: write-back */
    EQ32(0U, (d >> 5) & 0xFU);               /* domain 0 */
}

void test_user_and_device_sections(void) {
    uint32_t u = arm_section_desc(ARM_USER_BASE, ARM_AP_FULL, ARM_MEM_NORMAL, 0);
    uint32_t v = arm_section_desc(0x09012345U, ARM_AP_PRIV_ONLY, ARM_MEM_DEVICE, 1);
    EQ32(3U, (u >> 10) & 3U);
    EQ32(ARM_USER_BASE, u & 0xFFF00000U);
    EQ32(0x09000000U, v & 0xFFF00000U);      /* low bits of pa dropped */
    EQ32(1U, (v >> 4) & 1U);                 /* XN on devices */
    EQ32(1U, (v >> 2) & 3U);                 /* B only: shareable device */
    EQ32(0U, (v >> 12) & 7U);
    EQ32(0x401U, arm_section_index(ARM_USER_BASE));
    EQ32(0x090U, arm_section_index(0x09000000U));
}

void test_user_range_rule(void) {
    TEST_ASSERT_TRUE(arm_user_range_ok(ARM_USER_BASE, 1U));
    TEST_ASSERT_TRUE(arm_user_range_ok(ARM_USER_BASE + ARM_USER_SIZE - 4U, 4U));
    TEST_ASSERT_TRUE(arm_user_range_ok(ARM_USER_BASE, ARM_USER_SIZE));
    TEST_ASSERT_FALSE(arm_user_range_ok(ARM_USER_BASE - 1U, 1U));         /* kernel */
    TEST_ASSERT_FALSE(arm_user_range_ok(0x40000100U, 4U));
    TEST_ASSERT_FALSE(arm_user_range_ok(ARM_USER_BASE + ARM_USER_SIZE - 4U, 5U));
    TEST_ASSERT_FALSE(arm_user_range_ok(ARM_USER_BASE + 16U, 0xFFFFFFF0U)); /* wrap */
    TEST_ASSERT_FALSE(arm_user_range_ok(ARM_USER_BASE + ARM_USER_SIZE, 1U));
    TEST_ASSERT_FALSE(arm_user_range_ok(0xFFFFFFFFU, 2U));
}

void test_dfsr_decoding(void) {
    TEST_ASSERT_EQUAL(1, arm_dfsr_kind(0x80DU));       /* WnR + permission section */
    TEST_ASSERT_EQUAL(1, arm_dfsr_kind(0x00DU));
    TEST_ASSERT_EQUAL(2, arm_dfsr_kind(0x005U));
    TEST_ASSERT_EQUAL(0, arm_dfsr_kind(0x40DU));       /* FS[4] set: other fault */
    TEST_ASSERT_EQUAL(0, arm_dfsr_kind(0x001U));       /* alignment */
}

int main(void) {
    unity_init();
    RUN_TEST(test_kernel_section_is_privileged_only);
    RUN_TEST(test_user_and_device_sections);
    RUN_TEST(test_user_range_rule);
    RUN_TEST(test_dfsr_decoding);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
