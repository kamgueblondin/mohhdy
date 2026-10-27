/* test_net_ifcfg.c - Lot 47 (US-G04): net-if parsing, the networker side
 * (static lease validation) and the kernel rights predicate. */
#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "net_ifcfg.h"
#include "../../../kernel/net_dhcp.h"
#include "../../../kernel/net_relay.h"
#include <string.h>

static net_dhcp_lease_t g_lease;
static int g_static, g_calls;
static uint8_t g_dhcp_ip[4] = {10, 32, 0, 15};

/* a fake networker built on the real net_dhcp_static_apply */
static int fake_call(os_peer_data_request_t* r) {
    int i;
    g_calls++;
    if (r->op != OS_PEER_IFCONFIG) return OS_PEER_BAD_REQUEST;
    if (r->data[0] == OS_IFCONFIG_STATIC) {
        if (net_dhcp_static_apply(&g_lease, r->data + 1, r->data + 5, r->data + 9) != 0) return OS_PEER_BAD_ADDRESS;
        g_static = 1;
    } else if (r->data[0] == OS_IFCONFIG_DHCP) {
        g_static = 0;
        memset(&g_lease, 0, sizeof(g_lease));
        memcpy(g_lease.ipv4, g_dhcp_ip, 4); g_lease.valid = 1;
    }
    r->data[0] = g_static ? OS_IFCONFIG_MODE_STATIC : OS_IFCONFIG_MODE_DHCP;
    for (i = 0; i < 4; i++) {
        r->data[1 + i] = g_lease.ipv4[i];
        r->data[5 + i] = g_lease.subnet_mask[i];
        r->data[9 + i] = g_lease.router_ipv4[i];
    }
    r->data[13] = g_lease.valid;
    return 0;
}
static int denied_call(os_peer_data_request_t* r) { (void)r; return OS_PEER_DENIED; }

static int run(const char* a0, const char* a1, const char* a2, const char* a3, int argc, char* out) {
    static os_peer_data_request_t r;
    const char* av[4];
    av[0] = a0; av[1] = a1; av[2] = a2; av[3] = a3;
    return net_ifcfg_run(argc, av, fake_call, &r, out, 160);
}

static void test_parse_ip(void) {
    uint8_t ip[4];
    TEST_ASSERT_EQUAL(0, net_ifcfg_parse_ip("10.32.0.77", ip));
    TEST_ASSERT_EQUAL(77, ip[3]);
    TEST_ASSERT_EQUAL(-1, net_ifcfg_parse_ip("10.32.0", ip));
    TEST_ASSERT_EQUAL(-1, net_ifcfg_parse_ip("10.32.0.256", ip));
    TEST_ASSERT_EQUAL(-1, net_ifcfg_parse_ip("10.32..1", ip));
    TEST_ASSERT_EQUAL(-1, net_ifcfg_parse_ip("10.32.0.1.2", ip));
    TEST_ASSERT_EQUAL(-1, net_ifcfg_parse_ip("a.b.c.d", ip));
}

static void test_static_then_dhcp(void) {
    char out[160];
    memset(&g_lease, 0, sizeof(g_lease)); g_static = 0;
    TEST_ASSERT_EQUAL(0, run("static", "10.32.0.77", "255.255.255.0", "10.32.0.2", 4, out));
    TEST_ASSERT_EQUAL_STRING("net-if ok mode static ip 10.32.0.77 mask 255.255.255.0 gw 10.32.0.2 lease 1\n", out);
    TEST_ASSERT_EQUAL(0xFFFFFFFFU, g_lease.lease_seconds);
    TEST_ASSERT_EQUAL(0, run("", "", "", "", 0, out));
    TEST_ASSERT_NOT_NULL(strstr(out, "mode static ip 10.32.0.77"));
    TEST_ASSERT_EQUAL(0, run("dhcp", "", "", "", 1, out));
    TEST_ASSERT_NOT_NULL(strstr(out, "net-if ok mode dhcp ip 10.32.0.15"));
}

static void test_bad_addresses_refused_lease_kept(void) {
    char out[160];
    static const char* bad[][3] = {
        {"0.0.0.0", "255.255.255.0", "10.32.0.2"},       /* zero */
        {"127.0.0.5", "255.0.0.0", "127.0.0.1"},         /* loopback */
        {"224.0.0.5", "255.255.255.0", "224.0.0.1"},     /* multicast */
        {"10.32.0.77", "255.0.255.0", "10.32.0.2"},      /* holes in the mask */
        {"10.32.0.77", "255.255.255.255", "10.32.0.2"},  /* /32 */
        {"10.32.0.0", "255.255.255.0", "10.32.0.2"},     /* network address */
        {"10.32.0.255", "255.255.255.0", "10.32.0.2"},   /* broadcast */
        {"10.32.0.77", "255.255.255.0", "10.33.0.2"},    /* gateway off subnet */
        {"10.32.0.77", "255.255.255.0", "10.32.0.77"},   /* gateway = address */
    };
    int i;
    memset(&g_lease, 0, sizeof(g_lease)); g_static = 0;
    TEST_ASSERT_EQUAL(0, run("static", "10.32.0.50", "255.255.255.0", "10.32.0.2", 4, out));
    for (i = 0; i < (int)(sizeof(bad) / sizeof(bad[0])); i++) {
        TEST_ASSERT_EQUAL(1, run("static", bad[i][0], bad[i][1], bad[i][2], 4, out));
        TEST_ASSERT_NOT_NULL(strstr(out, "net-if error bad address"));
        TEST_ASSERT_EQUAL(50, g_lease.ipv4[3]);
    }
}

static void test_usage_and_denied(void) {
    static os_peer_data_request_t r;
    const char* av[4] = {"static", "10.32.0.77", "255.255.255.0", "10.32.0.2"};
    char out[160];
    g_calls = 0;
    TEST_ASSERT_EQUAL(1, run("static", "10.32.0.77", "", "", 2, out));
    TEST_ASSERT_NOT_NULL(strstr(out, "usage"));
    TEST_ASSERT_EQUAL(1, run("bogus", "", "", "", 1, out));
    TEST_ASSERT_EQUAL(1, run("static", "x", "255.255.255.0", "10.32.0.2", 4, out));
    TEST_ASSERT_NOT_NULL(strstr(out, "syntax"));
    TEST_ASSERT_EQUAL(0, g_calls);          /* nothing sent on a bad line */
    TEST_ASSERT_EQUAL(1, net_ifcfg_run(4, av, denied_call, &r, out, 160));
    TEST_ASSERT_EQUAL_STRING("net-if error denied (root shell only)\n", out);
}

static void test_rights_predicate(void) {
    TEST_ASSERT_EQUAL(1, net_ifconfig_allowed(OS_IFCONFIG_GET, 9, 3));
    TEST_ASSERT_EQUAL(1, net_ifconfig_allowed(OS_IFCONFIG_STATIC, 3, 3));
    TEST_ASSERT_EQUAL(0, net_ifconfig_allowed(OS_IFCONFIG_STATIC, 9, 3));
    TEST_ASSERT_EQUAL(0, net_ifconfig_allowed(OS_IFCONFIG_DHCP, 9, 3));
    TEST_ASSERT_EQUAL(0, net_ifconfig_allowed(OS_IFCONFIG_STATIC, 3, 0));   /* no root shell yet */
    TEST_ASSERT_EQUAL(0, net_ifconfig_allowed(7U, 9, 3));
}

int main(void) {
    unity_init();
    RUN_TEST(test_parse_ip);
    RUN_TEST(test_static_then_dhcp);
    RUN_TEST(test_bad_addresses_refused_lease_kept);
    RUN_TEST(test_usage_and_denied);
    RUN_TEST(test_rights_predicate);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
