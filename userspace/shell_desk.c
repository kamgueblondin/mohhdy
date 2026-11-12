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
static void print_int(int v) {
    char b[12];
    int k = 11;
    unsigned u = v < 0 ? 0u : (unsigned)v;
    b[k] = 0;
    do { b[--k] = (char)('0' + u % 10u); u /= 10u; } while (u && k > 0);
    print_string(b + k);
}

/* Lot 89: autofill dates follow the CMOS clock; read again
 * before a store or an expiry, so a session across midnight dates right */
extern int shell_rtc_utc(char out[16]);
static int save_fill(void);
/* Lot 90: the local date (the offset the operator set, saved in the
 * session file), and the automatic expiry once a day when it is on */
static void fill_clock(void) {
    char utc[16];
    int gone;
    utc[0] = 0;
    gone = wf_autofill_tick(&g_fill, shell_rtc_utc(utc) == 0 ? wf_autofill_day_tz(utc, g_fill.tz_min) : 0U);
    if (gone > 0) (void)save_fill();
}

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
    /* Lot 90: after the load (time zone and expiry come from the file) */
    fill_clock();
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
    fill_clock();
    n = wf_formfill_remember(&g_fill, p, form, url);
    if (n > 0) (void)save_fill();
    return n;
}

int shell_desk_offers(const wf_form_page_t *p, int field, const char *url, wf_fill_offer_t *o, int cap) {
    ready();
    return wf_formfill_offers(&g_fill, p, field, url, o, cap);
}

/* Lot 90: an offer accepted: the value becomes the newest of its key
 * (on the host that holds it) and is dated today */
static void accepted(const wf_fill_offer_t *o) {
    fill_clock();
    if (wf_autofill_touch(&g_fill, o->host, o->name, o->value) == 0) (void)save_fill();
}

int shell_desk_apply_offer(wf_form_page_t *p, int field, int offer, const char *url) {
    wf_fill_offer_t o[WF_FILL_OFFERS];
    int n;
    ready();
    n = wf_formfill_offers(&g_fill, p, field, url, o, WF_FILL_OFFERS);
    if (offer < 0 || offer >= n) return -1;
    if (wf_formfill_apply(p, field, &o[offer]) != 0) return -1;
    accepted(&o[offer]);
    return 0;
}

int shell_desk_fill_key(wf_form_page_t *p, int field, const char *url) {
    wf_fill_offer_t o[1];
    int n;
    ready();
    n = wf_formfill_offers(&g_fill, p, field, url, o, 1);
    if (wf_formfill_key(&g_fill, p, field, url) != 0) return -1;
    if (n == 1) accepted(&o[0]);
    return 0;
}

int shell_desk_offer_touch(const char *url, const char *name, const char *value) {
    char host[WF_AF_HOST];
    int r;
    if (!url || !name || !name[0] || !value) return -1;
    ready();
    if (wf_formfill_host(url, host, (int)sizeof(host)) != 0) return -1;
    fill_clock();
    r = wf_autofill_touch_site(&g_fill, host, name, value);
    if (r >= 0) (void)save_fill();
    return r;
}

int shell_desk_offer_at(const char *url, const char *name, int idx, char *out, int cap) {
    char host[64];
    int n;
    if (!out || cap <= 0) return -1;
    out[0] = 0;
    if (!url || !name || !name[0] || idx < 0) return -1;
    ready();
    if (wf_formfill_host(url, host, (int)sizeof(host)) != 0) return -1;
    /* Lot 88: the host's values, then the other hosts of its site */
    if (wf_autofill_get_site(&g_fill, host, name, idx, out, cap, 0) != 0) { out[0] = 0; return -1; }
    for (n = 0; out[n]; n++) {}
    return n > 0 ? n : -1;
}

int shell_desk_offer_drop(const char *url, const char *name, const char *value) {
    char host[64];
    int left;
    if (!url || !name || !name[0] || !value) return -1;
    ready();
    if (wf_formfill_host(url, host, (int)sizeof(host)) != 0) return -1;
    left = wf_autofill_drop_site(&g_fill, host, name, value);
    if (left >= 0) (void)save_fill();
    return left;
}

int shell_desk_offer(const char *url, const char *name, char *out, int cap) {
    return shell_desk_offer_at(url, name, 0, out, cap);
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
        if (pre(a, "add ") || pre(a, "offers ")) {
            /* Lot 88: "autofill add HOST NAME VALUE" stores a value by
             * hand (passwords cannot be named: the type is text, and the
             * store refuses control characters); "autofill offers URL
             * NAME" lists what a field of that page would be offered, the
             * same list as the desktop (the host, then its site) */
            char w1[WF_AF_HOST + 32], w2[WF_AF_NAME];
            int add = pre(a, "add "), k, r;
            a += add ? 4 : 7;
            skip(&a);
            for (k = 0; *a && *a != ' ' && k < (int)sizeof(w1) - 1; k++) w1[k] = *a++;
            w1[k] = 0;
            skip(&a);
            for (k = 0; *a && *a != ' ' && k < (int)sizeof(w2) - 1; k++) w2[k] = *a++;
            w2[k] = 0;
            skip(&a);
            if (!w1[0] || !w2[0] || (add && !*a)) {
                print_string("autofill error usage: autofill add HOST NAME VALUE | autofill offers URL NAME\n");
                return 1;
            }
            if (add) {
                fill_clock();
                r = wf_autofill_put(&g_fill, w1, w2, a, "text");
                print_string(r == 0 ? "autofill added\n" : "autofill refused\n");
                if (r == 0) (void)save_fill();
                return r == 0 ? 0 : 1;
            }
            {
                char host[WF_AF_HOST], v[WF_AF_VALUE];
                int i, other, n = 0;
                if (wf_formfill_host(w1, host, (int)sizeof(host)) != 0) {
                    print_string("autofill error not an absolute URL\n");
                    return 1;
                }
                for (i = 0; i < WF_AF_SITE_MAX; i++) {
                    if (wf_autofill_get_site(&g_fill, host, w2, i, v, (int)sizeof(v), &other) != 0) break;
                    print_string("autofill offer ");
                    print_int(i);
                    print_string(" ");
                    print_string(v);
                    print_string(other ? " site\n" : " host\n");
                    n++;
                }
                print_string("autofill offers ");
                print_int(n);
                print_string("\n");
            }
            return 0;
        }
        if (pre(a, "status")) {
            fill_clock();
            if (wf_autofill_status(&g_fill, text, (int)sizeof(text)) < 0) { print_string("autofill error\n"); return 1; }
            print_string(text);
            return 0;
        }
        if (pre(a, "expire")) {
            /* Lot 90: autofill expire DAYS (1..36500) | off: values older
             * than that go once a day, on their own */
            uint32_t days = 0;
            int k = 0, gone;
            a += 6;
            skip(&a);
            if (pre(a, "off") && !a[3]) days = 0;
            else {
                while (a[k] >= '0' && a[k] <= '9' && k < 6) { days = days * 10U + (uint32_t)(a[k] - '0'); k++; }
                if (k == 0 || a[k] || days == 0 || wf_autofill_set_expire(&g_fill, days) != 0) {
                    print_string("autofill error usage: autofill expire DAYS (1..36500) | off\n");
                    return 1;
                }
            }
            if (!days) (void)wf_autofill_set_expire(&g_fill, 0U);
            gone = g_fill.expired;
            fill_clock();
            print_string("autofill expire ");
            if (days) print_int((int)days); else print_string("off");
            print_string(" forgot ");
            print_int((int)g_fill.expired - gone);
            print_string(g_fill.today ? "\n" : " no clock\n");
            (void)save_fill();
            return 0;
        }
        if (pre(a, "tz")) {
            /* Lot 90: autofill tz MINUTES (-720..840) | utc: the local date
             * the values are dated with (the CMOS clock is UTC) */
            int k = 0, neg = 0, m = 0;
            a += 2;
            skip(&a);
            if (pre(a, "utc") && !a[3]) m = 0;
            else {
                if (*a == '-' || *a == '+') { neg = *a == '-'; a++; }
                while (a[k] >= '0' && a[k] <= '9' && k < 4) { m = m * 10 + (a[k] - '0'); k++; }
                if (k == 0 || a[k] || wf_autofill_set_tz(&g_fill, neg ? -m : m) != 0) {
                    print_string("autofill error usage: autofill tz MINUTES (-720..840) | utc\n");
                    return 1;
                }
                if (neg) m = -m;
            }
            (void)wf_autofill_set_tz(&g_fill, m);
            fill_clock();
            print_string("autofill tz ");
            print_string(m < 0 ? "-" : "+");
            print_int(m < 0 ? -m : m);
            print_string(" today ");
            print_int((int)g_fill.today);
            print_string("\n");
            (void)save_fill();
            return 0;
        }
        if (pre(a, "ages values")) {
            /* Lot 90: the age of every value */
            static char vages[32 * 3 * 180 + 16];
            fill_clock();
            if (wf_autofill_value_ages(&g_fill, vages, (int)sizeof(vages)) < 0) { print_string("autofill error\n"); return 1; }
            print_string(vages);
            return 0;
        }
        if (pre(a, "ages")) {
            /* Lot 89: days since each key was last stored */
            /* 32 keys of up to 96 + 48 bytes: more than text holds */
            static char ages[32 * 176 + 48];
            fill_clock();
            if (wf_autofill_ages(&g_fill, ages, (int)sizeof(ages)) < 0) { print_string("autofill error\n"); return 1; }
            print_string(ages);
            return 0;
        }
        if (pre(a, "forget-older")) {
            /* Lot 89: autofill forget-older DAYS (0..36500) */
            uint32_t days = 0;
            int k = 0, r, und = 0;
            a += 12;
            skip(&a);
            while (a[k] >= '0' && a[k] <= '9' && k < 6) { days = days * 10U + (uint32_t)(a[k] - '0'); k++; }
            fill_clock();
            if (k == 0 || a[k] || (r = wf_autofill_forget_older(&g_fill, days, &und)) < 0) {
                print_string(g_fill.today ? "autofill error usage: autofill forget-older DAYS (0..36500)\n"
                                          : "autofill error no clock\n");
                return 1;
            }
            print_string("autofill forgot older than ");
            print_int((int)days);
            print_string(" days keys ");
            print_int(r);
            print_string(" undated ");
            print_int(und);
            print_string(" left ");
            print_int(g_fill.n);
            print_string("\n");
            (void)save_fill();
            return 0;
        }
        if (pre(a, "forget")) {
            /* Lot 87: forget HOST, forget HOST NAME (the key), or forget
             * HOST NAME VALUE (one value, the rest of the line); saved to
             * the session file like the desktop's Delete */
            char host[WF_AF_HOST], name[WF_AF_NAME];
            int k, r;
            a += 6;
            skip(&a);
            for (k = 0; *a && *a != ' ' && k < (int)sizeof(host) - 1; k++) host[k] = *a++;
            host[k] = 0;
            skip(&a);
            for (k = 0; *a && *a != ' ' && k < (int)sizeof(name) - 1; k++) name[k] = *a++;
            name[k] = 0;
            skip(&a);
            if (!name[0]) {
                r = wf_autofill_forget(&g_fill, host);
                print_string("autofill forgot keys ");
                print_int(r);
                print_string("\n");
            } else if (!*a) {
                r = wf_autofill_forget_key(&g_fill, host, name);
                print_string(r ? "autofill forgot key\n" : "autofill none\n");
            } else {
                r = wf_autofill_drop(&g_fill, host, name, a);
                if (r < 0) { print_string("autofill none\n"); return 1; }
                print_string("autofill forgot value left ");
                print_int(r);
                print_string("\n");
            }
            (void)save_fill();
            return 0;
        }
        print_string("autofill error usage: autofill [list|ages [values]|status|expire DAYS|off|tz MINUTES|utc|forget HOST [NAME [VALUE]]|forget-older DAYS|add HOST NAME VALUE|offers URL NAME]\n");
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
