#include "ata_fsop.h"

/* Tranche 4 suite: see ata_fsop.h. */

static uint32_t g_store_flags;
static int32_t g_store_pid;

static int g_state;
static uint32_t g_generation;
static uint32_t g_op;
static uint32_t g_flags;
static int32_t g_driver_pid;
static uint32_t g_req_len;
static uint8_t g_req[OS_ATA_FSOP_BUFFER_SIZE];
static uint8_t g_out[OS_ATA_FSOP_BUFFER_SIZE];
static os_ata_fsop_reply_t g_reply;
static int g_committed;
static int g_published;
static int32_t g_published_result;

static uint8_t g_mirror[ATA_FSOP_MIRROR_BYTES];
static uint32_t g_mirror_size;

static uint32_t g_ops, g_kernel_live, g_aborts, g_redone, g_unavailable;
static uint32_t g_publishes, g_handovers, g_restores, g_persisted;
static uint32_t g_sectors_read, g_sectors_written;

#define ATA_FSOP_SNAP_MAGIC 0x564F4941u /* OV_SNAP_MAGIC */

static void fsop_copy(uint8_t* dst, const uint8_t* src, uint32_t n) {
    uint32_t i;
    for (i = 0U; i < n; i++) dst[i] = src[i];
}

void ata_fsop_init(void) {
    g_store_flags = 0U;
    g_store_pid = 0;
    g_state = ATA_FSOP_FREE;
    g_generation = 0U;
    g_op = 0U;
    g_flags = 0U;
    g_driver_pid = 0;
    g_req_len = 0U;
    g_committed = 0;
    g_published = 0;
    g_published_result = 0;
    g_mirror_size = 0U;
    g_ops = g_kernel_live = g_aborts = g_redone = g_unavailable = 0U;
    g_publishes = g_handovers = g_restores = g_persisted = 0U;
    g_sectors_read = g_sectors_written = 0U;
}

int ata_fsop_store_ready(int32_t driver_pid, uint32_t flags) {
    if (driver_pid <= 0 || (flags & ~OS_ATA_FS_STORE_ALL) != 0U) return -1;
    g_store_pid = driver_pid;
    g_store_flags = flags;
    return 0;
}

void ata_fsop_store_drop(void) {
    g_store_pid = 0;
    g_store_flags = 0U;
}

uint32_t ata_fsop_store_flags(int32_t live_driver_pid) {
    if (live_driver_pid <= 0 || live_driver_pid != g_store_pid) return 0U;
    return g_store_flags;
}

int32_t ata_fsop_store_pid(void) { return g_store_pid; }

uint8_t* ata_fsop_request_buffer(void) { return g_req; }
uint32_t ata_fsop_request_capacity(void) { return (uint32_t)sizeof(g_req); }

int ata_fsop_submit(uint32_t length, uint32_t op, int32_t driver_pid, uint32_t flags) {
    if (g_state != ATA_FSOP_FREE || driver_pid <= 0 || length < sizeof(os_ata_fsop_request_t) ||
        length > sizeof(g_req)) return -1;
    g_generation++;
    if (g_generation == 0U) g_generation = 1U;
    g_state = ATA_FSOP_SUBMITTED;
    g_op = op;
    g_flags = flags;
    g_driver_pid = driver_pid;
    g_req_len = length;
    g_committed = 0;
    g_published = 0;
    g_published_result = 0;
    g_reply.result = -1;
    g_reply.out_len = 0U;
    g_reply.sectors_read = 0U;
    g_reply.sectors_written = 0U;
    return (int)g_generation;
}

int ata_fsop_state(void) { return g_state; }
uint32_t ata_fsop_op(void) { return g_op; }

static int fsop_live(int32_t pid, uint32_t generation) {
    return pid > 0 && pid == g_driver_pid && generation == g_generation &&
           (g_state == ATA_FSOP_SUBMITTED || g_state == ATA_FSOP_FETCHED);
}

int ata_fsop_fetch(int32_t pid, os_ata_job_t* job) {
    if (!job || g_state != ATA_FSOP_SUBMITTED || pid != g_driver_pid) return 0;
    job->op = OS_ATA_JOB_FS_OP;
    job->drive = 0U;
    job->lba = g_op; /* informative: the request carries the real op */
    job->count = 0U;
    job->generation = g_generation;
    job->flags = g_flags;
    g_state = ATA_FSOP_FETCHED;
    return 1;
}

int ata_fsop_copy_request(int32_t pid, uint32_t generation, uint8_t* dst, uint32_t capacity) {
    if (!dst || g_state != ATA_FSOP_FETCHED || !fsop_live(pid, generation)) return OS_ATA_JOB_STALE;
    if (capacity < g_req_len) return OS_ATA_JOB_STALE;
    fsop_copy(dst, g_req, g_req_len);
    return (int)g_req_len;
}

int ata_fsop_note(int32_t pid, uint32_t generation, uint32_t kind) {
    if (kind == OS_ATA_FS_NOTE_PERSISTED || kind == OS_ATA_FS_NOTE_PERSIST_FAILED) {
        /* The driver persists its overlay store only inside an op. */
        if (!fsop_live(pid, generation)) return OS_ATA_JOB_STALE;
        if (kind == OS_ATA_FS_NOTE_PERSISTED) g_persisted++;
        return 0;
    }
    if (kind != OS_ATA_FS_NOTE_SECTOR_WRITTEN || !fsop_live(pid, generation)) return OS_ATA_JOB_STALE;
    g_committed = 1;
    return 0;
}

int ata_fsop_publish(int32_t pid, uint32_t generation, const uint8_t* image, uint32_t size,
                     int32_t result) {
    uint32_t magic;
    if (!image || !fsop_live(pid, generation) || size < 16U || size > sizeof(g_mirror))
        return OS_ATA_JOB_STALE;
    magic = (uint32_t)image[0] | ((uint32_t)image[1] << 8) | ((uint32_t)image[2] << 16) |
            ((uint32_t)image[3] << 24);
    if (magic != ATA_FSOP_SNAP_MAGIC) return OS_ATA_JOB_STALE;
    fsop_copy(g_mirror, image, size);
    g_mirror_size = size;
    g_published = 1;
    g_published_result = result;
    g_publishes++;
    return 0;
}

int ata_fsop_done(int32_t pid, uint32_t generation, const uint8_t* reply, uint32_t length) {
    os_ata_fsop_reply_t r;
    const os_ata_fsop_request_t* req = (const os_ata_fsop_request_t*)g_req;
    if (!reply || g_state != ATA_FSOP_FETCHED || !fsop_live(pid, generation) ||
        length < sizeof(r)) return OS_ATA_JOB_STALE;
    fsop_copy((uint8_t*)&r, reply, sizeof(r));
    if (r.out_len > req->out_cap || r.out_len > sizeof(g_out) ||
        r.out_len > length - (uint32_t)sizeof(r)) return OS_ATA_JOB_STALE;
    fsop_copy(g_out, reply + sizeof(r), r.out_len);
    g_reply = r;
    g_state = ATA_FSOP_DONE;
    g_ops++;
    g_sectors_read += r.sectors_read;
    g_sectors_written += r.sectors_written;
    return OS_ATA_JOB_FS_DONE;
}

int ata_fsop_take(uint8_t* out, uint32_t capacity, os_ata_fsop_reply_t* reply) {
    if (g_state != ATA_FSOP_DONE || !reply) return -1;
    if (g_reply.out_len > capacity || (g_reply.out_len != 0U && !out)) {
        g_state = ATA_FSOP_FREE;
        return -1;
    }
    fsop_copy(out, g_out, g_reply.out_len);
    *reply = g_reply;
    g_state = ATA_FSOP_FREE;
    return 0;
}

void ata_fsop_abort(void) {
    if (g_state != ATA_FSOP_SUBMITTED && g_state != ATA_FSOP_FETCHED) return;
    g_state = ATA_FSOP_ABORTED;
    g_aborts++;
}

int ata_fsop_committed(void) { return g_committed; }

int ata_fsop_published(int32_t* result) {
    if (!g_published) return 0;
    if (result) *result = g_published_result;
    return 1;
}

void ata_fsop_release(void) {
    if (g_state == ATA_FSOP_ABORTED || g_state == ATA_FSOP_DONE) g_state = ATA_FSOP_FREE;
}

int ata_fsop_mirror_set(const uint8_t* image, uint32_t size) {
    if (!image || size < 16U || size > sizeof(g_mirror)) return -1;
    fsop_copy(g_mirror, image, size);
    g_mirror_size = size;
    return 0;
}

uint8_t* ata_fsop_mirror_buffer(void) { return g_mirror; }

int ata_fsop_mirror_commit(uint32_t size) {
    if (size < 16U || size > sizeof(g_mirror)) {
        g_mirror_size = 0U;
        return -1;
    }
    g_mirror_size = size;
    return 0;
}

const uint8_t* ata_fsop_mirror(uint32_t* size) {
    if (size) *size = g_mirror_size;
    return g_mirror_size ? g_mirror : (const uint8_t*)0;
}

void ata_fsop_note_kernel_live(void) { g_kernel_live++; }
void ata_fsop_note_redone(void) { g_redone++; }
void ata_fsop_note_unavailable(void) { g_unavailable++; }
void ata_fsop_note_handover(void) { g_handovers++; }
void ata_fsop_note_restore(void) { g_restores++; }
uint32_t ata_fsop_persisted(void) { return g_persisted; }
uint32_t ata_fsop_sectors_read(void) { return g_sectors_read; }
uint32_t ata_fsop_sectors_written(void) { return g_sectors_written; }

void ata_fsop_fill_status(os_ata_status_t* out, int32_t live_driver_pid) {
    if (!out) return;
    out->fs_store_flags = ata_fsop_store_flags(live_driver_pid);
    out->fs_ops = g_ops;
    out->fs_kernel_live = g_kernel_live;
    out->fs_aborts = g_aborts;
    out->fs_redone = g_redone;
    out->fs_unavailable = g_unavailable;
    out->fs_publishes = g_publishes;
    out->fs_handovers = g_handovers;
    out->fs_restores = g_restores;
}
