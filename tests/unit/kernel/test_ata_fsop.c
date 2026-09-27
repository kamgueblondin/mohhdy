#include "../../framework/unity.h"
#include "../../../kernel/ata_fsop.h"
#include "../../../kernel/fs/fsop_exec.h"
#include "../../../fs/overlay.h"
#include <string.h>

/* Tranche 4 suite: FAT16/FAT32/overlay operations served by the Ring 3
 * atadriver. Kernel slot/store/mirror (kernel/ata_fsop.c) and the shared
 * codec + dispatch (kernel/fs/fsop_exec.c) the driver executes. The FAT16
 * volume is a synthetic in-memory image (unit fixture); the QEMU contract
 * make qemu-ata-driver proves the real driver path. */

#define TEST_SECTORS 32768U
static uint8_t disk[TEST_SECTORS * 512U];
static uint8_t buf[OS_ATA_FSOP_BUFFER_SIZE];
static uint8_t out[OS_ATA_FSOP_BUFFER_SIZE];
static uint32_t writes;

static void put16(uint32_t off, uint16_t v) { disk[off] = (uint8_t)v; disk[off + 1U] = (uint8_t)(v >> 8); }
static int rd(uint32_t lba, void* o) {
    uint32_t i;
    if (lba >= TEST_SECTORS) return -1;
    for (i = 0U; i < 512U; i++) ((uint8_t*)o)[i] = disk[lba * 512U + i];
    return 0;
}
static int wr(uint32_t lba, const void* in) {
    uint32_t i;
    if (lba >= TEST_SECTORS) return -1;
    writes++;
    for (i = 0U; i < 512U; i++) disk[lba * 512U + i] = ((const uint8_t*)in)[i];
    return 0;
}
static void make_volume(fat16_volume_t* v) {
    uint32_t fat, i;
    for (i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    put16(11U, 512U); disk[13] = 1U; put16(14U, 1U); disk[16] = 2U;
    put16(17U, 32U); put16(19U, TEST_SECTORS); disk[21] = 0xF8U; put16(22U, 17U);
    put16(510U, 0xAA55U);
    for (fat = 1U; fat <= 2U; fat++) {
        uint32_t base = (1U + (fat - 1U) * 17U) * 512U;
        put16(base, 0xFFF8U); put16(base + 2U, 0xFFFFU);
    }
    TEST_ASSERT_EQUAL(0, fat16_mount(v, rd, 0U));
    TEST_ASSERT_EQUAL(0, fat16_attach_writer(v, wr));
}

/* Plays the driver side of one op: fetch, copy, decode, execute, done. */
static int32_t drive_op(int32_t pid, fat16_volume_t* v16, uint32_t* out_len) {
    static uint8_t req_buf[OS_ATA_FSOP_BUFFER_SIZE];
    static uint8_t reply[OS_ATA_FSOP_BUFFER_SIZE];
    os_ata_job_t job;
    os_ata_fsop_request_t req;
    os_ata_fsop_reply_t* r = (os_ata_fsop_reply_t*)reply;
    const char* p1; const char* p2; const uint8_t* in;
    uint32_t n = 0U;
    int len;
    if (ata_fsop_fetch(pid, &job) != 1) return -1000;
    if (job.op != OS_ATA_JOB_FS_OP || job.count != 0U) return -1001;
    len = ata_fsop_copy_request(pid, job.generation, req_buf, sizeof(req_buf));
    if (len <= 0) return -1002;
    if (fsop_decode(req_buf, (uint32_t)len, &req, &p1, &p2, &in) != 0) return -1003;
    r->result = fsop_execute(&req, p1, p2, in, reply + sizeof(*r), sizeof(reply) - sizeof(*r), &n, v16, 0);
    r->out_len = n; r->sectors_read = 1U; r->sectors_written = 0U;
    if (ata_fsop_done(pid, job.generation, reply, (uint32_t)sizeof(*r) + n) != OS_ATA_JOB_FS_DONE) return -1004;
    if (out_len) *out_len = n;
    return r->result;
}

static int submit(uint32_t op, uint32_t a0, uint32_t a1, const char* p, const char* p2,
                  const void* in, uint32_t in_len, uint32_t out_cap, int32_t pid) {
    int len = fsop_encode(ata_fsop_request_buffer(), ata_fsop_request_capacity(), op, a0, a1, p, p2,
                          in, in_len, out_cap);
    if (len < 0) return -1;
    return ata_fsop_submit((uint32_t)len, op, pid, 0U);
}

static void test_store_ownership(void) {
    os_ata_status_t st;
    ata_fsop_init();
    TEST_ASSERT_EQUAL(0, (int)ata_fsop_store_flags(7));
    TEST_ASSERT_EQUAL(-1, ata_fsop_store_ready(0, OS_ATA_FS_STORE_ALL));
    TEST_ASSERT_EQUAL(-1, ata_fsop_store_ready(7, 0x80U));
    TEST_ASSERT_EQUAL(0, ata_fsop_store_ready(7, OS_ATA_FS_STORE_FAT16 | OS_ATA_FS_STORE_OVERLAY));
    TEST_ASSERT_EQUAL(5, (int)ata_fsop_store_flags(7));
    /* Another (or no) live driver never inherits the store. */
    TEST_ASSERT_EQUAL(0, (int)ata_fsop_store_flags(8));
    TEST_ASSERT_EQUAL(0, (int)ata_fsop_store_flags(0));
    ata_fsop_fill_status(&st, 7);
    TEST_ASSERT_EQUAL(5, (int)st.fs_store_flags);
    ata_fsop_store_drop();
    TEST_ASSERT_EQUAL(0, (int)ata_fsop_store_flags(7));
    TEST_ASSERT_EQUAL(0, ata_fsop_store_pid());
}

static void test_codec_roundtrip_and_bounds(void) {
    os_ata_fsop_request_t req;
    const char* p1; const char* p2; const uint8_t* in;
    char longpath[OS_ATA_FSOP_PATH_MAX + 4];
    int len, i;
    len = fsop_encode(buf, sizeof(buf), OS_ATA_FSOP_OVL_RENAME, 1U, 2U, "/a", "/b", "xyz", 3U, 64U);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_EQUAL(0, fsop_decode(buf, (uint32_t)len, &req, &p1, &p2, &in));
    TEST_ASSERT_EQUAL_STRING("/a", p1);
    TEST_ASSERT_EQUAL_STRING("/b", p2);
    TEST_ASSERT_EQUAL(3, (int)req.in_len);
    TEST_ASSERT_EQUAL('x', in[0]);
    TEST_ASSERT_EQUAL(64, (int)req.out_cap);
    /* Truncated, bad magic, unknown op, unterminated path. */
    TEST_ASSERT_EQUAL(-1, fsop_decode(buf, (uint32_t)len - 1U, &req, &p1, &p2, &in));
    buf[0] ^= 0xFFU;
    TEST_ASSERT_EQUAL(-1, fsop_decode(buf, (uint32_t)len, &req, &p1, &p2, &in));
    buf[0] ^= 0xFFU;
    TEST_ASSERT_EQUAL(-1, fsop_encode(buf, sizeof(buf), 99U, 0U, 0U, "/a", 0, 0, 0U, 0U));
    buf[sizeof(os_ata_fsop_request_t) + 2U] = 'q'; /* overwrite path NUL */
    TEST_ASSERT_EQUAL(-1, fsop_decode(buf, (uint32_t)len, &req, &p1, &p2, &in));
    for (i = 0; i < (int)sizeof(longpath) - 1; i++) longpath[i] = 'a';
    longpath[sizeof(longpath) - 1] = '\0';
    TEST_ASSERT_EQUAL(-1, fsop_encode(buf, sizeof(buf), OS_ATA_FSOP_OVL_READ, 1U, 0U, longpath, 0, 0, 0U, 1U));
    TEST_ASSERT_EQUAL(-1, fsop_encode(buf, sizeof(buf), OS_ATA_FSOP_OVL_WRITE, 0U, 0U, "/a", 0, out,
                                      OS_ATA_FSOP_BUFFER_SIZE, 0U));
    TEST_ASSERT_EQUAL(-1, fsop_encode(buf, sizeof(buf), OS_ATA_FSOP_OVL_READ, 0U, 0U, "/a", 0, 0, 0U,
                                      OS_ATA_FSOP_BUFFER_SIZE));
    TEST_ASSERT_EQUAL(-1, fsop_encode(buf, 16U, OS_ATA_FSOP_OVL_READ, 0U, 0U, "/a", 0, 0, 0U, 1U));
}

static void test_op_classes(void) {
    TEST_ASSERT_EQUAL(OS_ATA_FS_STORE_FAT16, (int)fsop_store_for(OS_ATA_FSOP_FAT16_READ));
    TEST_ASSERT_EQUAL(OS_ATA_FS_STORE_FAT32, (int)fsop_store_for(OS_ATA_FSOP_FAT16_RENAME + OS_ATA_FSOP_FAT32_BASE));
    TEST_ASSERT_EQUAL(OS_ATA_FS_STORE_OVERLAY, (int)fsop_store_for(OS_ATA_FSOP_OVL_IS_DIR));
    TEST_ASSERT_EQUAL(0, (int)fsop_store_for(0U));
    TEST_ASSERT_EQUAL(0, (int)fsop_store_for(28U));
    TEST_ASSERT_TRUE(fsop_is_fat_mutation(OS_ATA_FSOP_FAT16_CREATE + OS_ATA_FSOP_FAT32_BASE));
    TEST_ASSERT_FALSE(fsop_is_fat_mutation(OS_ATA_FSOP_FAT16_LIST_PATH));
    TEST_ASSERT_FALSE(fsop_is_fat_mutation(OS_ATA_FSOP_OVL_WRITE));
    TEST_ASSERT_TRUE(fsop_is_mutation(OS_ATA_FSOP_OVL_COPY));
    TEST_ASSERT_FALSE(fsop_is_mutation(OS_ATA_FSOP_OVL_STAT));
}

static void test_slot_lifecycle_and_stale(void) {
    os_ata_job_t job;
    os_ata_fsop_reply_t reply;
    uint8_t rbuf[sizeof(os_ata_fsop_reply_t) + 8U];
    os_ata_fsop_reply_t* r = (os_ata_fsop_reply_t*)rbuf;
    int gen;
    ata_fsop_init();
    TEST_ASSERT_EQUAL(0, ata_fsop_fetch(7, &job));
    gen = submit(OS_ATA_FSOP_OVL_READ, 8U, 0U, "/x", 0, 0, 0U, 8U, 7);
    TEST_ASSERT_TRUE(gen > 0);
    /* One op in flight. */
    TEST_ASSERT_EQUAL(-1, submit(OS_ATA_FSOP_OVL_READ, 8U, 0U, "/x", 0, 0, 0U, 8U, 7));
    /* Only the target driver fetches it; request copy only once fetched. */
    TEST_ASSERT_EQUAL(0, ata_fsop_fetch(8, &job));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_copy_request(7, (uint32_t)gen, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(1, ata_fsop_fetch(7, &job));
    TEST_ASSERT_EQUAL(gen, (int)job.generation);
    TEST_ASSERT_EQUAL(0, ata_fsop_fetch(7, &job)); /* not handed twice */
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_copy_request(7, (uint32_t)gen + 1U, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_copy_request(7, (uint32_t)gen, buf, 8U));
    TEST_ASSERT_TRUE(ata_fsop_copy_request(7, (uint32_t)gen, buf, sizeof(buf)) > 0);
    /* Reply larger than the caller asked for is refused. */
    r->result = 9; r->out_len = 9U; r->sectors_read = 0U; r->sectors_written = 0U;
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_done(7, (uint32_t)gen, rbuf, sizeof(rbuf)));
    r->out_len = 8U;
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_done(8, (uint32_t)gen, rbuf, sizeof(rbuf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_done(7, (uint32_t)gen, rbuf, 8U));
    rbuf[sizeof(*r)] = 'k';
    TEST_ASSERT_EQUAL(OS_ATA_JOB_FS_DONE, ata_fsop_done(7, (uint32_t)gen, rbuf, sizeof(rbuf)));
    TEST_ASSERT_EQUAL(ATA_FSOP_DONE, ata_fsop_state());
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_done(7, (uint32_t)gen, rbuf, sizeof(rbuf)));
    TEST_ASSERT_EQUAL(0, ata_fsop_take(out, sizeof(out), &reply));
    TEST_ASSERT_EQUAL(9, reply.result);
    TEST_ASSERT_EQUAL('k', out[0]);
    TEST_ASSERT_EQUAL(ATA_FSOP_FREE, ata_fsop_state());
}

static void test_abort_commit_publish_mirror(void) {
    os_ata_job_t job;
    os_ata_status_t st;
    uint8_t image[64];
    uint32_t size = 0U;
    int32_t res = 0;
    int gen, i;
    ata_fsop_init();
    for (i = 0; i < 64; i++) image[i] = (uint8_t)i;
    gen = submit(OS_ATA_FSOP_OVL_WRITE, 0U, 0U, "/w", 0, "ab", 2U, 0U, 7);
    TEST_ASSERT_EQUAL(1, ata_fsop_fetch(7, &job));
    /* Mirror only accepts an AIOV image, from the live op. */
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_publish(7, (uint32_t)gen, image, sizeof(image), 2));
    image[0] = 0x41; image[1] = 0x49; image[2] = 0x4F; image[3] = 0x56;
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_publish(7, (uint32_t)gen + 1U, image, sizeof(image), 2));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_publish(7, (uint32_t)gen, image, ATA_FSOP_MIRROR_BYTES + 1U, 2));
    TEST_ASSERT_EQUAL(0, ata_fsop_publish(7, (uint32_t)gen, image, sizeof(image), 2));
    TEST_ASSERT_EQUAL(0, ata_fsop_note(7, (uint32_t)gen, OS_ATA_FS_NOTE_SECTOR_WRITTEN));
    TEST_ASSERT_EQUAL(0, ata_fsop_note(7, (uint32_t)gen, OS_ATA_FS_NOTE_PERSISTED));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_note(7, (uint32_t)gen, 77U));
    TEST_ASSERT_EQUAL(1, (int)ata_fsop_persisted());
    /* Driver dies: late DONE is stale, facts survive for the caller. */
    ata_fsop_abort();
    TEST_ASSERT_EQUAL(ATA_FSOP_ABORTED, ata_fsop_state());
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_fsop_note(7, (uint32_t)gen, OS_ATA_FS_NOTE_SECTOR_WRITTEN));
    TEST_ASSERT_TRUE(ata_fsop_committed());
    TEST_ASSERT_EQUAL(1, ata_fsop_published(&res));
    TEST_ASSERT_EQUAL(2, res);
    TEST_ASSERT_TRUE(ata_fsop_mirror(&size) != 0);
    TEST_ASSERT_EQUAL(64, (int)size);
    TEST_ASSERT_EQUAL(5, ata_fsop_mirror(&size)[5]);
    ata_fsop_release();
    TEST_ASSERT_EQUAL(ATA_FSOP_FREE, ata_fsop_state());
    /* Next op starts clean. */
    gen = submit(OS_ATA_FSOP_OVL_MKDIR, 0U, 0U, "/d", 0, 0, 0U, 0U, 7);
    TEST_ASSERT_TRUE(gen > 0);
    TEST_ASSERT_FALSE(ata_fsop_committed());
    TEST_ASSERT_EQUAL(0, ata_fsop_published(&res));
    ata_fsop_abort();
    ata_fsop_release();
    ata_fsop_note_redone();
    ata_fsop_note_restore();
    ata_fsop_note_handover();
    ata_fsop_note_unavailable();
    ata_fsop_note_kernel_live();
    ata_fsop_fill_status(&st, 0);
    TEST_ASSERT_EQUAL(2, (int)st.fs_aborts);
    TEST_ASSERT_EQUAL(1, (int)st.fs_redone);
    TEST_ASSERT_EQUAL(1, (int)st.fs_publishes);
    TEST_ASSERT_EQUAL(1, (int)st.fs_restores);
    TEST_ASSERT_EQUAL(1, (int)st.fs_handovers);
    TEST_ASSERT_EQUAL(1, (int)st.fs_unavailable);
    TEST_ASSERT_EQUAL(1, (int)st.fs_kernel_live);
    TEST_ASSERT_EQUAL(0, (int)st.fs_store_flags);
    /* Handover buffer. */
    TEST_ASSERT_EQUAL(-1, ata_fsop_mirror_commit(3U));
    TEST_ASSERT_TRUE(ata_fsop_mirror(&size) == 0);
    TEST_ASSERT_EQUAL(0, ata_fsop_mirror_commit(64U));
}

static void test_overlay_ops_through_codec(void) {
    uint32_t n = 0U;
    os_dirent_t* ents = (os_dirent_t*)out;
    ata_fsop_init();
    overlay_init();
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_MKDIR, 0U, 0U, "/fsd", 0, 0, 0U, 0U, 3) > 0);
    TEST_ASSERT_EQUAL(OV_OK, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_WRITE, 0U, 0U, "/fsd/a.txt", 0, "hello", 5U, 0U, 3) > 0);
    TEST_ASSERT_EQUAL(5, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_APPEND, 0U, 0U, "/fsd/a.txt", 0, "!!", 2U, 0U, 3) > 0);
    TEST_ASSERT_EQUAL(2, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_READ, 64U, 0U, "/fsd/a.txt", 0, 0, 0U, 64U, 3) > 0);
    TEST_ASSERT_EQUAL(7, drive_op(3, 0, &n));
    TEST_ASSERT_EQUAL(7, (int)n);
    TEST_ASSERT_EQUAL(0, ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0}));
    TEST_ASSERT_EQUAL(0, memcmp(out, "hello!!", 7));
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_RENAME, 0U, 0U, "/fsd/a.txt", "/fsd/b.txt", 0, 0U, 0U, 3) > 0);
    TEST_ASSERT_EQUAL(OV_OK, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_STAT, 0U, 0U, "/fsd/b.txt", 0, 0, 0U,
                            (uint32_t)sizeof(os_dirent_t), 3) > 0);
    TEST_ASSERT_EQUAL(OV_OK, drive_op(3, 0, &n));
    TEST_ASSERT_EQUAL((int)sizeof(os_dirent_t), (int)n);
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_EQUAL(7, (int)ents[0].size);
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_IS_DIR, 0U, 0U, "/fsd", 0, 0, 0U, 0U, 3) > 0);
    TEST_ASSERT_EQUAL(1, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    /* listdir keeps the caller's first entries (initrd) and appends. */
    memset(out, 0, sizeof(out));
    strcpy(ents[0].name, "fromrd");
    ents[0].flags = OS_DIRENT_FILE;
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_LISTDIR, 1U, 4U, "/fsd", 0, ents,
                            (uint32_t)sizeof(os_dirent_t), 4U * (uint32_t)sizeof(os_dirent_t), 3) > 0);
    TEST_ASSERT_EQUAL(2, drive_op(3, 0, &n));
    TEST_ASSERT_EQUAL(0, ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0}));
    TEST_ASSERT_EQUAL_STRING("fromrd", ents[0].name);
    TEST_ASSERT_EQUAL_STRING("b.txt", ents[1].name);
    /* listdir with a lying input length is refused by the executor. */
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_LISTDIR, 2U, 4U, "/fsd", 0, ents,
                            (uint32_t)sizeof(os_dirent_t), 4U * (uint32_t)sizeof(os_dirent_t), 3) > 0);
    TEST_ASSERT_EQUAL(-1, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_LISTDIR_PAGE, 0U, 5U, "/fsd", 0, 0, 0U,
                            5U * (uint32_t)sizeof(os_dirent_t), 3) > 0);
    TEST_ASSERT_EQUAL(1, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_COPY, 0U, 0U, "/fsd/b.txt", "/fsd/c.txt", 0, 0U, 0U, 3) > 0);
    TEST_ASSERT_EQUAL(OV_OK, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_OVL_UNLINK, 0U, 0U, "/fsd/c.txt", 0, 0, 0U, 0U, 3) > 0);
    TEST_ASSERT_EQUAL(OV_OK, drive_op(3, 0, &n));
    ata_fsop_take(out, sizeof(out), &(os_ata_fsop_reply_t){0});
    TEST_ASSERT_EQUAL(OV_ERR_NOTFOUND, overlay_read("/fsd/c.txt", (char*)out, 8U));
}

static void test_fat16_ops_through_codec(void) {
    static fat16_volume_t v;
    uint32_t n = 0U;
    os_fat16_dirent_t* ents = (os_fat16_dirent_t*)out;
    os_ata_fsop_reply_t reply;
    make_volume(&v);
    ata_fsop_init();
    writes = 0U;
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_CREATE, 0U, 0U, "NOTE.TXT", 0, "fat16!", 6U, 0U, 4) > 0);
    TEST_ASSERT_EQUAL(0, drive_op(4, &v, &n));
    TEST_ASSERT_EQUAL(0, ata_fsop_take(out, sizeof(out), &reply));
    TEST_ASSERT_TRUE(writes > 0U);
    /* LFN name goes through the shared alias rule. */
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_CREATE, 0U, 0U, "long name file.txt", 0, "lfn", 3U, 0U, 4) > 0);
    TEST_ASSERT_EQUAL(0, drive_op(4, &v, &n));
    ata_fsop_take(out, sizeof(out), &reply);
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_READ, 32U, 0U, "NOTE.TXT", 0, 0, 0U, 32U, 4) > 0);
    TEST_ASSERT_EQUAL(6, drive_op(4, &v, &n));
    TEST_ASSERT_EQUAL(0, ata_fsop_take(out, sizeof(out), &reply));
    TEST_ASSERT_EQUAL(0, memcmp(out, "fat16!", 6));
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_READ, 32U, 0U, "long name file.txt", 0, 0, 0U, 32U, 4) > 0);
    TEST_ASSERT_EQUAL(3, drive_op(4, &v, &n));
    ata_fsop_take(out, sizeof(out), &reply);
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_LIST, 8U, 0U, 0, 0, 0, 0U,
                            8U * (uint32_t)sizeof(os_fat16_dirent_t), 4) > 0);
    TEST_ASSERT_EQUAL(2, drive_op(4, &v, &n));
    TEST_ASSERT_EQUAL(2 * (int)sizeof(os_fat16_dirent_t), (int)n);
    ata_fsop_take(out, sizeof(out), &reply);
    TEST_ASSERT_EQUAL(6, (int)ents[0].size);
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_RENAME, 0U, 0U, "NOTE.TXT", "MEMO.TXT", 0, 0U, 0U, 4) > 0);
    TEST_ASSERT_EQUAL(0, drive_op(4, &v, &n));
    ata_fsop_take(out, sizeof(out), &reply);
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_UNLINK, 0U, 0U, "MEMO.TXT", 0, 0, 0U, 0U, 4) > 0);
    TEST_ASSERT_EQUAL(0, drive_op(4, &v, &n));
    ata_fsop_take(out, sizeof(out), &reply);
    TEST_ASSERT_TRUE(fat16_read_path(&v, "MEMO.TXT", (char*)out, 8U) < 0);
    /* mkdir encoding (arg0 = 1, no data) then a file inside it. */
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_CREATE, 1U, 0U, "DIR", 0, 0, 0U, 0U, 4) > 0);
    TEST_ASSERT_EQUAL(0, drive_op(4, &v, &n));
    ata_fsop_take(out, sizeof(out), &reply);
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_CREATE, 0U, 0U, "DIR/IN.TXT", 0, "sub", 3U, 0U, 4) > 0);
    TEST_ASSERT_EQUAL(0, drive_op(4, &v, &n));
    ata_fsop_take(out, sizeof(out), &reply);
    TEST_ASSERT_EQUAL(3, fat16_read_path(&v, "DIR/IN.TXT", (char*)out, 8U));
    /* No FAT32 volume in this image: not mounted, nothing touched. */
    TEST_ASSERT_TRUE(submit(OS_ATA_FSOP_FAT16_READ + OS_ATA_FSOP_FAT32_BASE, 8U, 0U, "X", 0, 0, 0U, 8U, 4) > 0);
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_MOUNTED, drive_op(4, &v, &n));
    ata_fsop_take(out, sizeof(out), &reply);
    /* Kernel copy drops its caches after a driver mutation. */
    fat16_invalidate_caches(&v);
    TEST_ASSERT_EQUAL(0, (int)v.read_window_valid);
}

int main(void) {
    unity_init();
    RUN_TEST(test_store_ownership);
    RUN_TEST(test_codec_roundtrip_and_bounds);
    RUN_TEST(test_op_classes);
    RUN_TEST(test_slot_lifecycle_and_stale);
    RUN_TEST(test_abort_commit_publish_mirror);
    RUN_TEST(test_overlay_ops_through_codec);
    RUN_TEST(test_fat16_ops_through_codec);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
