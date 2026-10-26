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

static void test_ide_syntax_highlight(void) {
    unsigned char cls[OSUI_IDE_COLS + 1];
    const char *l = "if x < 50% then say \"a \\\" b\" 3x";
    int n = osui_ide_highlight(l, cls, (int)sizeof(cls)), i;
    TEST_ASSERT_EQUAL((int)strlen(l), n);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_KEYWORD, cls[0]);          /* if */
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_KEYWORD, cls[1]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, cls[3]);             /* x */
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, cls[5]);             /* < */
    for (i = 7; i < 10; i++) TEST_ASSERT_EQUAL(OSUI_IDE_TOK_NUMBER, cls[i]);  /* 50% */
    for (i = 11; i < 15; i++) TEST_ASSERT_EQUAL(OSUI_IDE_TOK_KEYWORD, cls[i]); /* then */
    for (i = 16; i < 19; i++) TEST_ASSERT_EQUAL(OSUI_IDE_TOK_KEYWORD, cls[i]); /* say */
    for (i = 20; i < 28; i++) TEST_ASSERT_EQUAL(OSUI_IDE_TOK_STRING, cls[i]);  /* "a \" b" */
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, cls[29]);            /* 3x is a word */
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, cls[30]);
    /* sayer, system.memory: words that only start like a keyword */
    n = osui_ide_highlight("sayer system.memory", cls, (int)sizeof(cls));
    for (i = 0; i < n; i++) TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, cls[i]);
    /* comments: '#' or // as first non-blank, not after code */
    n = osui_ide_highlight("  # say \"x\"", cls, (int)sizeof(cls));
    for (i = 2; i < n; i++) TEST_ASSERT_EQUAL(OSUI_IDE_TOK_COMMENT, cls[i]);
    n = osui_ide_highlight("// note", cls, (int)sizeof(cls));
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_COMMENT, cls[n - 1]);
    osui_ide_highlight("say 1 # x", cls, (int)sizeof(cls));
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, cls[6]);
    /* unterminated string runs to the end; max bounds the output */
    n = osui_ide_highlight("say \"abc", cls, (int)sizeof(cls));
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_STRING, cls[n - 1]);
    TEST_ASSERT_EQUAL(2, osui_ide_highlight("say", cls, 2));
    TEST_ASSERT_EQUAL(-1, osui_ide_highlight("say", 0, 4));
}

static void test_ide_frame_attr(void) {
    unsigned char attr[OSUI_IDE_ROW_LEN];
    int n;
    osui_ide_open(&g_ide, "a.pm", "say \"hi\" 2\nx\n", 13);
    osui_ide_frame_row(&g_ide, 1, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("  1> say \"hi\" 2", g_row);
    n = osui_ide_frame_attr(&g_ide, 1, attr, (int)sizeof(attr));
    TEST_ASSERT_EQUAL((int)strlen(g_row), n);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_GUTTER, attr[0]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_GUTTER, attr[4]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_KEYWORD, attr[5]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_STRING, attr[9]);
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_NUMBER, attr[14]);
    /* the cursor mark on a blank is plain text */
    g_ide.col = 3;
    osui_ide_frame_row(&g_ide, 1, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL('_', g_row[8]);
    osui_ide_frame_attr(&g_ide, 1, attr, (int)sizeof(attr));
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, attr[8]);
    /* title, status and '~' rows */
    osui_ide_frame_attr(&g_ide, 0, attr, (int)sizeof(attr));
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_TEXT, attr[0]);
    osui_ide_frame_attr(&g_ide, 3, attr, (int)sizeof(attr));
    TEST_ASSERT_EQUAL(OSUI_IDE_TOK_GUTTER, attr[3]);
    TEST_ASSERT_EQUAL(-1, osui_ide_frame_attr(&g_ide, 1, 0, 4));
}

static void test_ide_completion(void) {
    osui_ide_open(&g_ide, "c.pm", "set counter to 1\nset colour to 2\n", 33);
    g_ide.row = 1; g_ide.col = (int)strlen(g_ide.lines[1]);
    osui_ide_key(&g_ide, '\n');
    type_str("sa");
    TEST_ASSERT_EQUAL(2, osui_ide_complete(&g_ide));          /* say, says */
    TEST_ASSERT_EQUAL_STRING("say", g_ide.lines[2]);         /* common prefix */
    TEST_ASSERT_EQUAL_STRING("complete: say says", g_ide.status);
    type_str(" co");
    /* co: content, counter, colour -> common prefix "co", list shown */
    TEST_ASSERT_EQUAL(3, osui_ide_complete(&g_ide));
    TEST_ASSERT_EQUAL_STRING("say co", g_ide.lines[2]);
    TEST_ASSERT_EQUAL_STRING("complete: content counter colour", g_ide.status);
    type_str("u");
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_key(&g_ide, '\t'));  /* Tab key */
    TEST_ASSERT_EQUAL_STRING("say counter", g_ide.lines[2]);
    TEST_ASSERT_EQUAL_STRING("completed counter", g_ide.status);
    TEST_ASSERT_EQUAL(2, g_ide.completions);
    /* the word being typed is not its own candidate; unknown prefix */
    type_str(" zz");
    TEST_ASSERT_EQUAL(0, osui_ide_complete(&g_ide));
    TEST_ASSERT_EQUAL_STRING("no completion for zz", g_ide.status);
    /* no word before the cursor: Tab indents with two spaces */
    osui_ide_key(&g_ide, '\n');
    osui_ide_key(&g_ide, '\t');
    TEST_ASSERT_EQUAL_STRING("  ", g_ide.lines[3]);
    /* words inside strings and comments are not offered */
    osui_ide_open(&g_ide, "s.pm", "# zebra\nsay \"zulu\"\n", 19);
    g_ide.row = 1; g_ide.col = (int)strlen(g_ide.lines[1]);
    type_str(" z");
    TEST_ASSERT_EQUAL(0, osui_ide_complete(&g_ide));
    /* a full line keeps its "line full" status */
    osui_ide_open(&g_ide, "f.pm", "", 0);
    while ((int)strlen(g_ide.lines[0]) < OSUI_IDE_COLS - 2) osui_ide_key(&g_ide, ' ');
    type_str("sa");                                    /* say/says: "y" does not fit */
    osui_ide_complete(&g_ide);
    TEST_ASSERT_EQUAL_STRING("line full", g_ide.status);
}

static void test_ide_output_pane(void) {
    int i;
    osui_ide_open(&g_ide, "o.pm", "say 1\n", 6);
    TEST_ASSERT_EQUAL(OSUI_IDE_VIEW, osui_ide_text_rows(&g_ide));
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    osui_ide_key(&g_ide, 'o');
    TEST_ASSERT_EQUAL_STRING("no program output yet: ESC r runs", g_ide.status);
    osui_ide_out_append(&g_ide, "hello");
    osui_ide_out_append(&g_ide, "two\nlines\n");
    TEST_ASSERT_EQUAL(3, g_ide.out_n);
    TEST_ASSERT_EQUAL(1, g_ide.out_visible);
    TEST_ASSERT_EQUAL(OSUI_IDE_VIEW - 1 - OSUI_IDE_OUT_VIEW, osui_ide_text_rows(&g_ide));
    osui_ide_frame_row(&g_ide, osui_ide_text_rows(&g_ide) + 1, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("---- output 3 lines (ESC o hides) ----", g_row);
    osui_ide_frame_row(&g_ide, osui_ide_text_rows(&g_ide) + 2, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("    : hello", g_row);
    osui_ide_frame_row(&g_ide, osui_ide_text_rows(&g_ide) + 4, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("    : lines", g_row);
    osui_ide_frame_row(&g_ide, osui_ide_text_rows(&g_ide) + 5, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("    :", g_row);
    {
        unsigned char attr[OSUI_IDE_ROW_LEN];
        osui_ide_frame_attr(&g_ide, osui_ide_text_rows(&g_ide) + 1, attr, (int)sizeof(attr));
        TEST_ASSERT_EQUAL(OSUI_IDE_TOK_RULE, attr[0]);
        osui_ide_frame_attr(&g_ide, osui_ide_text_rows(&g_ide) + 2, attr, (int)sizeof(attr));
        TEST_ASSERT_EQUAL(OSUI_IDE_TOK_GUTTER, attr[0]);
        TEST_ASSERT_EQUAL(OSUI_IDE_TOK_OUTPUT, attr[6]);
    }
    /* the last OSUI_IDE_OUT_VIEW lines are shown, OSUI_IDE_OUT_LINES kept */
    for (i = 0; i < 20; i++) osui_ide_out_append(&g_ide, i & 1 ? "odd" : "even");
    TEST_ASSERT_EQUAL(OSUI_IDE_OUT_LINES, g_ide.out_n);
    TEST_ASSERT_EQUAL(23, g_ide.out_total);
    osui_ide_frame_row(&g_ide, osui_ide_text_rows(&g_ide) + 1 + OSUI_IDE_OUT_VIEW, g_row, (int)sizeof(g_row));
    TEST_ASSERT_EQUAL_STRING("    : odd", g_row);
    /* ESC o hides and shows; the cursor stays visible above the pane */
    for (i = 0; i < 15; i++) osui_ide_key(&g_ide, '\n');
    TEST_ASSERT_TRUE(g_ide.row - g_ide.top < osui_ide_text_rows(&g_ide));
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    osui_ide_key(&g_ide, 'o');
    TEST_ASSERT_EQUAL(0, g_ide.out_visible);
    TEST_ASSERT_EQUAL(OSUI_IDE_VIEW, osui_ide_text_rows(&g_ide));
    osui_ide_key(&g_ide, OSUI_IDE_KEY_ESC);
    osui_ide_key(&g_ide, 'o');
    TEST_ASSERT_EQUAL(1, g_ide.out_visible);
    TEST_ASSERT_TRUE(g_ide.row - g_ide.top < osui_ide_text_rows(&g_ide));
    osui_ide_out_clear(&g_ide);
    TEST_ASSERT_EQUAL(0, g_ide.out_n);
    TEST_ASSERT_EQUAL(OSUI_IDE_VIEW, osui_ide_text_rows(&g_ide));
    /* control characters are blanked */
    osui_ide_out_append(&g_ide, "a\tb");
    TEST_ASSERT_EQUAL_STRING("a b", g_ide.out[0]);
}

/* Lot 46: a refused Save As puts the previous name back (window title). */
static void test_ide_save_as_failed_restores_name(void) {
    int i;
    osui_ide_open_plain(&g_ide, "a.txt", "x\n", 2);
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_tool(&g_ide, OSUI_IDE_T_SAVE_AS));
    for (i = 0; i < 60; i++) (void)osui_ide_key(&g_ide, '\b');
    type_str("b.txt");
    TEST_ASSERT_EQUAL(OSUI_IDE_SAVE_AS, osui_ide_key(&g_ide, '\n'));
    TEST_ASSERT_EQUAL_STRING("b.txt", g_ide.file);
    osui_ide_save_as_failed(&g_ide);
    TEST_ASSERT_EQUAL_STRING("a.txt", g_ide.file);
    osui_ide_save_as_failed(&g_ide);   /* idempotent */
    TEST_ASSERT_EQUAL_STRING("a.txt", g_ide.file);
}

int main(void) {
    unity_init();
    RUN_TEST(test_ide_open_and_text_roundtrip);
    RUN_TEST(test_ide_editing_keys);
    RUN_TEST(test_ide_command_keys);
    RUN_TEST(test_ide_frame_and_scrolling);
    RUN_TEST(test_ide_limits);
    RUN_TEST(test_ide_syntax_highlight);
    RUN_TEST(test_ide_frame_attr);
    RUN_TEST(test_ide_completion);
    RUN_TEST(test_ide_output_pane);
    RUN_TEST(test_ide_save_as_failed_restores_name);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
