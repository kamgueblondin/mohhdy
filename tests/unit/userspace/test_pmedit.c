/* Full-screen PromptMessage editor model (userspace/pmedit.c, US-050). */
#include "../../framework/unity.h"
#include <string.h>
#include "../../../userspace/pmedit.h"

static pme_t g_e;
static uint16_t g_cells[PME_COLS * PME_ROWS];
static void keys(const char* s) { while (*s) pme_key(&g_e, *s++); }

static void test_load_edit_save_text(void) {
    char out[512], row[96];
    const char* src = "say \"hi\"\nexpect 1 == 1\n";
    pme_load(&g_e, "/a.pm", src, (int)strlen(src));
    TEST_ASSERT_EQUAL(2, g_e.n);
    TEST_ASSERT_EQUAL(0, g_e.modified);
    pme_key(&g_e, 31);            /* down */
    pme_key(&g_e, 29); pme_key(&g_e, 29); pme_key(&g_e, 29); pme_key(&g_e, 29); pme_key(&g_e, 29); pme_key(&g_e, 29);
    keys("2 == 2 and ");
    TEST_ASSERT_EQUAL_STRING("expect2 == 2 and  1 == 1", g_e.line[1]);
    TEST_ASSERT_EQUAL(1, g_e.modified);
    pme_key(&g_e, '\n');
    TEST_ASSERT_EQUAL(3, g_e.n);
    TEST_ASSERT_EQUAL_STRING(" 1 == 1", g_e.line[2]);
    pme_key(&g_e, '\b');          /* join back */
    TEST_ASSERT_EQUAL(2, g_e.n);
    { int n = pme_text(&g_e, out, sizeof(out)); TEST_ASSERT_EQUAL(n, (int)strlen(out)); }
    TEST_ASSERT_EQUAL_STRING("say \"hi\"\nexpect2 == 2 and  1 == 1\n", out);
    pme_render(&g_e, g_cells);
    pme_row(g_cells, 0, row, sizeof(row));
    TEST_ASSERT_NOT_NULL(strstr(row, "PromptMessage IDE  /a.pm [modified]"));
    pme_row(g_cells, 3, row, sizeof(row));
    TEST_ASSERT_EQUAL_STRING(" 2 expect2 == 2 and  1 == 1", row);
    /* cursor cell is highlighted */
    TEST_ASSERT_EQUAL(0x70, g_cells[3 * PME_COLS + 3 + g_e.cx] >> 8);
}

static void test_escape_commands_and_scroll(void) {
    char row[96];
    int i;
    pme_load(&g_e, "/b.pm", "", 0);
    TEST_ASSERT_EQUAL(1, g_e.n);
    for (i = 0; i < 30; i++) { keys("say 1"); pme_key(&g_e, '\n'); }
    TEST_ASSERT_EQUAL(31, g_e.n);
    TEST_ASSERT_EQUAL(30, g_e.cy);
    TEST_ASSERT_EQUAL(30 - PME_VIEW + 1, g_e.top);      /* scrolled */
    pme_render(&g_e, g_cells);
    pme_row(g_cells, 2, row, sizeof(row));
    TEST_ASSERT_EQUAL_STRING("11 say 1", row);
    TEST_ASSERT_EQUAL(PME_NONE, pme_key(&g_e, 27));
    TEST_ASSERT_EQUAL(PME_SAVE, pme_key(&g_e, 's'));
    pme_key(&g_e, 27);
    TEST_ASSERT_EQUAL(PME_CHECK, pme_key(&g_e, 'c'));
    pme_key(&g_e, 27);
    TEST_ASSERT_EQUAL(PME_QUIT, pme_key(&g_e, 'q'));
    pme_key(&g_e, 27);
    TEST_ASSERT_EQUAL(PME_SAVE, pme_key(&g_e, 'x'));
    TEST_ASSERT_EQUAL(2, g_e.esc);                      /* save then quit */
    g_e.esc = 0;
    pme_key(&g_e, 27);
    TEST_ASSERT_EQUAL(PME_NONE, pme_key(&g_e, 'z'));
    pme_status(&g_e, "check ok statements 3");
    pme_render(&g_e, g_cells);
    pme_row(g_cells, 23, row, sizeof(row));
    TEST_ASSERT_EQUAL_STRING("check ok statements 3", row);
}

static void test_limits(void) {
    int i;
    pme_load(&g_e, "/c.pm", "", 0);
    for (i = 0; i < PME_LEN + 5; i++) pme_key(&g_e, 'a');
    TEST_ASSERT_EQUAL(PME_LEN, (int)strlen(g_e.line[0]));
    for (i = 0; i < PME_LINES + 5; i++) pme_key(&g_e, '\n');
    TEST_ASSERT_EQUAL(PME_LINES, g_e.n);
    TEST_ASSERT_NOT_NULL(strstr(g_e.status, "buffer full"));
    pme_key(&g_e, 30); pme_key(&g_e, 28); pme_key(&g_e, 1); pme_key(&g_e, 200);
    TEST_ASSERT_TRUE(g_e.cy >= 0 && g_e.cy < g_e.n);
    pme_load(&g_e, "/d.pm", "x\ty\x01z\r\n", 7);
    TEST_ASSERT_EQUAL_STRING("x yz", g_e.line[0]);
    TEST_ASSERT_EQUAL(1, g_e.n);
}

int main(void) {
    unity_init();
    RUN_TEST(test_load_edit_save_text);
    RUN_TEST(test_escape_commands_and_scroll);
    RUN_TEST(test_limits);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
