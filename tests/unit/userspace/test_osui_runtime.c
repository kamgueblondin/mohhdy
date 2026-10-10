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

int main(void) {
    unity_init();
    RUN_TEST(test_bridge_flags);
    RUN_TEST(test_slash_help_and_linux_trap);
    RUN_TEST(test_prompt_chat_and_stage);
    RUN_TEST(test_sessions_isolated);
    RUN_TEST(test_session_lifecycle);
    RUN_TEST(test_mutation_confirm);
    RUN_TEST(test_scopes_and_agent_run);
    RUN_TEST(test_ai_worker_lost);
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
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
