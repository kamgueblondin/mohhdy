/* test_osui_form.c - GUI form component (Lot 44 PR 4): the collab forms
 * build the exact shell grammar, need a second Enter, and run through the
 * same src->run hook as a typed command. */
#include <string.h>
#include "../../framework/unity.h"
#include "../../../userspace/osui_form.h"
#include "../../../userspace/osui_tools.h"

static char g_ran[256];
static int g_runs;
static int fake_run(const char *cmd, char *out, int max) {
    strncpy(g_ran, cmd, sizeof(g_ran) - 1);
    g_runs++;
    strncpy(out, "collab-offer ok seq 3\nsecond line\n", (size_t)max - 1);
    out[max - 1] = 0;
    return (int)strlen(out);
}

static void type(osui_form_t *f, const char *s, osui_tool_src_t *src) {
    while (*s) osui_form_key(f, *s++, src);
}
static void clear_field(osui_form_t *f, osui_tool_src_t *src) {
    int k;
    for (k = 0; k < OSUI_FORM_VAL_MAX; k++) osui_form_key(f, '\b', src);
}

static void test_offer_form_builds_shell_grammar(void) {
    osui_form_t f;
    osui_tool_src_t src;
    osui_app_view_t v;
    char cmd[128];
    memset(&src, 0, sizeof(src));
    src.run = fake_run;
    g_runs = 0;
    osui_form_collab_offer_init(&f);
    TEST_ASSERT_EQUAL(4, f.num_fields);
    TEST_ASSERT_EQUAL(0, osui_form_build_cmd(&f, cmd, (int)sizeof(cmd)));
    TEST_ASSERT_EQUAL_STRING("collab-offer gpu 10 1", cmd);
    clear_field(&f, &src);
    type(&f, "cpu", &src);
    osui_form_key(&f, 9, &src);          /* Tab */
    clear_field(&f, &src);
    type(&f, "25", &src);
    TEST_ASSERT_EQUAL(0, osui_form_build_cmd(&f, cmd, (int)sizeof(cmd)));
    TEST_ASSERT_EQUAL_STRING("collab-offer cpu 25 1", cmd);
    /* go to the submit button: first Enter arms, second runs */
    osui_form_key(&f, 31, &src);
    osui_form_key(&f, 31, &src);
    TEST_ASSERT_EQUAL(3, f.active_field);
    osui_form_key(&f, '\n', &src);
    TEST_ASSERT_EQUAL(0, g_runs);
    TEST_ASSERT(strstr(f.status, "confirmer: collab-offer cpu 25 1") != 0);
    osui_form_key(&f, '\n', &src);
    TEST_ASSERT_EQUAL(1, g_runs);
    TEST_ASSERT_EQUAL_STRING("collab-offer cpu 25 1", g_ran);
    TEST_ASSERT_EQUAL_STRING("collab-offer ok seq 3", f.status);
    osui_form_render(&f, &v);
    TEST_ASSERT_EQUAL_STRING("$ collab-offer cpu 25 1", v.rows[4]);
    TEST_ASSERT_EQUAL_STRING("collab-offer ok seq 3", v.rows[5]);
    TEST_ASSERT_EQUAL_STRING("second line", v.rows[6]);
}

static void test_vote_escrow_settle_forms(void) {
    osui_form_t f;
    osui_tool_src_t src;
    char cmd[128];
    memset(&src, 0, sizeof(src));
    src.run = fake_run;
    osui_form_collab_vote_init(&f);
    TEST_ASSERT_EQUAL(-1, osui_form_build_cmd(&f, cmd, (int)sizeof(cmd)));   /* empty peer */
    type(&f, "bob", &src); osui_form_key(&f, 9, &src);
    type(&f, "7", &src); osui_form_key(&f, 9, &src);
    osui_form_key(&f, 29, &src);                                           /* Right: no */
    TEST_ASSERT_EQUAL(0, osui_form_build_cmd(&f, cmd, (int)sizeof(cmd)));
    TEST_ASSERT_EQUAL_STRING("collab-vote bob 7 no", cmd);

    osui_form_collab_escrow_init(&f);
    type(&f, "carol", &src); osui_form_key(&f, 9, &src);
    osui_form_key(&f, 9, &src);                                            /* keep none */
    type(&f, "50", &src);
    TEST_ASSERT_EQUAL(0, osui_form_build_cmd(&f, cmd, (int)sizeof(cmd)));
    TEST_ASSERT_EQUAL_STRING("collab-escrow-create carol none 50", cmd);   /* NAME optional */
    osui_form_key(&f, 9, &src);
    type(&f, "two words", &src);
    TEST_ASSERT_EQUAL(-1, osui_form_build_cmd(&f, cmd, (int)sizeof(cmd))); /* would shift args */

    osui_form_collab_settle_init(&f);
    osui_form_key(&f, 29, &src); osui_form_key(&f, 29, &src);              /* dispute-open */
    osui_form_key(&f, 9, &src); type(&f, "dave", &src);
    osui_form_key(&f, 9, &src); type(&f, "4", &src);
    osui_form_key(&f, 9, &src); type(&f, "late delivery", &src);
    TEST_ASSERT_EQUAL(0, osui_form_build_cmd(&f, cmd, (int)sizeof(cmd)));
    TEST_ASSERT_EQUAL_STRING("collab-dispute-open dave 4 late delivery", cmd);
}

static void test_form_without_shell_reports(void) {
    osui_form_t f;
    osui_tool_src_t src;
    memset(&src, 0, sizeof(src));
    osui_form_collab_offer_init(&f);
    f.active_field = 3;
    osui_form_key(&f, '\n', &src);
    osui_form_key(&f, '\n', &src);
    TEST_ASSERT_EQUAL_STRING("shell indisponible", f.status);
}

int main(void) {
    unity_init();
    RUN_TEST(test_offer_form_builds_shell_grammar);
    RUN_TEST(test_vote_escrow_settle_forms);
    RUN_TEST(test_form_without_shell_reports);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
