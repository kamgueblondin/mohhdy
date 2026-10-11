/* userspace/collab_cond.c - US-099 contract conditions (PromptMessage). */
#include <stdint.h>
#include "promptmessage.h"

/* US-099: collab contract conditions run in a sandboxed PromptMessage VM
 * (no files, no system state, no output): identical on every node. */
static int cond_read(void* c, const char* p, char* b, int cap) { (void)c; (void)p; (void)b; (void)cap; return -1; }
static int cond_write(void* c, const char* p, const char* d, int len, int app) { (void)c; (void)p; (void)d; (void)len; (void)app; return -1; }
static int cond_mem(void* c) { (void)c; return 0; }
static void cond_out(void* c, const char* s) { (void)c; (void)s; }
static const pm_host_t g_cond_host = {0, cond_read, cond_write, cond_mem, cond_out};
static void cond_num(char* d, int* k, int32_t v) {
    char t[12]; int n = 0;
    if (v < 0) { d[(*k)++] = '-'; v = -v; }
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) d[(*k)++] = t[--n];
}
int shell_collab_cond(const char* cond, int32_t bal, uint32_t rep10, uint32_t paid) {
    static char src[256];
    static pm_program_t prog;
    static pm_vm_t vm;
    int k = 0, i, r;
    const char* h;
    for (i = 0; cond[i]; i++) if (cond[i] == '"' || cond[i] == '\n') return -1;
    for (h = "set bal to "; *h; ) { src[k++] = *h++; }
    cond_num(src, &k, bal); src[k++] = '\n';
    for (h = "set rep to "; *h; ) { src[k++] = *h++; }
    cond_num(src, &k, (int32_t)rep10); src[k++] = '\n';
    for (h = "set paid to "; *h; ) { src[k++] = *h++; }
    cond_num(src, &k, (int32_t)paid); src[k++] = '\n';
    for (h = "expect "; *h; ) src[k++] = *h++;
    for (i = 0; cond[i] && k < 250; i++) src[k++] = cond[i];
    src[k++] = '\n'; src[k] = 0;
    if (pm_compile(src, k, &prog, 0) != PM_OK) return -1;
    if (prog.ntrig) return -1;                       /* a condition, not a program */
    pm_vm_reset(&vm);
    r = pm_run(&prog, &vm, &g_cond_host);
    if (r == PM_OK) return vm.expect_failed ? 0 : (vm.writes || vm.prints ? -1 : 1);
    if (r == PM_ERR_EXPECT) return 0;
    return -1;
}
