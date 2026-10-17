/* Lot 7 (US-108): the local CI/CD pipeline runs every stage for real. */
#include "../../framework/unity.h"
#include "../../../userspace/cicd.h"
#include <string.h>
#include <stdio.h>

#define NF 8
static struct { char path[64]; char data[PM_IMAGE_MAX]; int len; } g_fs[NF];
static int g_fail_write, g_corrupt;
static char out[4096];

static int find(const char* p) { int i; for (i = 0; i < NF; i++) if (g_fs[i].path[0] && strcmp(g_fs[i].path, p) == 0) return i; return -1; }
static int h_read(void* c, const char* p, char* buf, int cap) {
    int i = find(p);
    (void)c;
    if (i < 0) return -1;
    if (g_fs[i].len > cap) return -1;
    memcpy(buf, g_fs[i].data, (size_t)g_fs[i].len);
    if (g_corrupt && g_fs[i].len > 20) buf[20] ^= 1;
    return g_fs[i].len;
}
static int h_write(void* c, const char* p, const char* d, int len, int append) {
    int i = find(p);
    (void)c;
    if (g_fail_write) return -1;
    if (i < 0) { for (i = 0; i < NF && g_fs[i].path[0]; i++) {} if (i >= NF) return -1; snprintf(g_fs[i].path, 64, "%s", p); g_fs[i].len = 0; }
    if (!append) g_fs[i].len = 0;
    if (g_fs[i].len + len > (int)sizeof(g_fs[i].data)) return -1;
    memcpy(g_fs[i].data + g_fs[i].len, d, (size_t)len);
    g_fs[i].len += len;
    return len;
}
static int h_mem(void* c) { (void)c; return 10; }
static void h_out(void* c, const char* t) { (void)c; (void)t; }
static pm_host_t host = {0, h_read, h_write, h_mem, h_out};

static const char* prog = "create file \"doc.txt\" with content \"hi\"\nset greeting to \"bonjour\"\nprint $greeting\n";
static const char* tests_ok = "expect file \"doc.txt\" exists\nexpect 2 + 2 == 4\n";
static const char* tests_bad = "expect 2 + 2 == 5\nexpect 1 == 1\n";

static void reset(void) { memset(g_fs, 0, sizeof(g_fs)); g_fail_write = 0; g_corrupt = 0; }

static void test_pipeline_success(void) {
    cicd_result_t r;
    reset();
    TEST_ASSERT_EQUAL(0, cicd_run(prog, (int)strlen(prog), tests_ok, (int)strlen(tests_ok), &host, "/apps/p.pmc", &r, out, sizeof(out)));
    TEST_ASSERT_EQUAL(6, r.stage);
    TEST_ASSERT_EQUAL(5, r.passed);
    TEST_ASSERT_EQUAL(2, (int)r.expects);
    TEST_ASSERT_NOT_NULL(strstr(out, "cicd stage 1/5 lint ok statements 3"));
    TEST_ASSERT_NOT_NULL(strstr(out, "cicd stage 3/5 test ok expects 2 failed 0"));
    TEST_ASSERT_NOT_NULL(strstr(out, "cicd stage 4/5 deploy ok /apps/p.pmc"));
    TEST_ASSERT_NOT_NULL(strstr(out, "cicd pipeline status success passed 5/5"));
    /* the deployed file is a loadable image of the program */
    {
        static pm_program_t back;
        int i = find("/apps/p.pmc");
        TEST_ASSERT_TRUE(i >= 0);
        TEST_ASSERT_EQUAL(r.image_bytes, g_fs[i].len);
        TEST_ASSERT_EQUAL(PM_OK, pm_image_read((const unsigned char*)g_fs[i].data, g_fs[i].len, &back));
    }
}

static void test_pipeline_failures_stop(void) {
    cicd_result_t r;
    const char* bad = "print \"ok\"\nset x 3\n";
    reset();
    TEST_ASSERT_EQUAL(-1, cicd_run(bad, (int)strlen(bad), tests_ok, (int)strlen(tests_ok), &host, "/a", &r, out, sizeof(out)));
    TEST_ASSERT_EQUAL(1, r.failed_stage);
    TEST_ASSERT_NOT_NULL(strstr(out, "cicd stage 1/5 lint FAILED line 2"));
    TEST_ASSERT_NOT_NULL(strstr(out, "cicd stage 5/5 verify skipped"));
    TEST_ASSERT_EQUAL(-1, find("/a"));                       /* nothing deployed */
    reset();
    TEST_ASSERT_EQUAL(-3, cicd_run(prog, (int)strlen(prog), tests_bad, (int)strlen(tests_bad), &host, "/a", &r, out, sizeof(out)));
    TEST_ASSERT_NOT_NULL(strstr(out, "cicd stage 3/5 test FAILED expects 2 failed 1"));
    TEST_ASSERT_EQUAL(-1, find("/a"));
    reset();
    TEST_ASSERT_EQUAL(-3, cicd_run(prog, (int)strlen(prog), "print \"x\"\n", 10, &host, "/a", &r, out, sizeof(out)));
    TEST_ASSERT_NOT_NULL(strstr(out, "test FAILED no expect statement"));
    reset();
    g_fail_write = 1;
    /* the program itself writes doc.txt: with writes failing, test stage fails */
    TEST_ASSERT_TRUE(cicd_run(prog, (int)strlen(prog), tests_ok, (int)strlen(tests_ok), &host, "/a", &r, out, sizeof(out)) < 0);
    reset();
    g_corrupt = 1;                                           /* storage flips a bit on read back */
    TEST_ASSERT_EQUAL(-5, cicd_run(prog, (int)strlen(prog), tests_ok, (int)strlen(tests_ok), &host, "/a", &r, out, sizeof(out)));
    TEST_ASSERT_NOT_NULL(strstr(out, "verify FAILED deployed bytes differ"));
    TEST_ASSERT_EQUAL_STRING("verify", cicd_stage_name(5));
}

int main(void) {
    unity_init();
    RUN_TEST(test_pipeline_success);
    RUN_TEST(test_pipeline_failures_stop);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
