#ifndef ATA_FSOP_H
#define ATA_FSOP_H

/* Tranche 4 suite: kernel side of the filesystem operations served by the
 * Ring 3 atadriver's own FAT16/FAT32/overlay code (pure logic, no PIO, no
 * scheduler; kernel/syscall/syscall.c blocks the caller and wakes it).
 *
 *  - store: which volumes the live driver serves (OS_ATA_FS_STORE_*), set
 *    by OS_ATA_FS_READY, dropped when the driver dies;
 *  - slot: one request in flight (the caller is blocked on a kernel RPC):
 *    FREE -> SUBMITTED -> FETCHED -> DONE (or ABORTED) -> FREE;
 *  - mirror: the last overlay snapshot image the driver published, so the
 *    kernel can take the store back after a driver death without replaying
 *    anything (fallback only).
 */

#include <stdint.h>
#include "os_syscalls.h"

#define ATA_FSOP_FREE      0
#define ATA_FSOP_SUBMITTED 1
#define ATA_FSOP_FETCHED   2
#define ATA_FSOP_DONE      3
#define ATA_FSOP_ABORTED   4

/* 30224 bytes = OV_SNAP_SIZE (fs/overlay.h), checked in ata_fsop.c. */
#define ATA_FSOP_MIRROR_BYTES (16U + 64U * (1U + 1U + 2U + 4U + 80U + 384U))

void ata_fsop_init(void);

/* Store ownership. */
int ata_fsop_store_ready(int32_t driver_pid, uint32_t flags);
void ata_fsop_store_drop(void);
/* Flags served by live_driver_pid (0 when it does not own the store). */
uint32_t ata_fsop_store_flags(int32_t live_driver_pid);
int32_t ata_fsop_store_pid(void);

/* Request slot. The kernel encodes into the request buffer, then submits. */
uint8_t* ata_fsop_request_buffer(void);
uint32_t ata_fsop_request_capacity(void);
/* Returns the generation (> 0) or -1 if the slot is busy / bad length. */
int ata_fsop_submit(uint32_t length, uint32_t op, int32_t driver_pid, uint32_t flags);
int ata_fsop_state(void);
uint32_t ata_fsop_op(void);
/* Driver side. fetch: 1 = job filled (op OS_ATA_JOB_FS_OP), 0 = nothing. */
int ata_fsop_fetch(int32_t pid, os_ata_job_t* job);
int ata_fsop_copy_request(int32_t pid, uint32_t generation, uint8_t* dst, uint32_t capacity);
int ata_fsop_note(int32_t pid, uint32_t generation, uint32_t kind);
int ata_fsop_publish(int32_t pid, uint32_t generation, const uint8_t* image, uint32_t size,
                     int32_t result);
/* reply = os_ata_fsop_reply_t + out_len bytes. OS_ATA_JOB_FS_DONE or <0. */
int ata_fsop_done(int32_t pid, uint32_t generation, const uint8_t* reply, uint32_t length);
/* Caller side after wake-up (DONE): copy out, free the slot. 0 or -1. */
int ata_fsop_take(uint8_t* out, uint32_t capacity, os_ata_fsop_reply_t* reply);
/* Driver died or stalled: the in-flight op is ABORTED (late DONE is stale). */
void ata_fsop_abort(void);
/* ABORTED slot facts, then ata_fsop_release() frees it. */
int ata_fsop_committed(void);
int ata_fsop_published(int32_t* result);
void ata_fsop_release(void);

/* Mirror. */
int ata_fsop_mirror_set(const uint8_t* image, uint32_t size);
/* Handover: the kernel serialises its store straight into the mirror. */
uint8_t* ata_fsop_mirror_buffer(void);
int ata_fsop_mirror_commit(uint32_t size);
const uint8_t* ata_fsop_mirror(uint32_t* size);

/* Counters. */
void ata_fsop_note_kernel_live(void);
void ata_fsop_note_redone(void);
void ata_fsop_note_unavailable(void);
void ata_fsop_note_handover(void);
void ata_fsop_note_restore(void);
uint32_t ata_fsop_persisted(void);
uint32_t ata_fsop_sectors_read(void);
uint32_t ata_fsop_sectors_written(void);
void ata_fsop_fill_status(os_ata_status_t* out, int32_t live_driver_pid);

#endif
