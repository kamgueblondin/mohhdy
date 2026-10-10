/* userspace/shell_persist.c - persist-* commands (consolidation lot).
 * Saves the phase 8 production state, the collab ledger (signing key
 * included) and the P2P node seed as checksummed blobs on the overlay. With
 * an IDE disk attached, every overlay write is flushed to LBA 0-63 by the
 * Ring 3 atadriver (tranche 4) and reloaded at the next boot, so the state
 * survives a cold reboot. Overlay files hold at most 384 bytes, so a blob is
 * split into /persist/<name>.<k> chunks; chunk 0 starts with a header
 * (magic, length, FNV-1a) checked on load. */
#include <stdint.h>
#include "../include/os_syscalls.h"

void print_string(const char* s);
int sys_readfile(const char* path, char* buf, int max);
int sys_writefile(const char* path, const char* buf, int n);
int sys_mkdir(const char* path);
int sys_unlink(const char* path);
int shell_prod_save(uint8_t* o, int cap);
int shell_prod_load(const uint8_t* in, int len);
int shell_collab_save(uint8_t* out, int cap);
int shell_collab_load(const uint8_t* in, int len);
int shell_p2p_seed_get(uint8_t out[32]);
void shell_p2p_seed_set(const uint8_t in[32]);

#define CHUNK 384
#define BLOB_MAX 16384
#define MAGIC 0x31535250U /* "PRS1" */
static uint8_t g_blob[BLOB_MAX + 12];
static char g_chunk[CHUNK + 4];

static int s_eq(const char* a, const char* b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static void s_cat(char* d, const char* s, int cap) { int n = 0, i = 0; while (d[n]) n++; while (s[i] && n < cap - 1) d[n++] = s[i++]; d[n] = 0; }
static void cat_u(char* d, uint32_t v, int cap) {
    char t[11], r[12]; int n = 0, i = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v);
    while (n) r[i++] = t[--n];
    r[i] = 0; s_cat(d, r, cap);
}
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t get32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint32_t fnv(const uint8_t* p, uint32_t n) { uint32_t h = 2166136261U, i; for (i = 0; i < n; i++) { h ^= p[i]; h *= 16777619U; } return h; }
static void path_of(char* d, const char* name, int k) { d[0] = 0; s_cat(d, "/persist/", 64); s_cat(d, name, 64); s_cat(d, ".", 64); cat_u(d, (uint32_t)k, 64); }

/* data already at g_blob + 12; returns chunks written or -1 */
static int store(const char* name, int len) {
    char path[64];
    int total = len + 12, k = 0, off;
    put32(g_blob, MAGIC); put32(g_blob + 4, (uint32_t)len); put32(g_blob + 8, fnv(g_blob + 12, (uint32_t)len));
    (void)sys_mkdir("/persist");
    for (off = 0; off < total; off += CHUNK, k++) {
        int n = total - off > CHUNK ? CHUNK : total - off;
        path_of(path, name, k);
        (void)sys_unlink(path);
        if (sys_writefile(path, (const char*)g_blob + off, n) != n) return -1;
    }
    return k;
}
/* returns payload length at g_blob + 12, -1 missing, -2 corrupt */
static int fetch(const char* name) {
    char path[64];
    int total = 0, k, n, want = -1;
    for (k = 0; k < (BLOB_MAX + 12) / CHUNK + 1; k++) {
        path_of(path, name, k);
        n = sys_readfile(path, g_chunk, CHUNK);
        if (n <= 0) break;
        if (total + n > BLOB_MAX + 12) return -2;
        { int i; for (i = 0; i < n; i++) g_blob[total + i] = (uint8_t)g_chunk[i]; }
        total += n;
        if (want < 0 && total >= 12) {
            if (get32(g_blob) != MAGIC) return -2;
            want = (int)get32(g_blob + 4) + 12;
            if (want > BLOB_MAX + 12) return -2;
        }
        if (want >= 0 && total >= want) break;
    }
    if (total == 0) return -1;
    if (want < 0 || total < want) return -2;
    if (fnv(g_blob + 12, (uint32_t)(want - 12)) != get32(g_blob + 8)) return -2;
    return want - 12;
}

static void report(const char* what, const char* name, int a, const char* unit, int b) {
    char t[128]; t[0] = 0;
    s_cat(t, what, 128); s_cat(t, " ", 128); s_cat(t, name, 128); s_cat(t, " ", 128);
    cat_u(t, (uint32_t)a, 128); s_cat(t, unit, 128); cat_u(t, (uint32_t)b, 128); s_cat(t, "\n", 128);
    print_string(t);
}

int shell_persist_is(const char* l) {
    return s_eq(l, "persist-save") || s_eq(l, "persist-load") || s_eq(l, "persist-status");
}
int shell_persist_line(const char* l) {
    static const char* const names[3] = {"node", "prod", "collab"};
    int i, n, bad = 0;
    if (s_eq(l, "persist-save")) {
        for (i = 0; i < 3; i++) {
            if (i == 0) n = shell_p2p_seed_get(g_blob + 12) == 0 ? 32 : 0;
            else if (i == 1) n = shell_prod_save(g_blob + 12, BLOB_MAX);
            else n = shell_collab_save(g_blob + 12, BLOB_MAX);
            if (n < 0) { print_string("persist-save error "); print_string(names[i]); print_string(" too large\n"); bad = 1; continue; }
            if (n == 0) { print_string("persist-save skip "); print_string(names[i]); print_string(" empty\n"); continue; }
            { int k = store(names[i], n);
              if (k < 0) { print_string("persist-save error "); print_string(names[i]); print_string(" write failed\n"); bad = 1; continue; }
              report("persist-save", names[i], n, " bytes chunks ", k); }
        }
        print_string(bad ? "persist-save error\n" : "persist-save ok\n");
        return bad;
    }
    if (s_eq(l, "persist-load") || s_eq(l, "persist-status")) {
        int load = s_eq(l, "persist-load");
        for (i = 0; i < 3; i++) {
            n = fetch(names[i]);
            if (n == -1) { print_string(l); print_string(" "); print_string(names[i]); print_string(" missing\n"); continue; }
            if (n == -2) { print_string(l); print_string(" "); print_string(names[i]); print_string(" corrupt (checksum)\n"); bad = 1; continue; }
            if (!load) { report("persist-status", names[i], n, " bytes fnv ok ", 1); continue; }
            if (i == 0) { if (n == 32) shell_p2p_seed_set(g_blob + 12); else bad = 1; report("persist-load", names[i], n, " bytes seed ", n == 32); }
            else if (i == 1) { int rc = shell_prod_load(g_blob + 12, n); if (rc) bad = 1; report("persist-load", names[i], n, " bytes ok ", rc == 0); }
            else { int rc = shell_collab_load(g_blob + 12, n); if (rc < 0) bad = 1; report("persist-load", names[i], n, " bytes entries ", rc < 0 ? 0 : rc); }
        }
        print_string(bad ? (load ? "persist-load error\n" : "persist-status error\n") : (load ? "persist-load ok\n" : "persist-status ok\n"));
        return bad;
    }
    return 1;
}
