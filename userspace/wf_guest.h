/* wf_guest.h - Lot 67: the Mohhdy (Ring 3) side of the web stack: Mohhdy
 * sockets through the networker, DNS over TCP, entropy, the guest clock
 * and the root store from the initrd. One entry point shared by the shell
 * `fetch` / `http-get` commands and the GUI /browser. */
#ifndef WF_GUEST_H
#define WF_GUEST_H
#include "wf_fetch.h"

typedef void (*wf_guest_log_fn)(const char* line);

/* GET url (redirects <= 5). 0 = a final response (any status). */
int wf_guest_fetch(const char* url, uint8_t* body, int cap, wf_fetch_result_t* res, wf_guest_log_fn log);
/* "rdrand" or "weak-tsc" (no RDRAND: TSC jitter + ticks + RTC through
 * SHA-256, not a secure source; said in every fetch report). */
const char* wf_guest_entropy(void);
/* Root store: number of roots loaded (0 if /etc/ca-roots.pem is missing),
 * and whether the test CA (/etc/ca-test.pem, test initrd only) is in. */
int wf_guest_roots(int* test_ca);
/* Guest clock used for validity dates ("YYYYMMDDHHMMSS"), or 0 when the
 * RTC is unreadable or obviously unset (year < 2025): then the dates are
 * not checked and the report says so. */
const char* wf_guest_now(void);
#endif
