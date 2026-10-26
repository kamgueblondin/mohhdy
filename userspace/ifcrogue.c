/* ifcrogue.c - Lot 47 test-only probe (US-G04 rights): a program spawned by
 * the shell is not the root shell, so the kernel must refuse it any
 * interface change (OS_PEER_DENIED) while still answering a read. It runs
 * the same net_ifcfg code as the shell command net-if. Ships only in the
 * test initrd (TEST_ONLY_PROGRAMS). The console of a spawned program is
 * not on the serial log, so the verdict is the exit code: bit 0 static
 * refused, bit 1 dhcp refused, bit 2 read answered (7 = expected). */
#include "net_ifcfg.h"

static void puts_line(const char* s) {
    asm volatile("int $0x80" : : "a"(3), "b"(s) : "memory");   /* SYS_PUTS adds the newline */
}
static int call(os_peer_data_request_t* r) {
    int rc;
    asm volatile("int $0x80" : "=a"(rc) : "a"(SYS_PEER_DATA), "b"((uint32_t)r) : "memory");
    return rc;
}
static int run(int argc, const char* const* argv) {
    static os_peer_data_request_t r;
    static char out[160], line[176];
    int i = 0, k = 0;
    int rc = net_ifcfg_run(argc, argv, call, &r, out, (int)sizeof(out));
    for (; "ifcrogue "[k]; k++) line[i++] = "ifcrogue "[k];
    for (k = 0; out[k] && out[k] != '\n' && i < (int)sizeof(line) - 1; k++) line[i++] = out[k];
    line[i] = 0;
    puts_line(line);
    if (rc) return out[13] == 'd' && out[14] == 'e' ? 1 : 0;   /* "net-if error denied" */
    return 2;
}

int main(void) {
    static const char* const set_static[4] = {"static", "10.32.0.66", "255.255.255.0", "10.32.0.2"};
    static const char* const set_dhcp[1] = {"dhcp"};
    int code = 0;
    if (run(4, set_static) == 1) code |= 1;
    if (run(1, set_dhcp) == 1) code |= 2;
    if (run(0, set_dhcp) == 2) code |= 4;         /* read: allowed */
    puts_line("ifcrogue end");
    return code;
}
