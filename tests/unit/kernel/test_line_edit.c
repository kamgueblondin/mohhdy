/* Shared console line editor: history, Tab completion, cursor keys.
 * A fake screen line checks that what is echoed matches the buffer. */
#include "../../framework/unity.h"
#include "line_edit.h"
#include <string.h>
#include <stdio.h>

static char scr[4096];
static int sx, smax, reprompts;
static void s_put(char c) {
    if (c == '\n') { sx = 0; memset(scr, 0, sizeof(scr)); smax = 0; return; }
    scr[sx++] = c;
    if (sx > smax) smax = sx;
}
static void s_left(void) { if (sx > 0) sx--; }
static void s_reprompt(void) { reprompts++; }
static const line_edit_io_t io = { s_put, s_left, s_reprompt };
static const char *const words[] = { "help", "hello", "history", "ls", "p2p-status", "p2p-send" };
static line_edit_t le;

static void start(void) {
    memset(scr, 0, sizeof(scr)); sx = smax = 0; reprompts = 0;
    line_edit_begin(&le, 128, words, 6, &io);
}
static int keys(const char *s) { int r = 0; while (*s) r = line_edit_key(&le, *s++); return r; }
static void key(int c) { line_edit_key(&le, (char)c); }
/* the visible line equals the buffer, the screen cursor equals cur */
static void check(const char *want) {
    char shown[256];
    int n = smax;
    memcpy(shown, scr, (size_t)n); shown[n] = 0;
    while (n > le.len && shown[n - 1] == ' ') shown[--n] = 0;
    if (strcmp(want, le.buf) || strcmp(want, shown)) printf("want[%s] buf[%s] shown[%s]\n", want, le.buf, shown);
    TEST_ASSERT_EQUAL_STRING(want, le.buf);
    TEST_ASSERT_EQUAL_STRING(want, shown);
    TEST_ASSERT_EQUAL(le.cur, sx);
}

static void test_cursor_keys_insert_delete(void) {
    start();
    keys("hllo");
    key(LE_KEY_HOME); key(LE_KEY_RIGHT); keys("e");
    check("hello");
    TEST_ASSERT_EQUAL(2, le.cur);
    key(LE_KEY_END); keys(" world");
    check("hello world");
    key(LE_KEY_LEFT); key(LE_KEY_LEFT); key('\b');
    check("hello wold");
    key(LE_KEY_HOME); key(LE_KEY_DEL);
    check("ello wold");
    TEST_ASSERT_EQUAL(1, keys("\n"));
    TEST_ASSERT_EQUAL_STRING("ello wold", le.buf);
}

static void test_history_up_down(void) {
    line_edit_history_reset();
    start(); keys("ls\n");
    start(); keys("p2p-status\n");
    start(); keys("p2p-status\n");          /* duplicate not stored twice */
    TEST_ASSERT_EQUAL(2, line_edit_history_count());
    start(); keys("draft");
    key(LE_KEY_UP); check("p2p-status");
    key(LE_KEY_UP); check("ls");
    key(LE_KEY_UP); check("ls");            /* oldest stays */
    key(LE_KEY_DOWN); check("p2p-status");
    key(LE_KEY_DOWN); check("draft");       /* live line restored */
    key(LE_KEY_UP); key(LE_KEY_UP);
    TEST_ASSERT_EQUAL(1, keys("\n"));
    TEST_ASSERT_EQUAL_STRING("ls", le.buf);
}

static void test_tab_completion(void) {
    start(); keys("hi"); key('\t');
    check("history ");
    start(); keys("p2"); key('\t');
    check("p2p-s");                          /* common prefix */
    key('\t');
    TEST_ASSERT_EQUAL(0, reprompts);         /* single Tab: no list */
    key('\t');                               /* double Tab: list + reprompt */
    TEST_ASSERT_EQUAL(1, reprompts);
    check("p2p-s");
    start(); keys("ls -l"); key('\t');
    check("ls -l");                          /* only the first word completes */
    start(); keys("zz"); key('\t'); key('\t');
    check("zz");
    TEST_ASSERT_EQUAL(0, reprompts);
}

static void test_size_limit(void) {
    int i;
    line_edit_begin(&le, 8, words, 6, &io);
    memset(scr, 0, sizeof(scr)); sx = smax = 0;
    for (i = 0; i < 20; i++) key('a');
    TEST_ASSERT_EQUAL(7, le.len);
}

int main(void) {
    unity_init();
    RUN_TEST(test_cursor_keys_insert_delete);
    RUN_TEST(test_history_up_down);
    RUN_TEST(test_tab_completion);
    RUN_TEST(test_size_limit);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
