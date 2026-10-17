/* test_osui_ide.c - US-050 full-screen pm-ide model (userspace/osui_ide.c). */

#include "../../framework/unity.h"
#include "../../framework/test_kernel.h"
#include "osui_ide.h"
#include <string.h>

static osui_ide_t g_ide;
static char g_text[OSUI_IDE_TEXT_MAX + 1];
static char g_row[OSUI_IDE_ROW_LEN];

static void type_str(const char *s) {
    while (*s) TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_key(&g_ide, *s++));
}

static void test_ide_open_and_text_roundtrip(void) {
    const char *src = "say \"a\"\nexpect 1 == 1\n";
    osui_ide_open(&g_ide, "t.pm", src, (int)strlen(src));
    TEST_ASSERT_EQUAL(1, g_ide.active);
    TEST_ASSERT_EQUAL(2, g_ide.nlines);
    TEST_ASSERT_EQUAL(0, g_ide.modified);
    TEST_ASSERT_EQUAL((int)strlen(src), osui_ide_text(&g_ide, g_text, (int)sizeof(g_text)));
    TEST_ASSERT_EQUAL_STRING(src, g_text);
    /* Empty file: one empty line, text is a single newline. */
    osui_ide_open(&g_ide, "new.pm", "", 0);
    TEST_ASSERT_EQUAL(1, g_ide.nlines);
    TEST_ASSERT_EQUAL(1, osui_ide_text(&g_ide, g_text, (int)sizeof(g_text)));
    /* Output buffer too small is refused, not truncated silently. */
    osui_ide_open(&g_ide, "t.pm", src, (int)strlen(src));
    TEST_ASSERT_EQUAL(-1, osui_ide_text(&g_ide, g_text, 5));
}

static void test_ide_editing_keys(void) {
    osui_ide_open(&g_ide, "e.pm", "", 0);
    type_str("abc");
    TEST_ASSERT_EQUAL(1, g_ide.modified);
    TEST_ASSERT_EQUAL(3, g_ide.col);
    osui_ide_key(&g_ide, OSUI_IDE_KEY_LEFT);
    osui_ide_key(&g_ide, '\n');          /* split "ab" | "c" */
    TEST_ASSERT_EQUAL(2, g_ide.nlines);
    TEST_ASSERT_EQUAL_STRING("ab", g_ide.lines[0]);
    TEST_ASSERT_EQUAL_STRING("c", g_ide.lines[1]);
    TEST_ASSERT_EQUAL(1, g_ide.row);
    TEST_ASSERT_EQUAL(0, g_ide.col);
    osui_ide_key(&g_ide, '\b');          /* join back */
    TEST_ASSERT_EQUAL(1, g_ide.nlines);
    TEST_ASSERT_EQUAL_STRING("abc", g_ide.lines[0]);
    TEST_ASSERT_EQUAL(2, g_ide.col);
    osui_ide_key(&g_ide, '\b');          /* delete 'b' */
    TEST_ASSERT_EQUAL_STRING("ac", g_ide.lines[0]);
    osui_ide_key(&g_ide, OSUI_IDE_KEY_RIGHT);
    osui_ide_key(&g_ide, '\n');
    type_str("x");
    osui_ide_key(&g_ide, OSUI_IDE_KEY_UP);
    TEST_ASSERT_EQUAL(0, g_ide.row);
    TEST_ASSERT_EQUAL(1, g_ide.col);     /* column kept: "ac" is long enough */
    osui_ide_key(&g_ide, OSUI_IDE_KEY_DOWN);
    TEST_ASSERT_EQUAL(1, g_ide.row);
    TEST_ASSERT_EQUAL(1, g_ide.col);     /* "x" has length 1 */
    /* Left at column 0 wraps to the end of the previous line. */
    osui_ide_key(&g_ide, OSUI_IDE_KEY_LEFT);
    osui_ide_key(&g_ide, OSUI_IDE_KEY_LEFT);
    TEST_ASSERT_EQUAL(0, g_ide.row);
    TEST_ASSERT_EQUAL(2, g_ide.col);
    TEST_ASSERT_EQUAL(5, osui_ide_text(&g_ide, g_text, (int)sizeof(g_text)));
    TEST_ASSERT_EQUAL_STRING("ac\nx\n", g_text);
}

static void test_ide_command_keys(void) {
    osui_ide_open(&g_ide, "c.pm", "say 1\n", 6);
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC));
    TEST_ASSERT_EQUAL(1, g_ide.cmd_mode);
    TEST_ASSERT_EQUAL(OSUI_IDE_CHECK, osui_ide_key(&g_ide, 'c'));
    TEST_ASSERT_EQUAL(0, g_ide.cmd_mode);
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    TEST_ASSERT_EQUAL(OSUI_IDE_RUN, osui_ide_key(&g_ide, 'r'));
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    TEST_ASSERT_EQUAL(OSUI_IDE_SAVE, osui_ide_key(&g_ide, 'w'));
    /* Unmodified: q quits. */
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    TEST_ASSERT_EQUAL(OSUI_IDE_QUIT, osui_ide_key(&g_ide, 'q'));
    /* Modified: q refuses, Q discards; an unknown key cancels and is not typed. */
    osui_ide_open(&g_ide, "c.pm", "", 0);
    type_str("z");
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_key(&g_ide, 'q'));
    TEST_ASSERT_NOT_NULL(strstr(g_ide.status, "unsaved"));
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_key(&g_ide, 'k'));
    TEST_ASSERT_EQUAL_STRING("z", g_ide.lines[0]);
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    TEST_ASSERT_EQUAL(OSUI_IDE_QUIT, osui_ide_key(&g_ide, 'Q'));
}

static void test_ide_frame_and_scrolling(void) {
    int i;
    osui_ide_open(&g_ide, "s.pm", "", 0);
    for (i = 0; i < 20; i++) {
        osui_ide_key(&g_ide, 'a' + (i % 26));
        if (i < 19) osui_ide_key(&g_ide, '\n');
    }
    TEST_ASSERT_EQUAL(20, g_ide.nlines);
    TEST_ASSERT_EQUAL(19, g_ide.row);
    TEST_ASSERT_EQUAL(20 - OSUI_IDE_VIEW, g_ide.top);
    osui_ide_frame_row(&g_ide, 0, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("pm-ide s.pm [+] Ln 20 Col 2 /20", g_row);
    osui_ide_frame_row(&g_ide, OSUI_IDE_VIEW, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING(" 20> t_", g_row);
    osui_ide_frame_row(&g_ide, 1, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("  9| i", g_row);
    for (i = 0; i < 19; i++) osui_ide_key(&g_ide, OSUI_IDE_KEY_UP);
    TEST_ASSERT_EQUAL(0, g_ide.top);
    osui_ide_frame_row(&g_ide, 1, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("  1> a_", g_row);
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    osui_ide_frame_row(&g_ide, OSUI_IDE_FRAME - 1, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL(0, strncmp(g_row, "CMD command:", 12));
    /* Rows past the end show '~'. */
    osui_ide_open(&g_ide, "one.pm", "x", 1);
    osui_ide_frame_row(&g_ide, 2, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("   ~", g_row);
}

static void test_ide_limits(void) {
    int i;
    char longline[OSUI_IDE_COLS + 10];
    memset(longline, 'q', sizeof(longline) - 1);
    longline[sizeof(longline) - 1] = 0;
    osui_ide_open(&g_ide, "big.pm", longline, (int)strlen(longline));
    TEST_ASSERT_EQUAL(1, g_ide.truncated);
    TEST_ASSERT_EQUAL(OSUI_IDE_COLS, (int)strlen(g_ide.lines[0]));
    /* A full line refuses more characters. */
    g_ide.col = OSUI_IDE_COLS;
    osui_ide_key(&g_ide, 'x');
    TEST_ASSERT_EQUAL(OSUI_IDE_COLS, (int)strlen(g_ide.lines[0]));
    TEST_ASSERT_EQUAL_STRING("line full", g_ide.status);
    /* Long cursor column scrolls the view horizontally. */
    osui_ide_key(&g_ide, OSUI_IDE_KEY_LEFT);
    TEST_ASSERT_TRUE(g_ide.left > 0);
    /* The buffer stops at OSUI_IDE_LINES lines. */
    osui_ide_open(&g_ide, "lines.pm", "", 0);
    for (i = 0; i < OSUI_IDE_LINES + 5; i++) osui_ide_key(&g_ide, '\n');
    TEST_ASSERT_EQUAL(OSUI_IDE_LINES, g_ide.nlines);
    TEST_ASSERT_EQUAL_STRING("buffer full", g_ide.status);
    /* Inactive editor ignores keys. */
    g_ide.active = 0;
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC));
    TEST_ASSERT_EQUAL(0, g_ide.cmd_mode);
}

int main(void) {
    unity_init();
    RUN_TEST(test_ide_open_and_text_roundtrip);
    RUN_TEST(test_ide_editing_keys);
    RUN_TEST(test_ide_command_keys);
    RUN_TEST(test_ide_frame_and_scrolling);
    RUN_TEST(test_ide_limits);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
