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
#include "../kernel/sha256.h"
#include "../kernel/aes_gcm.h"

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
void shell_collab_key_install(const uint8_t* sk);
int shell_collab_signing(void);
int shell_p2p_kv_save(uint8_t* out, int cap);
int shell_p2p_kv_load(const uint8_t* in, int len);
unsigned int sys_ticks(void);

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

/* ------------------------------------------------------ key at rest */
/* collab blob = [mode 1][salt 16][iv 4][nonce 8][tag 16] + collab_save()
 * output, whose secret key (bytes 32..51) is AES-128-GCM encrypted under a
 * key derived from the passphrase (mode 1) or zeroed (mode 0, no passphrase:
 * the key is not stored and signing stays locked after a reload). */
#define KHDR 45
#define SK_OFF (KHDR + 32)
static char g_pass[64];
static int g_collab_locked; /* saved key not unlocked: never overwrite it */
static int g_pass_set;
static void entropy(uint8_t* out, int n) {
    sha256_ctx_t h; uint8_t d[32]; uint32_t lo, hi, t; int i, k;
    for (k = 0; k < n; k += 32) {
        sha256_init(&h);
        for (i = 0; i < 4; i++) { asm volatile("rdtsc" : "=a"(lo), "=d"(hi)); sha256_update(&h, (uint8_t*)&lo, 4); sha256_update(&h, (uint8_t*)&hi, 4); }
        t = sys_ticks(); sha256_update(&h, (uint8_t*)&t, 4); sha256_update(&h, (uint8_t*)&k, 4);
        sha256_final(&h, d);
        for (i = 0; i < 32 && k + i < n; i++) out[k + i] = d[i];
    }
}
/* PBKDF2-HMAC-SHA256, one block, 2000 iterations; first 16 bytes */
static void kdf(const uint8_t salt[16], uint8_t key[16]) {
    uint8_t u[32], acc[32], in[20];
    int i, k, pl = 0;
    while (g_pass[pl]) pl++;
    for (i = 0; i < 16; i++) in[i] = salt[i];
    in[16] = 0; in[17] = 0; in[18] = 0; in[19] = 1;
    hmac_sha256((const uint8_t*)g_pass, (uint32_t)pl, in, 20, u);
    for (i = 0; i < 32; i++) acc[i] = u[i];
    for (k = 1; k < 2000; k++) { hmac_sha256((const uint8_t*)g_pass, (uint32_t)pl, u, 32, u); for (i = 0; i < 32; i++) acc[i] ^= u[i]; }
    for (i = 0; i < 16; i++) key[i] = acc[i];
}
/* blob payload is at g_blob+12, collab_save() output at +KHDR */
static int seal_key(void) {
    uint8_t* b = g_blob + 12;
    uint8_t key[16], ct[20];
    int i;
    if (!g_pass_set || !shell_collab_signing()) { b[0] = 0; for (i = 1; i < KHDR; i++) b[i] = 0; for (i = 0; i < 20; i++) b[SK_OFF + i] = 0; return 0; }
    b[0] = 1;
    entropy(b + 1, 28);
    kdf(b + 1, key);
    if (aes128_gcm_encrypt(key, b + 17, b + 21, b + KHDR + 4, 4, b + SK_OFF, 20, ct, b + 29) != 0) return -1;
    for (i = 0; i < 20; i++) b[SK_OFF + i] = ct[i];
    return 1;
}
/* 1 unlocked, 0 no key stored, -1 wrong passphrase / no passphrase */
static int open_key(const uint8_t* b, uint8_t sk[20]) {
    uint8_t key[16];
    if (b[0] == 0) return 0;
    if (!g_pass_set) return -1;
    kdf(b + 1, key);
    return aes128_gcm_decrypt(key, b + 17, b + 21, b + KHDR + 4, 4, b + SK_OFF, 20, b + 29, sk) == 0 ? 1 : -1;
}

/* ------------------------------------------------------- save engine */
#define NBLOB 4
static const char* const k_names[NBLOB] = {"node", "prod", "collab", "p2pkv"};
static uint32_t g_saved_hash[NBLOB];
static int g_auto_secs = -1;   /* -1: default (60 s when a disk is present) */
static uint32_t g_last_auto;
static int build(int i) {
    int n;
    if (i == 0) return shell_p2p_seed_get(g_blob + 12) == 0 ? 32 : 0;
    if (i == 1) return shell_prod_save(g_blob + 12, BLOB_MAX);
    if (i == 3) return shell_p2p_kv_save(g_blob + 12, BLOB_MAX);
    { int z; for (z = 0; z < KHDR; z++) g_blob[12 + z] = 0; }
    n = shell_collab_save(g_blob + 12 + KHDR, BLOB_MAX - KHDR);
    return n > 0 ? n + KHDR : n;
}
/* only=-1: all blobs; changed_only: skip blobs identical to the last save */
static int save_all(int verbose, int changed_only, int* wrote) {
    int i, n, bad = 0;
    *wrote = 0;
    for (i = 0; i < NBLOB; i++) {
        uint32_t h; int k, sealed = 0;
        n = build(i);
        if (n < 0) { if (verbose) { print_string("persist-save error "); print_string(k_names[i]); print_string(" too large\n"); } bad = 1; continue; }
        if (n == 0) { if (verbose) { print_string("persist-save skip "); print_string(k_names[i]); print_string(" empty\n"); } continue; }
        h = fnv(g_blob + 12, (uint32_t)n) ^ (uint32_t)(g_pass_set ? 0x9e3779b9U : 0U);
        if (changed_only && g_saved_hash[i] == h) continue;
        if (i == 2 && g_collab_locked) { if (verbose) print_string("persist-save skip collab key locked (persist-unlock first)\n"); continue; }
        if (i == 2) { sealed = seal_key(); if (sealed < 0) { bad = 1; continue; } }
        k = store(k_names[i], n);
        if (k < 0) { if (verbose) { print_string("persist-save error "); print_string(k_names[i]); print_string(" write failed\n"); } bad = 1; continue; }
        g_saved_hash[i] = h; (*wrote)++;
        if (verbose) {
            report("persist-save", k_names[i], n, " bytes chunks ", k);
            if (i == 2) print_string(sealed ? "persist-save collab key encrypted aes128-gcm pbkdf2-sha256\n"
                                            : "persist-save collab key not stored (no passphrase: signing locked after reload)\n");
        }
    }
    return bad;
}
static int auto_interval(void) {
    if (g_auto_secs >= 0) return g_auto_secs;
    {
        static os_ata_status_t st; int r;
        asm volatile("int $0x80" : "=a"(r) : "a"(SYS_ATA_STATUS), "b"(&st) : "memory");
        return (r == 0 && st.boot_driver_pid > 0) ? 60 : 0;
    }
}
/* called after every command and while the shell polls for keys */
void shell_persist_tick(void) {
    uint32_t now = sys_ticks();
    int secs = auto_interval(), wrote = 0;
    if (secs <= 0) return;
    if (g_last_auto == 0) { g_last_auto = now; return; }
    if (now - g_last_auto < (uint32_t)secs * 100U) return;
    g_last_auto = now;
    if (save_all(0, 1, &wrote) == 0 && wrote) { print_string("persist autosave ok blobs "); { char t[12]; t[0] = 0; cat_u(t, (uint32_t)wrote, 12); print_string(t); } print_string("\n"); }
}
/* reboot / shutdown / exit: last save of whatever changed */
void shell_persist_shutdown(void) {
    int wrote = 0;
    if (auto_interval() <= 0 && g_auto_secs != 0) return;
    if (g_auto_secs == 0) return;
    if (save_all(0, 1, &wrote) == 0) { print_string("persist shutdown save ok blobs "); { char t[12]; t[0] = 0; cat_u(t, (uint32_t)wrote, 12); print_string(t); } print_string("\n"); }
    else print_string("persist shutdown save error\n");
}

static int s_pre(const char* s, const char* p) { while (*p && *s == *p) { s++; p++; } return *p == 0; }
int shell_persist_is(const char* l) {
    return s_eq(l, "persist-save") || s_eq(l, "persist-load") || s_eq(l, "persist-status") ||
           s_pre(l, "persist-auto") || s_pre(l, "persist-passphrase ") || s_pre(l, "persist-unlock ");
}
int shell_persist_line(const char* l) {
    int i, n, bad = 0, wrote;
    if (s_pre(l, "persist-passphrase ") || s_pre(l, "persist-unlock ")) {
        const char* p = l; int k = 0, unlock = s_pre(l, "persist-unlock ");
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        while (p[k] && k < 63) { g_pass[k] = p[k]; k++; }
        g_pass[k] = 0;
        if (k < 8) { g_pass_set = 0; print_string(unlock ? "persist-unlock" : "persist-passphrase"); print_string(" error at least 8 characters\n"); return 1; }
        g_pass_set = 1;
        for (i = 0; i < NBLOB; i++) g_saved_hash[i] = 0; /* re-seal on next save */
        if (!unlock) { print_string("persist-passphrase ok (kept in RAM only)\n"); return 0; }
        n = fetch("collab");
        if (n < KHDR) { print_string("persist-unlock error no saved ledger\n"); return 1; }
        { uint8_t sk[20]; int r = open_key(g_blob + 12, sk);
          if (r == 0) { print_string("persist-unlock error no key stored\n"); return 1; }
          if (r < 0) { g_pass_set = 0; print_string("persist-unlock error wrong passphrase\n"); return 1; }
          shell_collab_key_install(sk);
          g_collab_locked = 0;
          for (i = 0; i < 20; i++) sk[i] = 0;
          print_string("persist-unlock ok signing on\n"); return 0; }
    }
    if (s_pre(l, "persist-auto")) {
        const char* p = l + 12; uint32_t v = 0; int any = 0;
        while (*p == ' ') p++;
        if (s_eq(p, "off")) g_auto_secs = 0;
        else if (*p) { while (*p >= '0' && *p <= '9') { v = v * 10U + (uint32_t)(*p++ - '0'); any = 1; } if (!any || *p || v < 5 || v > 3600) { print_string("persist-auto error usage: persist-auto [SECONDS 5..3600|off]\n"); return 1; } g_auto_secs = (int)v; }
        g_last_auto = sys_ticks();
        { char t[64]; t[0] = 0; s_cat(t, "persist-auto ok every ", 64); cat_u(t, (uint32_t)auto_interval(), 64); s_cat(t, auto_interval() ? " s\n" : " s (off)\n", 64); print_string(t); }
        return 0;
    }
    if (s_eq(l, "persist-save")) {
        bad = save_all(1, 0, &wrote);
        print_string(bad ? "persist-save error\n" : "persist-save ok\n");
        return bad;
    }
    if (s_eq(l, "persist-load") || s_eq(l, "persist-status")) {
        int load = s_eq(l, "persist-load");
        for (i = 0; i < NBLOB; i++) {
            n = fetch(k_names[i]);
            if (n == -1) { print_string(l); print_string(" "); print_string(k_names[i]); print_string(" missing\n"); continue; }
            if (n == -2) { print_string(l); print_string(" "); print_string(k_names[i]); print_string(" corrupt (checksum)\n"); bad = 1; continue; }
            if (!load) { report("persist-status", k_names[i], n, " bytes fnv ok ", 1); continue; }
            if (i == 0) { if (n == 32) shell_p2p_seed_set(g_blob + 12); else bad = 1; report("persist-load", k_names[i], n, " bytes seed ", n == 32); }
            else if (i == 1) { int rc = shell_prod_load(g_blob + 12, n); if (rc) bad = 1; report("persist-load", k_names[i], n, " bytes ok ", rc == 0); }
            else if (i == 3) { int rc = shell_p2p_kv_load(g_blob + 12, n); if (rc < 0) bad = 1; report("persist-load", k_names[i], n, " bytes items ", rc < 0 ? 0 : rc); }
            else {
                uint8_t sk[20]; int rc, kr;
                if (n < KHDR) { bad = 1; continue; }
                kr = open_key(g_blob + 12, sk);
                rc = shell_collab_load(g_blob + 12 + KHDR, n - KHDR);
                if (rc < 0) { bad = 1; continue; }
                shell_collab_key_install(kr == 1 ? sk : 0);
                g_collab_locked = kr < 0;
                for (n = 0; n < 20; n++) sk[n] = 0;
                report("persist-load", k_names[i], rc, " entries key ", kr == 1);
                if (kr != 1) print_string(kr == 0 ? "persist-load collab key not stored, signing locked\n"
                                                  : "persist-load collab key locked (persist-unlock PASSPHRASE)\n");
            }
        }
        if (load) { int w; for (i = 0; i < NBLOB; i++) { n = build(i); g_saved_hash[i] = n > 0 ? (fnv(g_blob + 12, (uint32_t)n) ^ (uint32_t)(g_pass_set ? 0x9e3779b9U : 0U)) : 0; } (void)w; }
        print_string(bad ? (load ? "persist-load error\n" : "persist-status error\n") : (load ? "persist-load ok\n" : "persist-status ok\n"));
        return bad;
    }
    return 1;
}
