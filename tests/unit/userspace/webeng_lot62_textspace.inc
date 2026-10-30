/* Lot 62 round 2: letter-spacing, word-spacing and text-align: justify. */
static int l62_tx(const char *text) {
    const os_fb_web_item_t *it = find_text(text);
    return it ? it->x : -9999;                            /* a missing run fails, not crashes */
}

static int l62_ty(const char *text) {
    const os_fb_web_item_t *it = find_text(text);
    return it ? it->y : -9999;
}

static void test_l62_textspace_justify_soft_lines(void) {
    lot52_render(L62_BODY "<div style='width:100px;text-align:justify'>aa bb cc dd ee ff gg hh</div></body>", 320);
    TEST_ASSERT_EQUAL(0, l62_tx("aa"));
    TEST_ASSERT_EQUAL(28, l62_tx("bb"));            /* 12 free px over 3 spaces */
    TEST_ASSERT_EQUAL(56, l62_tx("cc"));
    TEST_ASSERT_EQUAL(84, l62_tx("dd"));
    TEST_ASSERT_NOT_NULL(find_text("ee ff gg hh"));       /* the last line stays at the start */
    TEST_ASSERT_EQUAL(0, l62_tx("ee ff gg hh"));
    lot52_render(L62_BODY "<div style='width:101px;text-align:justify'>aa bb cc dd ee</div></body>", 320);
    TEST_ASSERT_EQUAL(28, l62_tx("bb"));            /* 13 px: 4, 4, then 5 */
    TEST_ASSERT_EQUAL(56, l62_tx("cc"));
    TEST_ASSERT_EQUAL(85, l62_tx("dd"));
}

static void test_l62_textspace_justify_exceptions(void) {
    lot52_render(L62_BODY "<div style='width:100px;text-align:justify'>aa bb<br>cc dd ee ff gg hh</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("aa bb"));             /* a forced break is not justified */
    TEST_ASSERT_EQUAL(28, l62_tx("dd"));
    lot52_render(L62_BODY "<div style='width:100px;text-align:justify'>aaaaaaaaaaaaaaa</div>"
                 "<div style='width:60px;text-align:justify;word-break:break-all'>aaaaaaaaaa</div>"
                 "<div style='width:100px;text-align:justify'>aa <span style='display:inline-block;vertical-align:baseline'>X</span> bb cc dd ee ff gg hh</div></body>", 320);
    TEST_ASSERT_EQUAL(0, l62_tx("aaaaaaaaaaaaaaa")); /* no space to stretch */
    TEST_ASSERT_EQUAL(0, l62_tx("aaaaaaa"));
    TEST_ASSERT_EQUAL(0, l62_tx("aa"));
    TEST_ASSERT_EQUAL(24, l62_tx("bb cc dd"));       /* a line with an atomic box stays as is */
    lot52_render(L62_BODY "<div style='width:60px;text-align:justify;word-break:break-all'>aa bbbbbbbbbb</div></body>", 320);
    TEST_ASSERT_EQUAL(28, l62_tx("bbbb"));           /* a broken word wraps softly */
    lot52_render(L62_BODY "<div id=j style='text-align:justify'>x</div></body>", 320);
    g_out[0] = 0;
    we_inspect(&g_doc, we_find_id(&g_doc, "j"), cap, 0);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "align justify"));
}

static void test_l62_textspace_letter_spacing(void) {
    lot52_render(L62_BODY "<p style='letter-spacing:2px;margin:0'>AB C</p></body>", 320);
    TEST_ASSERT_EQUAL(3, (int)g_doc.nitems);              /* one TEXT per glyph */
    TEST_ASSERT_EQUAL(0, l62_tx("A"));
    TEST_ASSERT_EQUAL(10, l62_tx("B"));
    TEST_ASSERT_EQUAL(30, l62_tx("C"));             /* the space takes the 2 px too */
    lot52_render(L62_BODY "<div style='width:76px;letter-spacing:2px'>abcd efgh</div>"
                 "<div style='width:76px'>abcd efgh</div></body>", 320);
    TEST_ASSERT_EQUAL(10, l62_ty("e") - l62_ty("a"));  /* 90 px: wraps */
    TEST_ASSERT_NOT_NULL(find_text("abcd efgh"));         /* 72 px fit without spacing */
    lot52_render(L62_BODY "<span id=s style='display:inline-block;letter-spacing:4px'>AB</span>"
                 "<div style='width:60px;text-align:center;letter-spacing:2px'>AB</div></body>", 320);
    TEST_ASSERT_EQUAL(24, lot52_node("s")->w);            /* intrinsic width 2 x 12 */
    lot52_render(L62_BODY "<span id=s style='display:inline-block;word-spacing:4px'>a b</span></body>", 320);
    TEST_ASSERT_EQUAL(28, lot52_node("s")->w);            /* 8 + 12 + 8 */
    lot52_render(L62_BODY "<span id=s style='display:inline-block;letter-spacing:4px'>AB</span>"
                 "<div style='width:60px;text-align:center;letter-spacing:2px'>AB</div></body>", 320);
    TEST_ASSERT_EQUAL(20, g_doc.items[2].x);              /* centred over 20 px */
    TEST_ASSERT_EQUAL(30, g_doc.items[3].x);
}

static void test_l62_textspace_word_spacing_and_decorations(void) {
    lot52_render(L62_BODY "<p style='word-spacing:4px;margin:0'>ab cd</p></body>", 320);
    TEST_ASSERT_EQUAL(2, (int)g_doc.nitems);              /* a wider space splits the run */
    TEST_ASSERT_EQUAL(28, l62_tx("cd"));
    lot52_render(L62_BODY "<p style='word-spacing:-8px;letter-spacing:-4px;margin:0'>ab cd</p></body>", 320);
    TEST_ASSERT_EQUAL(4, l62_tx("b"));              /* 4 px cells */
    TEST_ASSERT_EQUAL(8, l62_tx("c"));              /* the space clamps to 0 */
    lot52_render(L62_BODY "<p style='letter-spacing:2px;margin:0;text-decoration:underline'>ab</p></body>", 320);
    l61_rect_at(0, 9, 20, 1, 0x000000);                   /* the line spans both cells */
}

static void test_l62_textspace_cascade_and_refusals(void) {
    lot52_render(L62_BODY "<div id=a style='letter-spacing:3px;word-spacing:5px'><span id=b>x</span>"
                 "<span id=c style='letter-spacing:normal;word-spacing:normal'>y</span></div>"
                 "<div id=d style='letter-spacing:2px;letter-spacing:17px;letter-spacing:-5px;letter-spacing:auto;"
                 "word-spacing:1px;word-spacing:33px;word-spacing:-9px'></div></body>", 320);
    TEST_ASSERT_EQUAL(3, lot52_node("b")->letter_spacing); /* inherited */
    TEST_ASSERT_EQUAL(5, lot52_node("b")->word_spacing);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->letter_spacing);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->word_spacing);
    TEST_ASSERT_EQUAL(2, lot52_node("d")->letter_spacing); /* refused values keep the previous one */
    TEST_ASSERT_EQUAL(1, lot52_node("d")->word_spacing);
    TEST_ASSERT_EQUAL(3, l61_diag_count("letter-spacing refused"));
    TEST_ASSERT_EQUAL(2, l61_diag_count("word-spacing refused"));
    lot52_render(L62_BODY "<div id=e style='letter-spacing:16px;word-spacing:32px'></div><div id=f style='letter-spacing:-4px;word-spacing:-8px'></div></body>", 320);
    TEST_ASSERT_EQUAL(16, lot52_node("e")->letter_spacing);
    TEST_ASSERT_EQUAL(32, lot52_node("e")->word_spacing);
    TEST_ASSERT_EQUAL(-4, lot52_node("f")->letter_spacing);
    TEST_ASSERT_EQUAL(-8, lot52_node("f")->word_spacing);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l62_textspace_align_last_and_text_justify(void) {
    lot52_render(L62_BODY "<div style='width:100px;text-align:justify;text-align-last:right'>aa bb cc dd ee ff</div>"
                 "<div style='width:100px;text-align:justify;text-align-last:justify'>gg hh<br>ii jj</div>"
                 "<div style='width:100px;text-align-last:center'>kk</div></body>", 320);
    TEST_ASSERT_EQUAL(28, l62_tx("bb"));                  /* the soft line is justified */
    TEST_ASSERT_EQUAL(60, l62_tx("ee ff"));               /* the last line goes right */
    TEST_ASSERT_EQUAL(84, l62_tx("hh"));                  /* a forced break and the end justify */
    TEST_ASSERT_EQUAL(84, l62_tx("jj"));
    TEST_ASSERT_EQUAL(42, l62_tx("kk"));                  /* one-line block: its last line */
    lot52_render(L62_BODY "<div style='width:100px;text-align:justify;text-justify:none'>aa bb cc dd ee</div>"
                 "<div style='width:100px;text-align:end;text-align-last:start'>ff gg hh ii jj kk</div>"
                 "<div style='width:100px;text-align:center;text-align-last:auto'>ll</div></body>", 320);
    TEST_ASSERT_NOT_NULL(find_text("aa bb cc dd"));       /* text-justify none: start */
    TEST_ASSERT_EQUAL(12, l62_tx("ff gg hh ii"));         /* end is right */
    TEST_ASSERT_EQUAL(0, l62_tx("jj kk"));                /* start is left */
    TEST_ASSERT_EQUAL(42, l62_tx("ll"));                  /* auto keeps text-align */
    TEST_ASSERT_EQUAL(0, l61_diag_count("refused"));      /* auto, start and none accepted */
}

static void test_l62_textspace_align_last_cascade(void) {
    lot52_render(L62_BODY "<div id=a style='text-align-last:center;text-justify:none'><p id=b>x</p></div>"
                 "<div id=c style='text-align-last:justify;text-align-last:middle;text-justify:auto;text-justify:distribute'></div>"
                 "<div id=e style='text-justify:none;text-justify:inter-word'></div></body>", 320);
    TEST_ASSERT_EQUAL(1 + WE_ALIGN_CENTER, lot52_node("b")->align_last);  /* inherited */
    TEST_ASSERT_EQUAL(1, lot52_node("b")->justify_none);
    TEST_ASSERT_EQUAL(1 + WE_ALIGN_JUSTIFY, lot52_node("c")->align_last);
    TEST_ASSERT_EQUAL(0, lot52_node("c")->justify_none);
    TEST_ASSERT_EQUAL(0, lot52_node("e")->justify_none);
    TEST_ASSERT_EQUAL(1, l61_diag_count("text-align-last refused"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("text-justify refused"));
}

static void lot62_textspace_run(void) {
    RUN_TEST(test_l62_textspace_align_last_and_text_justify);
    RUN_TEST(test_l62_textspace_align_last_cascade);
    RUN_TEST(test_l62_textspace_justify_soft_lines);
    RUN_TEST(test_l62_textspace_justify_exceptions);
    RUN_TEST(test_l62_textspace_letter_spacing);
    RUN_TEST(test_l62_textspace_word_spacing_and_decorations);
    RUN_TEST(test_l62_textspace_cascade_and_refusals);
}
