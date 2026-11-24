/* Lot 118 (US-031): the engine's math in thousandths (CSS Values 4 10:
 * the functions round, mod, rem, abs, sign, pow, sqrt, hypot, log, exp,
 * sin, cos, tan, asin, acos, atan, atan2, the constants e, pi, infinity,
 * -infinity, NaN, angles, typed arithmetic), the viewport-percentage
 * units (6.1.2), math in numbers, angles and the whole-px lengths, and
 * @property (CSS Properties and Values API 1). Hand cases next to the
 * generated test_lot118_*.c; the API table's values were computed with
 * scripts/lot118_model.py, the page values follow docs/lot118_plan.md. */
#include "../../framework/unity.h"
#include <string.h>
#include <stdio.h>
#include "webeng.h"

static we_doc_t g_doc;
static we_forms_t g_forms;
static we_node_t g_none;

static void lay_at(const char *html, int w, int h) {
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, html, (int)strlen(html)));
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, w, h));
}

static void lays(const char *html) { lay_at(html, 640, 480); }

static const we_node_t *el(const char *id) {
    int i = we_find_id(&g_doc, id);
    g_none.h = -1;
    g_none.w = -1;
    return i >= 0 ? &g_doc.nodes[i] : &g_none;
}

static int diag_count(const char *t) {
    uint32_t i;
    int c = 0;
    for (i = 0; i < g_doc.ndiag; i++) if (strstr(g_doc.diag[i], t)) c++;
    return c;
}

/* the text of custom property name on element id, "" when none */
static const char *var_text(const char *id, const char *name) {
    static char buf[200];
    const we_node_t *n = el(id);
    int k, len = (int)strlen(name);
    buf[0] = 0;
    for (k = 0; n != &g_none && k < n->nvars; k++)
        if (n->var_name_len[k] == len && memcmp(g_doc.pool + n->var_name_off[k], name, (size_t)len) == 0) {
            memcpy(buf, g_doc.pool + n->var_off[k], n->var_len[k]);
            buf[n->var_len[k]] = 0;
        }
    return buf;
}

/* the color of a div styled "color: #010203; color: css" (the first when
 * css is refused) */
static uint32_t color_of(const char *css) {
    char h[300];
    sprintf(h, "<body><div id=k style='color:#010203;color:%s'>k</div></body>", css);
    lays(h);
    return el("k")->color;
}

/* ---------- the API: type and used thousandths (-1: refused) ---------- */
static const struct { const char *expr; int basis, type, milli; } rows[] = {
    { "calc(1.5px + 2.25px)", 0, 1, 3750 },
    { "calc(10px * 1.5)", 0, 1, 15000 },
    { "calc(1.5 * 10px)", 0, 1, 15000 },
    { "calc(7px / 2)", 0, 1, 3500 },
    { "calc(10px / 4px)", 0, 0, 2500 },
    { "calc(10px / 3)", 0, 1, 3333 },
    { "calc(-10px / 3)", 0, 1, -3333 },
    { "calc(e)", 0, 0, 2718 },
    { "calc(pi)", 0, 0, 3142 },
    { "calc(PI * 2)", 0, 0, 6284 },
    { "calc(infinity * 1px)", 0, 1, 1000000000 },
    { "calc(-infinity * 1px)", 0, 1, -1000000000 },
    { "calc(NaN * 1px)", 0, 1, 0 },
    { "calc(nan)", 0, 0, 0 },
    { "calc(1px / 0)", 0, -1, 0 },
    { "calc(1 / (2 - 2))", 0, -1, 0 },
    { "calc(1px / sign(50% - 50%))", 100, 1, 1000000000 },
    { "calc(-1px / sign(50% - 50%))", 100, 1, -1000000000 },
    { "calc(0px / sign(50% - 50%))", 100, 1, 0 },
    { "calc(infinity - infinity)", 0, 0, 0 },
    { "calc(infinity + infinity)", 0, 0, 1000000000 },
    { "calc(infinity + 5)", 0, 0, 1000000000 },
    { "calc(5 - infinity)", 0, 0, -1000000000 },
    { "calc(infinity * 0)", 0, 0, 0 },
    { "calc(-infinity * 2)", 0, 0, -1000000000 },
    { "calc(infinity * -2)", 0, 0, -1000000000 },
    { "calc(infinity / infinity)", 0, 0, 0 },
    { "calc(infinity / -2)", 0, 0, -1000000000 },
    { "calc(5 / infinity)", 0, 0, 0 },
    { "calc(-infinity / 2)", 0, 0, -1000000000 },
    { "calc(NaN + 1)", 0, 0, 0 },
    { "calc(1 - NaN)", 0, 0, 0 },
    { "calc(NaN * 2)", 0, 0, 0 },
    { "calc(NaN / 2)", 0, 0, 0 },
    { "calc(2 / NaN)", 0, 0, 0 },
    { "min(NaN, 1)", 0, 0, 0 },
    { "max(1, NaN)", 0, 0, 0 },
    { "clamp(NaN, 1, 2)", 0, 0, 0 },
    { "clamp(1, NaN, 2)", 0, 0, 0 },
    { "clamp(1, 2, NaN)", 0, 0, 0 },
    { "clamp(1, 5, 3)", 0, 0, 3000 },
    { "clamp(4, 2, 3)", 0, 0, 4000 },
    { "min(3px, 1px, 2px)", 0, 1, 1000 },
    { "max(3px, 1px, 2px)", 0, 1, 3000 },
    { "calc(2000px * 1000)", 0, -1, 0 },
    { "calc(2000px * 2000 * 2000)", 0, -1, 0 },
    { "calc(2000px / 0.001)", 0, -1, 0 },
    { "calc(1000px + 1000px)", 0, 1, 2000000 },
    { "round(7.5px, 5px)", 0, 1, 10000 },
    { "round(nearest, 7.4px, 5px)", 0, 1, 5000 },
    { "round(nearest, 7.5px, 5px)", 0, 1, 10000 },
    { "round(up, 7px, 5px)", 0, 1, 10000 },
    { "round(up, -7px, 5px)", 0, 1, -5000 },
    { "round(down, 7px, 5px)", 0, 1, 5000 },
    { "round(down, -7px, 5px)", 0, 1, -10000 },
    { "round(to-zero, 7px, 5px)", 0, 1, 5000 },
    { "round(to-zero, -7px, 5px)", 0, 1, -5000 },
    { "round(up, 10px, 5px)", 0, 1, 10000 },
    { "round(7px, -5px)", 0, 1, 5000 },
    { "round(2.5)", 0, 0, 3000 },
    { "round(-2.5)", 0, 0, -2000 },
    { "round(-2.6)", 0, 0, -3000 },
    { "round(up, 2.1)", 0, 0, 3000 },
    { "round(7px, 0px)", 0, 1, 0 },
    { "round(infinity * 1px, 5px)", 0, 1, 1000000000 },
    { "round(up, 3px, infinity * 1px)", 0, 1, 1000000000 },
    { "round(up, -3px, infinity * 1px)", 0, 1, 0 },
    { "round(down, -3px, infinity * 1px)", 0, 1, -1000000000 },
    { "round(down, 3px, infinity * 1px)", 0, 1, 0 },
    { "round(3px, infinity * 1px)", 0, 1, 0 },
    { "round(to-zero, 3px, infinity * 1px)", 0, 1, 0 },
    { "round(NaN, 1)", 0, 0, 0 },
    { "round(1, NaN)", 0, 0, 0 },
    { "round(infinity, infinity)", 0, 0, 0 },
    { "mod(-7px, 5px)", 0, 1, 3000 },
    { "rem(-7px, 5px)", 0, 1, -2000 },
    { "mod(7px, -5px)", 0, 1, -3000 },
    { "rem(7px, -5px)", 0, 1, 2000 },
    { "mod(7px, 5px)", 0, 1, 2000 },
    { "mod(10px, 5px)", 0, 1, 0 },
    { "mod(1px, 0px)", 0, 1, 0 },
    { "mod(5px, infinity * 1px)", 0, 1, 5000 },
    { "mod(-5px, infinity * 1px)", 0, 1, 0 },
    { "rem(-5px, infinity * 1px)", 0, 1, -5000 },
    { "mod(0px, -infinity * 1px)", 0, 1, 0 },
    { "mod(infinity * 1px, 5px)", 0, 1, 0 },
    { "mod(NaN, 1)", 0, 0, 0 },
    { "rem(1, NaN)", 0, 0, 0 },
    { "abs(-3.5px)", 0, 1, 3500 },
    { "abs(3px)", 0, 1, 3000 },
    { "abs(-infinity)", 0, 0, 1000000000 },
    { "abs(NaN)", 0, 0, 0 },
    { "sign(-2px)", 0, 0, -1000 },
    { "sign(2deg)", 0, 0, 1000 },
    { "sign(0)", 0, 0, 0 },
    { "sign(NaN)", 0, 0, 0 },
    { "pow(2, 10)", 0, 0, 1024000 },
    { "pow(2, -2)", 0, 0, 250 },
    { "pow(-2, 3)", 0, 0, -8000 },
    { "pow(-2, -1)", 0, 0, -500 },
    { "pow(0, -1)", 0, 0, 1000000000 },
    { "pow(0, 2)", 0, 0, 0 },
    { "pow(4, 0.5)", 0, 0, 2000 },
    { "pow(2, 0.5)", 0, 0, 1414 },
    { "pow(-4, 0.5)", 0, 0, 0 },
    { "pow(0, 0.5)", 0, 0, 0 },
    { "pow(0, -0.5)", 0, 0, 1000000000 },
    { "pow(2, infinity)", 0, 0, 1000000000 },
    { "pow(0.5, infinity)", 0, 0, 0 },
    { "pow(2, -infinity)", 0, 0, 0 },
    { "pow(0.5, -infinity)", 0, 0, 1000000000 },
    { "pow(1, infinity)", 0, 0, 1000 },
    { "pow(-1, infinity)", 0, 0, 1000 },
    { "pow(infinity, -1)", 0, 0, 0 },
    { "pow(infinity, 2)", 0, 0, 1000000000 },
    { "pow(-infinity, 3)", 0, 0, -1000000000 },
    { "pow(-infinity, 2)", 0, 0, 1000000000 },
    { "pow(-infinity, 0.5)", 0, 0, 1000000000 },
    { "pow(10, 64)", 0, 0, 1000000000 },
    { "pow(10, -64)", 0, 0, 0 },
    { "pow(-10, 9)", 0, 0, -1000000000 },
    { "pow(0.1, -9)", 0, 0, 1000000000 },
    { "pow(1000, 1.5)", 0, 0, 31622776 },
    { "pow(0.001, 0.5)", 0, 0, 32 },
    { "pow(2, 1000000)", 0, -1, 0 },
    { "pow(0.5, 1000000)", 0, -1, 0 },
    { "pow(2, -1000000)", 0, -1, 0 },
    { "pow(NaN, 2)", 0, 0, 0 },
    { "pow(2, NaN)", 0, 0, 0 },
    { "pow(5, 0)", 0, 0, 1000 },
    { "pow(NaN, 0)", 0, 0, 0 },
    { "sqrt(2)", 0, 0, 1414 },
    { "sqrt(4)", 0, 0, 2000 },
    { "sqrt(-1)", 0, 0, 0 },
    { "sqrt(infinity)", 0, 0, 1000000000 },
    { "sqrt(NaN)", 0, 0, 0 },
    { "sqrt(1000000)", 0, -1, 0 },
    { "hypot(3px, 4px)", 0, 1, 5000 },
    { "hypot(-3px)", 0, 1, 3000 },
    { "hypot(3px, 4px, 12px)", 0, 1, 13000 },
    { "hypot(infinity * 1px, 1px)", 0, 1, 1000000000 },
    { "hypot(1px, NaN * 1px)", 0, 1, 0 },
    { "hypot(2000px, 2000px)", 0, 1, 2828427 },
    { "log(e)", 0, 0, 1000 },
    { "log(1)", 0, 0, 0 },
    { "log(8, 2)", 0, 0, 3000 },
    { "log(100, 10)", 0, 0, 2000 },
    { "log(0.5, 2)", 0, 0, -1000 },
    { "log(0)", 0, 0, -1000000000 },
    { "log(-1)", 0, 0, 0 },
    { "log(infinity)", 0, 0, 1000000000 },
    { "log(NaN)", 0, 0, 0 },
    { "log(5, 1)", 0, 0, 1000000000 },
    { "log(1, 1)", 0, 0, 0 },
    { "log(0.5, 1)", 0, 0, -1000000000 },
    { "log(infinity, 2)", 0, 0, 1000000000 },
    { "log(infinity, 0.5)", 0, 0, -1000000000 },
    { "log(0, 2)", 0, 0, -1000000000 },
    { "log(0, 0.5)", 0, 0, 1000000000 },
    { "log(2, infinity)", 0, 0, 0 },
    { "log(2, 0)", 0, 0, 0 },
    { "log(2, -1)", 0, 0, 0 },
    { "log(2, NaN)", 0, 0, 0 },
    { "log(1000000)", 0, -1, 0 },
    { "log(0.001)", 0, 0, -6908 },
    { "exp(0)", 0, 0, 1000 },
    { "exp(1)", 0, 0, 2718 },
    { "exp(-1)", 0, 0, 368 },
    { "exp(2.5)", 0, 0, 12182 },
    { "exp(-infinity)", 0, 0, 0 },
    { "exp(infinity)", 0, 0, 1000000000 },
    { "exp(NaN)", 0, 0, 0 },
    { "exp(30)", 0, 0, 1000000000 },
    { "exp(20)", 0, 0, 1000000000 },
    { "exp(-8)", 0, 0, 0 },
    { "exp(-30)", 0, 0, 0 },
    { "sin(30deg)", 0, 0, 500 },
    { "sin(90deg)", 0, 0, 1000 },
    { "sin(-90deg)", 0, 0, -1000 },
    { "sin(180deg)", 0, 0, 0 },
    { "sin(390deg)", 0, 0, 500 },
    { "sin(-200deg)", 0, 0, 342 },
    { "sin(270deg)", 0, 0, -1000 },
    { "cos(60deg)", 0, 0, 500 },
    { "cos(180deg)", 0, 0, -1000 },
    { "cos(-180deg)", 0, 0, -1000 },
    { "cos(0deg)", 0, 0, 1000 },
    { "cos(1turn)", 0, 0, 1000 },
    { "cos(100grad)", 0, 0, 0 },
    { "tan(45deg)", 0, 0, 1000 },
    { "tan(-45deg)", 0, 0, -1000 },
    { "tan(90deg)", 0, 0, 1000000000 },
    { "tan(-90deg)", 0, 0, -1000000000 },
    { "tan(135deg)", 0, 0, -1000 },
    { "sin(calc(pi / 2))", 0, 0, 1000 },
    { "sin(1)", 0, 0, 841 },
    { "cos(pi)", 0, 0, -1000 },
    { "tan(1rad)", 0, 0, 1557 },
    { "sin(0.5turn)", 0, 0, 0 },
    { "sin(200grad)", 0, 0, 0 },
    { "sin(infinity)", 0, 0, 0 },
    { "cos(-infinity * 1deg)", 0, 0, 0 },
    { "tan(NaN)", 0, 0, 0 },
    { "sin(720000deg)", 0, -1, 0 },
    { "cos(1000rad)", 0, 0, 562 },
    { "asin(1)", 0, 2, 90000 },
    { "asin(-1)", 0, 2, -90000 },
    { "asin(0.5)", 0, 2, 30000 },
    { "asin(0.999)", 0, 2, 87437 },
    { "acos(-1)", 0, 2, 180000 },
    { "acos(1)", 0, 2, 0 },
    { "acos(0)", 0, 2, 90000 },
    { "acos(0.5)", 0, 2, 60000 },
    { "asin(2)", 0, 2, 0 },
    { "acos(-1.001)", 0, 2, 0 },
    { "asin(NaN)", 0, 2, 0 },
    { "atan(1)", 0, 2, 45000 },
    { "atan(-1)", 0, 2, -45000 },
    { "atan(0)", 0, 2, 0 },
    { "atan(infinity)", 0, 2, 90000 },
    { "atan(-infinity)", 0, 2, -90000 },
    { "atan(NaN)", 0, 2, 0 },
    { "atan(1000)", 0, 2, 89943 },
    { "atan2(1px, -1px)", 0, 2, 135000 },
    { "atan2(-1, -1)", 0, 2, -135000 },
    { "atan2(0, 0)", 0, 2, 0 },
    { "atan2(0, -1)", 0, 2, 180000 },
    { "atan2(-1, 0)", 0, 2, -90000 },
    { "atan2(1deg, 0deg)", 0, 2, 90000 },
    { "atan2(infinity, 1)", 0, 2, 90000 },
    { "atan2(1, -infinity)", 0, 2, 180000 },
    { "atan2(NaN, 1)", 0, 2, 0 },
    { "atan2(1, NaN)", 0, 2, 0 },
    { "atan2(3, 4)", 0, 2, 36870 },
    { "atan2(50%, 100px)", 200, 2, 45000 },
    { "calc(1px + 1)", 0, -1, 0 },
    { "calc(1px * 1px)", 0, -1, 0 },
    { "calc(1 / 1px)", 0, -1, 0 },
    { "calc(1deg + 1px)", 0, -1, 0 },
    { "calc(1deg * 1deg)", 0, -1, 0 },
    { "calc(10deg / 2deg)", 0, 0, 5000 },
    { "sin(1px)", 0, -1, 0 },
    { "sqrt(1px)", 0, -1, 0 },
    { "exp(1px)", 0, -1, 0 },
    { "pow(1px, 2)", 0, -1, 0 },
    { "pow(2, 1px)", 0, -1, 0 },
    { "log(1px)", 0, -1, 0 },
    { "log(2, 1px)", 0, -1, 0 },
    { "round(1px)", 0, -1, 0 },
    { "round(1px, 1)", 0, -1, 0 },
    { "atan2(1px, 1)", 0, -1, 0 },
    { "asin(1deg)", 0, -1, 0 },
    { "atan(1px)", 0, -1, 0 },
    { "hypot(1px, 1)", 0, -1, 0 },
    { "min(1px, 1)", 0, -1, 0 },
    { "clamp(1px, 2px)", 0, -1, 0 },
    { "clamp(1px, 2px, 3px, 4px)", 0, -1, 0 },
    { "mod(1px)", 0, -1, 0 },
    { "mod(1px, 2px, 3px)", 0, -1, 0 },
    { "abs(1px, 2px)", 0, -1, 0 },
    { "sign(1, 2)", 0, -1, 0 },
    { "pow(1, 2, 3)", 0, -1, 0 },
    { "log(1, 2, 3)", 0, -1, 0 },
    { "sin(1deg, 2deg)", 0, -1, 0 },
    { "atan2(1)", 0, -1, 0 },
    { "round(1px, 2px, 3px)", 0, -1, 0 },
    { "sqrt(1, 2)", 0, -1, 0 },
    { "exp(1, 2)", 0, -1, 0 },
    { "calc(1px, 2px)", 0, -1, 0 },
    { "calc(-pi)", 0, -1, 0 },
    { "calc(+e)", 0, -1, 0 },
    { "calc(-e)", 0, -1, 0 },
    { "calc(+infinity)", 0, -1, 0 },
    { "calc(-infinity)", 0, 0, -1000000000 },
    { "calc(-NaN)", 0, -1, 0 },
    { "calc(tau)", 0, -1, 0 },
    { "foo(1px)", 0, -1, 0 },
    { "calc(foo(1px))", 0, -1, 0 },
    { "calc(1e3px)", 0, 1, 1000000 },
    /* rows added against mutants of the math core (mutations.txt) */
    { "atan2(-34, 13)", 0, 2, -69076 },
    { "exp(-3.295)", 0, 0, 37 },
    { "exp(-1.754)", 0, 0, 173 },
    { "round(down, 0, infinity)", 0, 0, 0 },
    { "log(0, 1)", 0, 0, 1000000000 },
    { "calc(-2000px * 500 / 1)", 0, 1, -1000000000 },
    { "calc(2000px * 500 / 1)", 0, 1, 1000000000 },
    { "calc(-2000px * 500)", 0, 1, -1000000000 },
    { "calc(2000px * 500)", 0, 1, 1000000000 },
    { "calc()", 0, -1, 0 },
    { "round(up)", 0, -1, 0 },
    { "round(up, 1px)", 0, -1, 0 },
    { "round(sideways, 1px, 2px)", 0, -1, 0 },
    { "round(up 1px, 2px)", 0, -1, 0 },
    { "calc(2001px)", 0, -1, 0 },
    { "calc(2000px)", 0, 1, 2000000 },
    { "calc(.5px)", 0, 1, 500 },
    { "calc(5.px)", 0, -1, 0 },
    { "calc(1turn)", 0, 2, 360000 },
    { "calc(1rad)", 0, 2, 57295 },
    { "calc(100grad)", 0, 2, 90000 },
    { "calc(0.001deg)", 0, 2, 1 },
    { "calc(2000turn)", 0, 2, 720000000 },
    { "calc(3000000grad)", 0, -1, 0 },
    { "calc(1.2345px)", 0, 1, 1234 },
    { "calc(10%)", 0, 1, 0 },
    { "calc(10%)", -1, -1, 0 },
    { "calc(33.333%)", 300, 1, 99999 },
    { "calc(50% - 0.5px)", 101, 1, 50000 },
    { "calc(-50% * 2)", 1000, 1, -1000000 },
    { "-calc(1px)", 0, -1, 0 },
    { "calc(-(1px))", 0, -1, 0 },
    { "calc(+(1px))", 0, 1, 1000 },
    { "calc(1px+2px)", 0, -1, 0 },
    { "calc(1px +2px)", 0, -1, 0 },
    { "calc(1px - -2px)", 0, 1, 3000 },
    { "calc(1px*2)", 0, 1, 2000 },
    { "calc(1px * -2)", 0, 1, -2000 },
    { "CALC(1PX)", 0, 1, 1000 },
    { "Min(1Px, 2pX)", 0, 1, 1000 },
    { "calc(2px * (3px / 1px))", 0, 1, 6000 },
    { "calc((1px))", 0, 1, 1000 },
    { "calc(1px))", 0, -1, 0 },
    { "calc((1px)", 0, -1, 0 },
    { "calc(1px / 3 * 3)", 0, 1, 999 },
    { "calc(1px * 3 / 3)", 0, 1, 1000 },
    { "calc(e * pi)", 0, 0, 8539 },
    { "calc(-infinity + -infinity)", 0, 0, -1000000000 },
    { "calc(infinity + -infinity)", 0, 0, 0 },
    { "calc(0 * infinity)", 0, 0, 0 },
    { "calc(-0.5px)", 0, 1, -500 },
    { "calc(-1999.999px)", 0, 1, -1999999 },
    { "sign(-0.001)", 0, 0, -1000 },
    { "abs(calc(-1px * infinity))", 0, 1, 1000000000 },
};

static void test_l118_math_table(void) {
    we_unit_ctx_t c;
    unsigned k;
    memset(&c, 0, sizeof(c));
    c.font100 = 1600; c.root100 = 1600;
    for (k = 0; k < sizeof(rows) / sizeof(rows[0]); k++) {
        int type = -7, milli = -7, rc = we_math_milli(rows[k].expr, (int)strlen(rows[k].expr), &c, rows[k].basis, &type, &milli);
        int ok = rows[k].type < 0 ? rc == -1 : rc == 0 && type == rows[k].type && milli == rows[k].milli;
        if (!ok) printf("  row %u: %s: rc %d type %d milli %d\n", k, rows[k].expr, rc, type, milli);
        if (rows[k].type < 0) { TEST_ASSERT_EQUAL(-1, rc); continue; }
        TEST_ASSERT_EQUAL(0, rc);
        TEST_ASSERT_EQUAL(rows[k].type, type);
        TEST_ASSERT_EQUAL(rows[k].milli, milli);
    }
}

/* the API's arguments and context */
static void test_l118_math_api(void) {
    we_unit_ctx_t c;
    int type = 9, milli = 9;
    memset(&c, 0, sizeof(c));
    TEST_ASSERT_EQUAL(-1, we_math_milli(0, 3, &c, 0, &type, &milli));
    TEST_ASSERT_EQUAL(-1, we_math_milli("calc(1px)", -1, &c, 0, &type, &milli));
    TEST_ASSERT_EQUAL(-1, we_math_milli("calc(1px)", 9, &c, 0, 0, &milli));
    TEST_ASSERT_EQUAL(-1, we_math_milli("calc(1px)", 9, &c, 0, &type, 0));
    /* a context of zeros: 16px font, 10px lines, a 640 x 480 viewport */
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1em + 1lh)", 15, &c, 0, &type, &milli));
    TEST_ASSERT_EQUAL(26000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(10vw + 10vh)", 17, 0, 0, &type, &milli));
    TEST_ASSERT_EQUAL(WE_MATH_LENGTH, type);
    TEST_ASSERT_EQUAL(112000, milli);
    c.vw = 1000; c.vh = 500;
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vw)", 9, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vh)", 9, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vi)", 9, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vb)", 9, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vmin)", 11, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vmax)", 11, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1.5svw)", 12, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(15000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1LVH)", 10, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1dvmax)", 12, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1svmin)", 12, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
    TEST_ASSERT_EQUAL(-1, we_math_milli("calc(1xvw)", 10, &c, 0, &type, &milli));
    TEST_ASSERT_EQUAL(-1, we_math_milli("calc(1rex)", 10, &c, 0, &type, &milli));   /* Lot 119: was 1cqw (read now) */
    /* a vertical root: vi is the height, vb the width */
    c.vertical = 1;
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vi)", 9, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(5000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vb)", 9, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vw)", 9, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(10000, milli);
    /* past the layout bound: the defaults */
    c.vertical = 0; c.vw = 40000; c.vh = -3;
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1vw + 1vh)", 15, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(11200, milli);
    /* a unit leaf is hundredths of a px: 1.333em of 15px is 20px (1999.5 rounds) */
    c.font100 = 1500;
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(1.333em)", 13, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(20000, milli);
    TEST_ASSERT_EQUAL(-1, we_math_milli("calc(134em)", 11, &c, 0, &type, &milli));    /* 2010px leaf */
    TEST_ASSERT_EQUAL(0, we_math_milli("calc(133em)", 11, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(1995000, milli);
    /* the node and depth bounds */
    /* eight nested groups inside calc() are kept, a ninth is refused */
    TEST_ASSERT_EQUAL(0, we_math_milli("calc((((((((1px))))))))", 23, &c, 0, &type, &milli));
    TEST_ASSERT_EQUAL(-1, we_math_milli("calc(((((((((1px)))))))))", 25, &c, 0, &type, &milli));
    {
        char e[400];
        int n = sprintf(e, "calc(1px");
        int k;
        for (k = 0; k < 15; k++) n += sprintf(e + n, " + 1px");
        n += sprintf(e + n, ")");
        TEST_ASSERT_EQUAL(0, we_math_milli(e, n, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(16000, milli);
        n = sprintf(e, "calc(1px");
        for (k = 0; k < 16; k++) n += sprintf(e + n, " + 1px");
        n += sprintf(e + n, ")");
        TEST_ASSERT_EQUAL(-1, we_math_milli(e, n, &c, 0, &type, &milli));
        /* round(x) takes a step node, hypot(x) an abs node */
        /* 15 ones and round(1): 15 + 14 sums + 1 + step + round + 1 sum = 33 */
        n = sprintf(e, "calc(1");
        for (k = 0; k < 13; k++) n += sprintf(e + n, " + 1");
        n += sprintf(e + n, " + round(1))");
        TEST_ASSERT_EQUAL(0, we_math_milli(e, n, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(15000, milli);
        n = sprintf(e, "calc(1");
        for (k = 0; k < 14; k++) n += sprintf(e + n, " + 1");
        n += sprintf(e + n, " + round(1))");
        TEST_ASSERT_EQUAL(-1, we_math_milli(e, n, &c, 0, &type, &milli));
        /* 15 ones and hypot(1): 15 + 14 + 1 + abs + 1 = 32 is kept */
        n = sprintf(e, "calc(1");
        for (k = 0; k < 14; k++) n += sprintf(e + n, " + 1");
        n += sprintf(e + n, " + hypot(1))");
        TEST_ASSERT_EQUAL(0, we_math_milli(e, n, &c, 0, &type, &milli)); TEST_ASSERT_EQUAL(16000, milli);
        n = sprintf(e, "calc(1");
        for (k = 0; k < 15; k++) n += sprintf(e + n, " + 1");
        n += sprintf(e + n, " + hypot(1))");
        TEST_ASSERT_EQUAL(-1, we_math_milli(e, n, &c, 0, &type, &milli));
    }
}

/* ---------- pages ---------- */
/* fractional math lengths are cut once, at use */
static void test_l118_math_used(void) {
    lays("<body style='margin:0'><div style='width:600px'>"
         "<div id=a style='width:calc(10.9px + 10.9px)'>a</div>"
         "<div id=b style='width:calc(33.333% * 3)'>b</div>"
         "<div id=c style='width:calc(100% / 3 * 3)'>c</div>"
         "<div id=d style='width:calc(1px * pi * 100)'>d</div>"
         "<div id=e style='width:calc(infinity * 1px)'>e</div>"
         "<div id=f style='width:50px;width:calc(NaN * 1px)'>f</div>"
         "<div id=g style='width:round(up, 101px, 25px)'>g</div>"
         "<div id=h style='width:calc(200px * sin(30deg))'>h</div>"
         "<div id=i style='width:hypot(30px, 40px)'>i</div>"
         "<div id=j style='width:calc(10px * pow(2, 3))'>j</div>"
         "<div id=k style='width:mod(-50px, 70px)'>k</div>"
         "<div id=l style='width:calc(100px * (50% / 300px))'>l</div>"
         "</div></body>");
    TEST_ASSERT_EQUAL(21, el("a")->w);
    TEST_ASSERT_EQUAL(599, el("b")->w);      /* 599.994 */
    TEST_ASSERT_EQUAL(600, el("c")->w);
    TEST_ASSERT_EQUAL(314, el("d")->w);      /* pi is 3.142 */
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, el("e")->w);
    TEST_ASSERT_EQUAL(0, el("f")->w);
    TEST_ASSERT_EQUAL(125, el("g")->w);
    TEST_ASSERT_EQUAL(100, el("h")->w);
    TEST_ASSERT_EQUAL(50, el("i")->w);
    TEST_ASSERT_EQUAL(80, el("j")->w);
    TEST_ASSERT_EQUAL(20, el("k")->w);
    TEST_ASSERT_EQUAL(100, el("l")->w);      /* 300px / 300px is 1 */
    TEST_ASSERT_EQUAL(0, diag_count("math length refused"));
    /* a number, an angle or a constant zero divisor is not a length */
    lays("<body style='margin:0'><div style='width:600px'>"
         "<div id=a style='width:50px;width:calc(2)'>a</div>"
         "<div id=b style='width:50px;width:calc(1deg)'>b</div>"
         "<div id=c style='width:50px;width:calc(1px / 0)'>c</div>"
         "<div id=d style='width:50px;width:sin(30deg)'>d</div>"
         "</div></body>");
    TEST_ASSERT_EQUAL(50, el("a")->w);
    TEST_ASSERT_EQUAL(50, el("b")->w);
    TEST_ASSERT_EQUAL(50, el("c")->w);
    TEST_ASSERT_EQUAL(50, el("d")->w);
    TEST_ASSERT_EQUAL(4, diag_count("math length refused"));
}

/* a percentage-dependent zero divisor is an infinity at layout */
static void test_l118_math_zero_basis(void) {
    lays("<body style='margin:0'><div style='width:100px'><div id=a style='width:50px;width:calc(10px / sign(100% - 100px))'>a</div>"
         "<div id=b style='width:calc(10px / sign(100%))'>b</div></div></body>");
    TEST_ASSERT_EQUAL(WE_LAYOUT_MAX, el("a")->w);
    TEST_ASSERT_EQUAL(10, el("b")->w);
}

/* viewport units in the style: the layout viewport, else the media size */
static void test_l118_viewport_units(void) {
    lays("<body style='margin:0'><div id=a style='width:50vw;height:10vh'>a</div>"
         "<div id=b style='width:100vmin;height:1vmax'>b</div>"
         "<div id=c style='width:25svw;height:5dvh;margin-left:2lvw'>c</div>"
         "<div id=d style='width:calc(100vw - 40px);padding-left:1vi'>d</div>"
         "<div id=e style='font-size:5vw'>e</div></body>");
    TEST_ASSERT_EQUAL(320, el("a")->w);
    TEST_ASSERT_EQUAL(48, el("a")->h);
    TEST_ASSERT_EQUAL(480, el("b")->w);
    TEST_ASSERT_EQUAL(6, el("b")->h);              /* 6.4 rounds to 6 */
    TEST_ASSERT_EQUAL(160, el("c")->w);
    TEST_ASSERT_EQUAL(24, el("c")->h);
    TEST_ASSERT_EQUAL(13, el("c")->margin[3]);     /* 12.8 */
    TEST_ASSERT_EQUAL(606, el("d")->w);            /* 600 and the 6px padding */
    TEST_ASSERT_EQUAL(6, el("d")->padding[3]);
    TEST_ASSERT_EQUAL(3200, el("e")->font100);
    TEST_ASSERT_EQUAL(1, g_doc.vp_used);
    TEST_ASSERT_EQUAL(640, g_doc.style_vw);
    TEST_ASSERT_EQUAL(480, g_doc.style_vh);
    TEST_ASSERT_EQUAL(0, g_doc.vp_restyles);
}

/* a layout on another viewport styles a page that used them again */
static void test_l118_viewport_restyle(void) {
    lay_at("<body style='margin:0'><div id=a style='width:50vw;height:10vh'>a</div></body>", 800, 300);
    TEST_ASSERT_EQUAL(400, el("a")->w);
    TEST_ASSERT_EQUAL(30, el("a")->h);
    TEST_ASSERT_EQUAL(1, g_doc.vp_restyles);
    TEST_ASSERT_EQUAL(800, g_doc.style_vw);
    TEST_ASSERT_EQUAL(300, g_doc.style_vh);
    /* the same viewport again: no restyle; another one: one more */
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 800, 300));
    TEST_ASSERT_EQUAL(1, g_doc.vp_restyles);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 800, 600));
    TEST_ASSERT_EQUAL(2, g_doc.vp_restyles);
    TEST_ASSERT_EQUAL(60, el("a")->h);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 1000, 600));
    TEST_ASSERT_EQUAL(3, g_doc.vp_restyles);
    TEST_ASSERT_EQUAL(500, el("a")->w);
    /* the count saturates at 255 */
    {
        int k;
        for (k = 0; k < 300; k++) TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, k & 1 ? 1000 : 900, 600));
        TEST_ASSERT_EQUAL(255, g_doc.vp_restyles);
    }
    /* a page without them keeps its style */
    lay_at("<body style='margin:0'><div id=a style='width:50%'>a</div></body>", 800, 300);
    TEST_ASSERT_EQUAL(0, g_doc.vp_used);
    TEST_ASSERT_EQUAL(0, g_doc.vp_restyles);
    TEST_ASSERT_EQUAL(400, el("a")->w);
    /* the media size before any layout */
    we_forms_init(&g_forms);
    we_set_forms(&g_doc, &g_forms);
    TEST_ASSERT_EQUAL(0, we_parse(&g_doc, "<body style='margin:0'><div id=a style='width:10vw;height:10vh'>a</div></body>", 79));
    we_set_media(&g_doc, 500, 200);
    TEST_ASSERT_EQUAL(0, we_style(&g_doc));
    TEST_ASSERT_EQUAL(500, g_doc.style_vw);
    TEST_ASSERT_EQUAL(200, g_doc.style_vh);
    TEST_ASSERT_EQUAL(0, we_layout_viewport(&g_doc, 500, 200));
    TEST_ASSERT_EQUAL(0, g_doc.vp_restyles);
    TEST_ASSERT_EQUAL(50, el("a")->w);
    TEST_ASSERT_EQUAL(20, el("a")->h);
}

/* vi and vb follow the root's writing mode */
static void test_l118_viewport_logical(void) {
    lays("<html style='writing-mode:vertical-rl'><body style='margin:0'><div id=a style='margin-left:10vi;margin-right:10vb'>a</div></body></html>");
    TEST_ASSERT_EQUAL(48, el("a")->margin[3]);
    TEST_ASSERT_EQUAL(64, el("a")->margin[1]);
    lays("<html><body style='margin:0;writing-mode:vertical-rl'><div id=a style='margin-left:10vi;margin-right:10vb'>a</div></body></html>");
    TEST_ASSERT_EQUAL(64, el("a")->margin[3]);      /* the root is horizontal */
    TEST_ASSERT_EQUAL(48, el("a")->margin[1]);
}

/* math in the lengths read as whole px */
static void test_l118_fixed_lengths(void) {
    lays("<body style='margin:0'><div id=a style='text-indent:calc(10px + 5.5px);letter-spacing:calc(1px * 3);word-spacing:min(4px, 2px)'>a b</div>"
         "<div id=b style='text-indent:5px;text-indent:calc(10% + 1px);letter-spacing:calc(100px)'>b</div>"
         "<div id=c style='text-indent:calc(2.5em)'>c</div></body>");
    TEST_ASSERT_EQUAL(15, el("a")->text_indent);
    TEST_ASSERT_EQUAL(3, el("a")->letter_spacing);
    TEST_ASSERT_EQUAL(2, el("a")->word_spacing);
    TEST_ASSERT_EQUAL(5, el("b")->text_indent);
    TEST_ASSERT_EQUAL(0, el("b")->letter_spacing);
    TEST_ASSERT_EQUAL(1, diag_count("text indent refused"));
    TEST_ASSERT_EQUAL(1, diag_count("letter-spacing refused"));
    TEST_ASSERT_EQUAL(40, el("c")->text_indent);
    /* past +-2000 px it is 2000 */
    lays("<body style='margin:0'><div id=a style='margin-top:calc(1px * 5000);vertical-align:calc(-1px * 3000)'>a</div></body>");
    TEST_ASSERT_EQUAL(0, diag_count("vertical align refused") - 1);
}

/* number math: integers round half up and clamp, numbers clamp */
static void test_l118_number_props(void) {
    lays("<body style='margin:0'><div style='display:flex'>"
         "<div id=a style='order:calc(1 + 1);z-index:calc(2.5 * 2);flex-grow:calc(1 / 2);flex-shrink:round(down, 7.9)'>a</div>"
         "<div id=b style='order:calc(-2.5);z-index:calc(-1.5);flex-grow:calc(1000 * 5);flex-shrink:calc(-3)'>b</div>"
         "<div id=c style='order:calc(infinity);z-index:calc(-infinity);flex-grow:calc(NaN)'>c</div>"
         "<div id=d style='order:3;order:calc(1px);z-index:4;z-index:calc(1deg);flex-grow:6;flex-grow:calc(50%)'>d</div>"
         "</div></body>");
    TEST_ASSERT_EQUAL(2, el("a")->item_order);
    TEST_ASSERT_EQUAL(5, el("a")->z_index);
    TEST_ASSERT_EQUAL(1, el("a")->z_set);
    TEST_ASSERT_EQUAL(1, el("a")->flex_grow);      /* 0.5 rounds up */
    TEST_ASSERT_EQUAL(7, el("a")->flex_shrink);
    TEST_ASSERT_EQUAL(-2, el("b")->item_order);    /* -2.5 rounds up */
    TEST_ASSERT_EQUAL(-1, el("b")->z_index);
    TEST_ASSERT_EQUAL(1000, el("b")->flex_grow);
    TEST_ASSERT_EQUAL(0, el("b")->flex_shrink);
    TEST_ASSERT_EQUAL(WE_ORDER_MAX, el("c")->item_order);
    TEST_ASSERT_EQUAL(-WE_Z_MAX, el("c")->z_index);
    TEST_ASSERT_EQUAL(0, el("c")->flex_grow);
    TEST_ASSERT_EQUAL(3, el("d")->item_order);
    TEST_ASSERT_EQUAL(4, el("d")->z_index);
    TEST_ASSERT_EQUAL(6, el("d")->flex_grow);
    TEST_ASSERT_EQUAL(1, diag_count("order refused"));
    TEST_ASSERT_EQUAL(1, diag_count("z-index refused"));
    TEST_ASSERT_EQUAL(1, diag_count("flex-grow refused"));
    /* order and z-index of whole values keep their bounds */
    lays("<body><div id=a style='order:1;order:500;z-index:2;z-index:500'>a</div></body>");
    TEST_ASSERT_EQUAL(1, el("a")->item_order);
    TEST_ASSERT_EQUAL(2, el("a")->z_index);
}

static void test_l118_opacity(void) {
    lays("<body><div id=a style='opacity:calc(1 / 4)'>a</div><div id=b style='opacity:calc(2)'>b</div>"
         "<div id=c style='opacity:calc(-1)'>c</div><div id=d style='opacity:calc(0.123)'>d</div>"
         "<div id=e style='opacity:0.5;opacity:calc(1px)'>e</div><div id=f style='opacity:calc(1)'>f</div></body>");
    TEST_ASSERT_EQUAL(75, el("a")->transparency);
    TEST_ASSERT_EQUAL(0, el("b")->transparency);
    TEST_ASSERT_EQUAL(100, el("c")->transparency);
    TEST_ASSERT_EQUAL(88, el("d")->transparency);  /* 12.3 % cut to 12 */
    TEST_ASSERT_EQUAL(50, el("e")->transparency);
    TEST_ASSERT_EQUAL(0, el("f")->transparency);
    TEST_ASSERT_EQUAL(1, diag_count("opacity refused"));
}

/* line-height: a number expression is unitless, a length keeps hundredths */
static void test_l118_line_height(void) {
    lays("<body style='margin:0'><div id=a style='font-size:15px;line-height:calc(1.5)'>a</div>"
         "<div id=b style='font-size:15px;line-height:calc(1.5em)'>b</div>"
         "<div id=c style='line-height:calc(-2)'>c</div>"
         "<div id=d style='line-height:calc(100)'>d</div>"
         "<div id=e style='line-height:calc(0.001)'>e</div>"
         "<div id=f style='line-height:calc(10.555px + 0px)'>f</div>"
         "<div id=g style='line-height:calc(0px)'>g</div></body>");
    TEST_ASSERT_EQUAL(150, el("a")->line_height);
    TEST_ASSERT_EQUAL(1, el("a")->lh_unitless);
    TEST_ASSERT_EQUAL(22, el("b")->line_height);
    TEST_ASSERT_EQUAL(50, el("b")->lh_frac);
    TEST_ASSERT_EQUAL(0, el("b")->lh_unitless);
    TEST_ASSERT_EQUAL(1, el("c")->line_height);    /* clamped to 0: the least step */
    TEST_ASSERT_EQUAL(1, el("c")->lh_unitless);
    TEST_ASSERT_EQUAL(WE_LH_NUMBER_MAX * 100, el("d")->line_height);
    TEST_ASSERT_EQUAL(1, el("e")->line_height);
    TEST_ASSERT_EQUAL(10, el("f")->line_height);
    TEST_ASSERT_EQUAL(55, el("f")->lh_frac);
    TEST_ASSERT_EQUAL(0, el("g")->line_height);
    TEST_ASSERT_EQUAL(1, el("g")->lh_frac);
    TEST_ASSERT_EQUAL(0, diag_count("line height refused"));
}

/* font-size math keeps hundredths */
static void test_l118_font_size(void) {
    lays("<body style='margin:0'><div id=a style='font-size:calc(10px + 0.555px)'>a</div>"
         "<div id=b style='font-size:calc(100px * pi)'>b</div>"
         "<div id=c style='font-size:12px;font-size:calc(0.4px)'>c</div>"
         "<div id=d style='font-size:calc(50% + 0.25px)'>d</div></body>");
    TEST_ASSERT_EQUAL(1055, el("a")->font100);
    TEST_ASSERT_EQUAL(6400, el("b")->font100);
    TEST_ASSERT_EQUAL(1200, el("c")->font100);
    TEST_ASSERT_EQUAL(825, el("d")->font100);
}

/* angles and numbers in transforms */
static void test_l118_transforms(void) {
    lays("<body><div id=a style='transform:rotate(calc(45deg * 2))'>a</div>"
         "<div id=b style='transform:rotate(atan2(1, 1))'>b</div>"
         "<div id=c style='transform:scale(calc(1 / 2))'>c</div>"
         "<div id=d style='transform:rotate(calc(0.25turn + 10grad))'>d</div>"
         "<div id=e style='transform:rotate(5deg);transform:rotate(calc(1px))'>e</div>"
         "<div id=f style='transform:scale(calc(10))'>f</div>"
         "<div id=g style='transform:rotate(acos(-1))'>g</div>"
         "<div id=h style='transform:rotate(atan2(1, 1)) translate(calc(3px * 2), max(1px, 4px))'>h</div>"
         "<div id=i style='transform:translateX(7px);transform:translate(calc(10%))'>i</div>"
         "<div id=j style='transform-origin:calc(2px * 3) min(9px, 4px)'>j</div>"
         "<div id=k style='transform:scale(2);transform:scale(calc(1px))'>k</div>"
         "<div id=l style='transform:scale(2);transform:scale(x)'>l</div></body>");
    TEST_ASSERT_EQUAL(WE_TF_ROTATE, el("a")->tf[0].kind);
    TEST_ASSERT_EQUAL(90, el("a")->tf[0].x);
    TEST_ASSERT_EQUAL(45, el("b")->tf[0].x);
    TEST_ASSERT_EQUAL(WE_TF_SCALE, el("c")->tf[0].kind);
    TEST_ASSERT_EQUAL(500, el("c")->tf[0].x);
    TEST_ASSERT_EQUAL(99, el("d")->tf[0].x);
    TEST_ASSERT_EQUAL(5, el("e")->tf[0].x);
    TEST_ASSERT_EQUAL(0, el("f")->tf_count);
    TEST_ASSERT_EQUAL(180, el("g")->tf[0].x);
    /* a comma inside a math argument does not split the transform arguments */
    TEST_ASSERT_EQUAL(2, el("h")->tf_count);
    TEST_ASSERT_EQUAL(45, el("h")->tf[0].x);
    TEST_ASSERT_EQUAL(WE_TF_TRANSLATE, el("h")->tf[1].kind);
    TEST_ASSERT_EQUAL(6, el("h")->tf[1].x);
    TEST_ASSERT_EQUAL(4, el("h")->tf[1].y);
    TEST_ASSERT_EQUAL(0, el("h")->tf[1].pct);
    /* a math percentage stays refused in a translation */
    TEST_ASSERT_EQUAL(7, el("i")->tf[0].x);
    TEST_ASSERT_EQUAL(2000, el("k")->tf[0].x);   /* a length is not a scale factor */
    TEST_ASSERT_EQUAL(2000, el("l")->tf[0].x);
    TEST_ASSERT_EQUAL(5, diag_count("transform refused"));
    TEST_ASSERT_EQUAL(6, el("j")->tf_origin[0]);
    TEST_ASSERT_EQUAL(4, el("j")->tf_origin[1]);
    TEST_ASSERT_EQUAL(0, el("j")->tf_origin_pct);
}

/* ---------- @property ---------- */
#define PROP(name, syn, inh, init) "@property " name " { syntax: '" syn "'; inherits: " inh "; initial-value: " init " } "

/* a registered length is px where it is declared */
static void test_l118_property_length(void) {
    lays("<html><head><style>" PROP("--w", "<length>", "true", "10px")
         "#a { font-size: 10px; --w: 2em } #b { font-size: 20px; width: var(--w) } #c { width: var(--w) }"
         " #d { --w: 3em; font-size: 20px; width: var(--w) } #e { --u: 2em; font-size: 10px } #f { font-size: 20px; width: var(--u) }"
         "</style></head><body style='margin:0'><div id=a><div id=b>x</div></div><div id=c>y</div><div id=d>z</div>"
         "<div id=e><div id=f>w</div></div></body></html>");
    TEST_ASSERT_EQUAL(20, el("b")->w);      /* 2em of #a */
    TEST_ASSERT_EQUAL(10, el("c")->w);      /* the initial value */
    TEST_ASSERT_EQUAL(60, el("d")->w);      /* 3em of #d's own 20px */
    TEST_ASSERT_EQUAL(40, el("f")->w);      /* not registered: 2em of #f */
    TEST_ASSERT_EQUAL(1, g_doc.nprops);
}

/* inherits: false, invalid values, var() fallbacks */
static void test_l118_property_inherit(void) {
    lays("<html><head><style>" PROP("--m", "<length>", "false", "7px")
         "#a { --m: 30px; margin-left: var(--m) } #b { margin-left: var(--m, 99px) } #c { --m: red; margin-left: var(--m) }"
         "</style></head><body style='margin:0'><div id=a><div id=b>x</div><div id=c>y</div></div></body></html>");
    TEST_ASSERT_EQUAL(30, el("a")->margin[3]);
    TEST_ASSERT_EQUAL(7, el("b")->margin[3]);
    TEST_ASSERT_EQUAL(7, el("c")->margin[3]);
    TEST_ASSERT_EQUAL(1, diag_count("css property syntax refused"));
    /* an inherited one: an invalid value inherits */
    lays("<html><head><style>" PROP("--m", "<length>", "true", "7px")
         "#a { --m: 30px } #b { --m: 1deg; margin-left: var(--m) } #c { --m: 1deg; margin-left: var(--m) }"
         "</style></head><body style='margin:0'><div id=a><div id=b>x</div></div><div id=c>y</div></body></html>");
    TEST_ASSERT_EQUAL(30, el("b")->margin[3]);
    TEST_ASSERT_EQUAL(7, el("c")->margin[3]);
    TEST_ASSERT_EQUAL(2, diag_count("css property syntax refused"));
    /* '*' without an initial value: the fallback */
    lays("<html><head><style>@property --any { syntax: '*'; inherits: false } #a { margin-left: var(--any, 9px) }"
         " #b { --any: 4px; margin-left: var(--any, 9px) }</style></head><body style='margin:0'><div id=a>x</div><div id=b>y</div></body></html>");
    TEST_ASSERT_EQUAL(9, el("a")->margin[3]);
    TEST_ASSERT_EQUAL(4, el("b")->margin[3]);
    TEST_ASSERT_EQUAL(0, diag_count("css property"));
}

/* the syntaxes and their computed values */
static void test_l118_property_syntax(void) {
    lays("<html><head><style>" PROP("--n", "<number>", "true", "1.5") PROP("--i", "<integer>", "true", "2")
         PROP("--a", "<angle>", "true", "0.25turn") PROP("--p", "<percentage>", "true", "50%")
         PROP("--lp", "<length-percentage>", "true", "25%") PROP("--c", "<color>", "true", "#ff0000")
         "#a { opacity: calc(var(--n) / 3); order: var(--i); transform: rotate(var(--a)); width: var(--p); color: var(--c) }"
         " #b { --n: calc(1 / 4); --i: calc(2.6); --a: calc(pi * 1rad); --p: 10%; --lp: calc(10% + 2em); --c: blue;"
         " opacity: var(--n); order: var(--i); transform: rotate(var(--a)); width: var(--p); margin-left: var(--lp); color: var(--c) }"
         " #c { --i: 2.5; --n: 1px; --a: 1px; --p: 1px; --lp: red; --c: 1px; order: var(--i); width: var(--lp) }"
         "</style></head><body style='margin:0'><div style='width:400px'><div id=a>x</div><div id=b>y</div><div id=c>z</div></div></body></html>");
    TEST_ASSERT_EQUAL(50, el("a")->transparency);
    TEST_ASSERT_EQUAL(2, el("a")->item_order);
    TEST_ASSERT_EQUAL(90, el("a")->tf[0].x);
    TEST_ASSERT_EQUAL(200, el("a")->w);
    TEST_ASSERT_EQUAL(0xff0000u, el("a")->color & 0xffffffu);
    TEST_ASSERT_EQUAL(75, el("b")->transparency);
    TEST_ASSERT_EQUAL(3, el("b")->item_order);
    TEST_ASSERT_EQUAL(180, el("b")->tf[0].x);
    TEST_ASSERT_EQUAL(40, el("b")->w);
    TEST_ASSERT_EQUAL(0x0000ffu, el("b")->color & 0xffffffu);
    TEST_ASSERT_EQUAL(2, el("c")->item_order);     /* 2.5 is not an integer: inherited 2 */
    TEST_ASSERT_EQUAL(100, el("c")->w);           /* --lp inherited: 25% */
    TEST_ASSERT_EQUAL(6, diag_count("css property syntax refused"));
}

/* the rules refused, the last of a name, the table */
static void test_l118_property_rules(void) {
    static const char *const refused[] = {
        "@property --a { syntax: '<length>'; initial-value: 1px }",
        "@property --b { inherits: true; initial-value: 1px }",
        "@property --c { syntax: '<length>+#'; inherits: true; initial-value: 1px }",
        "@property --d { syntax: <length>; inherits: true; initial-value: 1px }",
        "@property --e { syntax: '<length>'; inherits: yes; initial-value: 1px }",
        "@property --f { syntax: '<length>'; inherits: true }",
        "@property --g { syntax: '<length>'; inherits: true; initial-value: 1em }",
        "@property --h { syntax: '<length>'; inherits: true; initial-value: 10vw }",
        "@property --i { syntax: '<length>'; inherits: true; initial-value: red }",
        "@property x { syntax: '*'; inherits: true }",
        "@property --j k { syntax: '*'; inherits: true }", 0 };
    int r;
    for (r = 0; refused[r]; r++) {
        char h[200];
        sprintf(h, "<style>%s</style><p>x</p>", refused[r]);
        lays(h);
        if (g_doc.nprops) printf("  kept: %s\n", refused[r]);
        TEST_ASSERT_EQUAL(0, g_doc.nprops);
        TEST_ASSERT_EQUAL(1, diag_count("css property rule refused"));
    }
    lays("<html><head><style>"
         "@property --a { syntax: '<length>'; initial-value: 1px } "           /* no inherits */
         "@property --b { inherits: true; initial-value: 1px } "               /* no syntax */
         "@property --c { syntax: '<length>+#'; inherits: true; initial-value: 1px } "
         "@property --d { syntax: <length>; inherits: true; initial-value: 1px } "
         "@property --e { syntax: '<length>'; inherits: yes; initial-value: 1px } "
         "@property --f { syntax: '<length>'; inherits: true } "               /* no initial value */
         "@property --g { syntax: '<length>'; inherits: true; initial-value: 1em } "
         "@property --h { syntax: '<length>'; inherits: true; initial-value: 10vw } "
         "@property --i { syntax: '<length>'; inherits: true; initial-value: red } "
         "@property x { syntax: '*'; inherits: true } "
         "@property --j k { syntax: '*'; inherits: true } "
         "@property --ok { syntax: '<length>'; inherits: true; initial-value: calc(1px + 2.5px) } "
         "@property --ok { syntax: '<number>'; inherits: true; initial-value: 3 } "
         "@property --sp { syntax: ' <length> '; inherits: true; initial-value: 0 } "
         "#a { order: var(--ok); margin-left: var(--g, 5px); margin-right: var(--sp) }"
         "</style></head><body style='margin:0'><div id=a>x</div></body></html>");
    TEST_ASSERT_EQUAL(8, diag_count("css property rule refused"));   /* WE_MAX_DIAG */
    TEST_ASSERT_EQUAL(3, g_doc.diag_dropped);
    TEST_ASSERT_EQUAL(2, g_doc.nprops);
    TEST_ASSERT_EQUAL(3, el("a")->item_order);
    TEST_ASSERT_EQUAL(5, el("a")->margin[3]);
    TEST_ASSERT_EQUAL(0, el("a")->margin[1]);
    {
        char css[2048];
        int n = sprintf(css, "<html><head><style>"), k;
        for (k = 0; k < WE_PROPS_MAX + 1; k++) n += sprintf(css + n, "@property --p%d { syntax: '*'; inherits: true } ", k);
        sprintf(css + n, "</style></head><body>x</body></html>");
        lays(css);
        TEST_ASSERT_EQUAL(WE_PROPS_MAX, g_doc.nprops);
        TEST_ASSERT_EQUAL(1, diag_count("css property table full"));
    }
}


/* Lot 118: math, angles, none, hwb() and the modern forms in colors */
static void test_l118_colors(void) {
    static const struct { const char *css; uint32_t want; } cases[] = {
        { "rgb(calc(255 / 2) 0 none)", 0x800000u },
        { "rgb(calc( 10 + 20 ) 0 0)", 0x1e0000u },
        { "rgb(100% 0 50%)", 0xff0080u },               /* the space form mixes */
        { "rgb(100%, 0, 50%)", 0x010203u },             /* the comma form does not */
        { "rgb(255, 0, 128)", 0xff0080u },
        { "rgb(1, 2, none)", 0x010203u },               /* none: spaces only */
        { "rgb(0 0 255 / none)", 0xff0000ffu },          /* alpha none is 0 */
        { "rgb(0 0 255 / calc(1 / 2))", 0x7f0000ffu },
        { "rgb(calc(50%) 0 0)", 0x010203u },
        { "rgb(calc(1px) 0 0)", 0x010203u },
        { "rgb(calc(10deg) 0 0)", 0x010203u },          /* an angle is for the hue only */
        { "rgb(0 0 calc(1turn / 4))", 0x010203u },
        { "rgb(calc(1 0 0)", 0x010203u },
        { "rgb(1) 0 (0)", 0x010203u },
        { "rgb(calc(1)) 0 0)", 0x010203u },
        { "rgb(0 0 calc((1))", 0x010203u },
        { "rgb(0deg 0 0)", 0x010203u },                 /* Lot 120: an angle on the hue only */
        { "hsl(0.5turn 100 50)", 0x00ffffu },           /* numbers as percentages */
        { "hsl(180deg, 100%, 50%)", 0x00ffffu },
        { "hsl(120, 50, 50)", 0x010203u },
        { "hsl(calc(60deg * 2) 100% 25%)", 0x008000u },
        { "hsl(calc(120) 100% 25%)", 0x008000u },
        { "hsl(200grad 100% 50%)", 0x00ffffu },
        { "hsl(3.14159rad 100% 50%)", 0x00fffeu },       /* 57.3 deg a radian: 180.013 */
        { "hsl(10% 100% 50%)", 0x010203u },
        { "hsl(none 100% 50%)", 0xff0000u },
        { "hsl(none, 100%, 50%)", 0x010203u },
        { "hsla(120, 100%, 25%, calc(1 / 2))", 0x7f008000u },
        { "hsl(calc(1px) 100% 50%)", 0x010203u },
        { "hwb(120 20% 30%)", 0x33b333u },
        { "hwb(0 60% 60%)", 0x808080u },
        { "hwb(0 100% 0%)", 0xffffffu },
        { "hwb(0 0% 100%)", 0x000000u },
        { "hwb(240 0 0)", 0x0000ffu },
        { "hwb(0 0% 0% / 0.5)", 0x7fff0000u },
        { "hwb(0, 20%, 30%)", 0x010203u },
        { "hwb(30% 0% 0%)", 0x010203u },
        { "hwb(60 10% 10% 1)", 0x010203u },
        { "HWB(0.25turn 0% 50%)", 0x408000u },
    };
    unsigned k;
    for (k = 0; k < sizeof(cases) / sizeof(cases[0]); k++) {
        uint32_t got = color_of(cases[k].css);
        if (got != cases[k].want) printf("  %s: %08x\n", cases[k].css, (unsigned)got);
        TEST_ASSERT_EQUAL(cases[k].want, got);
    }
}

/* Lot 118: numbers in scientific notation in math (CSS Syntax 3 4.3.12) */
static void test_l118_scientific(void) {
    static const struct { const char *expr; int basis, type, milli; } cases[] = {
        { "calc(1e3)", 0, 0, 1000000 },
        { "calc(1E3)", 0, 0, 1000000 },
        { "calc(2.5e-1)", 0, 0, 250 },
        { "calc(1e+2)", 0, 0, 100000 },
        { "calc(.5e1)", 0, 0, 5000 },
        { "calc(12e-3)", 0, 0, 12 },
        { "calc(3e-4)", 0, 0, 0 },                      /* cut toward zero */
        { "calc(-2e1)", 0, 0, -20000 },
        { "calc(1.5e1px)", 0, 1, 15000 },
        { "calc(5e1%)", 200, 1, 100000 },
        { "calc(20009e-1)", 0, 0, 2000900 },
        { "calc(2000999e-3)", 0, 0, 2000999 },
        { "calc(2001e0)", 0, -1, 0 },                   /* the literal bound */
        { "calc(2001000e-3)", 0, -1, 0 },
        { "calc(0.000000000123456789e10)", 0, 0, 1234 },
        { "calc(1234567891234e-10)", 0, 0, 123456 },   /* seven significant digits */
        { "calc(1234567e-3)", 0, 0, 1234567 },
        { "calc(0.001234567e6)", 0, 0, 1234567 },
        { "calc(0e100)", 0, -1, 0 },                    /* three exponent digits */
        { "calc(1e-99)", 0, 0, 0 },
        { "calc(10000000000e-10)", 0, 0, 1000 },
        { "calc(0e99)", 0, 0, 0 },
        { "calc(1e99)", 0, -1, 0 },
        { "calc(1e123)", 0, -1, 0 },                    /* three exponent digits */
        { "calc(1.e2)", 0, -1, 0 },
        { "calc(1..5e2)", 0, -1, 0 },
        { "calc(1e3em)", 0, -1, 0 },                    /* units other than px */
        { "calc(1e1deg)", 0, -1, 0 },
        { "calc(1e)", 0, -1, 0 },
        { "calc(1e+)", 0, -1, 0 },
        { "calc(2ex)", 0, 1, 10000 },                   /* a unit, not an exponent */
        { "calc(1e1 * 2px)", 0, 1, 20000 },
    };
    unsigned k;
    we_unit_ctx_t c;
    memset(&c, 0, sizeof(c));
    c.font100 = 1600; c.root100 = 1600; c.vw = 640; c.vh = 480;
    for (k = 0; k < sizeof(cases) / sizeof(cases[0]); k++) {
        int type = -9, milli = 0;
        int rc = we_math_milli(cases[k].expr, (int)strlen(cases[k].expr), &c, cases[k].basis, &type, &milli);
        int ok = cases[k].type < 0 ? rc == -1 : rc == 0 && type == cases[k].type && milli == cases[k].milli;
        if (!ok) printf("  %s: rc %d type %d milli %d\n", cases[k].expr, rc, type, milli);
        TEST_ASSERT_TRUE(ok);
    }
    lays("<body style='margin:0'><div id=a style='width:calc(1e2px);margin-left:calc(2.5e1px)'>a</div></body>");
    TEST_ASSERT_EQUAL(100, el("a")->w);
    TEST_ASSERT_EQUAL(25, el("a")->margin[3]);
}

/* Lot 118: @property syntaxes with |, + and #, <time>, <custom-ident> and
 * identifiers */
static void test_l118_property_lists(void) {
    lays("<html><head><style>"
         "@property --l { syntax: '<length>+ | auto'; inherits: true; initial-value: 1px 2px } "
         "@property --c { syntax: '<color># | none'; inherits: false; initial-value: red } "
         "@property --t { syntax: '<time>'; inherits: true; initial-value: 250ms } "
         "@property --i { syntax: '<custom-ident>'; inherits: true; initial-value: foo } "
         "@property --n { syntax: '<number> | <length>'; inherits: true; initial-value: 0 } "
         "@property --s { syntax: \"<integer>#\"; inherits: true; initial-value: 1 } "
         "</style></head><body style='margin:0'>"
         "<div id=a style='--l: calc(1px * 3)  1em;--n: 1.50;--s: calc(2.5),3 ,4;--t:1.5s'>a"
         "<div id=a2 style='--n: 2em'>a2</div></div>"
         "<div id=b style='--l:auto;--c: red , blue;--t:2s;--i:Bar'>b</div>"
         "<div id=c style='--l:Auto;--c:none;--i:inherit;--n:x'>c</div>"
         "<div id=e style='--c: red,;--t:1.5;--l: ;--i:default;--s:1,,2'>e</div>"
         "<div id=f style='--t:-500ms;--c:,red;--l:1px auto'>f</div>"
         "</body></html>");
    TEST_ASSERT_EQUAL(6, g_doc.nprops);
    TEST_ASSERT_EQUAL_STRING("3px 16px", var_text("a", "--l"));
    TEST_ASSERT_EQUAL_STRING("1.5", var_text("a", "--n"));        /* <number> first */
    TEST_ASSERT_EQUAL_STRING("32px", var_text("a2", "--n"));      /* then <length> */
    TEST_ASSERT_EQUAL_STRING("3, 3, 4", var_text("a", "--s"));
    TEST_ASSERT_EQUAL_STRING("1.5s", var_text("a", "--t"));
    TEST_ASSERT_EQUAL_STRING("auto", var_text("b", "--l"));
    TEST_ASSERT_EQUAL_STRING("red, blue", var_text("b", "--c"));
    TEST_ASSERT_EQUAL_STRING("2s", var_text("b", "--t"));
    TEST_ASSERT_EQUAL_STRING("Bar", var_text("b", "--i"));
    TEST_ASSERT_EQUAL_STRING("", var_text("c", "--l"));             /* identifiers are case-sensitive */
    TEST_ASSERT_EQUAL_STRING("none", var_text("c", "--c"));
    TEST_ASSERT_EQUAL_STRING("", var_text("c", "--n"));
    TEST_ASSERT_EQUAL_STRING("", var_text("e", "--c"));
    TEST_ASSERT_EQUAL_STRING("", var_text("e", "--t"));
    TEST_ASSERT_EQUAL_STRING("", var_text("e", "--i"));
    TEST_ASSERT_EQUAL_STRING("", var_text("e", "--s"));
    TEST_ASSERT_EQUAL_STRING("-0.5s", var_text("f", "--t"));
    TEST_ASSERT_EQUAL_STRING("", var_text("f", "--c"));
    TEST_ASSERT_EQUAL_STRING("", var_text("f", "--l"));
    TEST_ASSERT_EQUAL(8, diag_count("css property syntax refused"));
    TEST_ASSERT_EQUAL(0, diag_count("css property rule refused"));
}

/* Lot 118: the computed initial values and the refused syntaxes */
static void test_l118_property_lists_rules(void) {
    /* each with an initial value that the refused syntax would match */
    static const char *const refused[][2] = {
        { "'<length> |'", "1px" }, { "'| <length>'", "1px" }, { "'<length>++'", "1px" }, { "'* | <length>'", "1px" },
        { "'inherit'", "inherit" }, { "'default'", "default" }, { "'unset'", "unset" }, { "'<transform-list>'", "1px" },
        { "'<length> +'", "1px" }, { "'a|b|c|d|e|f|g|h|i'", "a" }, { "'--x'", "--x" }, { "'-1a'", "-1a" },
        { "'1a'", "1a" }, { "'<length>' x", "1px" }, { "'-'", "-" }, { "'<foo> | <length>'", "1px" }, { 0, 0 } };
    int r;
    for (r = 0; refused[r][0]; r++) {
        char h[300];
        sprintf(h, "<style>@property --z { syntax: %s; inherits: true; initial-value: %s }</style><p>x</p>", refused[r][0],
                refused[r][1]);
        lays(h);
        if (g_doc.nprops) printf("  kept: %s\n", refused[r][0]);
        TEST_ASSERT_EQUAL(0, g_doc.nprops);
        TEST_ASSERT_EQUAL(1, diag_count("css property rule refused"));
    }
    /* eight components and identifiers with digits and dashes are kept */
    lays("<style>@property --z { syntax: 'a|b|c|d|e|f|g|h'; inherits: true; initial-value: h } "
         "@property --y { syntax: 'x-1 | _y | -z'; inherits: true; initial-value: -z } "
         "@property --t { syntax: '<time>#'; inherits: true; initial-value: 250ms, 1S } "
         "@property --u { syntax: '<length>'; inherits: true; initial-value: 1em } "
         "@property --w { syntax: '<length>#'; inherits: true; initial-value: 1px, 1vw } "
         "#p { --y: x-1 }</style><p id=p>x</p>");
    TEST_ASSERT_EQUAL(3, g_doc.nprops);
    TEST_ASSERT_EQUAL(2, diag_count("css property rule refused"));   /* --u, --w: not independent */
    TEST_ASSERT_EQUAL_STRING("x-1", var_text("p", "--y"));
    lays("<style>@property --t { syntax: '<time>#'; inherits: true; initial-value: 250ms, 1S } "
         "#p { margin-left: 1px }</style><p id=p>x</p>");
    TEST_ASSERT_EQUAL(1, g_doc.nprops);
    TEST_ASSERT_EQUAL(9, g_doc.props[0].init_len);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.pool + g_doc.props[0].init_off, "0.25s, 1s", 9));
    /* a computed list is kept up to WE_PSYN_LIST_MAX characters: 26 items
     * of "100px" (155) fit, 27 (161) do not */
    {
        char h[600];
        int k, n;
        for (r = 26; r <= 27; r++) {
            n = sprintf(h, "<style>@property --q { syntax: '<length>+'; inherits: true; initial-value:");
            for (k = 0; k < r; k++) n += sprintf(h + n, " 100px");
            sprintf(h + n, " }</style><p>x</p>");
            lays(h);
            TEST_ASSERT_EQUAL(r == 26 ? 1 : 0, g_doc.nprops);
            if (r == 26) TEST_ASSERT_EQUAL(155, g_doc.props[0].init_len);
        }
        /* one item is kept up to 63 characters */
        for (r = 63; r <= 64; r++) {
            n = sprintf(h, "<style>@property --q { syntax: '<custom-ident>+'; inherits: true; initial-value: b ");
            for (k = 0; k < r; k++) h[n++] = 'a';
            sprintf(h + n, " }</style><p>x</p>");
            lays(h);
            TEST_ASSERT_EQUAL(r == 63 ? 1 : 0, g_doc.nprops);
            if (r == 63) TEST_ASSERT_EQUAL(65, g_doc.props[0].init_len);
        }
    }
}

/* Lot 118: the values of the single types, their bounds and the quotes
 * of the descriptors */
static void test_l118_property_values(void) {
    lays("<html><head><style>"
         "@property --n { syntax: '<number>'; inherits: false; initial-value: 1 } "
         "@property --l { syntax: '<length>'; inherits: false; initial-value: 0 } "
         "@property --lp { syntax: '<length-percentage>'; inherits: false; initial-value: 0 } "
         "@property --t { syntax: '<time>'; inherits: false; initial-value: 1s } "
         "@property --q { syntax: '*'; inherits: false; initial-value: 'a;b' } "
         "@property --c { syntax: '<color>'; inherits: false; initial-value: red } "
         "@property --i { syntax: '<integer>'; inherits: false; initial-value: 0 } "
         "</style></head><body style='margin:0'>"
         "<div id=a style='--n: 1000000;--l: calc(1px + 2px);--lp: 12%;--t: 1000000ms;--c: rgb(1 2 3)'>a</div>"
         "<div id=b style='--n: 1000001;--l: calc(1px +);--lp: 12;--t: 1000000.5ms;--c: rgb(x 2 3)'>b</div>"
         "<div id=c style='--n: 1.5a;--t: xms;--lp: calc(1% +);--i: calc(1px)'>c</div>"
         "<div id=d style='--n: 0;--t: xs;--i: calc(2.5)'>d</div>"
         "<div id=e style='--n: -0.25;--t: 0.5S'>e</div><div id=f style='--t: 5x'>f</div>"
         "</body></html>");
    TEST_ASSERT_EQUAL(7, g_doc.nprops);
    TEST_ASSERT_EQUAL_STRING("", var_text("c", "--lp"));
    TEST_ASSERT_EQUAL_STRING("", var_text("c", "--i"));
    TEST_ASSERT_EQUAL_STRING("3", var_text("d", "--i"));
    TEST_ASSERT_EQUAL_STRING("", var_text("f", "--t"));
    TEST_ASSERT_EQUAL_STRING("rgb(1 2 3)", var_text("a", "--c"));
    TEST_ASSERT_EQUAL_STRING("", var_text("b", "--c"));   /* a refused channel refuses the color */
    TEST_ASSERT_EQUAL_STRING("1000000", var_text("a", "--n"));
    TEST_ASSERT_EQUAL_STRING("3px", var_text("a", "--l"));
    TEST_ASSERT_EQUAL_STRING("12%", var_text("a", "--lp"));
    TEST_ASSERT_EQUAL_STRING("1000s", var_text("a", "--t"));
    TEST_ASSERT_EQUAL_STRING("", var_text("b", "--n"));
    TEST_ASSERT_EQUAL_STRING("", var_text("b", "--l"));
    TEST_ASSERT_EQUAL_STRING("", var_text("b", "--lp"));
    TEST_ASSERT_EQUAL_STRING("", var_text("b", "--t"));
    TEST_ASSERT_EQUAL_STRING("", var_text("c", "--n"));
    TEST_ASSERT_EQUAL_STRING("", var_text("c", "--t"));
    TEST_ASSERT_EQUAL_STRING("0", var_text("d", "--n"));
    TEST_ASSERT_EQUAL_STRING("", var_text("d", "--t"));
    TEST_ASSERT_EQUAL_STRING("-0.25", var_text("e", "--n"));
    TEST_ASSERT_EQUAL_STRING("0.5s", var_text("e", "--t"));
    TEST_ASSERT_EQUAL(8, diag_count("css property syntax refused"));   /* WE_MAX_DIAG */
    TEST_ASSERT_EQUAL(3, g_doc.diag_dropped);
    /* the semicolon inside the quoted initial value does not end it */
    TEST_ASSERT_EQUAL(5, g_doc.props[4].init_len);
    TEST_ASSERT_EQUAL(0, memcmp(g_doc.pool + g_doc.props[4].init_off, "'a;b'", 5));
}

static int var_len(const char *id, const char *name) {
    const we_node_t *n = el(id);
    int k, len = (int)strlen(name);
    for (k = 0; n != &g_none && k < n->nvars; k++)
        if (n->var_name_len[k] == len && memcmp(g_doc.pool + n->var_name_off[k], name, (size_t)len) == 0) return n->var_len[k];
    return -1;
}

/* Lot 118: an initial value through var() up to the expansion buffer, and
 * a refused value inherited from the root element */
static void test_l118_property_expand(void) {
    static char h[1400];
    int r, n, k;
    for (r = 512; r <= 513; r++) {
        n = sprintf(h, "<html><head><style>@property --q { syntax: '*'; inherits: false; initial-value: ");
        for (k = 0; k < r; k++) h[n++] = 'x';
        sprintf(h + n, " } #a { --r: var(--q) }</style></head><body><p id=a>x</p></body></html>");
        lays(h);
        TEST_ASSERT_EQUAL(1, g_doc.nprops);
        TEST_ASSERT_EQUAL(r == 512 ? 512 : -1, var_len("a", "--r"));
        TEST_ASSERT_EQUAL(r == 512 ? 0 : 1, diag_count("css variable undefined"));
    }
    lays("<html style='--w: 5px'><head><style>@property --w { syntax: '<length>'; inherits: true; initial-value: 0px }"
         "</style></head><body id=b style='--w: red'><p id=p style='--w: 1x'>x</p></body></html>");
    TEST_ASSERT_EQUAL_STRING("5px", var_text("b", "--w"));
    TEST_ASSERT_EQUAL_STRING("5px", var_text("p", "--w"));
    TEST_ASSERT_EQUAL(2, diag_count("css property syntax refused"));
}

int main(void) {
    unity_init();
    RUN_TEST(test_l118_math_table);
    RUN_TEST(test_l118_math_api);
    RUN_TEST(test_l118_math_used);
    RUN_TEST(test_l118_math_zero_basis);
    RUN_TEST(test_l118_viewport_units);
    RUN_TEST(test_l118_viewport_restyle);
    RUN_TEST(test_l118_viewport_logical);
    RUN_TEST(test_l118_fixed_lengths);
    RUN_TEST(test_l118_number_props);
    RUN_TEST(test_l118_opacity);
    RUN_TEST(test_l118_line_height);
    RUN_TEST(test_l118_font_size);
    RUN_TEST(test_l118_transforms);
    RUN_TEST(test_l118_property_length);
    RUN_TEST(test_l118_property_inherit);
    RUN_TEST(test_l118_property_syntax);
    RUN_TEST(test_l118_property_rules);
    RUN_TEST(test_l118_colors);
    RUN_TEST(test_l118_scientific);
    RUN_TEST(test_l118_property_lists);
    RUN_TEST(test_l118_property_lists_rules);
    RUN_TEST(test_l118_property_values);
    RUN_TEST(test_l118_property_expand);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
