/* Phase 4 PromptMessage: compiler, validator, optimizer, image, VM, triggers,
 * libraries, documentation, debugger (US-046..US-057). */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "../../../userspace/promptmessage.h"

#define FILES 8
typedef struct { char path[64]; char data[512]; int len; } fake_file_t;
static fake_file_t g_files[FILES];
static char g_out[8192];
static int g_mem = 40;
static pm_program_t g_prog;
static pm_vm_t g_vm;

static int f_find(const char* p) { int i; for (i = 0; i < FILES; i++) if (g_files[i].path[0] && !strcmp(g_files[i].path, p)) return i; return -1; }
static int f_read(void* c, const char* p, char* buf, int cap) {
    int i = f_find(p), n; (void)c;
    if (i < 0) return -1;
    n = g_files[i].len < cap ? g_files[i].len : cap;
    memcpy(buf, g_files[i].data, (size_t)n);
    return n;
}
static int f_write(void* c, const char* p, const char* d, int len, int append) {
    int i = f_find(p); (void)c;
    if (i < 0) { for (i = 0; i < FILES && g_files[i].path[0]; i++) {} if (i == FILES) return -1; strcpy(g_files[i].path, p); g_files[i].len = 0; }
    if (!append) g_files[i].len = 0;
    if (g_files[i].len + len >= (int)sizeof(g_files[i].data)) return -1;
    memcpy(g_files[i].data + g_files[i].len, d, (size_t)len);
    g_files[i].len += len;
    g_files[i].data[g_files[i].len] = 0;
    return len;
}
static int f_mem(void* c) { (void)c; return g_mem; }
static void f_out(void* c, const char* s) { (void)c; if (strlen(g_out) + strlen(s) + 2 < sizeof(g_out)) { strcat(g_out, s); strcat(g_out, "\n"); } }
static pm_host_t g_host = {0, f_read, f_write, f_mem, f_out};

void setUp(void) { memset(g_files, 0, sizeof(g_files)); g_out[0] = 0; g_mem = 40; pm_vm_reset(&g_vm); }
void tearDown(void) {}

static int run_src(const char* src) {
    int st = pm_compile(src, (int)strlen(src), &g_prog, &g_host);
    if (st) return st;
    pm_vm_reset(&g_vm);
    return pm_run(&g_prog, &g_vm, &g_host);
}

void test_spec_examples_run(void) {
    setUp();
    const char* src =
        "## demo of the US-046 examples\n"
        "create file \"document.txt\" with content \"Hello MOHHDY\"\n"
        "when user says \"ouvre mes photos\" then print \"gallery ~/Pictures\"\n"
        "if system.memory < 50% then print \"memory ok\"\n";
    TEST_ASSERT_EQUAL(PM_OK, run_src(src));
    TEST_ASSERT_EQUAL_STRING("Hello MOHHDY\n", g_files[f_find("document.txt")].data);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> memory ok"));
    TEST_ASSERT_NULL(strstr(g_out, "gallery"));
    g_out[0] = 0;
    TEST_ASSERT_EQUAL(PM_OK, pm_say(&g_prog, &g_vm, &g_host, "OUVRE mes photos"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> gallery ~/Pictures"));
    TEST_ASSERT_EQUAL(PM_ERR_RUNTIME, pm_say(&g_prog, &g_vm, &g_host, "inconnu"));
    TEST_ASSERT_EQUAL_STRING("no trigger", g_vm.err);
    g_mem = 80; g_out[0] = 0;
    TEST_ASSERT_EQUAL(PM_OK, pm_run(&g_prog, &g_vm, &g_host));
    TEST_ASSERT_NULL(strstr(g_out, "memory ok"));
}

void test_variables_blocks_loops(void) {
    setUp();
    const char* src =
        "set name to \"MOHHDY\"\n"
        "set n to 2 + 1\n"
        "repeat $n times\n"
        "  append \"line \" + $name to file \"log.txt\"\n"
        "end\n"
        "if $n == 3 and $name contains \"HH\" then\n"
        "  print \"three\"\n"
        "else\n"
        "  print \"other\"\n"
        "end\n"
        "if not file \"missing.txt\" exists then print \"absent\"\n"
        "show file \"log.txt\"\n";
    TEST_ASSERT_EQUAL(PM_OK, run_src(src));
    TEST_ASSERT_EQUAL_STRING("line MOHHDY\nline MOHHDY\nline MOHHDY\n", g_files[f_find("log.txt")].data);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> three"));
    TEST_ASSERT_NULL(strstr(g_out, "pm> other"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> absent"));
    TEST_ASSERT_EQUAL(3, (int)g_vm.writes);
    TEST_ASSERT_GREATER_THAN(0, (int)g_prog.folded); /* 2 + 1 folded */
}

void test_syntax_errors_have_line_and_column(void) {
    setUp();
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("print \"ok\"\nprint \"open\n", (int)strlen("print \"ok\"\nprint \"open\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL(2, g_prog.err_line); TEST_ASSERT_EQUAL(7, g_prog.err_col);
    TEST_ASSERT_EQUAL_STRING("unterminated string", g_prog.err);
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("fly away\n", (int)strlen("fly away\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("unknown statement", g_prog.err);
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("set x 3\n", (int)strlen("set x 3\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("expected 'to'", g_prog.err); TEST_ASSERT_EQUAL(7, g_prog.err_col);
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("print $nope\n", (int)strlen("print $nope\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("unknown variable", g_prog.err);
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("if 1 == 1 then\nprint 1\n", (int)strlen("if 1 == 1 then\nprint 1\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("missing end", g_prog.err);
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("end\n", (int)strlen("end\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("if 1 then repeat 2 times\n", (int)strlen("if 1 then repeat 2 times\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("inline statement cannot open a block", g_prog.err);
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("print 1 = 2\n", (int)strlen("print 1 = 2\n"), &g_prog, &g_host));
}

void test_constant_folding_removes_dead_test(void) {
    setUp();
    char dis[2048];
    TEST_ASSERT_EQUAL(PM_OK, pm_compile("if 2 + 3 > 4 then print \"yes\"\n", (int)strlen("if 2 + 3 > 4 then print \"yes\"\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL(3, g_prog.folded); /* add, compare, constant jz */
    TEST_ASSERT_GREATER_THAN(0, pm_disasm(&g_prog, dis, sizeof(dis)));
    TEST_ASSERT_NULL(strstr(dis, "JZ"));
    TEST_ASSERT_NULL(strstr(dis, "ADD"));
    TEST_ASSERT_NOT_NULL(strstr(dis, "PRINT"));
    pm_vm_reset(&g_vm);
    TEST_ASSERT_EQUAL(PM_OK, pm_run(&g_prog, &g_vm, &g_host));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> yes"));
    g_out[0] = 0;
    TEST_ASSERT_EQUAL(PM_OK, run_src("if 1 > 4 then print \"no\"\nprint \"after\"\n"));
    TEST_ASSERT_NULL(strstr(g_out, "pm> no"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> after"));
}

void test_image_roundtrip_and_tamper(void) {
    setUp();
    static unsigned char img[PM_IMAGE_MAX];
    static pm_program_t back;
    int n;
    TEST_ASSERT_EQUAL(PM_OK, pm_compile("set a to 5\nwhen user says \"hi\" then print \"x\" + $a\n", (int)strlen("set a to 5\nwhen user says \"hi\" then print \"x\" + $a\n"), &g_prog, &g_host));
    n = pm_image_write(&g_prog, img, sizeof(img));
    TEST_ASSERT_GREATER_THAN(16, n);
    TEST_ASSERT_EQUAL(PM_OK, pm_image_read(img, n, &back));
    TEST_ASSERT_EQUAL(g_prog.code_len, back.code_len);
    TEST_ASSERT_EQUAL(0, memcmp(g_prog.code, back.code, g_prog.code_len));
    TEST_ASSERT_EQUAL(PM_OK, pm_run(&back, &g_vm, &g_host));
    TEST_ASSERT_EQUAL(PM_OK, pm_say(&back, &g_vm, &g_host, "hi"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> x5"));
    img[20] ^= 1;
    TEST_ASSERT_EQUAL(PM_ERR_IMAGE, pm_image_read(img, n, &back));
    img[20] ^= 1;
    TEST_ASSERT_EQUAL(PM_ERR_IMAGE, pm_image_read(img, n - 1, &back));
    img[0] = 'X';
    TEST_ASSERT_EQUAL(PM_ERR_IMAGE, pm_image_read(img, n, &back));
}

void test_expect_reports_failures(void) {
    setUp();
    TEST_ASSERT_EQUAL(PM_ERR_EXPECT, run_src("set v to \"abc\"\nexpect $v contains \"b\"\nexpect $v == \"zzz\"\n"));
    TEST_ASSERT_EQUAL(2, (int)g_vm.expects);
    TEST_ASSERT_EQUAL(1, (int)g_vm.expect_failed);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm-expect failed line 3"));
    TEST_ASSERT_EQUAL(PM_OK, run_src("create file \"t\" with content \"v\"\nexpect file \"t\" exists\n"));
}

void test_sandbox_budget_and_runtime_errors(void) {
    setUp();
    TEST_ASSERT_EQUAL(PM_ERR_DENIED, run_src("create file \"../x\" with content \"no\"\n"));
    TEST_ASSERT_EQUAL(PM_ERR_DENIED, run_src("create file \"/models/gpt2.gguf\" with content \"no\"\n"));
    TEST_ASSERT_EQUAL(PM_ERR_DENIED, run_src("append \"x\" to file \"/bin/shell\"\n"));
    TEST_ASSERT_EQUAL(PM_ERR_RUNTIME, run_src("repeat 100000 times print 1\n"));
    TEST_ASSERT_EQUAL_STRING("step budget exhausted", g_vm.err);
    TEST_ASSERT_EQUAL(PM_ERR_RUNTIME, run_src("show file \"nothing\"\n"));
    TEST_ASSERT_EQUAL_STRING("file not found", g_vm.err);
    TEST_ASSERT_EQUAL(PM_ERR_RUNTIME, run_src("if \"a\" < \"b\" then print 1\n"));
    TEST_ASSERT_EQUAL(PM_OK, run_src("print 1\nstop\nprint \"unreached\"\n"));
    TEST_ASSERT_NULL(strstr(g_out, "unreached"));
}

void test_libraries_doc_and_debugger(void) {
    setUp();
    char doc[1024];
    const char* lib = "## greeting library\nset greeting to \"bonjour\"\n";
    const char* src = "## main program\nuse \"lib.pm\"\nset who to \"monde\"\nprint $greeting + \" \" + $who\nwhen user dit \"salut\" then print $who\n";
    f_write(0, "lib.pm", lib, (int)strlen(lib), 0);
    TEST_ASSERT_EQUAL(PM_OK, run_src(src));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm> bonjour monde"));
    TEST_ASSERT_EQUAL(1, g_prog.includes);
    TEST_ASSERT_GREATER_THAN(0, pm_doc(src, (int)strlen(src), &g_prog, doc, sizeof(doc)));
    TEST_ASSERT_NOT_NULL(strstr(doc, "pm-doc about main program"));
    TEST_ASSERT_NOT_NULL(strstr(doc, "pm-doc trigger \"salut\""));
    TEST_ASSERT_NOT_NULL(strstr(doc, "pm-doc variable greeting"));
    TEST_ASSERT_NOT_NULL(strstr(doc, "pm-doc variable who"));
    TEST_ASSERT_NOT_NULL(strstr(doc, "libraries 1"));
    /* Debugger: trace then break at line 4 with variables dumped. */
    g_out[0] = 0; pm_vm_reset(&g_vm); g_vm.trace = 1;
    TEST_ASSERT_EQUAL(PM_OK, pm_run(&g_prog, &g_vm, &g_host));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm-trace line 3"));
    g_out[0] = 0; pm_vm_reset(&g_vm); g_vm.break_line = 4;
    TEST_ASSERT_EQUAL(PM_BREAK, pm_run(&g_prog, &g_vm, &g_host));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm-break line 4"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "pm-var who=monde"));
    TEST_ASSERT_NULL(strstr(g_out, "pm> bonjour"));
    /* Missing library and nested use. */
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("use \"none.pm\"\n", (int)strlen("use \"none.pm\"\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("library not found", g_prog.err);
    f_write(0, "nest.pm", "use \"lib.pm\"\n", 13, 0);
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile("use \"nest.pm\"\n", (int)strlen("use \"nest.pm\"\n"), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("use is not allowed inside a library", g_prog.err);
}

void test_limits(void) {
    setUp();
    static char big[PM_SRC_MAX + 2];
    int i;
    memset(big, 'a', sizeof(big) - 1); big[sizeof(big) - 1] = 0;
    TEST_ASSERT_EQUAL(PM_ERR_LIMIT, pm_compile(big, PM_SRC_MAX + 1, &g_prog, &g_host));
    big[0] = 0;
    for (i = 0; i < PM_VARS + 1; i++) {
        char line[32]; sprintf(line, "set v%d to %d\n", i, i); strcat(big, line);
    }
    TEST_ASSERT_EQUAL(PM_ERR_SYNTAX, pm_compile(big, (int)strlen(big), &g_prog, &g_host));
    TEST_ASSERT_EQUAL_STRING("bad or too many variables", g_prog.err);
}

int main(void) {
    unity_init();
    RUN_TEST(test_spec_examples_run);
    RUN_TEST(test_variables_blocks_loops);
    RUN_TEST(test_syntax_errors_have_line_and_column);
    RUN_TEST(test_constant_folding_removes_dead_test);
    RUN_TEST(test_image_roundtrip_and_tamper);
    RUN_TEST(test_expect_reports_failures);
    RUN_TEST(test_sandbox_budget_and_runtime_errors);
    RUN_TEST(test_libraries_doc_and_debugger);
    RUN_TEST(test_limits);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
