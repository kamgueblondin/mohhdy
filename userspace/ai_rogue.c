/* airogue: a task that is not aiworker tries every worker-only path of
 * SYS_AI_ENGINE. Each must be refused; the forged reply is counted. */
#include "ai_common.h"

int main(void) {
    static os_ai_engine_reply_t reply;
    static os_ai_engine_job_t job;
    os_ai_engine_map_t map;
    ai_line_t l;
    int reg, ren, rep, mp, fe, go, gr, gy;
    unsigned int i;
    int self;
    reg = ai_service_register("ai-engine");
    asm volatile("int $0x80" : "=a"(self) : "a"(SYS_GETPID));
    asm volatile("int $0x80" : "=a"(ren) : "a"(SYS_TASK_SET_NAME), "b"(self), "c"("aiworker") : "memory");
    for (i = 0U; i < sizeof(reply); i++) ((char*)&reply)[i] = 0;
    reply.job_id = 1U;
    reply.result = 3;
    reply.text_length = 3U;
    reply.text[0] = 'b'; reply.text[1] = 'a'; reply.text[2] = 'd';
    rep = ai_engine(OS_AI_ENGINE_REPLY, (unsigned int)&reply, 0U);
    mp = ai_engine(OS_AI_ENGINE_MAP, OS_AI_ENGINE_BLOB_CHECKPOINT, (unsigned int)&map);
    fe = ai_engine(OS_AI_ENGINE_FETCH, 1U, (unsigned int)&job);
    go = ai_engine(OS_AI_ENGINE_GGUF_OPEN, (unsigned int)&map, 0U);
    gr = ai_engine(OS_AI_ENGINE_GGUF_READ, 0U, 4096U);
    gy = ai_engine(OS_AI_ENGINE_GGUF_READY, 0U, 0U);
    ai_line_reset(&l);
    ai_line_add(&l, "airogue register rc ");
    ai_line_int(&l, reg);
    ai_line_add(&l, " rename rc ");
    ai_line_int(&l, ren);
    ai_line_add(&l, " reply rc ");
    ai_line_int(&l, rep);
    ai_line_add(&l, " map rc ");
    ai_line_int(&l, mp);
    ai_line_add(&l, " fetch rc ");
    ai_line_int(&l, fe);
    ai_line_add(&l, " gguf rc ");
    ai_line_int(&l, go);
    ai_line_add(&l, " ");
    ai_line_int(&l, gr);
    ai_line_add(&l, " ");
    ai_line_int(&l, gy);
    ai_line_add(&l, " end\n");
    ai_puts(l.text);
    return 0;
}
