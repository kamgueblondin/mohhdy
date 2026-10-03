/* aiworker: Ring 3 GPT-2 FP32 engine (inventory item 4).
 *
 * Registers "ai-engine" (pinned to this binary name), maps the initrd
 * tokenizer and llm.c checkpoint read-only into its own address space
 * (OS_AI_ENGINE_MAP: zero copy, the frames the Ring 0 fallback reads),
 * then sleeps in SYS_IPC_RECV_WAIT. Each kernel doorbell (sender 0,
 * OS_IPC_AI_ENGINE_REQUEST) is a SYS_GPT2_GENERATE call relayed from
 * another task: the worker pulls the normalised prompt (OS_AI_ENGINE_FETCH),
 * runs gpt2_generate_fp32() (kernel/llm, built here at CPL 3 with the same
 * -O3 -msse2 -mfpmath=sse flags as the kernel) and answers with
 * OS_AI_ENGINE_REPLY (text + token ids). Activations and the KV cache are
 * this program's own .bss (gpt2_infer.c static workspace).
 *
 * GGUF slice: when the kernel has the FAT16 GGUF profile, the worker opens a
 * window of its own memory (OS_AI_ENGINE_GGUF_OPEN), fills it with the
 * worker-only bulk read (OS_AI_ENGINE_GGUF_READ, 1 MiB chunks), points the
 * GGUF runtime (kernel/llm/gpt2_gguf*.c + gpt2_quant.c K-quant kernels,
 * built here at CPL 3) at it through ai_fat16_shim.c and declares
 * OS_AI_ENGINE_GGUF_READY. A doorbell of kind OS_AI_JOB_GGUF_STEP is then one
 * gpt2_gguf_generate_next_sampled() step of the kernel's 109/110 session. */
#include "ai_common.h"
#include "ai_fat16_shim.h"
#include "llm/gpt2_generate.h"
#include "llm/gpt2_gguf_infer.h"
#include "llm/gpt2_model.h"
#include "llm/gpt2_tokenizer.h"

static os_ai_engine_reply_t reply;
static os_ai_engine_job_t job;
static ai_line_t line;
static fat16_volume_t gguf_volume;
static int gguf_ready;

static void wlog(void) {
    if (ai_engine(OS_AI_ENGINE_LOG, (unsigned int)line.text, line.length) != 0) ai_puts(line.text);
}

static void exit_program(int code) {
    asm volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code));
    for (;;) {}
}

static void serve(const os_ipc_message_t* message) {
    gpt2_generate_trace_t trace;
    int rc, result;
    unsigned int i;
    if (message->sender_pid != 0 || message->type != OS_IPC_AI_ENGINE_REQUEST) {
        ai_line_reset(&line);
        ai_line_add(&line, "aiworker ignored message from pid ");
        ai_line_int(&line, message->sender_pid);
        ai_line_add(&line, "\n");
        wlog();
        return;
    }
    rc = ai_engine(OS_AI_ENGINE_FETCH, message->request_id, (unsigned int)&job);
    if (rc != 0) {
        ai_line_reset(&line);
        ai_line_add(&line, "aiworker fetch rc ");
        ai_line_int(&line, rc);
        ai_line_add(&line, "\n");
        wlog();
        return;
    }
    for (i = 0U; i < sizeof(reply); i++) ((char*)&reply)[i] = 0;
    if (job.kind == OS_AI_JOB_GGUF_STEP) {
        unsigned int next = 0U, rng = job.rng_state;
        result = gguf_ready ? gpt2_gguf_generate_next_sampled(job.tokens, job.token_count,
                                                              job.generated, &next, &rng)
                            : -1;
        reply.job_id = job.job_id;
        reply.kind = OS_AI_JOB_GGUF_STEP;
        reply.result = result;
        reply.next_token = next;
        reply.rng_state = rng;
        rc = ai_engine(OS_AI_ENGINE_REPLY, (unsigned int)&reply, 0U);
        ai_line_reset(&line);
        ai_line_add(&line, "aiworker gguf job ");
        ai_line_int(&line, (int)job.job_id);
        ai_line_add(&line, " rc ");
        ai_line_int(&line, result);
        ai_line_add(&line, " next ");
        ai_line_int(&line, (int)next);
        ai_line_add(&line, " reply rc ");
        ai_line_int(&line, rc);
        ai_line_add(&line, "\n");
        wlog();
        return;
    }
    result = gpt2_generate_fp32(job.prompt, reply.text, job.max, &trace);
    reply.job_id = job.job_id;
    reply.result = result;
    reply.text_length = result > 0 ? (unsigned int)result : 0U;
    reply.prompt_tokens = trace.prompt_tokens;
    reply.token_count = trace.token_count;
    for (i = 0U; i < trace.token_count && i < OS_AI_ENGINE_TOKENS_MAX; i++) reply.tokens[i] = trace.tokens[i];
    rc = ai_engine(OS_AI_ENGINE_REPLY, (unsigned int)&reply, 0U);
    ai_line_reset(&line);
    ai_line_add(&line, "aiworker job ");
    ai_line_int(&line, (int)job.job_id);
    ai_line_add(&line, " result ");
    ai_line_int(&line, result);
    ai_line_add(&line, " tokens ");
    ai_line_int(&line, (int)trace.token_count);
    ai_line_add(&line, " reply rc ");
    ai_line_int(&line, rc);
    ai_line_add(&line, "\n");
    wlog();
}

/* Worker-owned copy of GPT2.GGU through the worker-only bulk read. */
static int gguf_load(void) {
    os_ai_engine_map_t window;
    unsigned int offset = 0U, chunks = 0U;
    int rc = ai_engine(OS_AI_ENGINE_GGUF_OPEN, (unsigned int)&window, 0U);
    if (rc != 0) return rc;
    while (offset < window.size) {
        rc = ai_engine(OS_AI_ENGINE_GGUF_READ, offset, OS_AI_ENGINE_GGUF_CHUNK_MAX);
        if (rc <= 0) return rc < 0 ? rc : -1;
        offset += (unsigned int)rc;
        chunks++;
    }
    rc = ai_fat16_shim_attach(&gguf_volume, "GPT2.GGU", (const uint8_t*)window.address, window.size);
    if (rc == 0) rc = gpt2_gguf_infer_init_fat16(&gguf_volume, "GPT2.GGU");
    if (rc == 0) rc = ai_engine(OS_AI_ENGINE_GGUF_READY, 0U, 0U);
    if (rc != 0) return rc;
    gguf_ready = 1;
    ai_line_reset(&line);
    ai_line_add(&line, "aiworker gguf ready bytes ");
    ai_line_int(&line, (int)window.size);
    ai_line_add(&line, " at ");
    ai_line_hex(&line, window.address);
    ai_line_add(&line, " chunks ");
    ai_line_int(&line, (int)chunks);
    ai_line_add(&line, "\n");
    wlog();
    return 0;
}

int main(void) {
    os_ai_engine_map_t tok, ckpt;
    os_ipc_message_t message;
    int rc, fp32_rc, gguf_rc;

    rc = ai_service_register("ai-engine");
    if (rc != 0) {
        ai_line_reset(&line);
        ai_line_add(&line, "aiworker register rc ");
        ai_line_int(&line, rc);
        ai_line_add(&line, "\n");
        ai_puts(line.text);
        exit_program(1);
    }
    ckpt.address = 0U;
    ckpt.size = 0U;
    rc = ai_engine(OS_AI_ENGINE_MAP, OS_AI_ENGINE_BLOB_TOKENIZER, (unsigned int)&tok);
    if (rc == 0) rc = gpt2_tokenizer_load_from_buffer((const uint8_t*)tok.address, tok.size);
    fp32_rc = rc;
    if (fp32_rc == 0) fp32_rc = ai_engine(OS_AI_ENGINE_MAP, OS_AI_ENGINE_BLOB_CHECKPOINT, (unsigned int)&ckpt);
    if (fp32_rc == 0) fp32_rc = gpt2_model_load_from_buffer((const uint8_t*)ckpt.address, ckpt.size);
    if (fp32_rc != 0) {
        ckpt.address = 0U;
        ckpt.size = 0U;
    }
    gguf_rc = rc == 0 ? gguf_load() : rc;
    if (rc != 0 || (fp32_rc != 0 && gguf_rc != 0)) {
        /* No usable model: leave; the kernel keeps the Ring 0 path. */
        ai_line_reset(&line);
        ai_line_add(&line, "aiworker model unavailable rc ");
        ai_line_int(&line, rc != 0 ? rc : fp32_rc);
        ai_line_add(&line, " gguf rc ");
        ai_line_int(&line, gguf_rc);
        ai_line_add(&line, "\n");
        wlog();
        exit_program(2);
    }
    /* A reply for a job that is not in flight must be refused (stale). */
    for (rc = 0; rc < (int)sizeof(reply); rc++) ((char*)&reply)[rc] = 0;
    reply.job_id = 0x7FFFFFF0U;
    rc = ai_engine(OS_AI_ENGINE_REPLY, (unsigned int)&reply, 0U);
    ai_line_reset(&line);
    ai_line_add(&line, "aiworker stale reply refused rc ");
    ai_line_int(&line, rc);
    ai_line_add(&line, "\n");
    wlog();
    ai_line_reset(&line);
    ai_line_add(&line, "aiworker ready ai-engine checkpoint ");
    ai_line_int(&line, (int)ckpt.size);
    ai_line_add(&line, " at ");
    ai_line_hex(&line, ckpt.address);
    ai_line_add(&line, " tokenizer ");
    ai_line_int(&line, (int)tok.size);
    ai_line_add(&line, " layers ");
    ai_line_int(&line, (int)gpt2_model_current()->config.num_layers);
    ai_line_add(&line, " channels ");
    ai_line_int(&line, (int)gpt2_model_current()->config.channels);
    ai_line_add(&line, "\n");
    wlog();
    for (;;) {
        rc = ai_recv_wait(&message, OS_IPC_WAIT_FOREVER);
        if (rc == 0) serve(&message);
    }
    return 0;
}
