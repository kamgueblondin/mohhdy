/* shell_desk.c - Lot 86: see shell_desk.h. */
#include "shell_desk.h"
#include "wf_nav.h"
#include "wf_tabs.h"
#include "wf_autofill.h"
#include "wf_fetch.h"
#include "shell_search.h"
#include "shell_fetch.h"
#include "osui_gui.h"

void print_string(const char *s);

static desk_cfg_t g_cfg;
static wf_nav_t g_nav;
static wf_tabs_t g_tabs;
static wf_autofill_t g_fill;
static int g_ready;
static int g_hold;

static int slen(const char *s) { int n = 0; while (s && s[n]) n++; return n; }
static int seq(const char *a, const char *b) {
    int i = 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}
static int pre(const char *s, const char *p) {
    while (*p) { if (*s != *p) return 0; s++; p++; }
    return 1;
}
static void skip(const char **s) { while (**s == ' ') (*s)++; }

static void ready(void) {
    char buf[512];
    int n;
    if (g_ready) return;
    g_ready = 1;
    desk_cfg_init(&g_cfg);
    wf_nav_init(&g_nav);
    wf_tabs_init(&g_tabs);
    wf_autofill_init(&g_fill);
    n = osui_gui_persist_read("/persist/settings.cfg", buf, (int)sizeof(buf) - 1);
    if (n > 0) { buf[n] = 0; (void)desk_cfg_load(&g_cfg, buf, n); }
    n = osui_gui_persist_read("/persist/nav.txt", buf, (int)sizeof(buf) - 1);
    if (n > 0) (void)wf_nav_load(&g_nav, buf, n);
    {
        static char fillb[4096];
        n = osui_gui_persist_read("/persist/autofill.txt", fillb, (int)sizeof(fillb) - 1);
        if (n > 0) (void)wf_autofill_load(&g_fill, fillb, n);
    }
}

static int save_fill(void) {
    static char buf[4096];
    int n = wf_autofill_save(&g_fill, buf, (int)sizeof(buf));
    if (n < 0) return -1;
    return osui_gui_persist_write("/persist/autofill.txt", buf, n);
}

int shell_desk_remember_form(const wf_form_page_t *p, int form, const char *url) {
    int n;
    ready();
    n = wf_formfill_remember(&g_fill, p, form, url);
    if (n > 0) (void)save_fill();
    return n;
}

int shell_desk_offers(const wf_form_page_t *p, int field, const char *url, wf_fill_offer_t *o, int cap) {
    ready();
    return wf_formfill_offers(&g_fill, p, field, url, o, cap);
}

int shell_desk_apply_offer(wf_form_page_t *p, int field, int offer, const char *url) {
    wf_fill_offer_t o[WF_FILL_OFFERS];
    int n;
    ready();
    n = wf_formfill_offers(&g_fill, p, field, url, o, WF_FILL_OFFERS);
    if (offer < 0 || offer >= n) return -1;
    return wf_formfill_apply(p, field, &o[offer]);
}

desk_cfg_t *shell_desk_cfg(void) { ready(); return &g_cfg; }
int shell_desk_nav_held(void) { return g_hold; }

int shell_desk_apply(void) {
    char line[48], out[160];
    ready();
    line[0] = 0;
    {
        int n = 0;
        const char *p = "ui-scale ";
        while (*p && n < (int)sizeof(line) - 1) line[n++] = *p++;
        {
            char t[8]; int k = 0, v = g_cfg.scale_q;
            if (v == 0) t[k++] = '0';
            while (v && k < 7) { t[k++] = (char)('0' + v % 10); v /= 10; }
            while (k && n < (int)sizeof(line) - 1) line[n++] = t[--k];
        }
        line[n] = 0;
    }
    out[0] = 0;
    (void)osui_gui_scale_command(line, out, (int)sizeof(out));
    wf_fetch_set_offline(g_cfg.net_on ? 0 : 1);
    {
        char eng[40];
        int n = 0;
        const char *p = "search-engine ";
        const char *name = desk_cfg_engine_name(g_cfg.engine);
        while (*p && n < (int)sizeof(eng) - 1) eng[n++] = *p++;
        while (*name && n < (int)sizeof(eng) - 1) eng[n++] = *name++;
        eng[n] = 0;
        (void)shell_search_line(eng);
    }
    return 0;
}

static int save_cfg(void) {
    char buf[256];
    int n = desk_cfg_save(&g_cfg, buf, (int)sizeof(buf));
    if (n < 0) return -1;
    return osui_gui_persist_write("/persist/settings.cfg", buf, n);
}

static int save_nav(void) {
    static char buf[12 * 1024];
    int n = wf_nav_save(&g_nav, buf, (int)sizeof(buf));
    if (n < 0) return -1;
    return osui_gui_persist_write("/persist/nav.txt", buf, n);
}

void shell_desk_note(const char *url, const char *title) {
    if (g_hold || !url || !url[0]) return;
    ready();
    if (wf_nav_visit(&g_nav, url, title, 0) == 0) (void)save_nav();
    (void)wf_tabs_open(&g_tabs, url, title, 0);
}

int shell_desk_is(const char *line) {
    return (pre(line, "settings") && (line[8] == ' ' || !line[8])) ||
           (pre(line, "bookmark") && (line[8] == ' ' || !line[8])) ||
           (pre(line, "web-history") && (line[11] == ' ' || !line[11])) ||
           (pre(line, "web-back") && (line[8] == ' ' || !line[8])) ||
           (pre(line, "web-forward") && (line[11] == ' ' || !line[11])) ||
           (pre(line, "web-tab") && (line[7] == ' ' || !line[7])) ||
           (pre(line, "autofill") && (line[8] == ' ' || !line[8]));
}

static void print_nav_list(int marks) {
    int i, n = marks ? wf_nav_mark_count(&g_nav) : wf_nav_hist_count(&g_nav);
    print_string(marks ? "bookmark count " : "web-history count ");
    {
        char t[12]; int k = 0, v = n;
        if (!v) print_string("0");
        while (v && k < 11) { t[k++] = (char)('0' + v % 10); v /= 10; }
        while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; print_string(c); }
    }
    print_string("\n");
    for (i = 0; i < n; i++) {
        const wf_nav_ent_t *e = marks ? wf_nav_mark_at(&g_nav, i) : wf_nav_hist_at(&g_nav, i);
        if (!e) continue;
        print_string(marks ? "bookmark " : "hist ");
        print_string(e->url);
        if (e->title[0]) { print_string(" | "); print_string(e->title); }
        print_string("\n");
    }
}

static int go(int back) {
    char url[WF_NAV_URL];
    char line[WF_NAV_URL + 8];
    int n = 0, rc;
    ready();
    if ((back ? wf_nav_back(&g_nav, url, (int)sizeof(url)) : wf_nav_forward(&g_nav, url, (int)sizeof(url))) != 0) {
        print_string(back ? "web-back none\n" : "web-forward none\n");
        return 1;
    }
    print_string(back ? "web-back " : "web-forward ");
    print_string(url);
    print_string("\n");
    line[n++] = 'f'; line[n++] = 'e'; line[n++] = 't'; line[n++] = 'c'; line[n++] = 'h'; line[n++] = ' ';
    while (url[n - 6] && n < (int)sizeof(line) - 1) { line[n] = url[n - 6]; n++; }
    line[n] = 0;
    g_hold = 1;
    rc = shell_fetch_line(line);
    g_hold = 0;
    (void)save_nav();
    return rc;
}

int shell_desk_line(const char *line) {
    const char *a;
    char rep[700];
    ready();
    if (pre(line, "web-tab")) {
        char text[800];
        a = line + 7;
        skip(&a);
        if (!*a || pre(a, "list")) {
            if (wf_tabs_text(&g_tabs, text, (int)sizeof(text)) > 0) print_string(text);
            return 0;
        }
        if (pre(a, "next")) { print_string(wf_tabs_next(&g_tabs, 1) >= 0 ? "web-tab ok\n" : "web-tab none\n"); return 0; }
        if (pre(a, "prev")) { print_string(wf_tabs_next(&g_tabs, -1) >= 0 ? "web-tab ok\n" : "web-tab none\n"); return 0; }
        print_string("web-tab error usage: web-tab [list|next|prev]\n");
        return 1;
    }
    if (pre(line, "autofill")) {
        char text[800];
        a = line + 8;
        skip(&a);
        if (!*a || pre(a, "list")) {
            if (wf_autofill_text(&g_fill, text, (int)sizeof(text)) > 0) print_string(text);
            return 0;
        }
        if (pre(a, "forget")) {
            a += 6;
            skip(&a);
            print_string("autofill forgot\n");
            (void)wf_autofill_forget(&g_fill, a);
            return 0;
        }
        print_string("autofill error usage: autofill [list|forget HOST]\n");
        return 1;
    }
    if (pre(line, "web-back")) return go(1);
    if (pre(line, "web-forward")) return go(0);
    if (pre(line, "web-history")) { print_nav_list(0); return 0; }
    if (pre(line, "bookmark")) {
        a = line + 8;
        skip(&a);
        if (!*a || pre(a, "list")) { print_nav_list(1); return 0; }
        if (pre(a, "add ")) {
            char url[WF_NAV_URL], title[WF_NAV_TITLE];
            int i = 0;
            a += 4;
            skip(&a);
            while (*a && *a != ' ' && i < WF_NAV_URL - 1) url[i++] = *a++;
            url[i] = 0;
            skip(&a);
            i = 0;
            while (*a && i < WF_NAV_TITLE - 1) title[i++] = *a++;
            title[i] = 0;
            if (wf_nav_mark_add(&g_nav, url, title) != 0) { print_string("bookmark error\n"); return 1; }
            (void)save_nav();
            print_string("bookmark ok "); print_string(url); print_string("\n");
            return 0;
        }
        if (pre(a, "del ")) {
            a += 4;
            skip(&a);
            if (wf_nav_mark_del(&g_nav, a) != 0) { print_string("bookmark error missing\n"); return 1; }
            (void)save_nav();
            print_string("bookmark deleted\n");
            return 0;
        }
        print_string("bookmark error usage: bookmark [list|add URL TITLE|del URL]\n");
        return 1;
    }
    if (pre(line, "settings")) {
        a = line + 8;
        skip(&a);
        if (!*a) {
            if (desk_cfg_report(&g_cfg, rep, (int)sizeof(rep)) > 0) print_string(rep);
            return 0;
        }
        if (pre(a, "save") && !a[4]) {
            print_string(save_cfg() == 0 ? "settings saved\n" : "settings save error\n");
            return 0;
        }
        if (pre(a, "load") && !a[4]) {
            char buf[512];
            int n = osui_gui_persist_read("/persist/settings.cfg", buf, (int)sizeof(buf) - 1);
            if (n > 0 && desk_cfg_load(&g_cfg, buf, n) == 0) {
                (void)shell_desk_apply();
                print_string("settings loaded\n");
                return 0;
            }
            print_string("settings load error\n");
            return 1;
        }
        if (pre(a, "scale ")) {
            if (desk_cfg_set_scale_word(&g_cfg, a + 6) != 0) { print_string("settings scale error\n"); return 1; }
        } else if (pre(a, "net on") && !a[6]) {
            (void)desk_cfg_set_net(&g_cfg, 1);
        } else if (pre(a, "net off") && !a[7]) {
            (void)desk_cfg_set_net(&g_cfg, 0);
        } else if (pre(a, "engine ")) {
            if (desk_cfg_set_engine(&g_cfg, a + 7) != 0) { print_string("settings engine error\n"); return 1; }
        } else if (pre(a, "button ")) {
            int b = a[7] ? a[7] - '0' : -1;
            if (!desk_cfg_button(&g_cfg, b)) { print_string("settings button error\n"); return 1; }
        } else {
            print_string("settings error usage: settings [scale N|net on|off|engine NAME|save|load|button N]\n");
            return 1;
        }
        (void)shell_desk_apply();
        (void)save_cfg();
        if (desk_cfg_report(&g_cfg, rep, (int)sizeof(rep)) > 0) print_string(rep);
        return 0;
    }
    (void)slen;
    (void)seq;
    return 1;
}
