/* Lot 69 (US-076): the portable self-tests the ARM kernel runs at boot
 * (arch/arm/arm_portable.c) also pass on the host, so the expected values
 * are the i386/x86 results and the ARM boot log shows they match. */
#include "../../framework/unity.h"
#include "arch/arm/arm_portable.h"

static void test_aes_gcm_reference(void) { TEST_ASSERT_EQUAL(1, arm_port_aes_gcm()); }
static void test_edid_reference(void) { TEST_ASSERT_EQUAL(1, arm_port_edid()); }
static void test_flow_reference(void) { TEST_ASSERT_EQUAL(1, arm_port_flow()); }
static void test_mask(void) {
    TEST_ASSERT_EQUAL(ARM_PORT_ALL, arm_port_selftest());
    TEST_ASSERT_EQUAL(ARM_PORT_ALL, arm_port_selftest());   /* repeatable */
}

int main(void) {
    unity_init();
    RUN_TEST(test_aes_gcm_reference);
    RUN_TEST(test_edid_reference);
    RUN_TEST(test_flow_reference);
    RUN_TEST(test_mask);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
