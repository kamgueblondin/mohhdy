/* userspace/fleet.c - see fleet.h */
#include "fleet.h"

static int slen(const char* s) { int n = 0; while (s[n]) n++; return n; }
static int seq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void scpy(char* d, const char* s, int cap) { int i = 0; while (s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void scat(char* d, const char* s, int cap) { int n = slen(d); scpy(d + n, s, cap - n); }
static void catu(char* d, uint32_t v, int cap) { char t[11]; int n = 0; char r[12]; int i = 0; do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v); while (n) r[i++] = t[--n]; r[i] = 0; scat(d, r, cap); }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t get32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static const char* nm(const fleet_host_t* h, uint32_t id) { return h->name ? h->name(h->ctx, id) : "?"; }
static void say(const fleet_host_t* h, const char* s) { if (h->out) h->out(h->ctx, s); }

uint32_t fleet_sum(const char* d, int len) {
    uint32_t x = 2166136261U; int i;
    for (i = 0; i < len; i++) { x ^= (uint8_t)d[i]; x *= 16777619U; }
    return x;
}
static uint32_t tnow(const fleet_host_t* h) { return h->now ? h->now(h->ctx) : 0; }
static void send_chunk(const fleet_host_t* h, fl_out_t* o, int i) {
    static uint8_t b[8 + FL_CHUNK];
    int off = i * FL_CHUNK, n = o->len - off, k;
    if (n > FL_CHUNK) n = FL_CHUNK;
    b[0] = FL_FRAG; put32(b + 1, o->mid); b[5] = (uint8_t)i; b[6] = (uint8_t)o->cnt;
    for (k = 0; k < n; k++) b[7 + k] = o->d[off + k];
    (void)h->send(h->ctx, o->to, b, 7 + n);
}
static int queue_one(fleet_t* f, const fleet_host_t* h, uint32_t to, const uint8_t* d, int len) {
    int i, k;
    for (i = 0; i < FL_OUTBOX && f->out[i].used; i++) {}
    if (i == FL_OUTBOX) return -1;
    {
        fl_out_t* o = &f->out[i];
        o->used = 1; o->to = to; o->mid = ++f->next_mid; o->len = len;
        o->cnt = (len + FL_CHUNK - 1) / FL_CHUNK; if (o->cnt < 1) o->cnt = 1;
        o->acked = 0; o->tries = 0; o->last = tnow(h);
        for (k = 0; k < len; k++) o->d[k] = d[k];
        for (k = 0; k < o->cnt; k++) send_chunk(h, o, k);
    }
    return 0;
}
int fleet_xmit(fleet_t* f, const fleet_host_t* h, uint32_t to, const uint8_t* d, int len) {
    uint32_t ids[FL_NODES]; int n, i, q = 0;
    if (len < 1 || len > FL_PAYLOAD + 64) return -1;
    if (to) return queue_one(f, h, to, d, len) == 0 ? 1 : -1;
    n = h->peers ? h->peers(h->ctx, ids, FL_NODES) : 0;
    for (i = 0; i < n; i++) if (ids[i] != h->self && queue_one(f, h, ids[i], d, len) == 0) q++;
    return q > 0 ? q : -1;
}
int fleet_pending(const fleet_t* f) { int i, n = 0; for (i = 0; i < FL_OUTBOX; i++) if (f->out[i].used) n++; return n; }
static int xmit_tick(fleet_t* f, const fleet_host_t* h, uint32_t now) {
    int i, k, sent = 0;
    for (i = 0; i < FL_OUTBOX; i++) {
        fl_out_t* o = &f->out[i];
        if (!o->used || (uint32_t)(now - o->last) < FL_XRESEND_TICKS) continue;
        if (o->tries >= FL_XRESENDS) {
            char line[96]; line[0] = 0;
            scat(line, "fleet send to ", 96); scat(line, h->name ? h->name(h->ctx, o->to) : "?", 96);
            scat(line, " failed after ", 96); catu(line, (uint32_t)o->tries, 96); scat(line, " re-sends", 96);
            if (h->out) h->out(h->ctx, line);
            o->used = 0; f->failed++; continue;
        }
        o->tries++; o->last = now;
        for (k = 0; k < o->cnt; k++) if (!(o->acked & (1U << k))) { send_chunk(h, o, k); f->resent++; sent++; }
    }
    return sent;
}
void fleet_init(fleet_t* f) { int i; char* z = (char*)f; for (i = 0; i < (int)sizeof(*f); i++) z[i] = 0; }

int fleet_session_send(fleet_t* f, const fleet_host_t* h, uint32_t to, const char* text, int len) {
    static uint8_t b[1 + FL_PAYLOAD]; int i;
    if (len < 0 || len > FL_PAYLOAD || !to) return -1;
    b[0] = FL_SESSION;
    for (i = 0; i < len; i++) b[1 + i] = (uint8_t)text[i];
    return fleet_xmit(f, h, to, b, 1 + len) > 0 ? 0 : -1;
}

static fl_file_t* file_slot(fleet_t* f, const char* path, int make) {
    int i, free_i = -1;
    for (i = 0; i < FL_FILES; i++) {
        if (f->files[i].used && seq(f->files[i].path, path)) return &f->files[i];
        if (!f->files[i].used && free_i < 0) free_i = i;
    }
    if (!make || free_i < 0) return 0;
    f->files[free_i].used = 1; scpy(f->files[free_i].path, path, 40);
    f->files[free_i].ver = 0; f->files[free_i].origin = 0;
    return &f->files[free_i];
}

/* [tag][ver 4][origin 4][sum 4] path \0 data */
int fleet_sync_push(fleet_t* f, const fleet_host_t* h, const char* path, const char* d, int len) {
    static uint8_t b[FL_PAYLOAD + 64]; int pl = slen(path), i; fl_file_t* s;
    if (pl < 1 || pl > 39 || len < 0 || len > FL_DATA_MAX) return -1;
    s = file_slot(f, path, 1);
    if (!s) return -1;
    s->ver++; s->origin = h->self; s->sum = fleet_sum(d, len);
    b[0] = FL_SYNC; put32(b + 1, s->ver); put32(b + 5, s->origin); put32(b + 9, s->sum);
    for (i = 0; i <= pl; i++) b[13 + i] = (uint8_t)path[i];
    for (i = 0; i < len; i++) b[14 + pl + i] = (uint8_t)d[i];
    if (fleet_xmit(f, h, 0, b, 14 + pl + len) < 0) return -1;
    return (int)s->ver;
}

int fleet_report(fleet_t* f, const fleet_host_t* h, uint32_t to, const char* metric, const char* log) {
    uint8_t b[200]; int n, i, sent = 0;
    n = slen(metric); if (n > 95) n = 95;
    b[0] = FL_METRIC; for (i = 0; i < n; i++) b[1 + i] = (uint8_t)metric[i];
    if (fleet_xmit(f, h, to, b, 1 + n) > 0) sent++;
    if (log && log[0]) {
        n = slen(log); if (n > 79) n = 79;
        b[0] = FL_LOG; for (i = 0; i < n; i++) b[1 + i] = (uint8_t)log[i];
        (void)fleet_xmit(f, h, to, b, 1 + n);
    }
    return sent ? 0 : -1;
}

fl_deploy_t* fleet_deploy_find(fleet_t* f, const char* name) {
    int i;
    for (i = 0; i < FL_APPS; i++) if (f->dep[i].used && seq(f->dep[i].name, name)) return &f->dep[i];
    return 0;
}
/* [tag][stage][target 4][ver 4][sum 4] name \0 content */
static int deploy_send(fleet_t* f, const fleet_host_t* h, const fl_deploy_t* dp, int stage, uint32_t to, const char* d, int len, uint32_t ver, uint32_t sum) {
    static uint8_t b[FL_PAYLOAD + 64]; int nl = slen(dp->name), i;
    b[0] = FL_DEPLOY; b[1] = (uint8_t)stage; put32(b + 2, to); put32(b + 6, ver); put32(b + 10, sum);
    for (i = 0; i <= nl; i++) b[14 + i] = (uint8_t)dp->name[i];
    for (i = 0; i < len; i++) b[15 + nl + i] = (uint8_t)d[i];
    return fleet_xmit(f, h, stage == FL_STAGE_CANARY ? to : 0, b, 15 + nl + len);
}
static fl_ack_t* ack_of(fl_deploy_t* dp, uint32_t id, int make);
static void ack_reset(fl_deploy_t* dp) { int i; for (i = 0; i < FL_NODES; i++) dp->ack[i].used = 0; dp->resends = 0; dp->last_send = 0; }

int fleet_deploy_stage(fleet_t* f, const fleet_host_t* h, const char* name, const char* d, int len, uint32_t canary) {
    fl_deploy_t* dp = fleet_deploy_find(f, name);
    int i;
    if (slen(name) < 1 || slen(name) > 23 || len < 0 || len > FL_DATA_MAX || !canary) return -1;
    if (!dp) {
        for (i = 0; i < FL_APPS && f->dep[i].used; i++) {}
        if (i == FL_APPS) return -1;
        dp = &f->dep[i];
        dp->used = 1; scpy(dp->name, name, 24); dp->ver = 0; dp->clen = 0; dp->plen = 0; dp->sum = 0;
    }
    /* the live version becomes the rollback target */
    for (i = 0; i < dp->clen; i++) dp->prev[i] = dp->content[i];
    dp->plen = dp->clen; dp->prev_ver = dp->ver; dp->prev_sum = dp->sum;
    for (i = 0; i < len; i++) dp->content[i] = d[i];
    dp->clen = len; dp->ver++; dp->sum = fleet_sum(d, len);
    dp->stage = FL_STAGE_CANARY; dp->canary = canary;
    ack_reset(dp);
    return deploy_send(f, h, dp, FL_STAGE_CANARY, canary, d, len, dp->ver, dp->sum) > 0 ? (int)dp->ver : -1;
}
static fl_ack_t* ack_of(fl_deploy_t* dp, uint32_t id, int make) {
    int i, fr = -1;
    for (i = 0; i < FL_NODES; i++) { if (dp->ack[i].used && dp->ack[i].id == id) return &dp->ack[i]; if (!dp->ack[i].used && fr < 0) fr = i; }
    if (!make || fr < 0) return 0;
    dp->ack[fr].used = 1; dp->ack[fr].id = id; return &dp->ack[fr];
}
int fleet_tick(fleet_t* f, const fleet_host_t* h, uint32_t now) {
    int i, j, sent = xmit_tick(f, h, now) > 0 ? 0 : 0;
    for (i = 0; i < FL_APPS; i++) {
        fl_deploy_t* dp = &f->dep[i];
        int good = 0, need;
        if (!dp->used || !dp->stage || dp->resends >= FL_RESENDS) continue;
        if (!dp->last_send) { dp->last_send = now ? now : 1U; continue; }
        if ((uint32_t)(now - dp->last_send) < FL_RESEND_TICKS) continue;
        for (j = 0; j < FL_NODES; j++) if (dp->ack[j].used && dp->ack[j].ver == dp->ver) good++;
        if (dp->stage == FL_STAGE_CANARY) { fl_ack_t* a = ack_of(dp, dp->canary, 0); if (a && a->ver == dp->ver) continue; }
        else { need = h->members ? (int)h->members(h->ctx) - 1 : 0; if (good >= need) continue; }
        dp->resends++; dp->last_send = now;
        if (deploy_send(f, h, dp, dp->stage, dp->stage == FL_STAGE_CANARY ? dp->canary : 0, dp->content, dp->clen, dp->ver, dp->sum) > 0) sent++;
    }
    return sent;
}
int fleet_deploy_promote(fleet_t* f, const fleet_host_t* h, const char* name) {
    fl_deploy_t* dp = fleet_deploy_find(f, name);
    fl_ack_t* a;
    if (!dp) return -1;
    a = ack_of(dp, dp->canary, 0);
    if (dp->stage != FL_STAGE_CANARY || !a || !a->ok || a->ver != dp->ver || a->sum != dp->sum) return -2;
    dp->stage = FL_STAGE_ALL; dp->resends = 0; dp->last_send = 0;
    return deploy_send(f, h, dp, FL_STAGE_ALL, 0, dp->content, dp->clen, dp->ver, dp->sum) > 0 ? 0 : -3;
}
int fleet_deploy_rollback(fleet_t* f, const fleet_host_t* h, const char* name) {
    fl_deploy_t* dp = fleet_deploy_find(f, name);
    int i;
    if (!dp || dp->prev_ver == 0) return -1;
    /* the previous content is re-published under a new version number */
    for (i = 0; i < dp->plen; i++) dp->content[i] = dp->prev[i];
    dp->clen = dp->plen; dp->ver++; dp->sum = dp->prev_sum; dp->stage = FL_STAGE_ROLLBACK;
    ack_reset(dp);
    return deploy_send(f, h, dp, FL_STAGE_ROLLBACK, 0, dp->content, dp->clen, dp->ver, dp->sum) > 0 ? (int)dp->ver : -3;
}

static fl_node_t* node_of(fleet_t* f, uint32_t id) {
    int i, fr = -1;
    for (i = 0; i < FL_NODES; i++) { if (f->nodes[i].used && f->nodes[i].id == id) return &f->nodes[i]; if (!f->nodes[i].used && fr < 0) fr = i; }
    if (fr < 0) return 0;
    f->nodes[fr].used = 1; f->nodes[fr].id = id; f->nodes[fr].nlog = 0; f->nodes[fr].reports = 0; f->nodes[fr].metric[0] = 0;
    return &f->nodes[fr];
}

static void handle_msg(fleet_t* f, const fleet_host_t* h, uint32_t from, const uint8_t* d, int len) {
    char line[160];
    int i;
    if (len < 1) return;
    line[0] = 0;
    if (d[0] == FL_SESSION) {
        if (len - 1 > FL_PAYLOAD) { f->rejected++; return; }
        f->sess_from = from; f->sess_len = len - 1;
        for (i = 0; i < len - 1; i++) f->sess[i] = (char)d[1 + i];
        scpy(line, "fleet session offered by ", 160); scat(line, nm(h, from), 160);
        scat(line, " bytes ", 160); catu(line, (uint32_t)(len - 1), 160); scat(line, " (session-resume)", 160);
        say(h, line);
    } else if (d[0] == FL_SYNC && len >= 15) {
        uint32_t ver = get32(d + 1), origin = get32(d + 5), sum = get32(d + 9);
        const char* path = (const char*)d + 13; int pl = 0; fl_file_t* s;
        while (13 + pl < len && d[13 + pl]) pl++;
        if (13 + pl >= len || pl < 1 || pl > 39) { f->rejected++; return; }
        {
            const char* data = (const char*)d + 14 + pl; int dl = len - 14 - pl;
            char p[40]; scpy(p, path, 40);
            if (fleet_sum(data, dl) != sum) { f->rejected++; say(h, "fleet sync rejected (checksum)"); return; }
            s = file_slot(f, p, 1);
            if (!s) { f->rejected++; return; }
            /* last writer wins: higher version, then higher origin id */
            if (ver < s->ver || (ver == s->ver && origin <= s->origin)) {
                scpy(line, "fleet sync kept ", 160); scat(line, p, 160); scat(line, " v", 160); catu(line, s->ver, 160);
                scat(line, " (older v", 160); catu(line, ver, 160); scat(line, " from ", 160); scat(line, nm(h, from), 160); scat(line, ")", 160);
                say(h, line); return;
            }
            if (!h->write_file || h->write_file(h->ctx, p, data, dl) < 0) { say(h, "fleet sync write failed"); return; }
            s->ver = ver; s->origin = origin; s->sum = sum;
            scpy(line, "fleet sync applied ", 160); scat(line, p, 160); scat(line, " v", 160); catu(line, ver, 160);
            scat(line, " bytes ", 160); catu(line, (uint32_t)dl, 160); scat(line, " from ", 160); scat(line, nm(h, origin), 160);
            say(h, line);
        }
    } else if ((d[0] == FL_METRIC || d[0] == FL_LOG) && len >= 2) {
        fl_node_t* n = node_of(f, from);
        char t[96]; int k = len - 1 > 95 ? 95 : len - 1;
        if (!n) { f->rejected++; return; }
        for (i = 0; i < k; i++) { char c = (char)d[1 + i]; t[i] = (c < 32 || c > 126) ? '?' : c; }
        t[k] = 0;
        if (d[0] == FL_METRIC) { scpy(n->metric, t, 96); n->reports++; }
        else {
            if (n->nlog == FL_LOGS) { for (i = 1; i < FL_LOGS; i++) scpy(n->log[i - 1], n->log[i], 80); n->nlog--; }
            scpy(n->log[n->nlog++], t, 80);
        }
        scpy(line, d[0] == FL_METRIC ? "fleet metric from " : "fleet log from ", 160); scat(line, nm(h, from), 160);
        scat(line, ": ", 160); scat(line, t, 160);
        say(h, line);
    } else if (d[0] == FL_DEPLOY && len >= 16) {
        int stage = d[1]; uint32_t target = get32(d + 2), ver = get32(d + 6), sum = get32(d + 10);
        int nl = 0; char name[24], path[40]; fl_applied_t* ap = 0;
        uint8_t ack[64]; int ok;
        while (14 + nl < len && d[14 + nl]) nl++;
        if (14 + nl >= len || nl < 1 || nl > 23) { f->rejected++; return; }
        scpy(name, (const char*)d + 14, 24);
        if (stage == FL_STAGE_CANARY && target != h->self) return;
        for (i = 0; i < FL_APPS; i++) if (f->app[i].used && seq(f->app[i].name, name)) ap = &f->app[i];
        for (i = 0; !ap && i < FL_APPS; i++) if (!f->app[i].used) { ap = &f->app[i]; ap->used = 1; scpy(ap->name, name, 24); ap->ver = 0; }
        if (!ap) { f->rejected++; return; }
        {
            const char* data = (const char*)d + 15 + nl; int dl = len - 15 - nl;
            ok = fleet_sum(data, dl) == sum;
            if (ok && ver > ap->ver) {
                scpy(path, "/app/", 40); scat(path, name, 40);
                ok = h->write_file && h->write_file(h->ctx, path, data, dl) >= 0;
                if (ok) { ap->ver = ver; ap->sum = sum; }
            } else if (ok && ver == ap->ver) ok = ap->sum == sum;   /* repeat */
            else if (ok) ok = 0;                                   /* older version */
        }
        scpy(line, "fleet deploy ", 160); scat(line, name, 160); scat(line, " v", 160); catu(line, ver, 160);
        scat(line, stage == FL_STAGE_CANARY ? " canary" : stage == FL_STAGE_ALL ? " all" : " rollback", 160);
        scat(line, ok ? " applied" : " refused", 160); scat(line, " from ", 160); scat(line, nm(h, from), 160);
        say(h, line);
        ack[0] = FL_ACK; ack[1] = (uint8_t)ok; put32(ack + 2, ver); put32(ack + 6, ok ? sum : 0);
        for (i = 0; i <= nl; i++) ack[10 + i] = (uint8_t)name[i];
        (void)fleet_xmit(f, h, from, ack, 11 + nl);
    } else if (d[0] == FL_ACK && len >= 12) {
        char name[24]; fl_deploy_t* dp; fl_ack_t* a;
        int nl = 0;
        while (10 + nl < len && d[10 + nl]) nl++;
        if (10 + nl >= len || nl < 1 || nl > 23) { f->rejected++; return; }
        scpy(name, (const char*)d + 10, 24);
        dp = fleet_deploy_find(f, name);
        if (!dp || !(a = ack_of(dp, from, 1))) { f->rejected++; return; }
        a->ok = d[1]; a->ver = get32(d + 2); a->sum = get32(d + 6);
        scpy(line, "fleet deploy ack ", 160); scat(line, name, 160); scat(line, " v", 160); catu(line, a->ver, 160);
        scat(line, a->ok ? " ok from " : " failed from ", 160); scat(line, nm(h, from), 160);
        say(h, line);
    } else f->rejected++;
}

void fleet_receive(fleet_t* f, const fleet_host_t* h, uint32_t from, const uint8_t* d, int len) {
    int i;
    if (len < 1) return;
    if (d[0] == FL_FACK && len >= 6) {
        uint32_t mid = get32(d + 1); int idx = d[5];
        for (i = 0; i < FL_OUTBOX; i++) {
            fl_out_t* o = &f->out[i];
            if (!o->used || o->to != from || o->mid != mid || idx >= o->cnt) continue;
            o->acked |= 1U << idx;
            if (o->acked == ((o->cnt >= 32) ? 0xffffffffU : ((1U << o->cnt) - 1U))) { o->used = 0; f->delivered++; }
        }
        return;
    }
    if (d[0] == FL_FRAG && len >= 8) {
        uint32_t mid = get32(d + 1); int idx = d[5], cnt = d[6], n = len - 7, k;
        uint8_t ack[6]; fl_in_t* in = 0;
        if (cnt < 1 || cnt > FL_MAXCHUNKS || idx >= cnt || n > FL_CHUNK || (idx < cnt - 1 && n != FL_CHUNK)) { f->rejected++; return; }
        ack[0] = FL_FACK; put32(ack + 1, mid); ack[5] = (uint8_t)idx;
        (void)h->send(h->ctx, from, ack, 6);
        for (k = 0; k < 16; k++) if (f->done_from[k] == from && f->done_mid[k] == mid) { f->dup++; return; }
        /* one reassembly slot per (sender, message); evict the sender's
         * oldest partial message, else any partial message */
        for (k = 0; k < 2 * FL_NODES; k++) if (f->in[k].used && f->in[k].from == from && f->in[k].mid == mid) in = &f->in[k];
        for (k = 0; !in && k < 2 * FL_NODES; k++) if (!f->in[k].used) in = &f->in[k];
        for (k = 0; !in && k < 2 * FL_NODES; k++) if (f->in[k].from == from) in = &f->in[k];
        if (!in) in = &f->in[mid % (2 * FL_NODES)];
        if (!in->used || in->from != from || in->mid != mid) { in->used = 1; in->from = from; in->mid = mid; in->cnt = cnt; in->have = 0; in->len = 0; }
        if (in->have & (1U << idx)) { f->dup++; return; }
        for (k = 0; k < n; k++) in->d[idx * FL_CHUNK + k] = d[7 + k];
        in->have |= 1U << idx;
        if (idx == cnt - 1) in->len = idx * FL_CHUNK + n;
        if (in->have == ((1U << cnt) - 1U)) {
            f->done_from[f->done_next % 16] = from; f->done_mid[f->done_next % 16] = mid; f->done_next++;
            in->used = 0;
            handle_msg(f, h, from, in->d, in->len);
        }
        return;
    }
    handle_msg(f, h, from, d, len);
}
