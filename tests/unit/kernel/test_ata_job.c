#include "../../framework/unity.h"
#include "../../../kernel/ata_job.h"

/* Tranche 4 slice 2: controller ownership and overlay snapshot jobs between
 * the kernel and the Ring 3 atadriver (pure logic). */

static uint8_t fake_disk[ATA_JOB_SNAPSHOT_BYTES];
static uint8_t ram_state = 0x11U;
static int restore_calls;

static int fake_snapshot(uint8_t* buf, uint32_t capacity) {
    uint32_t i;
    for (i = 0U; i < capacity; i++) buf[i] = ram_state;
    return 0;
}

static int fake_restore(const uint8_t* buf, uint32_t size) {
    (void)size;
    restore_calls++;
    ram_state = buf[0];
    return 0;
}

/* Plays the driver: fetch, "PIO" against fake_disk, done. */
static int drive_job(int fail_at_chunk) {
    static uint8_t buf[OS_ATA_JOB_MAX_SECTORS * 512U];
    os_ata_job_t job;
    int chunk = 0, res = -1;
    uint32_t i;
    while (ata_job_fetch(&job, buf, sizeof(buf)) == 1) {
        if (job.count < 1U || job.count > OS_ATA_JOB_MAX_SECTORS ||
            job.lba + job.count > OS_ATA_OVERLAY_SECTORS) return -99;
        if (job.op == OS_ATA_JOB_WRITE) {
            for (i = 0U; i < job.count * 512U; i++) fake_disk[job.lba * 512U + i] = buf[i];
        } else {
            for (i = 0U; i < job.count * 512U; i++) buf[i] = fake_disk[job.lba * 512U + i];
        }
        res = ata_job_done(&job, chunk == fail_at_chunk ? -1 : 0, buf, sizeof(buf));
        chunk++;
        if (res != OS_ATA_JOB_CHUNK_OK) return res;
    }
    return res;
}

static void setup(void) {
    uint32_t i;
    for (i = 0U; i < sizeof(fake_disk); i++) fake_disk[i] = 0U;
    ram_state = 0x11U;
    restore_calls = 0;
    ata_job_init(fake_snapshot, fake_restore);
}

static void test_claim_exclusion(void) {
    setup();
    /* No driver: nobody may claim; kernel PIO allowed. */
    TEST_ASSERT_EQUAL(OS_ATA_DRIVER_REQUIRED, ata_owner_claim(5, 0));
    TEST_ASSERT_TRUE(ata_owner_kernel_may_pio(0));
    /* Only the live driver may claim. */
    TEST_ASSERT_EQUAL(OS_ATA_DRIVER_REQUIRED, ata_owner_claim(6, 5));
    TEST_ASSERT_EQUAL(0, ata_owner_claim(5, 5));
    TEST_ASSERT_TRUE(ata_owner_ports_open(5, 5));
    TEST_ASSERT_FALSE(ata_owner_ports_open(6, 5));
    /* Claimed: kernel PIO refused and counted. */
    TEST_ASSERT_FALSE(ata_owner_kernel_may_pio(5));
    {
        os_ata_status_t st;
        ata_job_fill_status(&st, 5);
        TEST_ASSERT_EQUAL(1, (int)st.kernel_pio_refused);
        TEST_ASSERT_EQUAL(5, st.claim_pid);
    }
    /* Release: ports closed, kernel PIO back. */
    TEST_ASSERT_EQUAL(OS_ATA_DRIVER_REQUIRED, ata_owner_release(6));
    TEST_ASSERT_EQUAL(0, ata_owner_release(5));
    TEST_ASSERT_FALSE(ata_owner_ports_open(5, 5));
    TEST_ASSERT_TRUE(ata_owner_kernel_may_pio(5));
    /* A claim left by a driver that died is stale. */
    TEST_ASSERT_EQUAL(0, ata_owner_claim(5, 5));
    TEST_ASSERT_TRUE(ata_owner_kernel_may_pio(0));
    TEST_ASSERT_EQUAL(0, ata_owner_holder(0));
    TEST_ASSERT_FALSE(ata_owner_ports_open(5, 0));
}

static void test_flush_then_load_roundtrip(void) {
    os_ata_status_t st;
    setup();
    TEST_ASSERT_FALSE(ata_job_pending());
    ata_job_request_flush();
    TEST_ASSERT_TRUE(ata_job_pending());
    TEST_ASSERT_EQUAL(OS_ATA_JOB_FLUSH_DONE, drive_job(-1));
    TEST_ASSERT_EQUAL(0x11, (int)fake_disk[0]);
    TEST_ASSERT_EQUAL(0x11, (int)fake_disk[sizeof(fake_disk) - 1U]);
    TEST_ASSERT_FALSE(ata_job_pending());
    ram_state = 0x22U; /* pretend RAM got reset; the load brings 0x11 back */
    ata_job_request_load();
    TEST_ASSERT_EQUAL(OS_ATA_JOB_LOAD_DONE, drive_job(-1));
    TEST_ASSERT_EQUAL(1, restore_calls);
    TEST_ASSERT_EQUAL(0x11, (int)ram_state);
    ata_job_fill_status(&st, 5);
    TEST_ASSERT_EQUAL(1, (int)st.flush_done);
    TEST_ASSERT_EQUAL(1, (int)st.load_done);
    TEST_ASSERT_EQUAL(0, (int)st.pending);
}

static void test_stale_and_failed_chunks(void) {
    static uint8_t buf[OS_ATA_JOB_MAX_SECTORS * 512U];
    os_ata_job_t job, bad;
    setup();
    TEST_ASSERT_EQUAL(0, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_job_done(&job, 0, buf, sizeof(buf)));
    ata_job_request_flush();
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&bad, buf, sizeof(buf))); /* idempotent */
    TEST_ASSERT_EQUAL(job.lba, bad.lba);
    TEST_ASSERT_EQUAL(job.generation, bad.generation);
    bad.lba = job.lba + 8U;
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_job_done(&bad, 0, buf, sizeof(buf)));
    bad = job; bad.generation++;
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_job_done(&bad, 0, buf, sizeof(buf)));
    /* Too small a buffer is refused for a write chunk. */
    TEST_ASSERT_TRUE(ata_job_fetch(&job, buf, 512U) < 0);
    /* A failed chunk re-queues the whole snapshot. */
    TEST_ASSERT_EQUAL(OS_ATA_JOB_FAILED, drive_job(2));
    TEST_ASSERT_TRUE(ata_job_pending());
    TEST_ASSERT_EQUAL(OS_ATA_JOB_FLUSH_DONE, drive_job(-1));
}

static void test_mutation_invalidates_load_and_driver_loss(void) {
    static uint8_t buf[OS_ATA_JOB_MAX_SECTORS * 512U];
    os_ata_job_t job;
    setup();
    ata_job_request_load();
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_READ, (int)job.op);
    /* RAM mutates while the load is in flight: the load must not win. */
    ata_job_request_flush();
    while (ata_job_done(&job, 0, buf, sizeof(buf)) == OS_ATA_JOB_CHUNK_OK)
        TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(0, restore_calls);
    /* The pending flush is still owed; a load request does not jump it. */
    ata_job_request_load();
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_WRITE, (int)job.op);
    /* Driver dies mid-flush: caller must fall back to kernel PIO. */
    TEST_ASSERT_EQUAL(1, ata_job_driver_gone());
    TEST_ASSERT_FALSE(ata_job_pending());
    TEST_ASSERT_EQUAL(0, ata_job_driver_gone());
}

static void test_client_fences(void) {
    setup();
    TEST_ASSERT_FALSE(ata_job_client_write_allowed(0U, 0U));
    TEST_ASSERT_FALSE(ata_job_client_write_allowed(0U, 63U));
    TEST_ASSERT_TRUE(ata_job_client_write_allowed(0U, 64U));
    TEST_ASSERT_TRUE(ata_job_client_write_allowed(1U, 0U));
    ata_job_set_fences(4224U, 1U);
    TEST_ASSERT_FALSE(ata_job_client_write_allowed(0U, 64U));
    TEST_ASSERT_FALSE(ata_job_client_write_allowed(0U, 4223U));
    TEST_ASSERT_TRUE(ata_job_client_write_allowed(0U, 4224U));
    TEST_ASSERT_FALSE(ata_job_client_write_allowed(1U, 100U));
    TEST_ASSERT_FALSE(ata_job_client_write_allowed(2U, 5000U));
    ata_job_set_fences(10U, 0U); /* never below the overlay snapshot */
    TEST_ASSERT_FALSE(ata_job_client_write_allowed(0U, 63U));
    TEST_ASSERT_EQUAL(-85, OS_ATA_CONTROLLER_BUSY);
    TEST_ASSERT_EQUAL(-86, OS_ATA_JOB_STALE);
}

/* Slice 3: synchronous FAT sector job slotted between overlay chunks. */
static void test_fat_io_job(void) {
    static uint8_t buf[OS_ATA_JOB_MAX_SECTORS * 512U];
    static uint8_t data[2U * 512U];
    static uint8_t out[2U * 512U];
    os_ata_job_t job, ov;
    os_ata_status_t st;
    uint32_t i;
    setup();
    for (i = 0U; i < sizeof(data); i++) data[i] = (uint8_t)(i ^ 0x5AU);
    /* Bad requests are refused; one job at a time. */
    TEST_ASSERT_EQUAL(-1, ata_job_io_submit(2U, 10U, 1U, 0, 0));
    TEST_ASSERT_EQUAL(-1, ata_job_io_submit(0U, 10U, 9U, 0, 0));
    TEST_ASSERT_EQUAL(-1, ata_job_io_submit(1U, 10U, 1U, 1, 0));
    /* An overlay flush is in flight: its first chunk is handed out... */
    ata_job_request_flush();
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&ov, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_WRITE, (int)ov.op);
    TEST_ASSERT_EQUAL(0, ata_job_io_submit(1U, 700U, 2U, 1, data));
    TEST_ASSERT_EQUAL(-1, ata_job_io_submit(0U, 800U, 1U, 0, 0));
    /* ...and re-fetching before done still returns that overlay chunk. */
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_WRITE, (int)job.op);
    TEST_ASSERT_EQUAL(OS_ATA_JOB_CHUNK_OK, ata_job_done(&ov, 0, buf, sizeof(buf)));
    /* The FAT write goes next, before the remaining overlay chunks. */
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_IO_WRITE, (int)job.op);
    TEST_ASSERT_EQUAL(1, (int)job.drive);
    TEST_ASSERT_EQUAL(700, (int)job.lba);
    TEST_ASSERT_EQUAL(2, (int)job.count);
    TEST_ASSERT_EQUAL(0x5A, (int)buf[0]);
    TEST_ASSERT_EQUAL(ATA_IO_HANDED, (int)ata_job_io_state());
    /* A completion for the overlay chunk now is stale. */
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_job_done(&ov, 0, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_IO_DONE, ata_job_done(&job, 0, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(ATA_IO_DONE, (int)ata_job_io_state());
    TEST_ASSERT_EQUAL(0, ata_job_io_take(0, 0U));
    TEST_ASSERT_EQUAL(ATA_IO_NONE, (int)ata_job_io_state());
    /* The overlay flush resumes where it stopped and completes. */
    TEST_ASSERT_EQUAL(OS_ATA_JOB_FLUSH_DONE, drive_job(-1));
    /* FAT read: data copied back to the caller on take. */
    TEST_ASSERT_EQUAL(0, ata_job_io_submit(0U, 900U, 2U, 0, 0));
    TEST_ASSERT_TRUE(ata_job_pending());
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_IO_READ, (int)job.op);
    for (i = 0U; i < 1024U; i++) buf[i] = (uint8_t)(i + 3U);
    TEST_ASSERT_EQUAL(OS_ATA_JOB_IO_DONE, ata_job_done(&job, 0, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(-1, ata_job_io_take(out, 512U)); /* too small */
    TEST_ASSERT_EQUAL(0, ata_job_io_submit(0U, 900U, 2U, 0, 0));
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_IO_DONE, ata_job_done(&job, 0, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(0, ata_job_io_take(out, sizeof(out)));
    TEST_ASSERT_EQUAL(3, (int)out[0]);
    TEST_ASSERT_EQUAL((int)(uint8_t)(1023U + 3U), (int)out[1023]);
    /* Driver failure is reported to the caller. */
    TEST_ASSERT_EQUAL(0, ata_job_io_submit(0U, 901U, 1U, 0, 0));
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(OS_ATA_JOB_FAILED, ata_job_done(&job, -1, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(ATA_IO_FAILED, (int)ata_job_io_state());
    TEST_ASSERT_EQUAL(-1, ata_job_io_take(out, sizeof(out)));
    /* Cancel (stalled driver): a late completion is stale. */
    TEST_ASSERT_EQUAL(0, ata_job_io_submit(0U, 902U, 1U, 0, 0));
    TEST_ASSERT_EQUAL(1, ata_job_fetch(&job, buf, sizeof(buf)));
    ata_job_io_cancel();
    TEST_ASSERT_EQUAL(OS_ATA_JOB_STALE, ata_job_done(&job, 0, buf, sizeof(buf)));
    /* Driver gone with a pending job: slot freed and counted. */
    TEST_ASSERT_EQUAL(0, ata_job_io_submit(0U, 903U, 1U, 0, 0));
    (void)ata_job_driver_gone();
    TEST_ASSERT_EQUAL(ATA_IO_NONE, (int)ata_job_io_state());
    TEST_ASSERT_FALSE(ata_job_pending());
    ata_job_note_fat_kernel_pio(3U, 0);
    ata_job_note_fat_kernel_pio(2U, 1);
    ata_job_set_boot_driver(2);
    ata_job_fill_status(&st, 0);
    TEST_ASSERT_EQUAL(2, (int)st.fat_driver_write_sectors);
    TEST_ASSERT_EQUAL(4, (int)st.fat_driver_read_sectors);
    TEST_ASSERT_EQUAL(5, (int)st.fat_kernel_pio_sectors);
    TEST_ASSERT_EQUAL(2, (int)st.fat_kernel_pio_live);
    TEST_ASSERT_EQUAL(2, (int)st.fat_rpc_aborts);
    TEST_ASSERT_EQUAL(2, st.boot_driver_pid);
}

int main(void) {
    unity_init();
    RUN_TEST(test_claim_exclusion);
    RUN_TEST(test_flush_then_load_roundtrip);
    RUN_TEST(test_stale_and_failed_chunks);
    RUN_TEST(test_mutation_invalidates_load_and_driver_loss);
    RUN_TEST(test_client_fences);
    RUN_TEST(test_fat_io_job);
    unity_print_results();
    unity_cleanup();
    return unity_stats.tests_failed == 0 ? 0 : 1;
}
