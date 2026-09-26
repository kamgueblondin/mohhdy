#ifndef ATA_JOB_H
#define ATA_JOB_H

#include <stdint.h>
#include "os_syscalls.h"

/* Tranche 4 slice 2: controller ownership and the overlay snapshot job queue
 * between the kernel and the Ring 3 atadriver. Pure logic (no port access,
 * no scheduler) so it is unit tested on the host.
 *
 * Ownership: at most one holder. The kernel PIO path may run only when no
 * live driver holds the controller; the driver gets its ports opened in the
 * TSS IOPB only while it holds the claim. A claim held by a PID that is no
 * longer the live driver is stale and is dropped on the next check. */

#define ATA_JOB_SNAPSHOT_BYTES (OS_ATA_OVERLAY_SECTORS * 512U)

typedef int (*ata_job_snapshot_fn)(uint8_t* buf, uint32_t capacity);
typedef int (*ata_job_restore_fn)(const uint8_t* buf, uint32_t size);

void ata_job_init(ata_job_snapshot_fn snapshot, ata_job_restore_fn restore);

/* Ownership. live_driver = current ata-driver owner PID (0 if none). */
int ata_owner_claim(int32_t pid, int32_t live_driver);
int ata_owner_release(int32_t pid);
int32_t ata_owner_holder(int32_t live_driver);
int ata_owner_ports_open(int32_t pid, int32_t live_driver);
/* 1 if kernel PIO may drive the controller now; 0 (and counted) if refused. */
int ata_owner_kernel_may_pio(int32_t live_driver);

/* Job queue. */
void ata_job_request_flush(void);
void ata_job_request_load(void);
int ata_job_pending(void);
/* Returns 1 and fills job (+ data for a write, capacity >= count*512), 0 if
 * idle, <0 on error. Fetching again before done returns the same chunk. */
int ata_job_fetch(os_ata_job_t* job, uint8_t* data, uint32_t capacity);
/* status 0 = chunk done. Returns OS_ATA_JOB_* result or OS_ATA_JOB_STALE. */
int ata_job_done(const os_ata_job_t* job, int32_t status, const uint8_t* data,
                 uint32_t capacity);
/* Driver disappeared: drop in-flight work. Returns 1 if a flush was queued or
 * in flight (the caller must then persist through the kernel PIO path). */
int ata_job_driver_gone(void);

void ata_job_note_kernel_overlay_write(void);
void ata_job_note_fallback_flush(void);
void ata_job_fill_status(os_ata_status_t* out, int32_t live_driver);

/* Client write fences (FAT areas) computed by the kernel at boot. */
void ata_job_set_fences(uint32_t client_min_lba, uint32_t slave_locked);
/* 1 if a client (not the kernel job) may write this drive/LBA. */
int ata_job_client_write_allowed(uint32_t drive, uint32_t lba);

#endif
