/* Lot 7 (US-108): local CI/CD pipeline for PromptMessage. See cicd.h. */
#include "cicd.h"

static pm_program_t g_prog, g_test, g_check;
static pm_vm_t g_vm;
static unsigned char g_img[PM_IMAGE_MAX], g_back[PM_IMAGE_MAX];

static int c_len(const char* s) { int n = 0; while (s && s[n]) n++; return n; }
static void c_cat(char* d, const char* s, int cap) { int n = c_len(d), i = 0; while (s && s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void c_num(char* d, unsigned int v, int cap) {
    char t[11], r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; c_cat(d, r, cap);
}
static void c_copy(char* d, const char* s, int cap) { d[0] = 0; c_cat(d, s, cap); }

static const char* k_names[] = {"", "lint", "build", "test", "deploy", "verify"};
const char* cicd_stage_name(int stage) { return stage >= 1 && stage <= 5 ? k_names[stage] : "done"; }

static void stage_line(char* out, int cap, int n, int ok, const char* detail) {
    c_cat(out, "cicd stage ", cap); c_num(out, (unsigned)n, cap); c_cat(out, "/5 ", cap);
    c_cat(out, k_names[n], cap); c_cat(out, ok ? " ok " : " FAILED ", cap);
    c_cat(out, detail, cap); c_cat(out, "\n", cap);
}

static int fail(cicd_result_t* r, char* out, int cap, int n, const char* why) {
    int k;
    r->failed_stage = n;
    r->stage = n;
    c_copy(r->error, why, (int)sizeof(r->error));
    stage_line(out, cap, n, 0, why);
    for (k = n + 1; k <= 5; k++) {
        c_cat(out, "cicd stage ", cap); c_num(out, (unsigned)k, cap); c_cat(out, "/5 ", cap);
        c_cat(out, k_names[k], cap); c_cat(out, " skipped\n", cap);
    }
    c_cat(out, "cicd pipeline status failed stage ", cap); c_cat(out, k_names[n], cap);
    c_cat(out, " passed ", cap); c_num(out, (unsigned)r->passed, cap); c_cat(out, "/5\n", cap);
    return -n;
}

int cicd_run(const char* src, int len, const char* test_src, int test_len, const pm_host_t* host,
             const char* deploy_path, cicd_result_t* r, char* out, int cap) {
    char d[96];
    int n, m, i, st;
    unsigned char* p = (unsigned char*)r;
    for (i = 0; i < (int)sizeof(*r); i++) p[i] = 0;
    out[0] = 0;
    /* 1 lint */
    r->stage = 1;
    if (pm_compile(src, len, &g_prog, host) != PM_OK) {
        d[0] = 0; c_cat(d, "line ", 96); c_num(d, (unsigned)(g_prog.err_line > 0 ? g_prog.err_line : 0), 96);
        c_cat(d, " ", 96); c_cat(d, g_prog.err, 96);
        return fail(r, out, cap, 1, d);
    }
    r->statements = g_prog.statements;
    d[0] = 0; c_cat(d, "statements ", 96); c_num(d, g_prog.statements, 96);
    c_cat(d, " triggers ", 96); c_num(d, g_prog.ntrig, 96);
    stage_line(out, cap, 1, 1, d); r->passed++;
    /* 2 build */
    r->stage = 2;
    n = pm_image_write(&g_prog, g_img, PM_IMAGE_MAX);
    if (n <= 0) return fail(r, out, cap, 2, "image write failed");
    if (pm_image_read(g_img, n, &g_check) != PM_OK || g_check.code_len != g_prog.code_len)
        return fail(r, out, cap, 2, "image does not load back");
    r->image_bytes = n;
    d[0] = 0; c_cat(d, "image_bytes ", 96); c_num(d, (unsigned)n, 96);
    c_cat(d, " bytecode ", 96); c_num(d, g_prog.code_len, 96);
    stage_line(out, cap, 2, 1, d); r->passed++;
    /* 3 test */
    r->stage = 3;
    if (!test_src || test_len <= 0) return fail(r, out, cap, 3, "no test file");
    if (pm_compile(test_src, test_len, &g_test, host) != PM_OK) return fail(r, out, cap, 3, "test file does not compile");
    /* the program runs first (it creates what the tests check) */
    pm_vm_reset(&g_vm);
    st = pm_run(&g_prog, &g_vm, host);
    if (st != PM_OK) { d[0] = 0; c_cat(d, "program error ", 96); c_cat(d, g_vm.err, 96); return fail(r, out, cap, 3, d); }
    pm_vm_reset(&g_vm);
    st = pm_run(&g_test, &g_vm, host);
    r->expects = g_vm.expects; r->expect_failed = g_vm.expect_failed;
    if (g_vm.expects == 0) return fail(r, out, cap, 3, "no expect statement");
    if (st != PM_OK || g_vm.expect_failed) {
        d[0] = 0; c_cat(d, "expects ", 96); c_num(d, g_vm.expects, 96);
        c_cat(d, " failed ", 96); c_num(d, g_vm.expect_failed, 96);
        return fail(r, out, cap, 3, d);
    }
    d[0] = 0; c_cat(d, "expects ", 96); c_num(d, g_vm.expects, 96); c_cat(d, " failed 0", 96);
    stage_line(out, cap, 3, 1, d); r->passed++;
    /* 4 deploy */
    r->stage = 4;
    if (!host || !host->write_file || host->write_file(host->ctx, deploy_path, (const char*)g_img, n, 0) < 0)
        return fail(r, out, cap, 4, "write failed");
    d[0] = 0; c_cat(d, deploy_path, 96);
    stage_line(out, cap, 4, 1, d); r->passed++;
    /* 5 verify */
    r->stage = 5;
    m = host->read_file ? host->read_file(host->ctx, deploy_path, (char*)g_back, PM_IMAGE_MAX) : -1;
    if (m != n) return fail(r, out, cap, 5, "deployed size differs");
    for (i = 0; i < n; i++) if (g_back[i] != g_img[i]) return fail(r, out, cap, 5, "deployed bytes differ");
    if (pm_image_read(g_back, m, &g_check) != PM_OK) return fail(r, out, cap, 5, "deployed image does not load");
    stage_line(out, cap, 5, 1, "read back identical, loads");
    r->passed++;
    r->stage = 6;
    c_cat(out, "cicd pipeline status success passed 5/5 image ", cap); c_cat(out, deploy_path, cap); c_cat(out, "\n", cap);
    return 0;
}
