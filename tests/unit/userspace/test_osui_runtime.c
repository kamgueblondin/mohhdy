/* test_osui_runtime.c - Parite OS-UI Ring 3 (chat, scene, sessions, MCP, FS). */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_runtime.h"
#include "mohhdy_osui_bridge.h"
#include <string.h>
#include <stdio.h>

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
    TEST_ASSERT_EQUAL(0, MOHHDY_OSUI_PYTHON_FACADE);
    TEST_ASSERT_EQUAL(1, MOHHDY_OSUI_STAGE_VGA);
    TEST_ASSERT_EQUAL(1, MOHHDY_OSUI_VGA_DESKTOP);
    TEST_ASSERT_EQUAL(0, MOHHDY_OSUI_DISPLAY_HOST);
    TEST_ASSERT_EQUAL_STRING("gui", MOHHDY_OSUI_GUI_COMMAND);
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

    rc = run_line("chat ai comment fonctionne le noyau ?");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "llm=gpt2_local") != NULL);
    TEST_ASSERT(strstr(g_out, "ai_status=ready") != NULL);
    TEST_ASSERT(strstr(g_out, "response=test local response") != NULL);
    TEST_ASSERT(strstr(g_out, "stage mode=reflecting llm=gpt2_local") != NULL);

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

    rc = run_line("gui-status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "canonical=gui") != NULL);
    TEST_ASSERT(strstr(g_out, "chrome=qemu_fb") != NULL);

    rc = run_line("gui");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "canonical=gui") != NULL);
    TEST_ASSERT(osui_gui_should_enter());
    osui_gui_ack_enter();
    run_line("gui-status");
    TEST_ASSERT(strstr(g_out, "gui_live=true") != NULL);
    rc = run_line("console");
    TEST_ASSERT_EQUAL(0, rc);
    run_line("gui-status");
    TEST_ASSERT(strstr(g_out, "gui_live=false") != NULL);
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

    rc = run_line("origin-check http://evil.com");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);

    rc = run_line("origin-check null");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);

    rc = run_line("origin-check untrusted");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);

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
    TEST_ASSERT(strstr(g_out, "confirm_required token=c0001") != NULL);
    TEST_ASSERT(strstr(g_out, "invoice_id=") == NULL);
    rc = run_line("confirm c0001");
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

    rc = run_line("fs-read demo/hello.txt"); /* no scope granted */
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("grant fs.read:demo/"));
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

static void test_browser_tabs_and_dom_acts(void) {
    int rc;
    setup();
    rc = run_line("browser-tab-new https://mohhdy.local/app");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "tab_id=t0001") != NULL);
    TEST_ASSERT(strstr(g_out, "https://mohhdy.local/app") != NULL);

    rc = run_line("browser-tab-new https://mohhdy.local/docs");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "tab_id=t0002") != NULL);

    rc = run_line("browser-tab-list");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "count=2") != NULL);
    TEST_ASSERT(strstr(g_out, "t0001") != NULL);
    TEST_ASSERT(strstr(g_out, "t0002") != NULL);

    rc = run_line("browser-tab-use t0001");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "tab_id=t0001") != NULL);

    rc = run_line("browser-tab-close t0002");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "tab_id=t0002") != NULL);

    rc = run_line("browser-form-fill Bob 42");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "customer=Bob") != NULL);
    TEST_ASSERT(strstr(g_out, "amount=42") != NULL);

    rc = run_line("browser-form-fill Bob invalid_amount");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "validation_failed") != NULL);

    rc = run_line("browser-dom-act submit");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "form_submitted=true") != NULL);

    rc = run_line("browser-dom-act reset");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "form_submitted=false") != NULL);
}

static void test_browser_fetch_and_storage(void) {
    int rc;
    setup();
    rc = run_line("browser-tab-new https://mohhdy.local/app");
    TEST_ASSERT_EQUAL(0, rc);

    rc = run_line("browser-fetch /api/v1/data origin=self");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=200") != NULL);
    TEST_ASSERT(strstr(g_out, "url=/api/v1/data") != NULL);

    rc = run_line("browser-fetch /api/v1/data origin=http://unauthorized.org");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);

    rc = run_line("browser-storage-set theme dark");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "key=theme") != NULL);

    rc = run_line("browser-storage-get theme");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "value=dark") != NULL);

    rc = run_line("browser-storage-get missing_key");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "not_found") != NULL);
}

static void test_mcp_connectors_extended(void) {
    int rc;
    setup();
    rc = run_line("mcp-list");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "mcp.invoice.create") != NULL);
    TEST_ASSERT(strstr(g_out, "mcp.payment.process") != NULL);

    rc = run_line("mcp-status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "active=true") != NULL);

    rc = run_line("mcp-invoke mcp.payment.process");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "authentication_required") != NULL);

    run_line("mcp-auth mcp.payment.process token1");
    rc = run_line("mcp-invoke mcp.payment.process");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "tool=mcp.payment.process") != NULL);

    run_line("mcp-auth mcp.document.sign token2");
    rc = run_line("mcp-invoke mcp.document.sign");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "tool=mcp.document.sign") != NULL);
}

static void test_osui_model(void) {
    int rc;
    setup();
    rc = run_line("osui-model");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "model=gpt2_124M.bin") != NULL);

    rc = run_line("/model list");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "list=gpt2_124M.bin,gpt2.gguf") != NULL);

    rc = run_line("osui-model use gpt2.gguf");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "model=gpt2.gguf") != NULL);

    rc = run_line("osui-model status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "model=gpt2.gguf") != NULL);
}

static void test_osui_provider(void) {
    int rc;
    setup();
    rc = run_line("osui-provider");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "provider=local") != NULL);

    rc = run_line("/provider openai");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "provider=openai") != NULL);

    rc = run_line("osui-provider status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "provider=openai") != NULL);

    rc = run_line("osui-provider local");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "provider=local") != NULL);
}

static void test_browser_navigation_dom_tree_and_audit(void) {
    int rc;
    setup();
    rc = run_line("browser-tab-new https://mohhdy.local/home");
    TEST_ASSERT_EQUAL(0, rc);

    rc = run_line("/navigate https://mohhdy.local/page1");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "url=https://mohhdy.local/page1") != NULL);

    rc = run_line("/back");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "url=https://mohhdy.local/home") != NULL);

    rc = run_line("browser-forward");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "url=https://mohhdy.local/page1") != NULL);

    rc = run_line("/dom");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "<button id=\"menu-toggle\">") != NULL);

    rc = run_line("browser-eval console.log('hello')");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "sandbox=js_simulator") != NULL);

    rc = run_line("/auth mcp.payment.process token123");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "authenticated=true") != NULL);

    rc = run_line("mcp-credentials");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "bearer=[masked]") != NULL);

    rc = run_line("/audit");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "count=") != NULL);
}


extern unsigned osui_test_now;

/* Roadmap step 3: create, expire, restore, end, clean up. */
static void test_session_lifecycle(void) {
    int rc;
    setup();
    osui_test_now = 100U;
    run_line("session-new demo");
    run_line("chat hello");
    rc = run_line("session-ttl 30");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "ttl=30") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("session-ttl abc"));
    osui_test_now = 120U;
    run_line("session-status");
    TEST_ASSERT(strstr(g_out, "status=open") != NULL);
    rc = run_line("session-restore s0002");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "not_expired") != NULL);
    osui_test_now = 200U; /* idle 80 s > 30 s */
    run_line("session-status");
    TEST_ASSERT(strstr(g_out, "ai_status=expired") != NULL);
    rc = run_line("chat again");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "session_closed") != NULL);
    rc = run_line("session-restore s0002");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=open history=") != NULL);
    TEST_ASSERT(strstr(g_out, "history=0") == NULL); /* history kept */
    TEST_ASSERT_EQUAL(0, run_line("chat back"));
    run_line("session-end");
    rc = run_line("session-restore s0002");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "ended_not_restorable") != NULL);
    run_line("session-new other");
    rc = run_line("session-cleanup");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "freed=2 remaining=1") != NULL);
    rc = run_line("session-use s0002");
    TEST_ASSERT_EQUAL(1, rc);
    run_line("admin-status"); /* traceable */
    TEST_ASSERT(strstr(g_out, "session.expire") != NULL);
    TEST_ASSERT(strstr(g_out, "session.restore") != NULL);
    TEST_ASSERT(strstr(g_out, "session.cleanup") != NULL);
    run_line("session-ttl 0");
    osui_test_now = 0U;
}

/* Mutations need an explicit confirm; refusal, revocation, expiry. */
static void test_mutation_confirm(void) {
    int rc;
    setup();
    osui_test_now = 10U;
    run_line("grant mcp.invoice.create");
    run_line("mcp-invoice bob 5");
    TEST_ASSERT(strstr(g_out, "token=c0001") != NULL);
    rc = run_line("confirm c9999");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "token_unknown") != NULL);
    rc = run_line("deny c0001");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "executed=false") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("confirm c0001")); /* single use */
    /* capability revoked between request and confirm */
    run_line("mcp-invoice bob 6");
    TEST_ASSERT(strstr(g_out, "token=c0002") != NULL);
    run_line("revoke mcp.invoice.create");
    rc = run_line("confirm c0002");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);
    /* confirmed, then revocable (void) and traced */
    run_line("grant mcp.invoice.create");
    run_line("mcp-invoice carol 7");
    TEST_ASSERT_EQUAL(0, run_line("confirm c0003"));
    TEST_ASSERT(strstr(g_out, "invoice_id=i0001") != NULL);
    rc = run_line("mcp-invoice-void i0001");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "voided=true") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("mcp-invoice-void i0042"));
    run_line("admin-status");
    TEST_ASSERT(strstr(g_out, "denied_by_user") != NULL);
    TEST_ASSERT(strstr(g_out, "mcp.invoice.void") != NULL);
    /* pending mutation dropped when the session expires */
    run_line("mcp-invoice dave 8");
    TEST_ASSERT(strstr(g_out, "token=c0004") != NULL);
    run_line("session-ttl 5");
    osui_test_now = 100U;
    rc = run_line("confirm c0004");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "token_unknown") != NULL);
    run_line("session-ttl 0");
    osui_test_now = 0U;
}


extern int osui_test_ai_rc;
extern unsigned osui_test_ai_error;
extern unsigned osui_test_ai_abort;

/* Roadmap step 3: scoped VFS reads, allowlisted agent commands. */
static void test_scopes_and_agent_run(void) {
    int rc;
    setup();
    TEST_ASSERT_EQUAL(1, run_line("grant fs.read:../"));
    TEST_ASSERT_EQUAL(1, run_line("grant fs.read:"));
    run_line("grant fs.read:docs/");
    rc = run_line("fs-read demo/hello.txt"); /* outside the scope */
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);
    rc = run_line("agent-run os-status");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability=agent.run") != NULL);
    run_line("grant agent.run");
    rc = run_line("agent-run os-status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "agent-run ok command=os-status") != NULL);
    TEST_ASSERT(strstr(g_out, "service=mohhdy-os") != NULL);
    rc = run_line("agent-run mcp-invoice eve 9"); /* mutation, not allowlisted */
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "command_not_allowlisted") != NULL);
    rc = run_line("agent-run fs-read demo/hello.txt"); /* scope still applies */
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "capability_denied") != NULL);
    run_line("grant fs.read:demo/");
    rc = run_line("agent-run fs-read demo/hello.txt");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "hello from guest FS sandbox") != NULL);
    run_line("revoke agent.run");
    TEST_ASSERT_EQUAL(1, run_line("agent-run os-status"));
    run_line("admin-status");
    TEST_ASSERT(strstr(g_out, "not_allowlisted") != NULL);
    TEST_ASSERT(strstr(g_out, "scope_denied") != NULL);
}

/* The AI worker disappears mid request: distinct state, session survives. */
static void test_ai_worker_lost(void) {
    int rc;
    setup();
    osui_test_ai_rc = -1;
    osui_test_ai_error = 2U;  /* model failed ... */
    osui_test_ai_abort = 1U;  /* ... because the worker was lost */
    rc = run_line("chat ai bonjour");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "error=ai_worker_lost llm=gpt2_no_worker ai_status=worker_lost") != NULL);
    {
        char row[96];
        osui_canvas_row(1, row, (int)sizeof(row)); /* VGA scene */
        TEST_ASSERT(strstr(row, "etat_ia=worker IA perdu") != NULL);
    }
    osui_test_ai_abort = 0U;
    osui_test_ai_error = 4U;  /* strict build, no worker at all */
    rc = run_line("chat ai encore");
    TEST_ASSERT(strstr(g_out, "ai_status=worker_lost") != NULL);
    osui_test_ai_rc = 0;
    osui_test_ai_error = 0U;
    rc = run_line("chat ai retry"); /* session kept open, retry works */
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "ai_status=ready session_id") != NULL);
    /* Worker stalled, Ring 0 answered: success, but the degradation shows. */
    osui_test_ai_abort = 2U;
    rc = run_line("chat ai degraded");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "ai_status=ready worker=stalled fallback=ring0") != NULL);
    {
        char row[96];
        osui_canvas_row(1, row, (int)sizeof(row)); /* VGA scene */
        TEST_ASSERT(strstr(row, "repli Ring 0") != NULL);
    }
    osui_test_ai_abort = 0U;
}


extern int osui_test_net_rc;
extern unsigned osui_test_net_llm_status;
extern int osui_test_net_worker;
extern int osui_test_peer_acquire_rc, osui_test_peer_tls_polls, osui_test_peer_request_rc, osui_test_peer_text_polls;
extern unsigned osui_test_peer_http;

/* Roadmap step 4: provider peer, explicit and observable local fallback. */
static void test_provider_peer_fallback(void) {
    int rc;
    setup();
    TEST_ASSERT_EQUAL(0, run_line("osui-provider peer"));
    run_line("osui-provider status");
    TEST_ASSERT(strstr(g_out, "provider=peer network=no_net_worker") != NULL);
    TEST_ASSERT(strstr(g_out, "secrets_in_image=false public_internet=false") != NULL);
    rc = run_line("chat ai bonjour");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "ai_status=ready provider=peer fallback=local reason=no_net_worker") != NULL);
    osui_test_net_rc = 0; osui_test_net_worker = 3; osui_test_net_llm_status = 0U;
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=no_nic") != NULL);
    osui_test_net_llm_status = 1U;
    run_line("osui-provider status");
    TEST_ASSERT(strstr(g_out, "network=ready") != NULL);
    /* Each failing step of the peer exchange is its own reason. */
    osui_test_peer_acquire_rc = -1;
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=acquire_failed") != NULL);
    osui_test_peer_acquire_rc = 0; osui_test_peer_tls_polls = -1;
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=tls_failed") != NULL);
    osui_test_peer_tls_polls = 100000;   /* never completes: deadline */
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=tls_timeout") != NULL);
    osui_test_peer_tls_polls = 3; osui_test_peer_request_rc = -1;
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=request_failed") != NULL);
    osui_test_peer_request_rc = 0; osui_test_peer_text_polls = -1;
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=response_failed") != NULL);
    osui_test_peer_text_polls = 100000;
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=response_timeout") != NULL);
    osui_test_peer_text_polls = 2; osui_test_peer_http = 500U;
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "reason=http_error") != NULL);
    osui_test_peer_http = 200U;
    TEST_ASSERT_EQUAL(0, run_line("chat ai x"));
    TEST_ASSERT(strstr(g_out, "osui chat ok llm=peer_qemu ai_status=ready provider=peer") != NULL);
    TEST_ASSERT(strstr(g_out, "response=peer says ok") != NULL);
    TEST_ASSERT(strstr(g_out, "fallback") == NULL);
    /* Second turn reuses the re-armed TLS session (no new acquire). */
    osui_test_peer_acquire_rc = -1;
    TEST_ASSERT_EQUAL(0, run_line("chat ai y"));
    TEST_ASSERT(strstr(g_out, "llm=peer_qemu") != NULL);
    osui_test_peer_request_rc = -1;      /* peer fails, then the fallback fails too */
    osui_test_ai_rc = -1; osui_test_ai_error = 1U; /* still explicit */
    rc = run_line("chat ai x");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "ai_status=no_model provider=peer fallback=local") != NULL);
    TEST_ASSERT(strstr(g_out, "reason=request_failed") != NULL);
    osui_test_ai_rc = 0; osui_test_ai_error = 0U; osui_test_peer_request_rc = 0;
    run_line("osui-provider local");
    run_line("chat ai x");
    TEST_ASSERT(strstr(g_out, "provider=peer") == NULL);
    osui_test_net_rc = -1; osui_test_net_worker = 0; osui_test_net_llm_status = 0U;
}


/* Roadmap step 5: controlled local API (routes, auth, errors, limits). */
static void test_local_api(void) {
    char tok[8];
    const char *t;
    char line[128];
    int i;
    setup();
    TEST_ASSERT_EQUAL(0, run_line("api GET /status"));
    TEST_ASSERT(strstr(g_out, "osui api status=200 route=/status") != NULL);
    TEST_ASSERT(strstr(g_out, "\"phase3_complete\":false,\"browser_engine\":\"none\"") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("api POST /status"));
    TEST_ASSERT(strstr(g_out, "status=405") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("api GET /nope"));
    TEST_ASSERT(strstr(g_out, "status=404") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("api DELETE /status"));
    TEST_ASSERT(strstr(g_out, "status=405") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("api GET /sessions"));
    TEST_ASSERT(strstr(g_out, "status=401") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("api GET /sessions t00000"));
    TEST_ASSERT(strstr(g_out, "status=401") != NULL);
    TEST_ASSERT_EQUAL(1, run_line("api-token"));
    TEST_ASSERT(strstr(g_out, "capability_denied capability=web.api") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("grant web.api"));
    TEST_ASSERT_EQUAL(0, run_line("api-token"));
    t = strstr(g_out, "token=");
    TEST_ASSERT(t != NULL);
    memcpy(tok, t + 6, 6); tok[6] = 0;
    TEST_ASSERT_EQUAL('t', tok[0]);
    snprintf(line, sizeof(line), "api GET /sessions %s", tok);
    TEST_ASSERT_EQUAL(0, run_line(line));
    TEST_ASSERT(strstr(g_out, "own_session_only") != NULL);
    /* Web supervision needs admin.observe. */
    snprintf(line, sizeof(line), "api GET /supervision %s", tok);
    TEST_ASSERT_EQUAL(1, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=403") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("grant admin.observe"));
    TEST_ASSERT_EQUAL(0, run_line(line));
    TEST_ASSERT(strstr(g_out, "\"ai_worker\":\"absent\",\"ai_last_abort\":\"none\",\"net_worker\":\"absent\"") != NULL);
    TEST_ASSERT(strstr(g_out, "web.api:/supervision:capability_denied") != NULL);
    osui_test_now += 11U;   /* new rate window for the rest of the test */
    /* VFS: same scope rules as the console. */
    snprintf(line, sizeof(line), "api GET /vfs/demo/hello.txt %s", tok);
    TEST_ASSERT_EQUAL(1, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=403") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("grant fs.read:demo/"));
    TEST_ASSERT_EQUAL(0, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=200 route=/vfs/demo/hello.txt") != NULL);
    TEST_ASSERT(strstr(g_out, "hello from guest FS sandbox") != NULL);
    snprintf(line, sizeof(line), "api GET /vfs/demo/../x %s", tok);
    TEST_ASSERT_EQUAL(1, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=400") != NULL);
    /* AI console over the API: same chat path, same capability. */
    snprintf(line, sizeof(line), "api POST /ai/chat %s bonjour", tok);
    TEST_ASSERT_EQUAL(0, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=202") != NULL);
    TEST_ASSERT(strstr(g_out, "osui chat ok llm=gpt2_local ai_status=ready") != NULL);
    snprintf(line, sizeof(line), "api POST /ai/chat %s %s", tok,
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    TEST_ASSERT_EQUAL(1, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=413") != NULL);
    /* Rate limit: 8 per 10 s per token (5 used in this window). */
    snprintf(line, sizeof(line), "api GET /sessions %s", tok);
    TEST_ASSERT_EQUAL(0, run_line(line));
    TEST_ASSERT_EQUAL(0, run_line(line));
    TEST_ASSERT_EQUAL(0, run_line(line));
    TEST_ASSERT_EQUAL(1, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=429") != NULL);
    osui_test_now += 11U;
    TEST_ASSERT_EQUAL(0, run_line(line));
    /* Revocation of the capability, then of the token. */
    TEST_ASSERT_EQUAL(0, run_line("revoke web.api"));
    TEST_ASSERT_EQUAL(1, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=403") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("api-revoke"));
    TEST_ASSERT_EQUAL(1, run_line(line));
    TEST_ASSERT(strstr(g_out, "status=401") != NULL);
    TEST_ASSERT(strstr(g_out, "hello") == NULL);
    (void)i;
}


extern const char *osui_test_web_req[4];
extern int osui_test_web_open_rc;
extern int osui_test_web_open_tls;
extern char osui_test_web_out[4][2048];
extern int osui_test_web_closed[4];
extern void osui_test_web_rewind(void);
extern int osui_web_active(void);
extern int osui_web_poll(char *out, int max);

/* Roadmap step 5: HTTP(S) front of the local API, several connections in
 * one poll, blocking and background modes. */
static void test_web_serve(void) {
    char tok[8], req[160], log[2048];
    const char *t;
    int i;
    setup();
    osui_test_web_rewind();
    osui_test_web_req[0] = "GET /status HTTP/1.0\r\nHost: g\r\n\r\n";
    osui_test_web_req[1] = "GET / HTTP/1.0\r\n\r\n";
    osui_test_web_req[2] = "GET /sessions HTTP/1.0\r\n\r\n";
    osui_test_web_req[3] = 0;
    TEST_ASSERT_EQUAL(0, run_line("web-serve 8080 3"));
    TEST_ASSERT(strstr(g_out, "listening port=8080 scheme=http slots=4 mode=blocking") != NULL);
    TEST_ASSERT(strstr(g_out, "osui web-serve ok served=3 failed=0") != NULL);
    TEST_ASSERT(strstr(g_out, "slot=0 scheme=http method=GET path=/status status=200 sent=ok") != NULL);
    TEST_ASSERT(strstr(g_out, "slot=2 scheme=http method=GET path=/sessions status=401 sent=ok") != NULL);
    TEST_ASSERT(strstr(osui_test_web_out[0], "HTTP/1.0 200 OK\r\n") != NULL);
    TEST_ASSERT(strstr(osui_test_web_out[0], "\"phase3_complete\":false") != NULL);
    TEST_ASSERT(strstr(osui_test_web_out[1], "MOHHDY local console") != NULL);
    TEST_ASSERT(strstr(osui_test_web_out[2], "HTTP/1.0 401 Error") != NULL);
    for (i = 0; i < 3; i++) TEST_ASSERT_EQUAL(1, osui_test_web_closed[i]);
    /* Bearer token over TLS: same session, scopes and capabilities. */
    TEST_ASSERT_EQUAL(0, run_line("grant web.api"));
    TEST_ASSERT_EQUAL(0, run_line("api-token"));
    t = strstr(g_out, "token=");
    memcpy(tok, t + 6, 6); tok[6] = 0;
    snprintf(req, sizeof(req), "POST /ai/chat HTTP/1.0\r\nAuthorization: Bearer %s\r\nContent-Length: 7\r\n\r\nbonjour", tok);
    osui_test_web_rewind();
    osui_test_web_req[0] = req; osui_test_web_req[1] = "garbage"; osui_test_web_req[2] = 0;
    TEST_ASSERT_EQUAL(0, run_line("web-serve 8443 2 tls"));
    TEST_ASSERT_EQUAL(1, osui_test_web_open_tls);
    TEST_ASSERT(strstr(g_out, "scheme=https method=POST path=/ai/chat status=202") != NULL);
    TEST_ASSERT(strstr(osui_test_web_out[0], "osui chat ok llm=gpt2_local ai_status=ready") != NULL);
    TEST_ASSERT(strstr(osui_test_web_out[1], "HTTP/1.0 400 Error") != NULL);
    /* Background: the command returns at once, polls serve later. */
    osui_test_web_rewind();
    osui_test_web_req[0] = 0; osui_test_web_req[1] = 0;
    TEST_ASSERT_EQUAL(0, run_line("web-serve start 8080"));
    TEST_ASSERT(strstr(g_out, "mode=background") != NULL);
    TEST_ASSERT_EQUAL(1, osui_web_active());
    TEST_ASSERT_EQUAL(1, run_line("web-serve start 8081"));
    TEST_ASSERT(strstr(g_out, "error=already_running") != NULL);
    TEST_ASSERT_EQUAL(0, osui_web_poll(log, sizeof(log)));
    TEST_ASSERT_EQUAL(0, run_line("status"));
    osui_test_web_req[3] = "GET /status HTTP/1.0\r\n\r\n";
    TEST_ASSERT(osui_web_poll(log, sizeof(log)) > 0);
    TEST_ASSERT(strstr(log, "slot=3 scheme=http method=GET path=/status status=200 sent=ok") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-serve status"));
    TEST_ASSERT(strstr(g_out, "status=running port=8080 scheme=http served=1") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-serve stop"));
    TEST_ASSERT(strstr(g_out, "osui web-serve ok served=1 failed=0") != NULL);
    TEST_ASSERT_EQUAL(0, osui_web_active());
    TEST_ASSERT_EQUAL(1, run_line("web-serve stop"));
    osui_test_web_req[3] = 0;
    TEST_ASSERT_EQUAL(1, run_line("web-serve 0"));
    osui_test_web_open_rc = -5;
    TEST_ASSERT_EQUAL(1, run_line("web-serve 8080 1"));
    TEST_ASSERT(strstr(g_out, "error=listen_failed") != NULL);
    osui_test_web_open_rc = 0;
}

static void test_browser_tab_extensions_and_fs_web_persistence(void) {
    setup();
    TEST_ASSERT_EQUAL(0, run_line("browser-tab-new about:blank"));
    TEST_ASSERT_EQUAL(0, run_line("browser-tab-new http://example.com/app app"));
    TEST_ASSERT(strstr(g_out, "kind=app") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-tab-rename MyCustomTitle"));
    TEST_ASSERT(strstr(g_out, "title=MyCustomTitle") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-tab-pin 1"));
    TEST_ASSERT(strstr(g_out, "pinned=true") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-storage-set token secret123"));
    TEST_ASSERT(strstr(g_out, "key=token") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-tab-duplicate"));
    TEST_ASSERT(strstr(g_out, "browser-tab-duplicate ok") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-tabs"));
    TEST_ASSERT(strstr(g_out, "count=3") != NULL);
    TEST_ASSERT(strstr(g_out, "pinned") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-tab-next"));
    TEST_ASSERT(strstr(g_out, "browser-tab-next ok") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-fs-save /web_test.json"));
    TEST_ASSERT(strstr(g_out, "browser-fs-save ok") != NULL);
    TEST_ASSERT(strstr(g_out, "encrypted=true") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-fs-load /web_test.json"));
    TEST_ASSERT(strstr(g_out, "browser-fs-load ok") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("browser-fs-sync"));
    TEST_ASSERT(strstr(g_out, "browser-fs-save ok") != NULL);

    /* Test PWA isolated origin execution & storage isolation */
    TEST_ASSERT_EQUAL(0, run_line("pwa-run pwa0001"));
    TEST_ASSERT(strstr(g_out, "pwa_run_id=pwa0001") != NULL);

    run_line("browser-storage-set secret_key secret_val origin=http://evil");
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);
}

static void test_agentic_workflows_p2p_collab_prod(void) {
    setup();
    TEST_ASSERT_EQUAL(0, run_line("chat p2p status"));
    TEST_ASSERT(strstr(g_out, "workflow=p2p") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("chat collab points"));
    TEST_ASSERT(strstr(g_out, "workflow=collab") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("chat prod metrics"));
    TEST_ASSERT(strstr(g_out, "workflow=prod") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("chat browser tabs"));
    TEST_ASSERT(strstr(g_out, "workflow=web") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("os-p2p"));
    TEST_ASSERT(strstr(g_out, "pane=p2p") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("os-collab"));
    TEST_ASSERT(strstr(g_out, "pane=collab") != NULL);

    TEST_ASSERT_EQUAL(0, run_line("os-prod"));
    TEST_ASSERT(strstr(g_out, "pane=prod") != NULL);
}

static void test_browser_pm_run_and_navigation(void) {
    setup();
    int rc = run_line("browser-pm-run print \"hello_mohhdy\"");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "browser-pm-run ok") != NULL);
    TEST_ASSERT(strstr(g_out, "output=") != NULL);

    rc = run_line("browser-tab-new about:blank");
    TEST_ASSERT_EQUAL(0, rc);

    rc = run_line("browser-navigate pm://script.pm");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "kind=pm") != NULL);

    rc = run_line("chat pm run script.pm");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "workflow=pm") != NULL);
}

static void test_multi_origin_security_and_p2p_socket_isolation(void) {
    setup();
    int rc = run_line("browser-origin-status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "isolation=active") != NULL);
    TEST_ASSERT(strstr(g_out, "allowed=") != NULL);

    rc = run_line("browser-fetch p2p://node/data origin=untrusted");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);
    TEST_ASSERT(strstr(g_out, "status=403") != NULL);

    rc = run_line("browser-tab-new http://127.0.0.1/app");
    TEST_ASSERT_EQUAL(0, rc);

    rc = run_line("browser-storage-set key val origin=untrusted");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);

    rc = run_line("browser-storage-get key origin=untrusted");
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT(strstr(g_out, "origin_denied") != NULL);
}

static void test_telemetry_pwa_and_media(void) {
    int rc;
    setup();

    /* Telemetry initial status */
    rc = run_line("browser-telemetry status");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=active") != NULL);
    TEST_ASSERT(strstr(g_out, "commands=") != NULL);

    /* Telemetry export */
    rc = run_line("browser-telemetry export");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "json={\"commands\":") != NULL);

    /* Telemetry reset */
    rc = run_line("browser-telemetry reset");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "browser-telemetry reset ok") != NULL);

    /* PWA Manifest */
    rc = run_line("pwa-manifest TestApp app://test/start app://test/ standalone");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "pwa-manifest ok") != NULL);
    TEST_ASSERT(strstr(g_out, "name=TestApp") != NULL);

    /* PWA List */
    rc = run_line("pwa-list");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "TestApp") != NULL);

    /* PWA Install */
    rc = run_line("pwa-install TestApp");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=installed") != NULL);

    /* PWA Run */
    rc = run_line("pwa-run TestApp");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "pwa_name=TestApp") != NULL);

    /* Media Stream Play */
    rc = run_line("media-play audio track1");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=playing") != NULL);
    TEST_ASSERT_EQUAL(1, osui_get_media_active());
    TEST_ASSERT_EQUAL(0, osui_get_media_paused());

    /* Media Status */
    rc = run_line("browser-media");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "playing=true") != NULL);

    /* Media Pause */
    rc = run_line("media-pause");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=paused") != NULL);
    TEST_ASSERT_EQUAL(1, osui_get_media_paused());

    /* Media Stream */
    rc = run_line("media-stream http://127.0.0.1/live.mp3 video");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=streaming") != NULL);

    /* Media Stop */
    rc = run_line("media-stop");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "status=stopped") != NULL);
    TEST_ASSERT_EQUAL(0, osui_get_media_active());
}

void test_html_css_rendering(void) {
    int rc;
    setup();
    rc = osui_dispatch_line("browser-html-render", g_out, sizeof(g_out));
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "MOHHDY Web Runtime") != NULL);
    TEST_ASSERT(strstr(g_out, "browser-html-render ok") != NULL);
    TEST_ASSERT(strstr(g_out, "===") == NULL);
    /* Flat text printer: tags, one id rule (margin), not a CSS engine. */
    rc = run_line("browser-html-render <h1 id=\"app\">Title</h1><p>Hello</p><a class=\"x\">docs</a><li>item</li><button>Go</button>");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "=== Title ===") != NULL);
    TEST_ASSERT(strstr(g_out, "Hello") != NULL);
    TEST_ASSERT(strstr(g_out, "[link] docs") != NULL);
    TEST_ASSERT(strstr(g_out, "* item") != NULL);
    TEST_ASSERT(strstr(g_out, "[ Go ]") != NULL);
    TEST_ASSERT(strstr(g_out, "margin=8") != NULL);
    rc = run_line("browser-html-render");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "=== Title ===") == NULL);
    TEST_ASSERT(strstr(g_out, "margin=8") == NULL);
}

void test_css_cascade_engine(void) {
    int rc;
    setup();
    /* Inline <style>: class beats type, id beats class, later rule wins. */
    rc = run_line("browser-html-render <style>p{color:green}.n{color:orange}#a{color:red}</style><p id=\"a\" class=\"n\">x</p><p class=\"n\">y</p><p>z</p>");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "  x {color:red}") != NULL);
    TEST_ASSERT(strstr(g_out, "  y {color:orange}") != NULL);
    TEST_ASSERT(strstr(g_out, "  z {color:green}") != NULL);
    TEST_ASSERT(strstr(g_out, "css author_rules=3") != NULL);
    /* display:none hides the node and its descendants; inline style wins. */
    rc = run_line("browser-html-render <style>.h{display:none}</style><div class=\"h\"><p>gone</p></div><p style=\"color:teal\">kept</p>");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "gone") == NULL);
    TEST_ASSERT(strstr(g_out, "  kept {color:teal}") != NULL);
    TEST_ASSERT(strstr(g_out, "hidden=2") != NULL);
    /* Descendant selector matches only inside the ancestor; inheritance. */
    rc = run_line("browser-html-render <style>div p{font-weight:bold}div{color:navy}</style><div><p>in</p></div><p>out</p>");
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT(strstr(g_out, "  in {color:navy; fw:700}") != NULL);
    TEST_ASSERT(strstr(g_out, "  out\n") != NULL);
}

/* Lot 5 Phase 3 (US-038 to US-044, partial): webrt blocks in the guest. */
void test_web_runtime_blocks_in_guest(void) {
    setup();
    TEST_ASSERT_EQUAL(0, run_line("web-cache"));
    TEST_ASSERT(strstr(g_out, "web-cache ok origin=simule verdicts=fresh,stale,fresh,miss hits=2 misses=2 stale=1 revalidated=1 evictions=1 refused=1") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-csp"));
    TEST_ASSERT(strstr(g_out, "script-src https://evil.example/x.js block") != NULL);
    TEST_ASSERT(strstr(g_out, "style-src inline allow") != NULL);
    TEST_ASSERT(strstr(g_out, "web-csp ok directives=4 allowed=5 blocked=3") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-csp script-src inline"));
    TEST_ASSERT(strstr(g_out, "verdict=block") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-sanitize"));
    TEST_ASSERT(strstr(g_out, "web-sanitize ok removed=5 active_left=0") != NULL);
    TEST_ASSERT(strstr(g_out, "steal") == NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-responsive 360 640"));
    TEST_ASSERT(strstr(g_out, "=== Titre === {color:red; fw:700}") != NULL);
    TEST_ASSERT(strstr(g_out, "menu") == NULL);
    TEST_ASSERT(strstr(g_out, "web-responsive ok width=360 height=640 kept=1 dropped=2") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-responsive 1280"));
    TEST_ASSERT(strstr(g_out, "  aside {color:blue}") != NULL);
    TEST_ASSERT(strstr(g_out, "  menu {color:green}") != NULL);
    TEST_ASSERT(strstr(g_out, "kept=1 dropped=2") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-theme dark"));
    TEST_ASSERT(strstr(g_out, "web-theme ok scheme=dark vars=2 unresolved=0 h1_color=white") != NULL);
    TEST_ASSERT(strstr(g_out, "  texte {color:teal}") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-theme light"));
    TEST_ASSERT(strstr(g_out, "h1_color=navy") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-components"));
    TEST_ASSERT(strstr(g_out, "=== Bonjour === {color:purple; fw:700}") != NULL);
    TEST_ASSERT(strstr(g_out, "=== Salut === {color:purple; fw:700}") != NULL);
    TEST_ASSERT(strstr(g_out, "web-components ok defined=1 expanded=2") != NULL);
    TEST_ASSERT_EQUAL(0, run_line("web-a11y"));
    TEST_ASSERT(strstr(g_out, "web-a11y ok images=2 img_no_alt=1 inputs=2 input_no_label=1 buttons=2 button_no_text=1 heading_skips=1 lang_missing=1 issues=5") != NULL);
}

void test_css_parser_rules_and_errors(void) {
    int before;
    setup();
    TEST_ASSERT_EQUAL(0, run_line("browser-css-demo"));
    TEST_ASSERT(strstr(g_out, "browser-css-demo ok") != NULL);
    TEST_ASSERT(strstr(g_out, "=== CSS demo === {fw:700}") != NULL);
    TEST_ASSERT(strstr(g_out, "  lead {color:red; fw:700}") != NULL);
    TEST_ASSERT(strstr(g_out, "  note {color:orange; fw:700}") != NULL);
    TEST_ASSERT(strstr(g_out, "secret") == NULL);
    /* !important author rule beats the inline style; uppercase inherits. */
    TEST_ASSERT(strstr(g_out, "  * FIRST {color:navy}") != NULL);
    TEST_ASSERT(strstr(g_out, "  * SECOND\n") != NULL);
    TEST_ASSERT(strstr(g_out, "  inline {color:purple}") != NULL);
    TEST_ASSERT(strstr(g_out, "author_rules=7 total_rules=19 errors=0 hidden=1") != NULL);
    TEST_ASSERT(strstr(g_out, "errors=0") != NULL);
    /* Direct parser: comma groups, comments, malformed input counted. */
    before = osui_css_rule_count();
    TEST_ASSERT_EQUAL(4, osui_css_parse("/* c */ h1, h2 , .x { color: red; margin: 4px } p{padding:2px}"));
    TEST_ASSERT_EQUAL(before + 4, osui_css_rule_count());
    before = osui_css_error_count();
    TEST_ASSERT_EQUAL(0, osui_css_parse("p..x{color:red} @media x{p{color:red}} q{nocolon}"));
    TEST_ASSERT(osui_css_error_count() >= before + 3);
    /* CSS Syntax: a block left open at end of input is closed there. */
    TEST_ASSERT_EQUAL(1, osui_css_parse("p{color:red"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_bridge_flags);
    RUN_TEST(test_css_cascade_engine);
    RUN_TEST(test_web_runtime_blocks_in_guest);
    RUN_TEST(test_css_parser_rules_and_errors);
    RUN_TEST(test_slash_help_and_linux_trap);
    RUN_TEST(test_prompt_chat_and_stage);
    RUN_TEST(test_sessions_isolated);
    RUN_TEST(test_session_lifecycle);
    RUN_TEST(test_mutation_confirm);
    RUN_TEST(test_scopes_and_agent_run);
    RUN_TEST(test_ai_worker_lost);
    RUN_TEST(test_provider_peer_fallback);
    RUN_TEST(test_local_api);
    RUN_TEST(test_web_serve);
    RUN_TEST(test_grant_revoke_chat);
    RUN_TEST(test_escalate_takeover);
    RUN_TEST(test_origin_denied);
    RUN_TEST(test_browser_allowlist);
    RUN_TEST(test_mcp_invoice_and_undeclared);
    RUN_TEST(test_fs_sandbox);
    RUN_TEST(test_guest_status_honesty);
    RUN_TEST(test_browser_tabs_and_dom_acts);
    RUN_TEST(test_browser_fetch_and_storage);
    RUN_TEST(test_mcp_connectors_extended);
    RUN_TEST(test_osui_model);
    RUN_TEST(test_osui_provider);
    RUN_TEST(test_browser_navigation_dom_tree_and_audit);
    RUN_TEST(test_browser_tab_extensions_and_fs_web_persistence);
    RUN_TEST(test_agentic_workflows_p2p_collab_prod);
    RUN_TEST(test_browser_pm_run_and_navigation);
    RUN_TEST(test_multi_origin_security_and_p2p_socket_isolation);
    RUN_TEST(test_telemetry_pwa_and_media);
    RUN_TEST(test_html_css_rendering);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
