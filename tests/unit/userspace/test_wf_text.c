/* Lot 71: display character set (include/os_charset.h), character
 * references and page charsets (userspace/wf_entity.c), accents kept by
 * the reader view (wf_html.c) and forms recorded by it. */
#include "../../framework/unity.h"
#include "../../../include/os_charset.h"
#include "../../../userspace/wf_entity.h"
#include "../../../userspace/wf_html.h"
#include <string.h>
#include <stdio.h>

static char g_out[65536];

static void test_glyph_index(void) {
    TEST_ASSERT_EQUAL(0, os_glyph_index(' '));
    TEST_ASSERT_EQUAL('A' - 32, os_glyph_index('A'));
    TEST_ASSERT_EQUAL(94, os_glyph_index('~'));
    TEST_ASSERT_EQUAL(OS_GLYPH_QMARK, os_glyph_index(0x7F));
    TEST_ASSERT_EQUAL(OS_GLYPH_QMARK, os_glyph_index(0x01));
    TEST_ASSERT_EQUAL(95, os_glyph_index(0x80));                 /* euro */
    TEST_ASSERT_EQUAL(OS_GLYPH_QMARK, os_glyph_index(0x81));     /* undefined in CP1252 */
    TEST_ASSERT_EQUAL(95 + 0x1C, os_glyph_index(0x9C));          /* oe */
    TEST_ASSERT_EQUAL(127, os_glyph_index(0xA0));
    TEST_ASSERT_EQUAL(127 + 0x49, os_glyph_index(0xE9));         /* e acute */
    TEST_ASSERT_EQUAL(OS_GLYPH_COUNT - 1, os_glyph_index(0xFF));
    TEST_ASSERT_TRUE(os_display_printable(0xE9));
    TEST_ASSERT_FALSE(os_display_printable(0x8D));
    TEST_ASSERT_FALSE(os_display_printable('\n'));
}

static void test_cp1252_mapping(void) {
    TEST_ASSERT_EQUAL(0xE9, os_cp1252_from_cp(0xE9));
    TEST_ASSERT_EQUAL(0x9C, os_cp1252_from_cp(0x153));
    TEST_ASSERT_EQUAL(0x8C, os_cp1252_from_cp(0x152));
    TEST_ASSERT_EQUAL(0x80, os_cp1252_from_cp(0x20AC));
    TEST_ASSERT_EQUAL(0x92, os_cp1252_from_cp(0x2019));
    TEST_ASSERT_EQUAL(0x85, os_cp1252_from_cp(0x2026));
    TEST_ASSERT_EQUAL('l', os_cp1252_from_cp(0x142));            /* l stroke: base letter */
    TEST_ASSERT_EQUAL('S', os_cp1252_from_cp(0x15A));
    TEST_ASSERT_EQUAL('z', os_cp1252_from_cp(0x17C));
    TEST_ASSERT_EQUAL('?', os_cp1252_from_cp(0x4E2D));           /* CJK */
    TEST_ASSERT_EQUAL(0, os_cp1252_from_cp(0x200B));             /* zero width: dropped */
    TEST_ASSERT_EQUAL(0, os_cp1252_from_cp(0x07));
    TEST_ASSERT_EQUAL('-', os_cp1252_from_cp(0x2212));
}

static void test_utf8_decode(void) {
    int i = 0;
    const char *s = "\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80" "a";
    TEST_ASSERT_EQUAL(0xE9, os_utf8_next(s, 10, &i));
    TEST_ASSERT_EQUAL(2, i);
    TEST_ASSERT_EQUAL(0x20AC, os_utf8_next(s, 10, &i));
    TEST_ASSERT_EQUAL(0x1F600, os_utf8_next(s, 10, &i));
    TEST_ASSERT_EQUAL('a', os_utf8_next(s, 10, &i));
    i = 0;
    TEST_ASSERT_EQUAL(OS_UNICODE_BAD, os_utf8_next("\xc0\xaf", 2, &i));   /* overlong */
    TEST_ASSERT_EQUAL(1, i);
    i = 0;
    TEST_ASSERT_EQUAL(OS_UNICODE_BAD, os_utf8_next("\xed\xa0\x80", 3, &i));   /* surrogate */
    i = 0;
    TEST_ASSERT_EQUAL(OS_UNICODE_BAD, os_utf8_next("\xe2\x82", 2, &i));   /* cut */
}

static void test_text_to_display(void) {
    char o[64];
    TEST_ASSERT_EQUAL(8, os_text_to_display("caf\xc3\xa9 \xc5\x93uf", 10, o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("caf\xe9 \x9cuf", o);
    /* idempotent on display bytes: a lone 0xE9 stays */
    os_text_to_display("caf\xe9!", 5, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("caf\xe9!", o);
    /* bounded */
    TEST_ASSERT_EQUAL(3, os_text_to_display("abcdef", 6, o, 4));
    TEST_ASSERT_EQUAL_STRING("abc", o);
    strcpy(o, "\xc3\x80 bient\xc3\xb4t\x01");
    TEST_ASSERT_EQUAL(9, os_text_to_display_inplace(o, (int)sizeof(o)));
    TEST_ASSERT_EQUAL_STRING("\xc0 bient\xf4t", o);
}

static void test_stream_and_back(void) {
    os_utf8_stream_t st;
    unsigned char b = 0;
    char u[4];
    os_utf8_stream_reset(&st);
    TEST_ASSERT_EQUAL(0, os_utf8_stream_feed(&st, 0xC3, &b));
    TEST_ASSERT_EQUAL(1, os_utf8_stream_feed(&st, 0xA7, &b));
    TEST_ASSERT_EQUAL(0xE7, b);
    TEST_ASSERT_EQUAL(0, os_utf8_stream_feed(&st, 0xE2, &b));
    TEST_ASSERT_EQUAL(2, os_utf8_stream_feed(&st, 'x', &b));   /* cut: '?' and x again */
    TEST_ASSERT_EQUAL('?', b);
    TEST_ASSERT_EQUAL(1, os_utf8_stream_feed(&st, 'x', &b));
    TEST_ASSERT_EQUAL(2, os_display_to_utf8(0xE9, u, 4));
    TEST_ASSERT_EQUAL_MEMORY("\xc3\xa9", u, 2);
    TEST_ASSERT_EQUAL(3, os_display_to_utf8(0x80, u, 4));
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x82\xac", u, 3);
    TEST_ASSERT_EQUAL(0, os_display_to_utf8(0x80, u, 2));
}

static void test_cp437(void) {
    TEST_ASSERT_EQUAL(0x82, os_cp437_from_cp(0xE9));   /* e acute in the VGA ROM font */
    TEST_ASSERT_EQUAL(0x87, os_cp437_from_cp(0xE7));
    TEST_ASSERT_EQUAL(0x85, os_cp437_from_cp(0xE0));
    TEST_ASSERT_EQUAL('E', os_cp437_from_cp(0xC8));    /* E grave is not in CP437 */
    TEST_ASSERT_EQUAL('o', os_cp437_from_cp(0x153));
    TEST_ASSERT_EQUAL(0xE9, os_cp437_to_display(0x82));
    TEST_ASSERT_EQUAL(0xB0, os_cp437_to_display(0xF8));
    TEST_ASSERT_EQUAL(' ', os_cp437_to_display(0xC4));
}

static void test_entities(void) {
    int used;
    TEST_ASSERT_EQUAL(0xE9, wf_entity_decode("&eacute; x", 10, &used));
    TEST_ASSERT_EQUAL(8, used);
    TEST_ASSERT_EQUAL('&', wf_entity_decode("&amp;", 5, &used));
    TEST_ASSERT_EQUAL(0x27, wf_entity_decode("&apos;", 6, &used));
    TEST_ASSERT_EQUAL(0x153, wf_entity_decode("&oelig;", 7, &used));
    TEST_ASSERT_EQUAL(0x20AC, wf_entity_decode("&euro;", 6, &used));
    TEST_ASSERT_EQUAL(233, wf_entity_decode("&#233;", 6, &used));
    TEST_ASSERT_EQUAL(0xE9, wf_entity_decode("&#xE9;", 6, &used));
    TEST_ASSERT_EQUAL(0xE9, wf_entity_decode("&#Xe9", 5, &used));
    TEST_ASSERT_EQUAL(5, used);
    TEST_ASSERT_EQUAL(0x2019, wf_entity_decode("&#146;", 6, &used));   /* CP1252 rule */
    TEST_ASSERT_EQUAL(OS_UNICODE_BAD, wf_entity_decode("&#0;", 4, &used));
    TEST_ASSERT_EQUAL(OS_UNICODE_BAD, wf_entity_decode("&#xD800;", 8, &used));
    TEST_ASSERT_EQUAL(OS_UNICODE_BAD, wf_entity_decode("&#99999999;", 11, &used));
    TEST_ASSERT_EQUAL(0xE9, wf_entity_decode("&eacute", 7, &used));     /* legacy, no ';' */
    TEST_ASSERT_EQUAL(0xA9, wf_entity_decode("&copy2026", 9, &used));
    TEST_ASSERT_EQUAL(5, used);
    TEST_ASSERT_EQUAL(0, wf_entity_decode("&nosuch;", 8, &used));
    TEST_ASSERT_EQUAL(0, used);
    TEST_ASSERT_EQUAL(0, wf_entity_decode("& x", 3, &used));
    TEST_ASSERT_EQUAL(0, wf_entity_lookup("eacutex", 7));
    TEST_ASSERT_EQUAL(0xC9, wf_entity_lookup("Eacute", 6));
}

static void test_charset(void) {
    TEST_ASSERT_EQUAL(WF_CS_LATIN1, wf_charset_from("text/html; charset=ISO-8859-1", -1));
    TEST_ASSERT_EQUAL(WF_CS_LATIN1, wf_charset_from("text/html;charset=\"windows-1252\"", -1));
    TEST_ASSERT_EQUAL(WF_CS_UTF8, wf_charset_from("text/html; charset=utf-8", -1));
    TEST_ASSERT_EQUAL(WF_CS_UTF8, wf_charset_from("text/html", -1));
    TEST_ASSERT_EQUAL(WF_CS_LATIN1, wf_charset_sniff("<html><head><meta charset=\"iso-8859-1\">", 40));
    TEST_ASSERT_EQUAL(WF_CS_LATIN1, wf_charset_sniff("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">", 76));
    TEST_ASSERT_EQUAL(WF_CS_UTF8, wf_charset_sniff("<meta charset=utf-8>", 20));
    TEST_ASSERT_EQUAL(-1, wf_charset_sniff("<p>nothing</p>", 14));
}

static void test_text_display(void) {
    char o[64];
    wf_text_display("  l&#39;&eacute;t&eacute;\n  &agrave; Paris ", 44, WF_CS_UTF8, 1, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("l'\xe9t\xe9 \xe0 Paris", o);
    wf_text_display("caf\xe9", 4, WF_CS_LATIN1, 1, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("caf\xe9", o);
    wf_text_display("\x93q\x94", 3, WF_CS_LATIN1, 0, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("\x93q\x94", o);
    TEST_ASSERT_EQUAL(4, wf_display_to_utf8("\xe9t\xe9", 3, o, (int)sizeof(o)) - 1);
    TEST_ASSERT_EQUAL_STRING("\xc3\xa9t\xc3\xa9", o);
}

static void test_reduce_accents(void) {
    static const char page[] =
        "<html><head><title>Caf\xc3\xa9 &amp; cr\xc3\xa8me</title></head><body>"
        "<p>L&rsquo;\xc3\xa9t\xc3\xa9 \xc3\xa0 Paris, &laquo;&nbsp;bient\xc3\xb4t&nbsp;&raquo; &oelig;uvre 5&euro; a&lt;b</p>"
        "<p>\xc5\x81\xc3\xb3" "d\xc5\xba \xe4\xb8\xad</p></body></html>";
    wf_html_stats_t st;
    wf_html_set_charset(WF_CS_UTF8);
    wf_html_reduce(page, (int)sizeof(page) - 1, 0, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_EQUAL_STRING("Caf\xe9 & cr\xe8me", st.title);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<title>Caf\xe9 &amp; cr\xe8me</title>"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "L\x92\xe9t\xe9 \xe0 Paris, \xab bient\xf4t \xbb \x9cuvre 5\x80 a&lt;b"));
    {
        /* Lot 72 (was "L\xf3dz ?": base letters): L stroke and z acute
         * keep their glyphs through the Latin Extended-A page */
        unsigned char bl = 0, bz = 0;
        char want[16], u8[32];
        const char *at;
        TEST_ASSERT_EQUAL(1, os_cp_page(0x141, &bl));
        TEST_ASSERT_EQUAL(1, os_cp_page(0x17A, &bz));
        snprintf(want, sizeof(want), "%c%c%c\xf3" "d%c%c ?%c", 0x8D, bl, 0x81, 0x8D, bz, 0x81);
        at = strstr(g_out, want);
        TEST_ASSERT_NOT_NULL(at);
        wf_display_to_utf8(at, (int)strlen(want), u8, (int)sizeof(u8));
        TEST_ASSERT_EQUAL_STRING("\xc5\x81\xc3\xb3" "d\xc5\xba ?", u8);
    }
    TEST_ASSERT_EQUAL(WF_CS_UTF8, st.charset);
}

static void test_reduce_latin1_and_href(void) {
    static const char page[] =
        "<html><head><meta charset=\"iso-8859-1\"><title>\xc9t\xe9</title></head><body>"
        "<p><a href=\"/s?a=1&amp;b=2\">r\xe9sultat</a></p></body></html>";
    wf_html_set_charset(WF_CS_NONE);   /* no charset in the header: the meta decides */
    wf_url_t base;
    wf_html_stats_t st;
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://ex.fr/dir/page", &base));
    wf_html_reduce(page, (int)sizeof(page) - 1, &base, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_EQUAL(WF_CS_LATIN1, st.charset);
    TEST_ASSERT_EQUAL_STRING("\xc9t\xe9", st.title);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<a href=\"https://ex.fr/s?a=1&b=2\">r\xe9sultat</a>"));
}

static void test_reduce_text_console_utf8(void) {
    static const char page[] = "<p>caf&eacute; &amp; cr&egrave;me &#8364; \xc3\xa0</p>";
    wf_html_text(page, (int)sizeof(page) - 1, g_out, (int)sizeof(g_out));
    TEST_ASSERT_EQUAL_STRING("caf\xc3\xa9 & cr\xc3\xa8me \xe2\x82\xac \xc3\xa0\n", g_out);
}

static void test_reduce_forms(void) {
    static const char page[] =
        "<html><head><title>Google</title></head><body>"
        "<form action=\"/search\" method=\"GET\" role=\"search\">"
        "<input type=hidden name=hl value=fr>"
        "<input name=\"q\" maxlength=\"2048\" title=\"Rechercher\" value=\"\">"
        "<input type=\"submit\" name=\"btnG\" value=\"Recherche Google\">"
        "<input type=submit name=btnI value=\"J'ai de la chance\" disabled>"
        "<select name=lr><option value=\"\">tout</option><option value=lang_fr selected>fran&ccedil;ais</option></select>"
        "<input type=checkbox name=safe value=on checked><input type=checkbox name=x>"
        "<textarea name=t>a&lt;b</textarea><button>Envoyer</button><button type=button>Non</button>"
        "</form><p>fin</p></body></html>";
    static wf_form_page_t fp;
    wf_url_t base;
    wf_html_stats_t st;
    TEST_ASSERT_EQUAL(0, wf_url_parse("https://www.google.com/", &base));
    wf_html_reduce_ex(page, (int)sizeof(page) - 1, &base, g_out, (int)sizeof(g_out), &st, 4000, &fp);
    TEST_ASSERT_EQUAL(1, fp.nforms);
    TEST_ASSERT_EQUAL_STRING("https://www.google.com/search", fp.form[0].action);
    TEST_ASSERT_EQUAL(WF_METHOD_GET, fp.form[0].method);
    TEST_ASSERT_EQUAL(8, fp.nfields);   /* hl, q, btnG, lr, safe, x, t, button */
    TEST_ASSERT_EQUAL(WF_FIELD_HIDDEN, fp.field[0].type);
    TEST_ASSERT_EQUAL(WF_FIELD_TEXT, fp.field[1].type);
    TEST_ASSERT_EQUAL_STRING("q", fp.field[1].name);
    TEST_ASSERT_EQUAL_STRING("Rechercher", fp.field[1].label);
    TEST_ASSERT_EQUAL(2048 > WF_FORM_VALUE - 1 ? WF_FORM_VALUE - 1 : 2048, fp.field[1].maxlen);
    TEST_ASSERT_EQUAL(WF_FIELD_SUBMIT, fp.field[2].type);
    TEST_ASSERT_EQUAL(WF_FIELD_SELECT, fp.field[3].type);
    TEST_ASSERT_EQUAL_STRING("lang_fr", fp.field[3].value);
    TEST_ASSERT_EQUAL(2, fp.field[3].nopt);
    TEST_ASSERT_EQUAL_STRING("fran\xe7" "ais", wf_form_option_label(&fp, 3, 1));
    TEST_ASSERT_EQUAL(WF_FIELD_CHECKBOX, fp.field[4].type);
    TEST_ASSERT_EQUAL_STRING("on", fp.field[4].value);
    TEST_ASSERT_EQUAL(1, fp.field[4].checked);
    TEST_ASSERT_EQUAL(0, fp.field[5].checked);
    TEST_ASSERT_EQUAL_STRING("a<b", fp.field[6].value);
    TEST_ASSERT_EQUAL_STRING("Envoyer", fp.field[7].label);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<a href=\"mohhdy-form:f:3\">\001" "3\002</a>"));   /* the select is clickable */
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<a href=\"mohhdy-form:f:1\">\001" "1\002</a>"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<a href=\"mohhdy-form:s:2\">\001" "2\002</a>"));
    TEST_ASSERT_NULL(strstr(g_out, "chance"));
    TEST_ASSERT_NULL(strstr(g_out, "Non"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "<p>fin</p>"));
    /* without a sink the form is dropped as in Lot 67 */
    wf_html_reduce(page, (int)sizeof(page) - 1, &base, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_NULL(strstr(g_out, "mohhdy-form"));
}

static void test_reduce_budget_ex(void) {
    static char page[200000];
    int n = 0, i;
    wf_html_stats_t st;
    n += sprintf(page + n, "<html><body>");
    for (i = 0; i < 1500; i++) n += sprintf(page + n, "<p>paragraphe %d</p>", i);
    n += sprintf(page + n, "</body></html>");
    wf_html_reduce(page, n, 0, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_TRUE(st.truncated);          /* the console budget stays at 140 */
    {
        static char big[256 * 1024];
        wf_html_reduce_ex(page, n, 0, big, (int)sizeof(big), &st, 8000, 0);
        TEST_ASSERT_FALSE(st.truncated);
        TEST_ASSERT_NOT_NULL(strstr(big, "paragraphe 1499"));
    }
}

/* Lot 71: console text of a reduced page: display bytes back to UTF-8 */
static void test_html_text_display(void) {
    static const char red[] = "<html><body><p>caf\xe9 \x9cuvre \x80 &amp; &lt;b&gt;</p></body></html>";
    char o[128];
    wf_html_text_display(red, (int)sizeof(red) - 1, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("caf\xc3\xa9 \xc5\x93uvre \xe2\x82\xac & <b>\n", o);
}


/* Lot 72: Greek and Cyrillic pages through the reader view: the reduced
 * page carries page selectors (every text run ends in page 0), the title
 * too, and the console text is the original UTF-8 again */
/* Lot 72: the HTML 4 names whose code point is in a glyph page (Greek,
 * arrows, math) are decoded like their numeric reference; a name with no
 * glyph (&spades;) is still not decoded */
static void test_named_refs_pages(void) {
    char o[64];
    TEST_ASSERT_EQUAL(0x3B1, (int)wf_entity_lookup("alpha", 5));
    TEST_ASSERT_EQUAL(0x391, (int)wf_entity_lookup("Alpha", 5));
    TEST_ASSERT_EQUAL(0x2192, (int)wf_entity_lookup("rarr", 4));
    TEST_ASSERT_EQUAL(0x221A, (int)wf_entity_lookup("radic", 5));
    TEST_ASSERT_EQUAL(0x3C9, (int)wf_entity_lookup("omega", 5));
    TEST_ASSERT_EQUAL(0x2019, (int)wf_entity_lookup("rsquo", 5));
    TEST_ASSERT_EQUAL(0, (int)wf_entity_lookup("spades", 6));
    wf_text_display("&alpha;&le;x", 12, WF_CS_UTF8, 1, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("\x8f\xe1\x9d\xa2x\x81", o);
}

/* Lot 72: Cyrillic and Greek single-byte page charsets: the meta tag or
 * the header names the charset, the bytes become glyph page letters, the
 * console text is UTF-8 again; an undefined byte (0x98 in windows-1251)
 * is U+FFFD, shown as '?' */
static void test_cyrillic_greek_charsets(void) {
    wf_html_stats_t st;
    char o[256];
    TEST_ASSERT_EQUAL(WF_CS_CP1251, wf_charset_from("text/html; charset=windows-1251", -1));
    TEST_ASSERT_EQUAL(WF_CS_KOI8R, wf_charset_from("KOI8-R", -1));
    TEST_ASSERT_EQUAL(WF_CS_KOI8U, wf_charset_from("koi8-u", -1));
    TEST_ASSERT_EQUAL(WF_CS_ISO8859_5, wf_charset_from("iso-8859-5", -1));
    TEST_ASSERT_EQUAL(WF_CS_ISO8859_7, wf_charset_from("text/html;charset=\"ISO-8859-7\"", -1));
    TEST_ASSERT_EQUAL(WF_CS_CP1253, wf_charset_from("windows-1253", -1));
    TEST_ASSERT_EQUAL(WF_CS_UTF8, wf_charset_from("windows-1250", -1));
    TEST_ASSERT_EQUAL_STRING("koi8-u", wf_charset_name(WF_CS_KOI8U));
    TEST_ASSERT_EQUAL_STRING("utf-8", wf_charset_name(42));
    {
        static const char page[] = "<html><head><meta charset=\"windows-1251\"><title>\xcf\xf0\xe8\xe2\xe5\xf2 \xec\xe8\xf0</title></head>"
                                   "<body><p>\xcf\xf0\xe8\xe2\xe5\xf2 \xec\xe8\xf0 \x88 \x98</p></body></html>";
        wf_html_set_charset(WF_CS_NONE);
        wf_html_reduce(page, (int)sizeof(page) - 1, 0, g_out, (int)sizeof(g_out), &st);
        TEST_ASSERT_EQUAL(WF_CS_CP1251, st.charset);
        TEST_ASSERT_EQUAL_STRING("\x90\xbf\xe0\xd8\xd2\xd5\xe2 \xdc\xd8\xe0\x81", st.title);
        TEST_ASSERT_NOT_NULL(strstr(g_out, "\x90\xbf\xe0\xd8\xd2\xd5\xe2 \xdc\xd8\xe0 \x81\x80 ?"));
        wf_html_text_display(g_out, (int)strlen(g_out), o, (int)sizeof(o));
        TEST_ASSERT_NOT_NULL(strstr(o, "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 \xd0\xbc\xd0\xb8\xd1\x80 \xe2\x82\xac ?"));
    }
    {
        static const char page[] = "<p>\xc5\xeb\xeb\xdc\xe4\xe1</p><p>\xeb\xc9\xa7\xd7</p>";
        wf_html_set_charset(WF_CS_ISO8859_7);
        wf_html_reduce(page, 13, 0, g_out, (int)sizeof(g_out), &st);
        TEST_ASSERT_NOT_NULL(strstr(g_out, "\x8f\xc5\xeb\xeb\xdc\xe4\xe1\x81"));
        wf_html_set_charset(WF_CS_KOI8U);
        wf_html_reduce(page + 13, (int)sizeof(page) - 14, 0, g_out, (int)sizeof(g_out), &st);
        TEST_ASSERT_NOT_NULL(strstr(g_out, "\x90\xba\xd8\xf7\xd2\x81"));
    }
    /* raw text of a koi8-r page (html_text without the reduce step) */
    wf_html_set_charset(WF_CS_KOI8R);
    wf_html_text("<p>\xed\xc9\xd2</p>", 10, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("\xd0\x9c\xd0\xb8\xd1\x80\n", o);
    TEST_ASSERT_EQUAL(0xCF, wf_charset_byte(WF_CS_CP1251, 0x41F));
    TEST_ASSERT_EQUAL(0, wf_charset_byte(WF_CS_CP1251, 0x3B1));
    TEST_ASSERT_EQUAL(0x80, wf_charset_byte(WF_CS_LATIN1, 0x20AC));
    TEST_ASSERT_EQUAL('a', wf_charset_byte(WF_CS_KOI8R, 'a'));
    TEST_ASSERT_EQUAL(0, wf_charset_byte(WF_CS_UTF8, 0xE9));
    wf_html_set_charset(WF_CS_UTF8);
}

static void test_reduce_greek_cyrillic(void) {
    static const char page[] =
        "<html><head><title>\xce\x95\xce\xbb\xce\xbb\xce\xb7\xce\xbd\xce\xb9\xce\xba\xce\xac</title></head><body>"
        "<p>\xce\xb1\xce\xb2\xce\xb3 caf\xc3\xa9</p><p>\xd0\xa2\xd0\x90\xd0\xa1\xd0\xa1 &#x416; \xe2\x86\x92 &amp;</p></body></html>";
    wf_html_stats_t st;
    char o[256];
    wf_html_set_charset(WF_CS_UTF8);
    wf_html_reduce(page, (int)sizeof(page) - 1, 0, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_EQUAL_STRING("\x8f\xc5\xeb\xeb\xe7\xed\xe9\xea\xdc\x81", st.title);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "\x8f\xe1\xe2\xe3 caf\x81\xe9"));
    TEST_ASSERT_NOT_NULL(strstr(g_out, "\x90\xc2\xb0\xc1\xc1 \xb6 \x9d\x83 &amp;\x81</p>"));
    wf_html_text_display(g_out, (int)strlen(g_out), o, (int)sizeof(o));
    TEST_ASSERT_NOT_NULL(strstr(o, "\xce\xb1\xce\xb2\xce\xb3 caf\xc3\xa9\n"));
    TEST_ASSERT_NOT_NULL(strstr(o, "\xd0\xa2\xd0\x90\xd0\xa1\xd0\xa1 \xd0\x96 \xe2\x86\x92 &\n"));
    /* the page is reset at every tag: page 0 bytes after a tag stay CP1252 */
    {
        static const char two[] = "<p>\x8f\xe1</p><p>\xe9</p>";
        wf_html_text_display(two, (int)sizeof(two) - 1, o, (int)sizeof(o));
    }
    TEST_ASSERT_EQUAL_STRING("\xce\xb1\n\xc3\xa9\n", o);
    /* attribute and title text (wf_text_display) and form values back */
    wf_text_display("\xce\xa9 x", 4, WF_CS_UTF8, 1, o, (int)sizeof(o));
    TEST_ASSERT_EQUAL_STRING("\x8f\xd9 x\x81", o);
    wf_text_display("\xce\xa9\xce\xa9\xce\xa9", 6, WF_CS_UTF8, 0, o, 5);   /* room kept for the closing selector */
    TEST_ASSERT_EQUAL_STRING("\x8f\xd9\xd9\x81", o);
    {
        char u[32];
        wf_display_to_utf8("\x8f\xd9\x81\xe9", 4, u, (int)sizeof(u));
        TEST_ASSERT_EQUAL_STRING("\xce\xa9\xc3\xa9", u);
    }
}

/* Lot 71: the Content-Type charset wins over a <meta> (www.google.com
 * sends ISO-8859-1 bytes with a UTF-8 meta) */
static void test_header_charset_wins(void) {
    static const char page[] =
        "<html><head><meta content=\"text/html; charset=UTF-8\" http-equiv=\"Content-Type\"><title>G</title></head>"
        "<body><p>Fran\xe7" "ais</p></body></html>";
    wf_html_stats_t st;
    TEST_ASSERT_EQUAL(WF_CS_LATIN1, wf_charset_header("text/html; charset=ISO-8859-1", -1));
    TEST_ASSERT_EQUAL(WF_CS_NONE, wf_charset_header("text/html", -1));
    wf_html_set_charset(wf_charset_header("text/html; charset=ISO-8859-1", -1));
    wf_html_reduce(page, (int)sizeof(page) - 1, 0, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_NOT_NULL(strstr(g_out, "Fran\xe7" "ais"));
    TEST_ASSERT_EQUAL(WF_CS_LATIN1, st.charset);
    wf_html_set_charset(WF_CS_NONE);
    wf_html_reduce(page, (int)sizeof(page) - 1, 0, g_out, (int)sizeof(g_out), &st);
    TEST_ASSERT_EQUAL(WF_CS_UTF8, st.charset);
}

int main(void) {
    unity_init();
    RUN_TEST(test_glyph_index);
    RUN_TEST(test_cp1252_mapping);
    RUN_TEST(test_utf8_decode);
    RUN_TEST(test_text_to_display);
    RUN_TEST(test_stream_and_back);
    RUN_TEST(test_cp437);
    RUN_TEST(test_entities);
    RUN_TEST(test_charset);
    RUN_TEST(test_text_display);
    RUN_TEST(test_reduce_accents);
    RUN_TEST(test_reduce_latin1_and_href);
    RUN_TEST(test_reduce_text_console_utf8);
    RUN_TEST(test_reduce_forms);
    RUN_TEST(test_reduce_budget_ex);
    RUN_TEST(test_html_text_display);
    RUN_TEST(test_reduce_greek_cyrillic);
    RUN_TEST(test_named_refs_pages);
    RUN_TEST(test_cyrillic_greek_charsets);
    RUN_TEST(test_header_charset_wins);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
