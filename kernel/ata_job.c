#include "ata_job.h"

static ata_job_snapshot_fn g_snapshot;
static ata_job_restore_fn g_restore;
static int32_t g_claim_pid;
static uint8_t g_buf[ATA_JOB_SNAPSHOT_BYTES];
static uint32_t g_flush_dirty;
static uint32_t g_load_pending;
static uint32_t g_active_op;
static uint32_t g_cursor;
static uint32_t g_generation;
static uint32_t g_ov_generation;
static uint32_t g_load_invalid;
static uint32_t g_client_min_lba = OS_ATA_KERNEL_RESERVED_LBAS;
static uint32_t g_slave_locked;
static os_ata_status_t g_stats;
/* Slice 3: one synchronous FAT sector job at a time (the caller is blocked
 * in the kernel until it completes). g_handed says which job the driver is
 * currently holding so the IO job can slot in between overlay chunks. */
#define ATA_HANDED_NONE 0U
#define ATA_HANDED_OVERLAY 1U
#define ATA_HANDED_IO 2U
static uint32_t g_handed;
static uint32_t g_io_state;
static uint32_t g_io_op, g_io_drive, g_io_lba, g_io_count, g_io_gen;
static uint8_t g_io_buf[OS_ATA_JOB_MAX_SECTORS * 512U];
static uint32_t g_io_flags;
static uint32_t g_debug_crash_armed;

void ata_job_init(ata_job_snapshot_fn snapshot, ata_job_restore_fn restore) {
    uint32_t i;
    uint8_t* s = (uint8_t*)&g_stats;
    g_snapshot = snapshot;
    g_restore = restore;
    g_claim_pid = 0;
    g_flush_dirty = 0U;
    g_load_pending = 0U;
    g_active_op = OS_ATA_JOB_NONE;
    g_cursor = 0U;
    g_generation = 0U;
    g_load_invalid = 0U;
    g_client_min_lba = OS_ATA_KERNEL_RESERVED_LBAS;
    g_slave_locked = 0U;
    g_handed = ATA_HANDED_NONE;
    g_io_state = ATA_IO_NONE;
    g_io_flags = 0U;
    g_debug_crash_armed = 0U;
    for (i = 0U; i < sizeof(g_stats); i++) s[i] = 0U;
}

int32_t ata_owner_holder(int32_t live_driver) {
    if (g_claim_pid != 0 && (live_driver <= 0 || g_claim_pid != live_driver)) g_claim_pid = 0;
    return g_claim_pid;
}

int ata_owner_claim(int32_t pid, int32_t live_driver) {
    if (pid <= 0 || live_driver <= 0 || pid != live_driver) return OS_ATA_DRIVER_REQUIRED;
    if (ata_owner_holder(live_driver) != 0 && g_claim_pid != pid) return OS_ATA_CONTROLLER_BUSY;
    g_claim_pid = pid;
    return 0;
}

int ata_owner_release(int32_t pid) {
    if (pid <= 0 || g_claim_pid != pid) return OS_ATA_DRIVER_REQUIRED;
    g_claim_pid = 0;
    return 0;
}

int ata_owner_ports_open(int32_t pid, int32_t live_driver) {
    return pid > 0 && pid == live_driver && ata_owner_holder(live_driver) == pid;
}

int ata_owner_kernel_may_pio(int32_t live_driver) {
    if (ata_owner_holder(live_driver) != 0) {
        g_stats.kernel_pio_refused++;
        return 0;
    }
    return 1;
}

void ata_job_request_flush(void) {
    g_flush_dirty = 1U;
    /* RAM is now newer than any snapshot a pending/in-flight load returns. */
    if (g_active_op == OS_ATA_JOB_READ) g_load_invalid = 1U;
    g_load_pending = 0U;
}

void ata_job_request_load(void) {
    if (g_flush_dirty) return; /* a newer RAM state must be written first */
    g_load_pending = 1U;
}

int ata_job_pending(void) {
    return g_flush_dirty || g_load_pending || g_active_op != OS_ATA_JOB_NONE ||
           g_io_state == ATA_IO_PENDING || g_io_state == ATA_IO_HANDED;
}

int ata_job_io_submit(uint32_t drive, uint32_t lba, uint32_t count, int write,
                      const uint8_t* data) {
    uint32_t i;
    if (g_io_state == ATA_IO_PENDING || g_io_state == ATA_IO_HANDED) return -1;
    if (drive > 1U || count == 0U || count > OS_ATA_JOB_MAX_SECTORS) return -1;
    if (write && !data) return -1;
    g_io_op = write ? OS_ATA_JOB_IO_WRITE : OS_ATA_JOB_IO_READ;
    g_io_drive = drive;
    g_io_lba = lba;
    g_io_count = count;
    g_io_gen = ++g_generation;
    if (write) for (i = 0U; i < count * 512U; i++) g_io_buf[i] = data[i];
    g_io_flags = 0U;
    if (write && g_debug_crash_armed) {
        g_io_flags = OS_ATA_JOB_FLAG_DEBUG_CRASH;
        g_debug_crash_armed = 0U; /* one shot */
    }
    g_io_state = ATA_IO_PENDING;
    return 0;
}

uint32_t ata_job_io_state(void) { return g_io_state; }

int ata_job_flush_queued(void) { return g_flush_dirty ? 1 : 0; }

int ata_job_io_take(uint8_t* out, uint32_t capacity) {
    uint32_t i, n;
    int rc;
    if (g_io_state != ATA_IO_DONE && g_io_state != ATA_IO_FAILED) return -1;
    rc = g_io_state == ATA_IO_DONE ? 0 : -1;
    if (rc == 0 && g_io_op == OS_ATA_JOB_IO_READ) {
        n = g_io_count * 512U;
        if (!out || capacity < n) rc = -1;
        else for (i = 0U; i < n; i++) out[i] = g_io_buf[i];
    }
    g_io_state = ATA_IO_NONE;
    return rc;
}

void ata_job_io_cancel(void) {
    if (g_io_state == ATA_IO_NONE) return;
    if (g_handed == ATA_HANDED_IO) g_handed = ATA_HANDED_NONE;
    g_io_state = ATA_IO_NONE;
    g_generation++; /* a late SYS_ATA_JOB_DONE for it is now stale */
    g_stats.fat_rpc_aborts++;
}

void ata_job_note_fat_kernel_pio(uint32_t sectors, int driver_live) {
    g_stats.fat_kernel_pio_sectors += sectors;
    if (driver_live) g_stats.fat_kernel_pio_live += sectors;
}

void ata_job_set_boot_driver(int32_t pid) { g_stats.boot_driver_pid = pid; }

void ata_job_debug_arm_crash(void) { g_debug_crash_armed = 1U; }

int ata_job_controller_in_use(void) {
    return g_claim_pid != 0 || g_handed != ATA_HANDED_NONE;
}

void ata_job_note_channel_reset(void) { g_stats.channel_resets++; }

static uint32_t chunk_count(void) {
    uint32_t left = OS_ATA_OVERLAY_SECTORS - g_cursor;
    return left < OS_ATA_JOB_MAX_SECTORS ? left : OS_ATA_JOB_MAX_SECTORS;
}

int ata_job_fetch(os_ata_job_t* job, uint8_t* data, uint32_t capacity) {
    uint32_t i, count, base;
    if (!job) return -1;
    /* Slice 3: a pending FAT sector job goes first, between overlay chunks. */
    if (g_handed == ATA_HANDED_IO ||
        (g_handed == ATA_HANDED_NONE && g_io_state == ATA_IO_PENDING)) {
        if (g_io_op == OS_ATA_JOB_IO_WRITE) {
            if (!data || capacity < g_io_count * 512U) return -1;
            for (i = 0U; i < g_io_count * 512U; i++) data[i] = g_io_buf[i];
        }
        job->op = g_io_op;
        job->drive = g_io_drive;
        job->lba = g_io_lba;
        job->count = g_io_count;
        job->generation = g_io_gen;
        job->flags = g_io_flags;
        g_io_state = ATA_IO_HANDED;
        g_handed = ATA_HANDED_IO;
        return 1;
    }
    if (g_active_op == OS_ATA_JOB_NONE) {
        if (g_flush_dirty) {
            for (i = 0U; i < sizeof(g_buf); i++) g_buf[i] = 0U;
            if (!g_snapshot || g_snapshot(g_buf, sizeof(g_buf)) != 0) return -1;
            g_flush_dirty = 0U;
            g_active_op = OS_ATA_JOB_WRITE;
        } else if (g_load_pending) {
            g_load_pending = 0U;
            g_load_invalid = 0U;
            g_active_op = OS_ATA_JOB_READ;
        } else {
            return 0;
        }
        g_cursor = 0U;
        g_ov_generation = ++g_generation;
    }
    count = chunk_count();
    if (g_active_op == OS_ATA_JOB_WRITE) {
        if (!data || capacity < count * 512U) return -1;
        base = g_cursor * 512U;
        for (i = 0U; i < count * 512U; i++) data[i] = g_buf[base + i];
    }
    job->op = g_active_op;
    job->drive = 0U;
    job->lba = g_cursor; /* the overlay snapshot starts at LBA 0 */
    job->count = count;
    job->generation = g_ov_generation;
    job->flags = 0U;
    g_handed = ATA_HANDED_OVERLAY;
    return 1;
}

int ata_job_done(const os_ata_job_t* job, int32_t status, const uint8_t* data,
                 uint32_t capacity) {
    uint32_t i, count, base, op;
    if (!job) return OS_ATA_JOB_STALE;
    if (g_handed == ATA_HANDED_IO) {
        if (g_io_state != ATA_IO_HANDED || job->op != g_io_op || job->generation != g_io_gen ||
            job->lba != g_io_lba || job->count != g_io_count || job->drive != g_io_drive)
            return OS_ATA_JOB_STALE;
        if (status == 0 && g_io_op == OS_ATA_JOB_IO_READ) {
            if (!data || capacity < g_io_count * 512U) return OS_ATA_JOB_STALE;
            for (i = 0U; i < g_io_count * 512U; i++) g_io_buf[i] = data[i];
        }
        g_handed = ATA_HANDED_NONE;
        if (status != 0) {
            g_stats.job_failures++;
            g_io_state = ATA_IO_FAILED;
            return OS_ATA_JOB_FAILED;
        }
        if (g_io_op == OS_ATA_JOB_IO_READ) g_stats.fat_driver_read_sectors += g_io_count;
        else g_stats.fat_driver_write_sectors += g_io_count;
        g_io_state = ATA_IO_DONE;
        return OS_ATA_JOB_IO_DONE;
    }
    if (g_handed != ATA_HANDED_OVERLAY || g_active_op == OS_ATA_JOB_NONE) return OS_ATA_JOB_STALE;
    count = chunk_count();
    if (job->op != g_active_op || job->generation != g_ov_generation ||
        job->lba != g_cursor || job->count != count || job->drive != 0U)
        return OS_ATA_JOB_STALE;
    op = g_active_op;
    g_handed = ATA_HANDED_NONE;
    if (status != 0) {
        g_stats.job_failures++;
        g_active_op = OS_ATA_JOB_NONE;
        if (op == OS_ATA_JOB_WRITE) g_flush_dirty = 1U; /* retry whole snapshot */
        return OS_ATA_JOB_FAILED;
    }
    if (op == OS_ATA_JOB_READ) {
        if (!data || capacity < count * 512U) return OS_ATA_JOB_STALE;
        base = g_cursor * 512U;
        for (i = 0U; i < count * 512U; i++) g_buf[base + i] = data[i];
    }
    g_cursor += count;
    if (g_cursor < OS_ATA_OVERLAY_SECTORS) return OS_ATA_JOB_CHUNK_OK;
    g_active_op = OS_ATA_JOB_NONE;
    g_cursor = 0U;
    if (op == OS_ATA_JOB_WRITE) {
        g_stats.flush_done++;
        return OS_ATA_JOB_FLUSH_DONE;
    }
    if (g_load_invalid || g_flush_dirty || !g_restore || g_restore(g_buf, sizeof(g_buf)) != 0) {
        g_stats.load_skipped++;
        return OS_ATA_JOB_LOAD_SKIPPED;
    }
    g_stats.load_done++;
    return OS_ATA_JOB_LOAD_DONE;
}

int ata_job_driver_gone(void) {
    int need_flush = g_flush_dirty || g_active_op == OS_ATA_JOB_WRITE;
    g_claim_pid = 0;
    g_active_op = OS_ATA_JOB_NONE;
    g_cursor = 0U;
    g_load_pending = 0U;
    g_flush_dirty = 0U;
    g_handed = ATA_HANDED_NONE;
    if (g_io_state == ATA_IO_PENDING || g_io_state == ATA_IO_HANDED) {
        g_io_state = ATA_IO_NONE;
        g_generation++;
        g_stats.fat_rpc_aborts++;
    }
    return need_flush;
}

void ata_job_note_kernel_overlay_write(void) { g_stats.kernel_overlay_writes++; }
void ata_job_note_fallback_flush(void) { g_stats.fallback_flushes++; }

void ata_job_set_fences(uint32_t client_min_lba, uint32_t slave_locked) {
    g_client_min_lba = client_min_lba < OS_ATA_KERNEL_RESERVED_LBAS ?
                       OS_ATA_KERNEL_RESERVED_LBAS : client_min_lba;
    g_slave_locked = slave_locked ? 1U : 0U;
}

int ata_job_client_write_allowed(uint32_t drive, uint32_t lba) {
    if (drive == 0U) return lba >= g_client_min_lba;
    if (drive == 1U) return !g_slave_locked;
    return 0;
}

void ata_job_fill_status(os_ata_status_t* out, int32_t live_driver) {
    if (!out) return;
    *out = g_stats;
    out->driver_pid = live_driver > 0 ? live_driver : 0;
    out->claim_pid = ata_owner_holder(live_driver);
    out->pending = (uint32_t)ata_job_pending();
    out->client_min_lba = g_client_min_lba;
    out->slave_write_locked = g_slave_locked;
    out->debug_crash_armed = g_debug_crash_armed;
}
