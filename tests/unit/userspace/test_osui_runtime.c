/* test_osui_runtime.c - Parite OS-UI Ring 3 (chat, scene, sessions, MCP, FS). */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_runtime.h"
#include "mohhdy_osui_bridge.h"
#include <string.h>

static char g_out[OSUI_OUT_MAX];

static int run_line(const char *line) {
    memset(g_out, 0, sizeof(g_out));
    return osui_dispatch_line(line, g_out, (int)sizeof(g_out));
}

static void setup(void) {
    osui_runtime_init();
}

static void test_bridge_flags(void) {
    TEST_ASSERT_EQUAL(0, MOHHDY_OSUI_GUEST_HTML_STAGE);
    TEST_ASSERT(MOHHDY_SHELL_COMMAND_COUNT > 100);
    TEST_ASSERT(osui_guest_command_count() == MOHHDY_SHELL_COMMAND_COUNT);
}

static void test_slash_help_and_linux_trap(void) {
    int rc;
    setup();
    rc = run_line("/help");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "llm=stub_echo") != NULL);
    TEST_ASSERT(strstr(g_out, "Pas un bash Linux") != NULL);
    TEST_ASSERT(strstr(g_out, "phase3_complete=false") != NULL);

    rc = run_line("apt install nginx");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "Pas un bash Linux") != NULL);
    TEST_ASSERT(osui_is_linux_trap("sudo"));
    TEST_ASSERT(!osui_is_linux_trap("ls"));
}

static void test_prompt_chat_and_stage(void) {
    int rc;
    setup();
    rc = run_line("chat bonjour");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "llm=stub_echo") != NULL);
    TEST_ASSERT(strstr(g_out, "session_id=s0001") != NULL);
    TEST_ASSERT(strstr(g_out, "request_id=") != NULL);
    TEST_ASSERT(strstr(g_out, "mode=reflecting") != NULL);
    TEST_ASSERT(strstr(g_out, "guest_html_stage=false") != NULL);

    rc = run_line("stage-prompt dessine trois boites");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "mode=presenting") != NULL);
    TEST_ASSERT(strstr(g_out, "[A] [B] [C]") != NULL);

    rc = run_line("/plan");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "mode=acting") != NULL);

    rc = run_line("prompt ouvre le shell");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "pane=shell") != NULL);
    TEST_ASSERT(strstr(g_out, "chat_mode=float") != NULL);

    rc = run_line("/center");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "chat_mode=center") != NULL);
}

static void test_sessions_isolated(void) {
    int rc;
    setup();
    rc = run_line("session-new site-a");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "session_id=s0002") != NULL);
    run_line("chat alpha-only");
    TEST_ASSERT(strstr(g_out, "session_id=s0002") != NULL);

    rc = run_line("session-new site-b");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "session_id=s0003") != NULL);
    run_line("chat beta-only");
    run_line("session-status");
    TEST_ASSERT(strstr(g_out, "beta-only") != NULL);
    TEST_ASSERT(strstr(g_out, "alpha-only") == NULL);

    run_line("session-use s0002");
    run_line("session-status");
    TEST_ASSERT(strstr(g_out, "alpha-only") != NULL);
    TEST_ASSERT(strstr(g_out, "beta-only") == NULL);
}

static void test_grant_revoke_chat(void) {
    int rc;
    setup();
    rc = run_line("revoke chat.reply");
    TEST_ASSERT_EQUAL(0, rc);
    rc = run_line("chat hello");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);
    TEST_ASSERT(strstr(g_out, "chat.reply") != NULL);
    TEST_ASSERT(strstr(g_out, "request_id=") != NULL);

    run_line("grant chat.reply");
    rc = run_line("chat hello");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "llm=stub_echo") != NULL);
}

static void test_escalate_takeover(void) {
    int rc;
    setup();
    rc = run_line("escalate besoin humain");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "waiting_human") != NULL);

    rc = run_line("takeover");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);
    TEST_ASSERT(strstr(g_out, "admin.takeover") != NULL);

    run_line("grant admin.takeover");
    rc = run_line("takeover");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "human_active") != NULL);
    TEST_ASSERT(strstr(g_out, "handoff=true") != NULL);

    rc = run_line("chat encore");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "auto_reply=false") != NULL);
}

static void test_origin_denied(void) {
    int rc;
    setup();
    rc = run_line("origin-check evil.example");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);
    TEST_ASSERT(strstr(g_out, "request_id=") != NULL);
    TEST_ASSERT(strstr(g_out, "status=403") != NULL);

    rc = run_line("origin-check self");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "origin-check ok") != NULL);
}

static void test_browser_allowlist(void) {
    int rc;
    setup();
    rc = run_line("browser-click menu-toggle");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);

    run_line("grant dom.click");
    rc = run_line("browser-click menu-toggle");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "harness=dom_simulator") != NULL);
    TEST_ASSERT(strstr(g_out, "menu_open=true") != NULL);

    rc = run_line("browser-click #not-allowed");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "selecteur non allowliste") != NULL);

    rc = run_line("browser-click menu-toggle origin=http://evil.example");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);
}

static void test_mcp_invoice_and_undeclared(void) {
    int rc;
    setup();
    rc = run_line("mcp-invoice alice 10");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);

    run_line("grant mcp.invoice.create");
    rc = run_line("mcp-invoice alice 10");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "invoice_id=i0001") != NULL);
    TEST_ASSERT(strstr(g_out, "session_id=s0001") != NULL);

    rc = run_line("mcp-invoke mcp.shell.exec");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "tool_undeclared") != NULL);
    TEST_ASSERT(strstr(g_out, "status=403") != NULL);
}

static void test_fs_sandbox(void) {
    int rc;
    setup();
    rc = run_line("fs-list demo/");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "demo/hello.txt") != NULL);
    TEST_ASSERT(strstr(g_out, "write=false") != NULL);

    rc = run_line("fs-read demo/hello.txt");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "hello from guest FS sandbox") != NULL);

    rc = run_line("fs-read ../secret");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "traversal_denied") != NULL);

    rc = run_line("fs-write demo/hello.txt pwn");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "write_denied") != NULL);
}

static void test_guest_status_honesty(void) {
    int rc;
    setup();
    rc = run_line("guest-status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "live_guest=true") != NULL);
    TEST_ASSERT(strstr(g_out, "python_facade=false") != NULL);
    TEST_ASSERT(strstr(g_out, "guest_html_stage=false") != NULL);
    TEST_ASSERT(strstr(g_out, "prompt=MOHHDY>") != NULL);

    rc = run_line("os-status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "service=mohhdy-os") != NULL);
    TEST_ASSERT(strstr(g_out, "us031_complete=false") != NULL);

    rc = run_line("stage-prompt <script>alert(1)</script> dessine");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "scripts_stripped=1") != NULL);
}

int main(void) {
    unity_init();
    RUN_TEST(test_bridge_flags);
    RUN_TEST(test_slash_help_and_linux_trap);
    RUN_TEST(test_prompt_chat_and_stage);
    RUN_TEST(test_sessions_isolated);
    RUN_TEST(test_grant_revoke_chat);
    RUN_TEST(test_escalate_takeover);
    RUN_TEST(test_origin_denied);
    RUN_TEST(test_browser_allowlist);
    RUN_TEST(test_mcp_invoice_and_undeclared);
    RUN_TEST(test_fs_sandbox);
    RUN_TEST(test_guest_status_honesty);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
