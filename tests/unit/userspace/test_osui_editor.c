/* Desktop text editor: plain mode of the pm-ide editor core (osui_ide.c). */
#include "../../framework/unity.h"
#include "osui_ide.h"
#include <string.h>

static osui_ide_t E;
static char T[OSUI_IDE_TEXT_MAX + 1];
static osui_app_view_t V;

static void type(const char *s) { while (*s) osui_ide_key(&E, *s++); }
static void esc(void) { osui_ide_key(&E, 27); }
static int cmd(int k) { osui_ide_key(&E, 27); return osui_ide_key(&E, k); }
static const char *text(void) { osui_ide_save_text(&E, T, (int)sizeof(T)); return T; }

static void test_open_edit_save_text(void) {
    osui_ide_open_plain(&E, "/home/a.txt", "un\ndeux\n", 8);
    TEST_ASSERT_EQUAL(1, E.plain);
    TEST_ASSERT_EQUAL(2, E.nlines);
    osui_ide_key(&E, 0x13);                       /* End */
    type(" !");
    TEST_ASSERT_EQUAL_STRING("un !\ndeux\n", text());
    TEST_ASSERT_EQUAL(1, E.modified);
    osui_ide_key(&E, 0x12);                       /* Home */
    osui_ide_key(&E, 0x14);                       /* Delete */
    TEST_ASSERT_EQUAL_STRING("n !\ndeux\n", text());
    TEST_ASSERT_EQUAL(OSUI_IDE_SAVE, cmd('w'));
    /* empty buffer saves as 0 bytes */
    osui_ide_open_plain(&E, "/home/e.txt", "", 0);
    TEST_ASSERT_EQUAL(0, osui_ide_save_text(&E, T, (int)sizeof(T)));
    /* PromptMessage keys of the IDE are not changed by the plain mode */
    osui_ide_open(&E, "x.pm", "", 0);
    TEST_ASSERT_EQUAL(OSUI_IDE_RUN, cmd('r'));
    (void)esc;
}

static void test_selection_cut_copy_paste(void) {
    osui_ide_open_plain(&E, "/t", "alpha beta\ngamma\n", 17);
    cmd('m');                                      /* mark at 0,0 */
    { int i; for (i = 0; i < 5; i++) osui_ide_key(&E, 29); }
    TEST_ASSERT_EQUAL(5, osui_ide_selection(&E, T, (int)sizeof(T)));
    TEST_ASSERT_EQUAL_STRING("alpha", T);
    cmd('c');
    TEST_ASSERT_EQUAL_STRING("alpha", osui_ide_clipboard());
    osui_ide_key(&E, 31);                          /* down: selection grows over lines */
    TEST_ASSERT_EQUAL_STRING("alpha beta\ngamma", (osui_ide_selection(&E, T, (int)sizeof(T)), T));
    cmd('x');
    TEST_ASSERT_EQUAL_STRING("", text());   /* empty buffer: 0 bytes */
    TEST_ASSERT_EQUAL_STRING("alpha beta\ngamma", osui_ide_clipboard());
    cmd('v');
    TEST_ASSERT_EQUAL_STRING("alpha beta\ngamma\n", text());
    /* typing replaces a selection */
    osui_ide_drag(&E, 0, 5 + 6, 0, 5 + 10);        /* gutter is 5 columns: "beta" */
    TEST_ASSERT_EQUAL_STRING("beta", (osui_ide_selection(&E, T, (int)sizeof(T)), T));
    type("B");
    TEST_ASSERT_EQUAL_STRING("alpha B\ngamma\n", text());
    /* render: selection class, caret, dirty title, line numbers */
    osui_ide_drag(&E, 1, 5, 1, 8);
    osui_ide_render_app(&E, &V);
    TEST_ASSERT_EQUAL_STRING("   1 alpha B", V.rows[0]);
    TEST_ASSERT_EQUAL_STRING("   2 gamma", V.rows[1]);
    TEST_ASSERT_EQUAL(OSUI_APP_A_SEL, V.attr[1][5]);
    TEST_ASSERT_EQUAL(OSUI_APP_A_TEXT, V.attr[1][8]);
    TEST_ASSERT_EQUAL(1, V.cur_row);
    TEST_ASSERT_EQUAL(8, V.cur_col);
    TEST_ASSERT(strstr(V.title, "/t *") != 0);
    TEST_ASSERT_EQUAL_STRING("   ~", V.rows[2]);
}

static void test_undo_redo(void) {
    osui_ide_open_plain(&E, "/u", "", 0);
    type("abc");
    osui_ide_key(&E, '\n');
    type("de");
    TEST_ASSERT_EQUAL_STRING("abc\nde\n", text());
    cmd('u');
    TEST_ASSERT_EQUAL_STRING("abc\n\n", text());   /* "de" typed as one step */
    cmd('u');
    TEST_ASSERT_EQUAL_STRING("abc\n", text());
    cmd('u');
    TEST_ASSERT_EQUAL_STRING("", text());
    cmd('u');
    TEST_ASSERT(strstr(E.status, "rien a annuler") != 0);
    cmd('y');
    TEST_ASSERT_EQUAL_STRING("abc\n", text());
    osui_ide_tool(&E, OSUI_IDE_T_REDO);
    osui_ide_tool(&E, OSUI_IDE_T_REDO);
    TEST_ASSERT_EQUAL_STRING("abc\nde\n", text());
    /* a new edit clears redo */
    cmd('u');
    type("X");
    cmd('y');
    TEST_ASSERT(strstr(E.status, "rien a retablir") != 0);
}

static void test_search_and_prompts(void) {
    osui_ide_open_plain(&E, "/s", "foo bar\nbaz foo\n", 16);
    cmd('f'); type("foo"); osui_ide_key(&E, '\n');
    TEST_ASSERT_EQUAL(0, E.row); TEST_ASSERT_EQUAL(3, E.col);
    TEST_ASSERT(strstr(E.status, "trouve ligne 1") != 0);
    cmd('n');
    TEST_ASSERT_EQUAL(1, E.row); TEST_ASSERT_EQUAL(7, E.col);
    cmd('n');                                       /* wraps */
    TEST_ASSERT_EQUAL(0, E.row);
    cmd('f'); { int i; for (i = 0; i < 3; i++) osui_ide_key(&E, '\b'); } type("zzz"); osui_ide_key(&E, '\n');
    TEST_ASSERT(strstr(E.status, "introuvable") != 0);
    /* save as */
    cmd('a');
    osui_ide_render_app(&E, &V);
    TEST_ASSERT(strstr(V.status, "Enregistrer sous: /s_") != 0);
    osui_ide_key(&E, '2');
    TEST_ASSERT_EQUAL(OSUI_IDE_SAVE_AS, osui_ide_key(&E, '\n'));
    TEST_ASSERT_EQUAL_STRING("/s2", E.file);
    /* open: refused while modified */
    type("x");
    osui_ide_tool(&E, OSUI_IDE_T_OPEN); type("/o"); 
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, osui_ide_key(&E, '\n'));
    TEST_ASSERT(strstr(E.status, "non enregistre") != 0);
}

static void test_close_prompt_on_unsaved(void) {
    osui_ide_open_plain(&E, "/q", "a\n", 2);
    TEST_ASSERT_EQUAL(OSUI_IDE_QUIT, cmd('q'));
    type("b");
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, cmd('q'));
    TEST_ASSERT(strstr(E.status, "non enregistre") != 0);
    TEST_ASSERT_EQUAL(OSUI_IDE_QUIT, cmd('q'));      /* second time: discard */
    osui_ide_open_plain(&E, "/q", "a\n", 2);
    type("b");
    TEST_ASSERT_EQUAL(OSUI_IDE_QUIT, cmd('Q'));
}

static void test_truncated_file_not_saved_over(void) {
    static char big[200];
    int i;
    for (i = 0; i < 199; i++) big[i] = 'x';
    big[199] = '\n';
    osui_ide_open_plain(&E, "/big", big, 200);
    TEST_ASSERT_EQUAL(1, E.truncated);
    TEST_ASSERT_EQUAL(OSUI_IDE_NONE, cmd('w'));
    TEST_ASSERT(strstr(E.status, "tronque") != 0);
}

static void test_click_places_caret(void) {
    osui_ide_open_plain(&E, "/c", "hello\nworld\n", 12);
    osui_ide_click(&E, 1, 5 + 3);
    TEST_ASSERT_EQUAL(1, E.row); TEST_ASSERT_EQUAL(3, E.col);
    osui_ide_click(&E, 9, 70);                       /* past the end: last line, end */
    TEST_ASSERT_EQUAL(1, E.row); TEST_ASSERT_EQUAL(5, E.col);
    osui_ide_click(&E, 0, 1);                        /* in the gutter: column 0 */
    TEST_ASSERT_EQUAL(0, E.col);
}

/* UX4: the shared desktop clipboard (kernel): copy pushes, paste pulls
 * what the terminal or the explorer copied in between. */
static char g_shared[4097];
static int g_shared_n, g_puts;
static int fake_put(const char *t, int n) { memcpy(g_shared, t, (size_t)n); g_shared[n] = 0; g_shared_n = n; g_puts++; return n; }
static int fake_take(char *o, int max) { int n = g_shared_n < max - 1 ? g_shared_n : max - 1; memcpy(o, g_shared, (size_t)n); o[n] = 0; return n; }
static void test_shared_clipboard_hooks(void) {
    osui_ide_set_clip_ops(fake_put, fake_take);
    osui_ide_open_plain(&E, "/t", "un deux\n", 8);
    cmd('m');
    osui_ide_key(&E, 29); osui_ide_key(&E, 29);
    cmd('c');
    TEST_ASSERT_EQUAL(1, g_puts);
    TEST_ASSERT_EQUAL_STRING("un", g_shared);
    /* the terminal selection replaced it: paste brings the new text */
    strcpy(g_shared, "/home/notes.txt"); g_shared_n = 15;
    osui_ide_click(&E, 0, 5 + 7);                  /* caret at the end, no selection */
    cmd('v');
    TEST_ASSERT_EQUAL_STRING("un deux/home/notes.txt\n", text());
    osui_ide_set_clip_ops(0, 0);
}

int main(void) {
    unity_init();
    RUN_TEST(test_open_edit_save_text);
    RUN_TEST(test_selection_cut_copy_paste);
    RUN_TEST(test_undo_redo);
    RUN_TEST(test_search_and_prompts);
    RUN_TEST(test_close_prompt_on_unsaved);
    RUN_TEST(test_truncated_file_not_saved_over);
    RUN_TEST(test_click_places_caret);
    RUN_TEST(test_shared_clipboard_hooks);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
