/* aistat: prints the SYS_AI_ENGINE relay counters. */
#include "ai_common.h"

int main(void) {
    static os_ai_engine_status_t st;
    ai_line_t l;
    int rc = ai_engine(OS_AI_ENGINE_STATUS, (unsigned int)&st, 0U);
    if (rc != 0) {
        ai_puts("aistat failed\n");
        return 1;
    }
    ai_status_line(&l, "aistat", &st);
    ai_puts(l.text);
    return 0;
}
