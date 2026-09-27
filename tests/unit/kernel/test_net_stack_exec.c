/* Tranche 5 pile: kernel/net_stack_exec.c, the relayed-syscall executor the
 * Ring 3 networker runs on its own copy of the stack (socket registry, TCP,
 * ARP/IPv4 framing, wire engine, LLM client hooks). */
#include "../../framework/unity.h"
#include "../../../kernel/net_stack_exec.h"
#include <string.h>

static const uint8_t k_local[4] = {10, 32, 0, 15};
static const uint8_t k_remote[4] = {10, 32, 0, 2};

static uint8_t g_sent[8][1536];
static uint16_t g_sent_len[8];
static int g_sent_count, g_polls, g_idles, g_rounds;
static uint8_t g_script[1536];
static uint16_t g_script_len;

static int fake_emit(void* context, const uint8_t* frame, uint16_t length) {
    (void)context;
    if (g_sent_count >= 8) return -1;
    memcpy(g_sent[g_sent_count], frame, length);
    g_sent_len[g_sent_count++] = length;
    return 0;
}

/* Delivers the scripted frame once, on the first poll after the first frame went out. */
static int fake_poll(void* user, uint8_t* frame, uint16_t capacity, uint16_t* length) {
    (void)user;
    g_polls++;
    if (g_script_len == 0U || g_sent_count == 0 || g_script_len > capacity) return -1;
    memcpy(frame, g_script, g_script_len);
    *length = g_script_len;
    g_script_len = 0U;
    return 0;
}

static void fake_idle(void* user) { (void)user; g_idles++; }
static void fake_round(void* user) { (void)user; g_rounds++; }

static ne2k_device_t g_dev;
static net_arp_cache_t g_cache;
static uint8_t g_tx[1536], g_rx[1536];

static void stack_setup(net_stack_t* st, int with_nic) {
    net_stack_init(st);
    net_socket_reset_all();
    net_wire_reset();
    memset(&g_dev, 0, sizeof(g_dev));
    g_dev.base_port = 0x300U;
    g_dev.mac[0] = 0x52; g_dev.mac[1] = 0x54; g_dev.mac[5] = 0x56;
    g_dev.mac_valid = 1U;
    (void)net_arp_cache_init(&g_cache);
    g_sent_count = 0; g_polls = 0; g_idles = 0; g_rounds = 0; g_script_len = 0U;
    if (!with_nic) return;
    st->emit = fake_emit; st->poll = fake_poll; st->idle = fake_idle; st->after_round = fake_round;
    st->device = &g_dev; st->cache = &g_cache;
    st->tx = g_tx; st->rx = g_rx; st->capacity = sizeof(g_tx);
}

static void request(os_net_relay_request_t* r, uint32_t op, uint32_t a0, uint32_t a1, uint32_t a2) {
    memset(r, 0, sizeof(*r));
    r->op = op; r->arg0 = a0; r->arg1 = a1; r->arg2 = a2;
    r->out_capacity = OS_NET_RELAY_MAX_OUT;
}

static void test_bulk_sizes(void) {
    TEST_ASSERT_EQUAL((int)sizeof(os_llm_acquire_start_request_t), (int)net_stack_bulk_in_size(SYS_LLM_ACQUIRE_START));
    TEST_ASSERT_EQUAL((int)sizeof(os_llm_request_t), (int)net_stack_bulk_in_size(SYS_LLM_REQUEST));
    TEST_ASSERT_EQUAL((int)sizeof(os_llm_openai_credential_request_t), (int)net_stack_bulk_in_size(SYS_LLM_OPENAI_CREDENTIAL));
    TEST_ASSERT_EQUAL(0, (int)net_stack_bulk_in_size(SYS_LLM_POLL_TLS));
    TEST_ASSERT_EQUAL((int)sizeof(os_llm_text_result_t), (int)net_stack_bulk_out_size(SYS_LLM_POLL_TEXT));
    TEST_ASSERT_EQUAL((int)sizeof(os_llm_text_result_t), (int)net_stack_bulk_out_size(SYS_LLM_POLL_SSE));
    TEST_ASSERT_EQUAL(0, (int)net_stack_bulk_out_size(SYS_LLM_REQUEST));
    TEST_ASSERT_TRUE(net_stack_bulk_in_size(SYS_LLM_REQUEST) <= OS_NET_RELAY_BULK_MAX);
    TEST_ASSERT_TRUE(net_stack_bulk_out_size(SYS_LLM_POLL_TEXT) <= OS_NET_RELAY_BULK_MAX);
}

static void test_socket_ops_local_registry(void) {
    net_stack_t st;
    os_net_relay_request_t r;
    uint8_t out[OS_NET_RELAY_MAX_OUT];
    uint16_t n = 99U;
    int32_t id;
    stack_setup(&st, 0);
    request(&r, SYS_SOCKET_OPEN, 40001U, 7U, 1000U);
    id = net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0);
    TEST_ASSERT_TRUE(id >= 0);
    TEST_ASSERT_EQUAL(0, (int)n);
    /* The SYN is framed by the local registry into the reply bytes. */
    request(&r, SYS_SOCKET_SEND, (uint32_t)id, 0U, 0U);
    TEST_ASSERT_TRUE(net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0) != 0 || n > 0U);
    request(&r, SYS_SOCKET_LISTEN, 40002U, 5000U, 0U);
    TEST_ASSERT_TRUE(net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0) >= 0);
    request(&r, SYS_SOCKET_CLOSE, (uint32_t)id, 0U, 0U);
    TEST_ASSERT_EQUAL(0, net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0));
    request(&r, SYS_SOCKET_CLOSE, (uint32_t)id, 0U, 0U);
    TEST_ASSERT_TRUE(net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0) < 0);
    TEST_ASSERT_EQUAL(5, (int)st.report.socket_ops);
    /* Malformed or foreign ops. */
    request(&r, SYS_SOCKET_ACCEPT_SYN, 0U, 0U, 0U);
    r.in_length = 3U;
    TEST_ASSERT_EQUAL(OS_SOCKET_BAD_ARGUMENT, net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0));
    request(&r, SYS_PEER_LISTEN, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL(OS_PEER_BAD_REQUEST, net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0));
    request(&r, SYS_NET_STATUS, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL(OS_SOCKET_BAD_ARGUMENT, net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0));
    TEST_ASSERT_EQUAL(OS_SOCKET_BAD_ARGUMENT, net_stack_exec(&st, 0, 0, 0U, out, &n, 0, 0));
    TEST_ASSERT_EQUAL(6, (int)st.report.socket_ops);
    TEST_ASSERT_EQUAL(0, (int)st.report.wire_ops);
}

static void connect_request(os_net_relay_request_t* r, uint16_t attempts) {
    os_socket_connect_request_t c;
    memset(&c, 0, sizeof(c));
    memcpy(c.local_ip, k_local, 4); memcpy(c.remote_ip, k_remote, 4);
    c.local_port = 40007U; c.remote_port = 7U; c.attempts = attempts;
    request(r, SYS_SOCKET_CONNECT, 0U, 0U, 0U);
    r->in_length = (uint16_t)sizeof(c);
    memcpy(r->in, &c, sizeof(c));
}

static void test_wire_without_nic(void) {
    net_stack_t st;
    os_net_relay_request_t r;
    uint8_t out[OS_NET_RELAY_MAX_OUT];
    uint16_t n;
    stack_setup(&st, 0);
    connect_request(&r, 5U);
    TEST_ASSERT_EQUAL(OS_NET_WIRE_UNAVAILABLE, net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0));
    TEST_ASSERT_EQUAL(0, (int)st.report.frames_built);
}

static void test_wire_connect_framed_in_ring3(void) {
    net_stack_t st;
    os_net_relay_request_t r;
    uint8_t out[OS_NET_RELAY_MAX_OUT];
    uint16_t n;
    int32_t rc;
    stack_setup(&st, 1);
    /* ARP reply from the peer, delivered after our ARP request went out. */
    memset(g_script, 0, 64);
    g_script[12] = 0x08; g_script[13] = 0x06;
    g_script[15] = 1; g_script[16] = 0x08; g_script[18] = 6; g_script[19] = 4; g_script[21] = 2;
    g_script[22] = 0x52; g_script[23] = 0x54; g_script[27] = 2;
    memcpy(g_script + 28, k_remote, 4);
    memcpy(g_script + 38, k_local, 4);
    g_script_len = 42U;
    connect_request(&r, 6U);
    rc = net_stack_exec(&st, &r, 0, 0U, out, &n, 0, 0);
    /* Nobody answers the SYN: timeout, but everything was built/decoded here. */
    TEST_ASSERT_EQUAL(OS_NET_WIRE_TIMEOUT, rc);
    TEST_ASSERT_TRUE(g_sent_count >= 2);
    TEST_ASSERT_EQUAL(0x06, g_sent[0][13]);                 /* ARP request */
    TEST_ASSERT_EQUAL(0x00, g_sent[1][13]);                 /* IPv4 */
    TEST_ASSERT_EQUAL(6, g_sent[1][23]);                    /* TCP */
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN, g_sent[1][47] & 0x3F);
    TEST_ASSERT_EQUAL(0x02, g_sent[1][5]);                  /* learned peer MAC */
    TEST_ASSERT_EQUAL(g_sent_count, (int)st.report.frames_built);
    TEST_ASSERT_EQUAL(1, (int)st.report.frames_parsed);
    TEST_ASSERT_EQUAL(1, (int)st.report.wire_ops);
    TEST_ASSERT_EQUAL(g_rounds, (int)st.report.rounds);
    TEST_ASSERT_EQUAL(g_rounds - 1, g_idles);
    TEST_ASSERT_FALSE(net_wire_op_active());
    TEST_ASSERT_FALSE(net_wire_is_bound(0));
}

static os_llm_acquire_start_request_t g_seen_acquire;
static int g_llm_calls;
static int f_acquire(const os_llm_acquire_start_request_t* r) { g_seen_acquire = *r; g_llm_calls++; return 0; }
static int f_poll_tls(void) { g_llm_calls++; return OS_LLM_TLS_BAD_PHASE; }
static int f_request(const os_llm_request_t* r) { g_llm_calls++; return r->prompt_length == 5U ? 0 : -1; }
static int f_poll_text(os_llm_text_result_t* t) {
    g_llm_calls++; t->status_code = 200U; t->text_length = 2U; t->text[0] = 'o'; t->text[1] = 'k'; return 0;
}
static int f_zero(void) { g_llm_calls++; return 0; }
static int f_cred(const os_llm_openai_credential_request_t* r) { g_llm_calls++; return r->bearer[0] == 's' ? 0 : -1; }
static uint32_t f_status(void) { return 0x2BU; }
static const net_stack_llm_ops_t k_llm = { f_acquire, f_poll_tls, f_request, f_poll_text, f_poll_text,
                                           f_zero, f_zero, f_cred, f_status };

static void test_llm_ops_bulk(void) {
    net_stack_t st;
    os_net_relay_request_t r;
    static os_llm_acquire_start_request_t a;
    static os_llm_request_t q;
    static os_llm_openai_credential_request_t c;
    static uint8_t bulk[OS_NET_RELAY_BULK_MAX];
    uint8_t out[OS_NET_RELAY_MAX_OUT];
    uint16_t n;
    uint32_t bn = 7U;
    os_llm_text_result_t t;
    stack_setup(&st, 0);
    request(&r, SYS_LLM_POLL_TLS, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL(OS_LLM_TLS_UNCONFIGURED, net_stack_exec(&st, &r, 0, 0U, out, &n, bulk, &bn));
    st.llm = &k_llm;
    g_llm_calls = 0;
    memset(&a, 0, sizeof(a));
    memcpy(a.hostname, "example.com", 12);
    request(&r, SYS_LLM_ACQUIRE_START, sizeof(a), 0U, 0U);
    TEST_ASSERT_EQUAL(OS_LLM_REQUEST_BAD_REQUEST, net_stack_exec(&st, &r, (const uint8_t*)&a, 10U, out, &n, bulk, &bn));
    TEST_ASSERT_EQUAL(0, g_llm_calls);
    TEST_ASSERT_EQUAL(0, net_stack_exec(&st, &r, (const uint8_t*)&a, sizeof(a), out, &n, bulk, &bn));
    TEST_ASSERT_EQUAL_STRING("example.com", g_seen_acquire.hostname);
    TEST_ASSERT_EQUAL(0, (int)bn);
    TEST_ASSERT_EQUAL(0x2B, (int)st.report.llm_status);
    request(&r, SYS_LLM_POLL_TLS, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL(OS_LLM_TLS_BAD_PHASE, net_stack_exec(&st, &r, 0, 0U, out, &n, bulk, &bn));
    memset(&q, 0, sizeof(q));
    q.prompt_length = 5U;
    request(&r, SYS_LLM_REQUEST, sizeof(q), 0U, 0U);
    TEST_ASSERT_EQUAL(0, net_stack_exec(&st, &r, (const uint8_t*)&q, sizeof(q), out, &n, bulk, &bn));
    request(&r, SYS_LLM_POLL_TEXT, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL(0, net_stack_exec(&st, &r, 0, 0U, out, &n, bulk, &bn));
    TEST_ASSERT_EQUAL((int)sizeof(t), (int)bn);
    memcpy(&t, bulk, sizeof(t));
    TEST_ASSERT_EQUAL(200, (int)t.status_code);
    TEST_ASSERT_EQUAL(2, (int)t.text_length);
    TEST_ASSERT_EQUAL('k', t.text[1]);
    memset(&c, 0, sizeof(c));
    c.bearer[0] = 's';
    request(&r, SYS_LLM_OPENAI_CREDENTIAL, sizeof(c), 0U, 0U);
    TEST_ASSERT_EQUAL(0, net_stack_exec(&st, &r, (const uint8_t*)&c, sizeof(c), out, &n, bulk, &bn));
    request(&r, SYS_LLM_CLOSE, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL(0, net_stack_exec(&st, &r, 0, 0U, out, &n, bulk, &bn));
    TEST_ASSERT_EQUAL(6, (int)st.report.llm_ops);
    TEST_ASSERT_EQUAL(6, g_llm_calls);
    TEST_ASSERT_EQUAL(0, (int)st.report.socket_ops);
}

int main(void) {
    unity_init();
    RUN_TEST(test_bulk_sizes);
    RUN_TEST(test_socket_ops_local_registry);
    RUN_TEST(test_wire_without_nic);
    RUN_TEST(test_wire_connect_framed_in_ring3);
    RUN_TEST(test_llm_ops_bulk);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
