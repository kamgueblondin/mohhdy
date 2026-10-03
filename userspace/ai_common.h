/* Shared helpers of the inventory item 4 programs (aiworker, aiclient,
 * aistat, airogue). */
#ifndef AI_COMMON_H
#define AI_COMMON_H

#include "os_syscalls.h"

static void ai_putc(char c) {
    asm volatile("int $0x80" : : "a"(SYS_PUTC), "b"(c));
}

/* SYS_PUTC per byte (SYS_PUTS has no serial echo). */
static void ai_puts(const char* text) {
    int i = 0;
    while (text[i] != '\0') ai_putc(text[i++]);
}

/* Appends to a line buffer (bounded). */
typedef struct {
    char text[OS_AI_ENGINE_LOG_MAX];
    unsigned int length;
} ai_line_t;

static void ai_line_reset(ai_line_t* l) { l->length = 0U; l->text[0] = '\0'; }

static void ai_line_add(ai_line_t* l, const char* s) {
    unsigned int i = 0U;
    while (s[i] != '\0' && l->length + 1U < sizeof(l->text)) l->text[l->length++] = s[i++];
    l->text[l->length] = '\0';
}

static void ai_line_int(ai_line_t* l, int value) {
    char digits[12];
    char out[13];
    int n = 0, k = 0;
    unsigned int number;
    if (value < 0) {
        out[k++] = '-';
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }
    if (number == 0U) digits[n++] = '0';
    while (number > 0U && n < 11) {
        digits[n++] = (char)('0' + (number % 10U));
        number /= 10U;
    }
    while (n > 0) out[k++] = digits[--n];
    out[k] = '\0';
    ai_line_add(l, out);
}

static void ai_line_hex(ai_line_t* l, unsigned int value) {
    char out[11];
    int i;
    out[0] = '0';
    out[1] = 'x';
    for (i = 0; i < 8; i++) {
        unsigned int nibble = (value >> (28 - 4 * i)) & 0xFU;
        out[2 + i] = (char)(nibble < 10U ? '0' + nibble : 'a' + nibble - 10U);
    }
    out[10] = '\0';
    ai_line_add(l, out);
}

static int ai_engine(unsigned int op, unsigned int ecx, unsigned int edx) {
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_AI_ENGINE), "b"(op), "c"(ecx), "d"(edx) : "memory");
    return result;
}

static int ai_service_register(const char* name) {
    int result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SYS_SERVICE_REGISTER), "b"(name) : "memory");
    return result;
}

static int ai_gpt2_generate(const char* prompt, char* out, int max) {
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_GPT2_GENERATE), "b"(prompt), "c"(out), "d"(max) : "memory");
    return result;
}

static int ai_recv_wait(os_ipc_message_t* message, unsigned int timeout) {
    int result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SYS_IPC_RECV_WAIT), "b"(message), "c"(timeout) : "memory");
    return result;
}

/* One status line, written with SYS_PUTC by the caller. */
static void ai_status_line(ai_line_t* l, const char* who, const os_ai_engine_status_t* st) {
    ai_line_reset(l);
    ai_line_add(l, who);
    ai_line_add(l, " status worker ");
    ai_line_int(l, st->worker_pid);
    ai_line_add(l, " fwd ");
    ai_line_int(l, (int)st->forwarded);
    ai_line_add(l, " done ");
    ai_line_int(l, (int)st->completed);
    ai_line_add(l, " aborted ");
    ai_line_int(l, (int)st->aborted);
    ai_line_add(l, " fallback ");
    ai_line_int(l, (int)st->fallbacks);
    ai_line_add(l, " kernel ");
    ai_line_int(l, (int)st->kernel_infer);
    ai_line_add(l, " live ");
    ai_line_int(l, (int)st->kernel_infer_while_live);
    ai_line_add(l, " rogue ");
    ai_line_int(l, (int)st->rogue_refused);
    ai_line_add(l, " stale ");
    ai_line_int(l, (int)st->stale_refused);
    ai_line_add(l, " pending ");
    ai_line_int(l, (int)st->pending);
    ai_line_add(l, " mapped ");
    ai_line_int(l, (int)st->checkpoint_mapped);
    ai_line_add(l, " end\n");
}

#endif
