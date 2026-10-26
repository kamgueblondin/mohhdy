/* test_overlay.c - snapshot / restore of the RAM overlay (no ATA ports) */

#include <string.h>
#include "../../framework/unity.h"
#include "../../../fs/overlay.h"

/* Lot 46: room for a v4 image (OV_SNAP_MAX_SIZE >= OV_SNAP_SIZE). */
static uint8_t g_snap[OV_SNAP_MAX_SIZE + 64];

static void put_u32(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t get_u32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void setUp(void) {
    overlay_init();
    overlay_set_capacity(OV_SNAP_MAX_SIZE);  /* Lot 46 default: disk with extension */
}

void tearDown(void) {
}

static void test_snapshot_empty_roundtrip(void) {
    uint32_t sz = 0;
    char buf[16];

    setUp();
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    TEST_ASSERT_EQUAL(1, overlay_write("keep.txt", "x", 1));
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_TRUE(overlay_read("keep.txt", buf, sizeof(buf)) < 0);
}

static void test_snapshot_write_roundtrip(void) {
    uint32_t sz = 0;
    char buf[16];
    int n;

    setUp();
    TEST_ASSERT_EQUAL(4, overlay_write("k.txt", "v7ok", 4));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    overlay_init();
    TEST_ASSERT_TRUE(overlay_read("k.txt", buf, sizeof(buf)) < 0);
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    n = overlay_read("k.txt", buf, sizeof(buf));
    TEST_ASSERT_EQUAL(4, n);
    buf[n] = '\0';
    TEST_ASSERT_EQUAL_STRING("v7ok", buf);
}

static void test_snapshot_mkdir_roundtrip(void) {
    uint32_t sz = 0;
    os_dirent_t st;
    char buf[16];
    int n;

    setUp();
    TEST_ASSERT_EQUAL(OV_OK, overlay_mkdir("qd"));
    TEST_ASSERT_EQUAL(5, overlay_write("qd/k.txt", "hello", 5));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_TRUE(overlay_is_dir("qd"));
    TEST_ASSERT_EQUAL(OV_OK, overlay_stat("qd", &st));
    TEST_ASSERT_EQUAL(OS_DIRENT_DIR, st.flags);
    n = overlay_read("qd/k.txt", buf, sizeof(buf));
    TEST_ASSERT_EQUAL(5, n);
    buf[n] = '\0';
    TEST_ASSERT_EQUAL_STRING("hello", buf);
}

static void test_snapshot_unlink_roundtrip(void) {
    uint32_t sz_before = 0;
    uint32_t sz_after = 0;
    uint8_t snap_after[OV_SNAP_SIZE];
    char buf[16];

    setUp();
    TEST_ASSERT_EQUAL(3, overlay_write("gone.txt", "bye", 3));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz_before));
    TEST_ASSERT_EQUAL(OV_OK, overlay_unlink("gone.txt"));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(snap_after, sizeof(snap_after), &sz_after));

    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz_before));
    TEST_ASSERT_EQUAL(3, overlay_read("gone.txt", buf, sizeof(buf)));

    TEST_ASSERT_EQUAL(0, overlay_restore(snap_after, sz_after));
    TEST_ASSERT_TRUE(overlay_read("gone.txt", buf, sizeof(buf)) < 0);
}

static void test_restore_rejects_bad_magic(void) {
    uint32_t sz = 0;
    char buf[16];
    int n;

    setUp();
    TEST_ASSERT_EQUAL(4, overlay_write("stay.txt", "keep", 4));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    g_snap[0] ^= 0xFF;
    TEST_ASSERT_TRUE(overlay_restore(g_snap, sz) < 0);
    n = overlay_read("stay.txt", buf, sizeof(buf));
    TEST_ASSERT_EQUAL(4, n);
    buf[n] = '\0';
    TEST_ASSERT_EQUAL_STRING("keep", buf);
}

static void test_restore_v1_snapshot_compatibility(void) {
    char out[16];
    uint32_t off = 16U;
    uint32_t i;

    setUp();
    for (i = 0U; i < OV_SNAP_V1_SIZE; i++) g_snap[i] = 0U;
    put_u32(g_snap + 0, OV_SNAP_MAGIC);
    put_u32(g_snap + 4, OV_SNAP_V1_VERSION);
    put_u32(g_snap + 8, OV_SNAP_V1_NODES);
    put_u32(g_snap + 12, 1U);
    g_snap[off] = 1U;
    put_u32(g_snap + off + 4U, 4U);
    memcpy(g_snap + off + 8U, "old.txt", 8U);
    memcpy(g_snap + off + 8U + OV_SNAP_V1_PATH, "v1ok", 4U);

    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, OV_SNAP_V1_SIZE));
    TEST_ASSERT_EQUAL(4, overlay_read("old.txt", out, sizeof(out)));
    out[4] = '\0';
    TEST_ASSERT_EQUAL_STRING("v1ok", out);
}

static void test_snapshot_v2_accepts_extended_file(void) {
    char input[OV_SNAP_DATA];
    char output[OV_SNAP_DATA + 1U];
    uint32_t sz = 0U;
    uint32_t i;

    setUp();
    for (i = 0U; i < OV_SNAP_DATA; i++) input[i] = (char)('a' + (i % 26U));
    TEST_ASSERT_EQUAL((int)OV_SNAP_DATA, overlay_write("large.txt", input, OV_SNAP_DATA));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_EQUAL((int)OV_SNAP_DATA, overlay_read("large.txt", output, OV_SNAP_DATA));
    for (i = 0U; i < OV_SNAP_DATA; i++) TEST_ASSERT_EQUAL(input[i], output[i]);
}

static void test_snapshot_rejects_small_buffer(void) {
    uint8_t tiny[16];
    uint32_t sz = 99;

    setUp();
    TEST_ASSERT_TRUE(overlay_snapshot(tiny, sizeof(tiny), &sz) < 0);
    TEST_ASSERT_TRUE(overlay_restore(tiny, sizeof(tiny)) < 0);
    TEST_ASSERT_EQUAL(99, sz);
}

/* Lot 44 (US-G16): a 30 KiB file is byte exact through write, read,
 * read_at, snapshot and restore; beyond the bound is refused and a refused
 * write leaves the old file untouched. Lot 46: this is now the behaviour of
 * a disk without the v4 extension area (capacity OV_SNAP_SIZE), every
 * assertion kept; the 64 KiB bound has its own test below. */
static void test_large_file_write_read_and_bounds(void) {
    static char big[OV_FILE_MAX_V3_BYTES + 1U];
    static char back[OV_FILE_MAX_V3_BYTES + 1U];
    uint32_t i, sz = 0;
    int n;

    setUp();
    overlay_set_capacity(OV_SNAP_SIZE);
    for (i = 0; i <= OV_FILE_MAX_V3_BYTES; i++) big[i] = (char)(i % 251U);
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_V3_BYTES, overlay_write("big.bin", big, OV_FILE_MAX_V3_BYTES));
    n = overlay_read("big.bin", back, sizeof(back));
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_V3_BYTES, n);
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_V3_BYTES);
    /* chunked reads used by the kernel relay */
    TEST_ASSERT_EQUAL(4096, overlay_read_at("big.bin", back, 4096, 10000U));
    TEST_ASSERT_EQUAL_MEMORY(big + 10000, back, 4096);
    TEST_ASSERT_EQUAL(20, overlay_read_at("big.bin", back, 4096, OV_FILE_MAX_V3_BYTES - 20U));
    TEST_ASSERT_EQUAL(0, overlay_read_at("big.bin", back, 4096, OV_FILE_MAX_V3_BYTES));

    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    for (i = 0; i < sizeof(back); i++) back[i] = 0;
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_V3_BYTES, overlay_read("big.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_V3_BYTES);

    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("too_big.bin", big, OV_FILE_MAX_V3_BYTES + 1U));
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_append("big.bin", "extra", 5));
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_V3_BYTES, overlay_read("big.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_V3_BYTES);
    /* a second large file does not fit the 32 KiB disk area: refused, no change */
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("second.bin", big, 4096));
    TEST_ASSERT_TRUE(overlay_read("second.bin", back, sizeof(back)) < 0);
    TEST_ASSERT_TRUE(overlay_snapshot_bytes() <= OV_SNAP_SIZE);
    overlay_set_capacity(OV_SNAP_MAX_SIZE);
}

/* Lot 46 (US-G16): a 64 KiB file is byte exact through write, read_at,
 * a v4 snapshot (primary area + extension) and restore; one byte more is
 * refused and leaves the file untouched. */
static void test_v4_64k_file_roundtrip_and_bound(void) {
    static char big[65537];
    static char back[65540];
    uint32_t i, sz = 0;

    setUp();
    TEST_ASSERT_EQUAL(65536U, OV_FILE_MAX_BYTES);
    for (i = 0; i < sizeof(big); i++) big[i] = (char)(i * 7U % 253U);
    TEST_ASSERT_EQUAL(65536, overlay_write("big64.bin", big, 65536U));
    TEST_ASSERT_EQUAL(65536, overlay_read("big64.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, 65536);
    TEST_ASSERT_EQUAL(36, overlay_read_at("big64.bin", back, 4096, 65500U));
    TEST_ASSERT_EQUAL_MEMORY(big + 65500, back, 36);
    TEST_ASSERT_TRUE(overlay_snapshot_bytes() > OV_SNAP_SIZE);
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_MAX_SIZE, sz);
    TEST_ASSERT_EQUAL(OV_SNAP_MAGIC, get_u32(g_snap));
    TEST_ASSERT_EQUAL(OV_SNAP_V4_VERSION, (int)get_u32(g_snap + 4));
    TEST_ASSERT_EQUAL(OV_SNAP_NODES, (int)get_u32(g_snap + 8));
    TEST_ASSERT_EQUAL(OV_SNAP_MAX_SIZE, overlay_image_bytes(g_snap));
    /* a v4 image does not fit a 32 KiB buffer: refused, not cut */
    TEST_ASSERT_TRUE(overlay_snapshot(g_snap, OV_SNAP_SIZE, &sz) < 0);
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    for (i = 0; i < sizeof(back); i++) back[i] = 0;
    TEST_ASSERT_EQUAL(65536, overlay_read("big64.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, 65536);
    /* refusals past the bound, the file stays as it was */
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("over64.bin", big, 65537U));
    TEST_ASSERT_TRUE(overlay_read("over64.bin", back, sizeof(back)) < 0);
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_append("big64.bin", "x", 1));
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("big64.bin", big, 65537U));
    TEST_ASSERT_EQUAL(65536, overlay_read("big64.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, 65536);
    /* small files still fit next to it in the v4 area */
    TEST_ASSERT_EQUAL(4096, overlay_write("small.bin", big, 4096U));
    TEST_ASSERT_TRUE(overlay_snapshot_bytes() <= OV_SNAP_MAX_SIZE);
}

/* Lot 46: a v4 image read without its extension area (32 KiB only) is
 * refused and RAM keeps what it had. */
static void test_v4_cut_image_refused(void) {
    static char big[40000];
    char buf[8];
    uint32_t i, sz = 0;
    setUp();
    for (i = 0; i < sizeof(big); i++) big[i] = (char)('a' + i % 26U);
    TEST_ASSERT_EQUAL(40000, overlay_write("v4.bin", big, 40000U));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_MAX_SIZE, sz);
    TEST_ASSERT_EQUAL(3, overlay_write("keep.txt", "abc", 3));
    TEST_ASSERT_TRUE(overlay_restore(g_snap, OV_SNAP_SIZE) < 0);
    TEST_ASSERT_EQUAL(3, overlay_read("keep.txt", buf, sizeof(buf)));
    /* a broken extension (records past the area) is refused too */
    memset(g_snap + OV_SNAP_SIZE, 0, OV_SNAP_MAX_SIZE - OV_SNAP_SIZE);
    TEST_ASSERT_TRUE(overlay_restore(g_snap, sz) < 0);
    TEST_ASSERT_EQUAL(3, overlay_read("keep.txt", buf, sizeof(buf)));
}

/* Lot 46: an overlay that fits 32 KiB is still written as v3 (header 3,
 * 128 nodes, 32 KiB), so v3 readers and the slice 2 jobs keep working; a
 * v4 overlay that shrinks goes back to v3. */
static void test_v3_kept_when_it_fits_and_after_shrink(void) {
    static char big[40000];
    uint32_t sz = 0;
    setUp();
    memset(big, 'q', sizeof(big));
    TEST_ASSERT_EQUAL(5, overlay_write("s.txt", "small", 5));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    TEST_ASSERT_EQUAL(OV_SNAP_VERSION, (int)get_u32(g_snap + 4));
    TEST_ASSERT_EQUAL(OV_SNAP_V3_NODES, (int)get_u32(g_snap + 8));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, overlay_image_bytes(g_snap));
    TEST_ASSERT_EQUAL(40000, overlay_write("b.bin", big, 40000U));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_MAX_SIZE, sz);
    TEST_ASSERT_EQUAL(OV_OK, overlay_unlink("b.bin"));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    TEST_ASSERT_EQUAL(3, (int)g_snap[4]);
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
}

/* Lot 46: without the extension area (capacity OV_SNAP_SIZE) the image
 * keeps at most 128 records, so it always stays a valid v3 image. */
static void test_v3_capacity_bounds_record_count(void) {
    char name[16];
    int i, ok = 0, rc = 0;
    uint32_t sz = 0;
    setUp();
    overlay_set_capacity(OV_SNAP_SIZE);
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, overlay_capacity());
    for (i = 0; i < 200; i++) {
        name[0] = 'd'; name[1] = (char)('0' + i / 100); name[2] = (char)('0' + (i / 10) % 10);
        name[3] = (char)('0' + i % 10); name[4] = '\0';
        rc = overlay_mkdir(name);
        if (rc != OV_OK) break;
        ok++;
    }
    TEST_ASSERT_EQUAL(OV_SNAP_V3_NODES, ok);
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, rc);
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    overlay_set_capacity(1U);                 /* clamped */
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, overlay_capacity());
    overlay_set_capacity(0xFFFFFFFFU);
    TEST_ASSERT_EQUAL(OV_SNAP_MAX_SIZE, overlay_capacity());
}

/* Lot 46: sector s of an image lives at LBA s in the AIOV area, then in
 * the extension area after the event journal, never in the FAT16 volume
 * (LBA 64-4221) nor in the journals (4222-4224). */
static void test_image_lba_layout(void) {
    uint32_t s;
    TEST_ASSERT_EQUAL(0U, overlay_image_lba(0U));
    TEST_ASSERT_EQUAL(63U, overlay_image_lba(63U));
    TEST_ASSERT_EQUAL(4225U, overlay_image_lba(64U));
    TEST_ASSERT_EQUAL(4352U, overlay_image_lba(OS_ATA_OVERLAY_ALL_SECTORS - 1U));
    for (s = 0U; s < OS_ATA_OVERLAY_ALL_SECTORS; s++) {
        uint32_t lba = overlay_image_lba(s);
        TEST_ASSERT_TRUE(lba < 64U || lba >= 4225U);
    }
    TEST_ASSERT_EQUAL(OV_SNAP_MAX_SIZE, OS_ATA_OVERLAY_ALL_SECTORS * 512U);
}

static void test_large_file_rewrite_copy_append(void) {
    static char big[12000];
    static char back[16000];
    uint32_t i, sz = 0;

    setUp();
    for (i = 0; i < sizeof(big); i++) big[i] = (char)('A' + i % 23U);
    TEST_ASSERT_EQUAL(12000, overlay_write("a.txt", big, 12000));
    TEST_ASSERT_EQUAL(OV_OK, overlay_copy("a.txt", "b.txt"));
    TEST_ASSERT_EQUAL(12000, overlay_read("b.txt", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, 12000);
    /* shrink then grow by append, still byte exact */
    TEST_ASSERT_EQUAL(5, overlay_write("a.txt", "hello", 5));
    TEST_ASSERT_EQUAL(1000, overlay_append("a.txt", big, 1000));
    TEST_ASSERT_EQUAL(1005, overlay_read("a.txt", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY("hello", back, 5);
    TEST_ASSERT_EQUAL_MEMORY(big, back + 5, 1000);
    TEST_ASSERT_EQUAL(OV_OK, overlay_rename("b.txt", "c.txt"));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_EQUAL(12000, overlay_read("c.txt", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, 12000);
    TEST_ASSERT_EQUAL(OV_OK, overlay_unlink("c.txt"));
    TEST_ASSERT_EQUAL(OV_OK, overlay_unlink("a.txt"));
    TEST_ASSERT_EQUAL(16U, overlay_snapshot_bytes());
}

/* Disks written before Lot 44 hold a fixed v2 image (64 slots): it still
 * loads, and the next snapshot is v3. */
static void test_restore_v2_disk_image(void) {
    static uint8_t v2[OV_SNAP_V2_SIZE];
    char out[16];
    uint32_t off = 16U + 5U * OV_SNAP_V2_NODE, sz = 0;
    memset(v2, 0, sizeof(v2));
    setUp();
    put_u32(v2 + 0, OV_SNAP_MAGIC);
    put_u32(v2 + 4, OV_SNAP_V2_VERSION);
    put_u32(v2 + 8, OV_SNAP_V2_NODES);
    put_u32(v2 + 12, 1U);
    v2[off] = 1U;
    put_u32(v2 + off + 4U, 4U);
    memcpy(v2 + off + 8U, "v2.txt", 7U);
    memcpy(v2 + off + 8U + OV_SNAP_V2_PATH, "v2ok", 4U);
    TEST_ASSERT_EQUAL(0, overlay_restore(v2, sizeof(v2)));
    TEST_ASSERT_EQUAL(4, overlay_read("v2.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_MEMORY("v2ok", out, 4);
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(3, (int)g_snap[4]);
}

static void test_restore_v3_rejects_broken_chain(void) {
    uint32_t sz = 0;
    char buf[8];
    setUp();
    TEST_ASSERT_EQUAL(1000, overlay_write("c.bin", (const char*)g_snap, 1000));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    put_u32(g_snap + 12, 2U);  /* drop the last block record */
    TEST_ASSERT_TRUE(overlay_restore(g_snap, sz) < 0);
    TEST_ASSERT_EQUAL(8, overlay_read("c.bin", buf, sizeof(buf)));
}

int main(void) {
    unity_init();
    RUN_TEST(test_snapshot_empty_roundtrip);
    RUN_TEST(test_snapshot_write_roundtrip);
    RUN_TEST(test_snapshot_mkdir_roundtrip);
    RUN_TEST(test_snapshot_unlink_roundtrip);
    RUN_TEST(test_restore_rejects_bad_magic);
    RUN_TEST(test_restore_v1_snapshot_compatibility);
    RUN_TEST(test_snapshot_v2_accepts_extended_file);
    RUN_TEST(test_snapshot_rejects_small_buffer);
    RUN_TEST(test_large_file_write_read_and_bounds);
    RUN_TEST(test_large_file_rewrite_copy_append);
    RUN_TEST(test_restore_v3_rejects_broken_chain);
    RUN_TEST(test_restore_v2_disk_image);
    RUN_TEST(test_v4_64k_file_roundtrip_and_bound);
    RUN_TEST(test_v4_cut_image_refused);
    RUN_TEST(test_v3_kept_when_it_fits_and_after_shrink);
    RUN_TEST(test_v3_capacity_bounds_record_count);
    RUN_TEST(test_image_lba_layout);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
