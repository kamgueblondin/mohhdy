#include "ai_relay.h"

typedef struct {
    uint32_t state;
    int32_t caller_pid;
    int32_t worker_pid;
    uint32_t job_id;
    uint32_t next_job;
    uint32_t started;
    uint32_t progress;   /* tick of the last fetch / heartbeat */
    uint32_t abort;      /* OS_AI_ABORT_* of the last failed job */
    os_ai_engine_job_t job;
    os_ai_engine_reply_t reply;
} ai_relay_slot_t;

static ai_relay_slot_t slot;
static os_ai_engine_status_t stats;
static int32_t gguf_worker;

static void ai_copy(void* dst, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    uint32_t i;
    for (i = 0U; i < n; i++) d[i] = s[i];
}

static void ai_zero(void* dst, uint32_t n) {
    uint8_t* d = (uint8_t*)dst;
    uint32_t i;
    for (i = 0U; i < n; i++) d[i] = 0U;
}

void ai_relay_init(void) {
    ai_zero(&slot, sizeof(slot));
    ai_zero(&stats, sizeof(stats));
    gguf_worker = 0;
    slot.next_job = 1U;
}

uint32_t ai_relay_state(void) { return slot.state; }
int32_t ai_relay_caller(void) { return slot.state == AI_RELAY_FREE ? 0 : slot.caller_pid; }
int32_t ai_relay_worker(void) { return slot.state == AI_RELAY_FREE ? 0 : slot.worker_pid; }
uint32_t ai_relay_job(void) { return slot.state == AI_RELAY_FREE ? 0U : slot.job_id; }

int32_t ai_relay_begin(int32_t caller_pid, int32_t worker_pid, const char* prompt,
                       uint32_t max, uint32_t now) {
    uint32_t n = 0U;
    if (slot.state != AI_RELAY_FREE || caller_pid <= 0 || worker_pid <= 0 ||
        caller_pid == worker_pid || !prompt || max < 2U)
        return -1;
    if (slot.next_job == 0U || slot.next_job > 0x7FFFFFFFU) slot.next_job = 1U;
    ai_zero(&slot.job, sizeof(slot.job));
    ai_zero(&slot.reply, sizeof(slot.reply));
    while (prompt[n] != '\0' && n + 1U < OS_AI_ENGINE_PROMPT_MAX) {
        slot.job.prompt[n] = prompt[n];
        n++;
    }
    slot.job.prompt[n] = '\0';
    slot.job.prompt_length = n;
    slot.job.max = max > OS_AI_ENGINE_TEXT_MAX ? OS_AI_ENGINE_TEXT_MAX : max;
    slot.job_id = slot.next_job++;
    slot.job.job_id = slot.job_id;
    slot.caller_pid = caller_pid;
    slot.worker_pid = worker_pid;
    slot.started = now;
    slot.progress = now;
    slot.abort = OS_AI_ABORT_NONE;
    slot.state = AI_RELAY_SENT;
    stats.forwarded++;
    return (int32_t)slot.job_id;
}

int32_t ai_relay_begin_gguf(int32_t caller_pid, int32_t worker_pid, const uint32_t* tokens,
                            uint32_t token_count, uint32_t generated, uint32_t rng_state,
                            uint32_t now) {
    uint32_t i;
    if (slot.state != AI_RELAY_FREE || caller_pid <= 0 || worker_pid <= 0 ||
        caller_pid == worker_pid || !tokens || token_count == 0U ||
        token_count > OS_AI_ENGINE_TOKENS_MAX || generated > token_count)
        return -1;
    if (slot.next_job == 0U || slot.next_job > 0x7FFFFFFFU) slot.next_job = 1U;
    ai_zero(&slot.job, sizeof(slot.job));
    ai_zero(&slot.reply, sizeof(slot.reply));
    slot.job.kind = OS_AI_JOB_GGUF_STEP;
    slot.job.rng_state = rng_state;
    slot.job.generated = generated;
    slot.job.token_count = token_count;
    for (i = 0U; i < token_count; i++) slot.job.tokens[i] = tokens[i];
    slot.job_id = slot.next_job++;
    slot.job.job_id = slot.job_id;
    slot.caller_pid = caller_pid;
    slot.worker_pid = worker_pid;
    slot.started = now;
    slot.progress = now;
    slot.abort = OS_AI_ABORT_NONE;
    slot.state = AI_RELAY_SENT;
    stats.gguf_forwarded++;
    return (int32_t)slot.job_id;
}

int32_t ai_relay_begin_gguf_session(int32_t caller_pid, int32_t worker_pid, uint32_t op,
                                    uint32_t session_id, const char* prompt, uint32_t max,
                                    const uint32_t* tokens, uint32_t token_count,
                                    uint32_t prompt_tokens, uint32_t rng_state, uint32_t now) {
    uint32_t i, n = 0U;
    if (slot.state != AI_RELAY_FREE || caller_pid <= 0 || worker_pid <= 0 ||
        caller_pid == worker_pid || max < 2U ||
        (op != OS_AI_GGUF_SESSION_START && op != OS_AI_GGUF_SESSION_STEP) ||
        (op == OS_AI_GGUF_SESSION_START && !prompt) ||
        (op == OS_AI_GGUF_SESSION_STEP &&
         (!tokens || token_count == 0U || token_count > OS_AI_ENGINE_TOKENS_MAX ||
          prompt_tokens > token_count)))
        return -1;
    if (slot.next_job == 0U || slot.next_job > 0x7FFFFFFFU) slot.next_job = 1U;
    ai_zero(&slot.job, sizeof(slot.job));
    ai_zero(&slot.reply, sizeof(slot.reply));
    slot.job.kind = OS_AI_JOB_GGUF_SESSION;
    slot.job.session_op = op;
    slot.job.session_id = session_id;
    slot.job.max = max > OS_AI_ENGINE_TEXT_MAX ? OS_AI_ENGINE_TEXT_MAX : max;
    if (op == OS_AI_GGUF_SESSION_START) {
        while (prompt[n] != '\0' && n + 1U < OS_AI_ENGINE_PROMPT_MAX) {
            slot.job.prompt[n] = prompt[n];
            n++;
        }
        slot.job.prompt[n] = '\0';
        slot.job.prompt_length = n;
    } else {
        slot.job.rng_state = rng_state;
        slot.job.token_count = token_count;
        slot.job.prompt_tokens = prompt_tokens;
        slot.job.generated = token_count - prompt_tokens;
        for (i = 0U; i < token_count; i++) slot.job.tokens[i] = tokens[i];
    }
    slot.job_id = slot.next_job++;
    slot.job.job_id = slot.job_id;
    slot.caller_pid = caller_pid;
    slot.worker_pid = worker_pid;
    slot.started = now;
    slot.progress = now;
    slot.abort = OS_AI_ABORT_NONE;
    slot.state = AI_RELAY_SENT;
    stats.gguf_forwarded++;
    return (int32_t)slot.job_id;
}

uint32_t ai_relay_kind(void) { return slot.state == AI_RELAY_FREE ? 0U : slot.job.kind; }

void ai_relay_cancel(void) {
    if (slot.state != AI_RELAY_SENT) return;
    if (slot.job.kind == OS_AI_JOB_GGUF_STEP || slot.job.kind == OS_AI_JOB_GGUF_SESSION) {
        if (stats.gguf_forwarded > 0U) stats.gguf_forwarded--;
    } else if (stats.forwarded > 0U) {
        stats.forwarded--;
    }
    slot.state = AI_RELAY_FREE;
}

int ai_relay_fetch(int32_t sender_pid, int32_t live_worker, uint32_t job_id,
                   os_ai_engine_job_t* out) {
    if (sender_pid <= 0 || live_worker <= 0 || sender_pid != live_worker) return OS_AI_ENGINE_REQUIRED;
    if (!out) return OS_AI_ENGINE_BAD_ARGUMENT;
    if (slot.state != AI_RELAY_SENT || slot.worker_pid != sender_pid || slot.job_id != job_id) {
        stats.stale_refused++;
        return OS_AI_ENGINE_STALE;
    }
    ai_copy(out, &slot.job, sizeof(*out));
    return 0;
}

int ai_relay_complete(int32_t sender_pid, int32_t live_worker, const os_ai_engine_reply_t* reply) {
    uint32_t i;
    if (sender_pid <= 0 || live_worker <= 0 || sender_pid != live_worker) {
        stats.rogue_refused++;
        return OS_AI_ENGINE_REQUIRED;
    }
    if (!reply) return OS_AI_ENGINE_BAD_ARGUMENT;
    if (slot.state != AI_RELAY_SENT || slot.worker_pid != sender_pid || slot.job_id != reply->job_id) {
        stats.stale_refused++;
        return OS_AI_ENGINE_STALE;
    }
    if (reply->kind != slot.job.kind) return OS_AI_ENGINE_BAD_ARGUMENT;
    if (slot.job.kind == OS_AI_JOB_GGUF_STEP) {
        if (reply->result > 0 || reply->text_length != 0U || reply->token_count != 0U)
            return OS_AI_ENGINE_BAD_ARGUMENT;
        ai_copy(&slot.reply, reply, sizeof(slot.reply));
        slot.state = AI_RELAY_DONE;
        return 0;
    }
    if (slot.job.kind == OS_AI_JOB_GGUF_SESSION) {
        /* The reply becomes the kernel mirror: bound every field. */
        if (reply->text_length >= OS_AI_ENGINE_TEXT_MAX || reply->text_length >= slot.job.max ||
            reply->token_count > OS_AI_ENGINE_TOKENS_MAX || reply->prompt_tokens > reply->token_count ||
            reply->session_active > 1U ||
            (reply->result >= 0 && (uint32_t)reply->result != reply->text_length))
            return OS_AI_ENGINE_BAD_ARGUMENT;
        ai_copy(&slot.reply, reply, sizeof(slot.reply));
        for (i = reply->text_length; i < OS_AI_ENGINE_TEXT_MAX; i++) slot.reply.text[i] = '\0';
        slot.state = AI_RELAY_DONE;
        return 0;
    }
    if (reply->text_length >= OS_AI_ENGINE_TEXT_MAX || reply->text_length >= slot.job.max ||
        reply->token_count > OS_AI_ENGINE_TOKENS_MAX || reply->prompt_tokens > reply->token_count ||
        (reply->result >= 0 && (uint32_t)reply->result != reply->text_length))
        return OS_AI_ENGINE_BAD_ARGUMENT;
    ai_copy(&slot.reply, reply, sizeof(slot.reply));
    for (i = reply->text_length; i < OS_AI_ENGINE_TEXT_MAX; i++) slot.reply.text[i] = '\0';
    slot.state = AI_RELAY_DONE;
    return 0;
}

int ai_relay_heartbeat(int32_t sender_pid, int32_t live_worker, uint32_t job_id, uint32_t now) {
    if (sender_pid <= 0 || live_worker <= 0 || sender_pid != live_worker) return OS_AI_ENGINE_REQUIRED;
    if (slot.state != AI_RELAY_SENT || slot.worker_pid != sender_pid || slot.job_id != job_id)
        return OS_AI_ENGINE_STALE;
    slot.progress = now;
    stats.heartbeats++;
    return 0;
}

uint32_t ai_relay_should_fail(int32_t live_worker, uint32_t now) {
    if (slot.state != AI_RELAY_SENT) return OS_AI_ABORT_NONE;
    if (live_worker <= 0 || live_worker != slot.worker_pid) return OS_AI_ABORT_WORKER_LOST;
    if (now - slot.progress > AI_RELAY_STALL_TICKS) return OS_AI_ABORT_STALLED;
    if (now - slot.started > AI_RELAY_TIMEOUT_TICKS) return OS_AI_ABORT_TIMEOUT;
    return OS_AI_ABORT_NONE;
}

void ai_relay_fail(uint32_t reason) {
    if (slot.state != AI_RELAY_SENT) return;
    slot.state = AI_RELAY_FAILED;
    slot.abort = reason;
    if (reason == OS_AI_ABORT_CANCELLED) {
        stats.cancelled++;
        return;
    }
    stats.aborted++;
    if (reason == OS_AI_ABORT_STALLED) stats.stalls++;
    else if (reason == OS_AI_ABORT_TIMEOUT) stats.timeouts++;
    else stats.lost++;
}

uint32_t ai_relay_last_abort(void) { return slot.abort; }

void ai_relay_note_cancelled(void) { stats.cancelled++; }

void ai_relay_record_outcome(int gguf, uint32_t latency_ticks, uint32_t error, uint32_t abort) {
    if (gguf) {
        stats.gguf_last_latency_ticks = latency_ticks;
        stats.gguf_last_error = error;
        stats.gguf_last_abort = abort;
    } else {
        stats.last_latency_ticks = latency_ticks;
        stats.last_error = error;
        stats.last_abort = abort;
    }
}

uint32_t ai_relay_take(int32_t caller_pid, os_ai_engine_reply_t* reply) {
    uint32_t state = slot.state;
    if ((state != AI_RELAY_DONE && state != AI_RELAY_FAILED) || slot.caller_pid != caller_pid)
        return AI_RELAY_FREE;
    if (state == AI_RELAY_DONE) {
        if (reply) ai_copy(reply, &slot.reply, sizeof(*reply));
        if (slot.job.kind == OS_AI_JOB_GGUF_STEP) stats.gguf_completed++;
        else if (slot.job.kind == OS_AI_JOB_GGUF_SESSION) {
            stats.gguf_completed++;
            stats.gguf_session_worker++;
            if (slot.reply.session_resumed) stats.gguf_session_resumed++;
        } else stats.completed++;
    }
    slot.state = AI_RELAY_FREE;
    return state;
}

void ai_relay_drop_caller(void) {
    if (slot.state == AI_RELAY_SENT) {
        stats.aborted++;
        stats.lost++;
    }
    slot.state = AI_RELAY_FREE;
}

void ai_relay_note_kernel_infer(int worker_live, int fallback) {
    stats.kernel_infer++;
    if (fallback) stats.fallbacks++;
    else if (worker_live) stats.kernel_infer_while_live++;
}

void ai_relay_note_gguf_kernel(int worker_ready, int fallback) {
    stats.gguf_kernel++;
    if (fallback) stats.gguf_fallbacks++;
    else if (worker_ready) stats.gguf_kernel_while_live++;
}

void ai_relay_note_gguf_refused(int32_t result) {
    stats.gguf_last_path = OS_AI_PATH_NONE;
    stats.gguf_last_result = result;
}

void ai_relay_record_gguf(uint32_t path, int32_t result, const uint32_t* tokens,
                          uint32_t prompt_tokens, uint32_t token_count) {
    uint32_t i;
    if (token_count > OS_AI_ENGINE_TOKENS_MAX) token_count = OS_AI_ENGINE_TOKENS_MAX;
    if (prompt_tokens > token_count) prompt_tokens = token_count;
    if (path != OS_AI_PATH_NONE) {
        stats.gguf_last_path = path;
        stats.gguf_last_result = result;
    }
    stats.gguf_prompt_tokens = prompt_tokens;
    stats.gguf_token_count = tokens ? token_count : 0U;
    for (i = 0U; i < OS_AI_ENGINE_TOKENS_MAX; i++)
        stats.gguf_tokens[i] = (tokens && i < token_count) ? tokens[i] : 0U;
}

void ai_relay_set_gguf_worker(int32_t pid, uint32_t bytes) {
    gguf_worker = pid;
    stats.gguf_bytes_loaded = bytes;
}

int32_t ai_relay_gguf_worker(int32_t live_worker) {
    return (live_worker > 0 && gguf_worker == live_worker) ? gguf_worker : 0;
}

void ai_relay_note_rogue(void) { stats.rogue_refused++; }

void ai_relay_note_gguf_session_kernel(void) { stats.gguf_session_kernel++; }

void ai_relay_record_last(uint32_t path, int32_t result, const uint32_t* tokens,
                          uint32_t prompt_tokens, uint32_t token_count) {
    uint32_t i;
    if (token_count > OS_AI_ENGINE_TOKENS_MAX) token_count = OS_AI_ENGINE_TOKENS_MAX;
    if (prompt_tokens > token_count) prompt_tokens = token_count;
    stats.last_path = path;
    stats.last_result = result;
    stats.last_prompt_tokens = prompt_tokens;
    stats.last_token_count = tokens ? token_count : 0U;
    for (i = 0U; i < OS_AI_ENGINE_TOKENS_MAX; i++)
        stats.last_tokens[i] = (tokens && i < token_count) ? tokens[i] : 0U;
}

void ai_relay_note_mapped(uint32_t which, uint32_t bytes) {
    if (which == OS_AI_ENGINE_BLOB_CHECKPOINT) stats.checkpoint_mapped = bytes;
    if (which == OS_AI_ENGINE_BLOB_TOKENIZER) stats.tokenizer_mapped = bytes;
}

void ai_relay_worker_reset(void) {
    stats.checkpoint_mapped = 0U;
    stats.tokenizer_mapped = 0U;
    gguf_worker = 0;
    stats.gguf_bytes_loaded = 0U;
}

void ai_relay_fill_status(os_ai_engine_status_t* out, int32_t live_worker) {
    if (!out) return;
    ai_copy(out, &stats, sizeof(*out));
    out->worker_pid = live_worker > 0 ? live_worker : 0;
    out->pending = slot.state == AI_RELAY_SENT ? 1U : 0U;
    out->gguf_worker_pid = ai_relay_gguf_worker(live_worker);
    if (out->gguf_worker_pid == 0) out->gguf_bytes_loaded = 0U;
}
