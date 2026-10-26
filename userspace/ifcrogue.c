/* ifcrogue.c - Lot 47 test-only probe (US-G04 rights): a program spawned by
 * the shell is not the root shell, so the kernel must refuse it any
 * interface change (OS_PEER_DENIED) while still answering a read. It runs
 * the same net_ifcfg code as the shell command net-if. Ships only in the
 * test initrd (TEST_ONLY_PROGRAMS). */
#include "net_ifcfg.h"

static void puts_line(const char* s) {
    asm volatile("int $0x80" : : "a"(3), "b"(s) : "memory");   /* SYS_PUTS adds the newline */
}
static int call(os_peer_data_request_t* r) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_PEER_DATA), "b"((uint32_t)r) : "memory");
    return rc;
}
static void run(int argc, const char* const* argv) {
    static os_peer_data_request_t r;
    static char out[160], line[176];
    int i = 0, k = 0;
    (void)net_ifcfg_run(argc, argv, call, &r, out, (int)sizeof(out));
    for (; "ifcrogue "[k]; k++) line[i++] = "ifcrogue "[k];
    for (k = 0; out[k] && out[k] != '\n' && i < (int)sizeof(line) - 1; k++) line[i++] = out[k];
    line[i] = 0;
    puts_line(line);
}

int main(void) {
    static const char* const set_static[4] = {"static", "10.32.0.66", "255.255.255.0", "10.32.0.2"};
    static const char* const set_dhcp[1] = {"dhcp"};
    run(4, set_static);
    run(1, set_dhcp);
    run(0, set_dhcp);         /* read: allowed */
    puts_line("ifcrogue end");
    return 0;
}
