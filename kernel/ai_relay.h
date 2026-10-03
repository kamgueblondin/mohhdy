#ifndef AI_RELAY_H
#define AI_RELAY_H

/* Inventory item 4: single-slot relay of SYS_GPT2_GENERATE (FP32) from any
 * task to the Ring 3 aiworker that owns "ai-engine" (pure logic, unit
 * tested in tests/unit/kernel/test_ai_relay.c). The IPC doorbell, the
 * caller block/wake and the user copies live in kernel/syscall/syscall.c. */

#include <stdint.h>
#include "../include/os_syscalls.h"

#define AI_RELAY_FREE 0U
#define AI_RELAY_SENT 1U  /* doorbell sent, worker computing */
#define AI_RELAY_DONE 2U  /* worker reply stored, caller not yet resumed */
#define AI_RELAY_FAILED 3U /* worker lost / timeout: caller runs the Ring 0 fallback */

/* A live worker computing 12 tokens of GPT-2 124M under TCG can take tens of
 * seconds; this only bounds a stalled (suspended) worker. 100 Hz ticks. */
#define AI_RELAY_TIMEOUT_TICKS 30000U

void ai_relay_init(void);
uint32_t ai_relay_state(void);
int32_t ai_relay_caller(void);
int32_t ai_relay_worker(void);
uint32_t ai_relay_job(void);
/* Takes the free slot: returns the job id (> 0) or -1 when busy / bad args.
 * prompt is the normalised prompt (truncated to OS_AI_ENGINE_PROMPT_MAX-1),
 * max is clamped to OS_AI_ENGINE_TEXT_MAX. */
int32_t ai_relay_begin(int32_t caller_pid, int32_t worker_pid, const char* prompt,
                       uint32_t max, uint32_t now);
/* GGUF slice: one 109/110 sampling step of the kernel-owned session. */
int32_t ai_relay_begin_gguf(int32_t caller_pid, int32_t worker_pid, const uint32_t* tokens,
                            uint32_t token_count, uint32_t generated, uint32_t rng_state,
                            uint32_t now);
/* OS_AI_JOB_* of the job in flight (0 when free). */
uint32_t ai_relay_kind(void);
/* Doorbell could not be sent: slot freed, nothing counted. */
void ai_relay_cancel(void);
/* Worker side: copy of the job in flight (0) or OS_AI_ENGINE_REQUIRED /
 * OS_AI_ENGINE_STALE. */
int ai_relay_fetch(int32_t sender_pid, int32_t live_worker, uint32_t job_id,
                   os_ai_engine_job_t* out);
/* Worker reply. 0 = stored (slot DONE), OS_AI_ENGINE_REQUIRED = sender is
 * not the live worker (rogue, counted), OS_AI_ENGINE_STALE = wrong or
 * finished job (counted), OS_AI_ENGINE_BAD_ARGUMENT = malformed. */
int ai_relay_complete(int32_t sender_pid, int32_t live_worker, const os_ai_engine_reply_t* reply);
/* 1 if the SENT job must be failed now: worker gone/changed or timeout. */
int ai_relay_should_fail(int32_t live_worker, uint32_t now);
/* SENT -> FAILED (counted as aborted). */
void ai_relay_fail(void);
/* Caller side after wake-up. Returns AI_RELAY_DONE with *reply filled,
 * AI_RELAY_FAILED (caller must run the fallback), or AI_RELAY_FREE if the
 * slot is not the caller's. The slot is freed in both first cases. */
uint32_t ai_relay_take(int32_t caller_pid, os_ai_engine_reply_t* reply);
/* Caller task gone: slot freed, a late reply becomes stale. */
void ai_relay_drop_caller(void);

/* Accounting of the Ring 0 path and of the last generation trace. */
void ai_relay_note_kernel_infer(int worker_live, int fallback);
void ai_relay_note_rogue(void);
void ai_relay_note_gguf_kernel(int worker_ready, int fallback);
/* path OS_AI_PATH_NONE only refreshes the session snapshot. */
void ai_relay_record_gguf(uint32_t path, int32_t result, const uint32_t* tokens,
                          uint32_t prompt_tokens, uint32_t token_count);
/* Worker that declared GGUF_READY after loading `bytes` (0, 0 to clear). */
void ai_relay_set_gguf_worker(int32_t pid, uint32_t bytes);
/* That worker if it is still the live ai-engine owner, else 0. */
int32_t ai_relay_gguf_worker(int32_t live_worker);
void ai_relay_record_last(uint32_t path, int32_t result, const uint32_t* tokens,
                          uint32_t prompt_tokens, uint32_t token_count);
void ai_relay_note_mapped(uint32_t which, uint32_t bytes);
void ai_relay_worker_reset(void);
void ai_relay_fill_status(os_ai_engine_status_t* out, int32_t live_worker);

#endif
