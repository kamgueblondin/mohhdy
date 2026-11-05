/* Lot 73: @media, @supports, @layer, :not/:is/:where, state
 * pseudo-classes and custom properties (webeng_cssq.inc). */

static int l73c(const char *id) { const we_node_t *n = lot52_node(id); return n ? (int)n->color : -1; }

static void l73_render_media(const char *html, int w, int h) {
    we_set_image_reader(0, 0);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    we_set_media(&g_doc, w, h);
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout(&g_doc, w ? w : 640));
}

#define L73_IDS "<p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p><p id=e>e</p><p id=f>f</p><p id=g>g</p><p id=h>h</p></body>"

static void test_l73_media_width(void) {
    lot52_render("<style>p{color:#000009}"
                 "@media (min-width: 300px){#a{color:#000001}}"
                 "@media (min-width: 321px){#b{color:#000002}}"
                 "@media screen and (max-width: 20em){#c{color:#000003}}"
                 "@media (max-width: 19.99em){#d{color:#000004}}"
                 "@media (width: 320px){#e{color:#000005}}"
                 "@media (width >= 320px) and (width < 321px){#f{color:#000006}}"
                 "@media (100px < width < 320px){#g{color:#000007}}"
                 "@media (320px <= width <= 400px){#h{color:#000008}}"
                 "</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(9, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(9, l73c("d"));
    TEST_ASSERT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(9, l73c("g"));
    TEST_ASSERT_EQUAL(8, l73c("h"));
    TEST_ASSERT_EQUAL(3, (int)g_doc.perf.media_skipped);
    /* value then name, the other way round */
    lot52_render("<style>@media (400px > width){#a{color:#000001}}@media (320px > width){#b{color:#000002}}"
                 "@media (height > 479px) and (height <= 480px){#c{color:#000003}}"
                 "@media (min-height: 481px){#d{color:#000004}}@media (max-height: 30rem){#e{color:#000005}}"
                 "@media (width = 320px){#f{color:#000006}}@media (320px >= width){#g{color:#000007}}"
                 "@media (320px < width){#h{color:#000008}}</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(0, l73c("d"));
    TEST_ASSERT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(7, l73c("g"));
    TEST_ASSERT_EQUAL(0, l73c("h"));
}

static void test_l73_media_types_and_lists(void) {
    lot52_render("<style>@media print{#a{color:#000001}}@media not print{#b{color:#000002}}"
                 "@media only screen and (min-width: 1px){#c{color:#000003}}@media print, (max-width: 400px){#d{color:#000004}}"
                 "@media not all and (min-width: 900px){#e{color:#000005}}@media all{#f{color:#000006}}"
                 "@media speech{#g{color:#000007}}@media {#h{color:#000008}}</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(0, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(4, l73c("d"));
    TEST_ASSERT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(0, l73c("g"));
    TEST_ASSERT_EQUAL(8, l73c("h"));
    /* conditions: or, not, nesting; a bad query is false, the list goes on */
    lot52_render("<style>@media (max-width: 10px) or (min-width: 300px){#a{color:#000001}}"
                 "@media not (min-width: 300px){#b{color:#000002}}@media ((min-width: 1px) and (max-width: 2px)) or (width: 320px){#c{color:#000003}}"
                 "@media (min-width: 1px) and (max-width: 2px) or (width: 320px){#d{color:#000004}}"
                 "@media screen and, (width: 320px){#e{color:#000005}}@media (min-width: 1px) and (max-width: 400px){#f{color:#000006}}"
                 "@media (min-width: 10){#g{color:#000007}}@media (min-width: 0){#h{color:#000008}}</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(0, l73c("d"));                  /* and / or mixed: refused */
    TEST_ASSERT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(0, l73c("g"));                  /* a number without unit */
    TEST_ASSERT_EQUAL(8, l73c("h"));
    TEST_ASSERT_EQUAL(3, l61_diag_count("media query refused"));
}

static void test_l73_media_features(void) {
    const char *html = "<style>@media (orientation: portrait){#a{color:#000001}}@media (orientation: landscape){#b{color:#000002}}"
                       "@media (prefers-color-scheme: dark){#c{color:#000003}}@media (prefers-color-scheme: light) and (hover: hover){#d{color:#000004}}"
                       "@media (prefers-reduced-motion: reduce){#e{color:#000005}}@media (min-resolution: 2dppx){#f{color:#000006}}"
                       "@media (min-aspect-ratio: 16/9){#g{color:#000007}}@media (color) and (pointer: fine) and (max-resolution: 96dpi){#h{color:#000008}}"
                       "</style>" L64_BODY L73_IDS;
    l73_render_media(html, 320, 0);                    /* 320 x 480: portrait */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(0, l73c("c"));
    TEST_ASSERT_EQUAL(4, l73c("d"));
    TEST_ASSERT_EQUAL(0, l73c("e"));
    TEST_ASSERT_EQUAL(0, l73c("f"));
    TEST_ASSERT_EQUAL(0, l73c("g"));
    TEST_ASSERT_EQUAL(8, l73c("h"));
    l73_render_media(html, 1000, 500);                 /* landscape, 2:1 */
    TEST_ASSERT_EQUAL(0, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(7, l73c("g"));
    l73_render_media("<style>@media (min-width: 900px){#a{color:#000001}}@media (scan: interlace){#b{color:#000002}}"
                     "@media (monochrome){#c{color:#000003}}@media (min-color: 8) and (max-monochrome: 0){#d{color:#000004}}"
                     "@media (min-color: 9){#e{color:#000005}}@media (grid){#f{color:#000006}}@media (max-foo: 2px){#g{color:#000007}}"
                     "@media (aspect-ratio: 2){#h{color:#000008}}</style>" L64_BODY L73_IDS, 1000, 500);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(0, l73c("c"));
    TEST_ASSERT_EQUAL(4, l73c("d"));
    TEST_ASSERT_EQUAL(0, l73c("e"));
    TEST_ASSERT_EQUAL(0, l73c("f"));
    TEST_ASSERT_EQUAL(0, l73c("g"));
    TEST_ASSERT_EQUAL(8, l73c("h"));
    TEST_ASSERT_EQUAL(2, l61_diag_count("media feature refused"));
    /* unset media: 640 x 480 */
    l73_render_media("<style>@media (width: 640px) and (height: 480px) and (orientation: landscape){#a{color:#000001}}</style>"
                     L64_BODY L73_IDS, 0, 0);
    TEST_ASSERT_EQUAL(1, l73c("a"));
}

static void test_l73_media_nesting_and_skips(void) {
    lot52_render("<style>@media screen{@media (min-width: 100px){#a{color:#000001}}#b{color:#000002}}"
                 "@media print{@media screen{#c{color:#000003}}}#d{color:#000004}@import url(x.css);#e{color:#000005}"
                 "@font-face{font-family:x;src:url(a)}#f{color:#000006}@media (min-width: 1px){#g{color:\"}\"}#h{color:#000008}}"
                 "</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(0, l73c("c"));
    TEST_ASSERT_EQUAL(4, l73c("d"));
    TEST_ASSERT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(8, l73c("h"));                  /* a quoted brace does not end the block */
    TEST_ASSERT_EQUAL(2, l61_diag_count("css at-rule skipped"));
    TEST_ASSERT_EQUAL(1, (int)g_doc.perf.media_skipped);
    /* nesting stops at WE_CSS_NEST_MAX levels */
    lot52_render("<style>@media all{@media all{@media all{@media all{#a{color:#000001}}}}}"
                 "@media all{@media all{@media all{@media all{@media all{#b{color:#000002}}}}}}#c{color:#000003}</style>"
                 L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("css at-rule skipped"));
    /* an unclosed at-rule ends the sheet */
    lot52_render("<style>#a{color:#000001}@media screen</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("css at-rule skipped"));
}

static void test_l73_supports(void) {
    lot52_render("<style>@supports (display: grid){#a{color:#000001}}@supports (display: bogus){#b{color:#000002}}"
                 "@supports not (frobnicate: 1){#c{color:#000003}}@supports (color: red) and (margin: 1px){#d{color:#000004}}"
                 "@supports (color: nope) or (padding: 2px){#e{color:#000005}}@supports selector(a > b){#f{color:#000006}}"
                 "@supports selector(a:::foo){#g{color:#000007}}@supports (--x: anything){#h{color:#000008}}</style>"
                 L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);                 /* the tries leave no diag */
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(4, l73c("d"));
    TEST_ASSERT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(0, l73c("g"));
    TEST_ASSERT_EQUAL(8, l73c("h"));
    TEST_ASSERT_EQUAL(2, (int)g_doc.perf.media_skipped);
    TEST_ASSERT_EQUAL(0, (int)g_doc.perf.unknown_props);
    lot52_render("<style>@supports display: grid{#a{color:#000001}}@supports (color: red) and (x: 1) or (y: 2){#b{color:#000002}}"
                 "@supports ((color: red)){#c{color:#000003}}@supports (color){#d{color:#000004}}"
                 "@supports (content: 'x'){#e{color:#000005}}#f{color:#000006}</style>" L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(0, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(0, l73c("d"));
    TEST_ASSERT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(2, l61_diag_count("supports condition refused"));
}

static void test_l73_layer(void) {
    lot52_render("<style>@layer base{#a{color:#000001}#b{color:#000002}}#b{color:#000003}@layer;#c{color:#000004}</style>"
                 L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(3, l73c("b"));
    TEST_ASSERT_EQUAL(4, l73c("c"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("css layer order ignored"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("css at-rule skipped"));
}

static void test_l73_selector_not_is_where(void) {
    lot52_render("<style>p:not(#a){color:#000001}p:is(#b,#c){color:#000002}:where(#d){color:#000003}#d{color:#000004}"
                 ":where(#e) {color:#000005}p{color:#000006}"
                 "body :is(p.x){color:#000007}p:not(.x,#g):not(#a){color:#000008}</style>"
                 L64_BODY "<p id=a>a</p><p id=b>b</p><p id=c class=x>c</p><p id=d>d</p><p id=e>e</p><p id=f class=x>f</p>"
                 "<p id=g>g</p><p id=h>h</p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(6, l73c("a"));
    TEST_ASSERT_EQUAL(8, l73c("b"));                  /* 2 ids beat 1 id: later and more specific */
    TEST_ASSERT_EQUAL(2, l73c("c"));                  /* :is takes its #c: beats body :is(p.x) */
    TEST_ASSERT_EQUAL(8, l73c("d"));                  /* two :not of ids beat #d */
    TEST_ASSERT_EQUAL(8, l73c("e"));
    TEST_ASSERT_EQUAL(1, l73c("f"));
    TEST_ASSERT_EQUAL(1, l73c("g"));
    TEST_ASSERT_EQUAL(8, l73c("h"));
    /* combinators inside, nesting, forgiving :is, strict :not */
    lot52_render("<style>p:is(div > p){color:#000001}p:is(section p, :not(div) > p){color:#000002}"
                 "p:is(#e, ::before){color:#000003}p:not(#f, ::before){color:#000004}p:matches(#g){color:#000005}"
                 "p:-webkit-any(#h){color:#000006}p:not(){color:#000007}p:is(:is(:is(:is(#a)))){color:#000008}</style>"
                 L64_BODY "<div><p id=a>a</p></div><section><p id=b>b</p></section><p id=c>c</p><p id=d>d</p><p id=e>e</p>"
                 "<p id=f>f</p><p id=g>g</p><p id=h>h</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));                  /* parent body is not a div */
    TEST_ASSERT_EQUAL(3, l73c("e"));
    TEST_ASSERT_EQUAL(2, l73c("f"));                  /* :not(#f, ::before) refused, :not(div) > p holds */
    TEST_ASSERT_EQUAL(5, l73c("g"));
    TEST_ASSERT_EQUAL(6, l73c("h"));
    TEST_ASSERT_EQUAL(3, l61_diag_count("css selector unsupported"));   /* :not(::before), :not(), depth 4 */
    lot52_render("<style>p:not(#a):not(#b):not(#c){color:#000001}p:is(#a)x{color:#000002}p:is(#a{color:#000003}</style>"
                 L64_BODY L73_IDS, 320);
    TEST_ASSERT_EQUAL(0, l73c("d"));                  /* three :not on one part: refused */
    TEST_ASSERT_EQUAL(0, l73c("a"));
    TEST_ASSERT_EQUAL(3, l61_diag_count("css selector unsupported"));
}

static void test_l73_selector_states(void) {
    lot52_render("<style>a:link{color:#000001}a:any-link.k{color:#000002}a:visited{color:#000003}:hover{color:#000004}"
                 "input:checked{color:#000005}:disabled{color:#000006}button:enabled{color:#000007}option:checked{color:#000008}"
                 "p:not(:hover){color:#000009}</style>" L64_BODY
                 "<a id=a href=x>a</a><a id=b>b</a><a id=c class=k href=y>c</a><input id=d type=checkbox checked><input id=e type=checkbox>"
                 "<button id=f disabled>f</button><button id=g>g</button><select><option id=h>h</option><option id=i selected>i</option></select>"
                 "<p id=j>j</p><p id=k disabled>k</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_NOT_EQUAL(1, l73c("b"));              /* no href: not a link */
    TEST_ASSERT_EQUAL(2, l73c("c"));
    TEST_ASSERT_EQUAL(5, l73c("d"));
    TEST_ASSERT_NOT_EQUAL(5, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(7, l73c("g"));
    TEST_ASSERT_EQUAL(8, l73c("i"));
    TEST_ASSERT_NOT_EQUAL(8, l73c("h"));
    TEST_ASSERT_EQUAL(9, l73c("j"));
    TEST_ASSERT_EQUAL(9, l73c("k"));                  /* disabled on a p is not a form state */
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l73_selector_of_type(void) {
    lot52_render("<style>p:first-of-type{color:#000001}p:last-of-type{color:#000002}span:only-of-type{color:#000003}"
                 "i:only-of-type{color:#000004}b:first-of-type:last-of-type{color:#000005}</style>" L64_BODY
                 "<div><span id=s>s</span><p id=a>a</p><p id=b>b</p><p id=c>c</p><i id=i1>i</i><i id=i2>i</i><b id=x>x</b></div></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(2, l73c("c"));
    TEST_ASSERT_EQUAL(3, l73c("s"));
    TEST_ASSERT_EQUAL(0, l73c("i1"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("css selector unsupported"));   /* two structural pseudo-classes on one part */
    lot52_render("<style>p:first-of-type{color:#000001}#a{width:10px}</style>" L64_BODY "<p id=a>a</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(2, (int)g_doc.nrules);
}

static void test_l73_custom_properties(void) {
    lot52_render("<style>:root{--main:#000001;--w:40px;--Gap: 3px }p{color:var(--main)}#b{--main:#000002}"
                 "#c{color:var(--none, #000003)}#d{color:var(--none, var(--main))}#e{width:var(--w);height:10px}"
                 "#f{--main:#000006}#f span{color:var(--main)}#g{margin-left:var(--Gap)}#h{color:var(--gap, #000008)}</style>"
                 L64_BODY "<p id=a>a</p><p id=b>b</p><p id=c>c</p><p id=d>d</p><div id=e></div><div id=f><span id=s>s</span></div>"
                 "<div id=g>g</div><p id=h>h</p></body>", 320);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(2, l73c("b"));                  /* custom properties apply before var() */
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(1, l73c("d"));
    TEST_ASSERT_EQUAL(40, lot52_node("e")->w);
    TEST_ASSERT_EQUAL(6, l73c("s"));                  /* inherited */
    TEST_ASSERT_EQUAL(3, lot52_node("g")->x);
    TEST_ASSERT_EQUAL(8, l73c("h"));                  /* names are case-sensitive */
    TEST_ASSERT_TRUE(g_doc.perf.vars_used >= 7);
}

static void test_l73_custom_properties_resolution(void) {
    /* resolved where declared: the child sees the root value of --b */
    lot52_render("<style>:root{--a:var(--b);--b:#000001}#x{--b:#000002;color:var(--a)}#y{color:var(--b)}"
                 "#z{--c:var(--c);color:var(--c, #000003)}#w{--d:#000004;--d:initial;color:var(--d, #000005)}"
                 "#v{color:var(--q)}#v{color:var(--q}#u{--e:inherit;color:var(--e, #000009)}"
                 "#t{--f:#00000a}#t i{--f:inherit;color:var(--f)}</style>"
                 L64_BODY "<p id=x>x</p><p id=y>y</p><p id=z>z</p><p id=w>w</p><p id=v>v</p><p id=u>u</p>"
                 "<div id=t><i id=ti>t</i></div></body>", 320);
    TEST_ASSERT_EQUAL(0, l73c("x"));                  /* --a was undefined when declared on the root */
    TEST_ASSERT_EQUAL(1, l73c("y"));
    TEST_ASSERT_EQUAL(3, l73c("z"));
    TEST_ASSERT_EQUAL(5, l73c("w"));
    TEST_ASSERT_EQUAL(0, l73c("v"));
    TEST_ASSERT_EQUAL(9, l73c("u"));
    TEST_ASSERT_EQUAL(10, l73c("ti"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("css variable cycle"));
    TEST_ASSERT_EQUAL(5, l61_diag_count("css variable undefined"));   /* --a on the two :root elements (style, body), #x, #v twice */
    /* --a after --b in the same rule: declared order does not matter for the value use */
    lot52_render("<style>:root{--b:#000001;--a:var(--b)}#x{color:var(--a)}#y{color:var( --b )}"
                 "#z{border:var(--bw, 2px) solid var(--b)}</style>" L64_BODY "<p id=x>x</p><p id=y>y</p><p id=z>z</p></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("x"));
    TEST_ASSERT_EQUAL(1, l73c("y"));
    TEST_ASSERT_EQUAL(2, lot52_node("z")->border);
    TEST_ASSERT_EQUAL(0, g_doc.ndiag);
}

static void test_l73_custom_properties_limits(void) {
    char html[2400];
    int k, at;
    at = snprintf(html, sizeof(html), "<style>:root{");
    for (k = 0; k < WE_VARS_MAX + 1; k++) at += snprintf(html + at, sizeof(html) - at, "--v%d:#%06x;", k, k + 1);
    at += snprintf(html + at, sizeof(html) - at, "}#a{color:var(--v%d)}#b{color:var(--v%d)}#c{color:var(--v0)}"
                   "#d{--v0:#0000ff}#d{color:var(--v0)}</style>" L64_BODY L73_IDS, WE_VARS_MAX - 1, WE_VARS_MAX);
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(WE_VARS_MAX, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));
    TEST_ASSERT_EQUAL(1, l73c("c"));
    TEST_ASSERT_EQUAL(0xFF, l73c("d"));               /* a full table still replaces a known name */
    TEST_ASSERT_TRUE(l61_diag_count("css variables full") >= 1);
    /* a value over WE_VAR_VALUE_MAX after expansion is undefined */
    at = snprintf(html, sizeof(html), "<style>:root{--s:");
    for (k = 0; k < 300; k++) html[at++] = 'x';
    at += snprintf(html + at, sizeof(html) - at, "}#a{font-family:var(--s) var(--s);color:#000001}</style>" L64_BODY L73_IDS);
    lot52_render(html, 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(1, l61_diag_count("css variable undefined"));
}


static void test_l73_media_edges(void) {
    /* em with one decimal; dpi; ranges with < first; mixed directions */
    l73_render_media("<style>@media (max-width: 20.5em){#a{color:#000001}}@media (max-resolution: 95dpi){#b{color:#000002}}"
                     "@media (100px < width < 400px){#c{color:#000003}}@media (400px > width < 500px){#d{color:#000004}}"
                     "@media (((((((width: 325px))))))){#e{color:#000005}}@media print and (min-width: 1px){#f{color:#000006}}"
                     "@media (x, (width: 325px){#g{color:#000007}}@media print{#h{content:\"}\"}#h{color:#000008}}</style>"
                     L64_BODY L73_IDS, 325, 0);
    TEST_ASSERT_EQUAL(1, l73c("a"));                  /* 20.5em = 328px */
    TEST_ASSERT_EQUAL(0, l73c("b"));                  /* 95dpi < 1dppx */
    TEST_ASSERT_EQUAL(3, l73c("c"));
    TEST_ASSERT_EQUAL(0, l73c("d"));                  /* > then <: refused */
    TEST_ASSERT_EQUAL(0, l73c("e"));                  /* nested deeper than WE_CSS_NEST_MAX */
    TEST_ASSERT_EQUAL(0, l73c("f"));                  /* a print query stays false with a true condition */
    TEST_ASSERT_EQUAL(0, l73c("g"));                  /* a comma inside parentheses splits nothing */
    TEST_ASSERT_EQUAL(0, l73c("h"));                  /* a quoted brace stays inside the skipped block */
    TEST_ASSERT_EQUAL(3, l61_diag_count("media query refused"));
    /* selector() takes pseudo-elements; :where adds nothing */
    lot52_render("<style>@supports selector(p::before){#a{color:#000001}}:where(#b){color:#000005}p{color:#000006}"
                 ":disabled{color:#000007}:enabled{color:#000008}</style>" L64_BODY
                 "<p id=a>a</p><p id=b>b</p><p id=c disabled>c</p><div id=d>d</div><progress id=e disabled value=1></progress></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(6, l73c("b"));
    TEST_ASSERT_EQUAL(6, l73c("c"));                  /* a p is no form control: neither */
    TEST_ASSERT_EQUAL(0, l73c("d"));
    TEST_ASSERT_NOT_EQUAL(7, l73c("e"));              /* progress keeps a disabled flag but is no control */
}

static void test_l73_selector_has_and_form_states(void) {
    lot52_render("<style>div:has(p){color:#000001}div:has(> span){color:#000002}h1:has(+ p){color:#000003}h2:has(~ i){color:#000004}"
                 "div:has(:has(p)){color:#000005}input:placeholder-shown{color:#000006}progress:indeterminate{color:#000007}"
                 "details:open{color:#000008}</style>" L64_BODY
                 "<div id=a><p>x</p></div><div id=b><i><span>s</span></i></div><div id=c><span>q</span></div><h1 id=d>h</h1><p>p</p>"
                 "<h2 id=e>e</h2><p>p</p><i>i</i><input id=f placeholder=z><input id=g placeholder=z value=v><progress id=h></progress>"
                 "<progress id=i value=1></progress><details id=j open><summary>s</summary></details><details id=k></details>"
                 "<h1 id=l>l</h1><i>n</i><p>after</p><h2 id=m>m</h2></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
    TEST_ASSERT_EQUAL(0, l73c("b"));                  /* span is a grandchild: > fails */
    TEST_ASSERT_EQUAL(2, l73c("c"));
    TEST_ASSERT_EQUAL(3, l73c("d"));
    TEST_ASSERT_EQUAL(4, l73c("e"));
    TEST_ASSERT_EQUAL(6, l73c("f"));
    TEST_ASSERT_EQUAL(0, l73c("g"));                  /* a value hides the placeholder */
    TEST_ASSERT_EQUAL(7, l73c("h"));
    TEST_ASSERT_EQUAL(0, l73c("i"));
    TEST_ASSERT_EQUAL(8, l73c("j"));
    TEST_ASSERT_EQUAL(0, l73c("k"));
    TEST_ASSERT_EQUAL(0, l73c("l"));                  /* + is the next element only */
    TEST_ASSERT_EQUAL(0, l73c("m"));                  /* ~ looks after, not before */
    TEST_ASSERT_EQUAL(1, l61_diag_count("css selector unsupported"));   /* :has inside :has */
    /* specificity of the most specific argument; a bad argument drops the rule */
    lot52_render("<style>div:has(#x, p){color:#000001}div#a{color:#000002}div:has(p, ::before){color:#000003}"
                 "div:has(){color:#000004}</style>" L64_BODY "<div id=a><p id=x>x</p></div><div id=b><p>y</p></div></body>", 320);
    TEST_ASSERT_EQUAL(2, l73c("a"));                  /* div:has(#x, p) = div#a = 4097: the later one wins */
    TEST_ASSERT_EQUAL(1, l73c("b"));
    TEST_ASSERT_EQUAL(2, l61_diag_count("css selector unsupported"));
    /* a state pseudo-class counts as a class: a:link beats a later a */
    lot52_render("<style>a:link{color:#000001}a{color:#000002}</style>" L64_BODY "<a id=a href=x>t</a></body>", 320);
    TEST_ASSERT_EQUAL(1, l73c("a"));
}

static void lot73_cssq_run(void) {
    RUN_TEST(test_l73_media_width);
    RUN_TEST(test_l73_media_types_and_lists);
    RUN_TEST(test_l73_media_features);
    RUN_TEST(test_l73_media_nesting_and_skips);
    RUN_TEST(test_l73_media_edges);
    RUN_TEST(test_l73_supports);
    RUN_TEST(test_l73_layer);
    RUN_TEST(test_l73_selector_not_is_where);
    RUN_TEST(test_l73_selector_states);
    RUN_TEST(test_l73_selector_of_type);
    RUN_TEST(test_l73_selector_has_and_form_states);
    RUN_TEST(test_l73_custom_properties);
    RUN_TEST(test_l73_custom_properties_resolution);
    RUN_TEST(test_l73_custom_properties_limits);
}
