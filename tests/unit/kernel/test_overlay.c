/* test_overlay.c - snapshot / restore of the RAM overlay (no ATA ports) */

#include <string.h>
#include "../../framework/unity.h"
#include "../../../fs/overlay.h"

static uint8_t g_snap[OV_SNAP_SIZE_MAX + 64];

static void put_u32(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

void setUp(void) {
    overlay_init();
    (void)overlay_set_ext(0U);
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
 * write leaves the old file untouched. */
static void test_large_file_write_read_and_bounds(void) {
    static char big[OV_FILE_MAX_BYTES + 1U];
    static char back[OV_FILE_MAX_BYTES + 1U];
    uint32_t i, sz = 0;
    int n;

    setUp();
    for (i = 0; i <= OV_FILE_MAX_BYTES; i++) big[i] = (char)(i % 251U);
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_BYTES, overlay_write("big.bin", big, OV_FILE_MAX_BYTES));
    n = overlay_read("big.bin", back, sizeof(back));
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_BYTES, n);
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_BYTES);
    /* chunked reads used by the kernel relay */
    TEST_ASSERT_EQUAL(4096, overlay_read_at("big.bin", back, 4096, 10000U));
    TEST_ASSERT_EQUAL_MEMORY(big + 10000, back, 4096);
    TEST_ASSERT_EQUAL(20, overlay_read_at("big.bin", back, 4096, OV_FILE_MAX_BYTES - 20U));
    TEST_ASSERT_EQUAL(0, overlay_read_at("big.bin", back, 4096, OV_FILE_MAX_BYTES));

    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    for (i = 0; i < sizeof(back); i++) back[i] = 0;
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_BYTES, overlay_read("big.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_BYTES);

    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("too_big.bin", big, OV_FILE_MAX_BYTES + 1U));
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_append("big.bin", "extra", 5));
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_BYTES, overlay_read("big.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_BYTES);
    /* a second large file does not fit the 32 KiB disk area: refused, no change */
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("second.bin", big, 4096));
    TEST_ASSERT_TRUE(overlay_read("second.bin", back, sizeof(back)) < 0);
    TEST_ASSERT_TRUE(overlay_snapshot_bytes() <= OV_SNAP_SIZE);
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

/* Lot 46 PR B (US-G16): with the extension area (after the FAT16 volume
 * and the journals) a 64 KiB file is byte exact through write, snapshot
 * (v4, 96 KiB, two disk areas) and restore; 64 KiB + 1 is refused and
 * leaves the old file; the store is refused past 96 KiB. */
static void test_v4_extension_64k_file(void) {
    static char big[OV_FILE_MAX_EXT_BYTES + 1U];
    static char back[OV_FILE_MAX_EXT_BYTES + 16U];
    uint32_t i, sz = 0, s;
    setUp();
    TEST_ASSERT_EQUAL(-1, overlay_set_ext(4224U));           /* the event journal: refused */
    TEST_ASSERT_EQUAL(-1, overlay_set_ext(100U));            /* inside FAT16: refused */
    TEST_ASSERT_EQUAL(0, overlay_set_ext(OV_EXT_MIN_LBA));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE_MAX, overlay_capacity());
    TEST_ASSERT_EQUAL(OV_FILE_MAX_EXT_BYTES, overlay_file_max());
    for (i = 0; i <= OV_FILE_MAX_EXT_BYTES; i++) big[i] = (char)((i * 7U + 3U) % 253U);
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_EXT_BYTES, overlay_write("big64.bin", big, OV_FILE_MAX_EXT_BYTES));
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("big64.bin", big, OV_FILE_MAX_EXT_BYTES + 1U));
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_append("big64.bin", "x", 1));
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_EXT_BYTES, overlay_read("big64.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_EXT_BYTES);
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE_MAX, sz);
    TEST_ASSERT_EQUAL(4, (int)g_snap[4]);
    TEST_ASSERT_EQUAL(OV_EXT_MIN_LBA, overlay_image_ext_lba(g_snap));
    TEST_ASSERT_EQUAL(OV_SNAP_SECTORS_MAX, overlay_image_sectors(g_snap));
    /* no image sector lands in the FAT16 volume (LBA 64 .. 4221) or on the
     * journals (4222 .. 4224) */
    for (s = 0; s < OV_SNAP_SECTORS_MAX; s++) {
        uint32_t lba = overlay_image_lba(OV_EXT_MIN_LBA, s);
        TEST_ASSERT_TRUE(lba < 64U || lba >= OV_EXT_MIN_LBA);
    }
    TEST_ASSERT_EQUAL(OV_EXT_MIN_LBA + OV_EXT_SECTORS - 1U, overlay_image_lba(OV_EXT_MIN_LBA, OV_SNAP_SECTORS_MAX - 1U));
    /* the data really crosses into the extension (past 32 KiB) */
    for (i = OV_SNAP_SIZE, s = 0; i < OV_SNAP_SIZE + 512U; i++) if (g_snap[i]) s++;
    TEST_ASSERT_TRUE(s > 0U);
    /* "reboot": a store without the extension learns it from the image */
    overlay_init();
    (void)overlay_set_ext(0U);
    TEST_ASSERT_TRUE(overlay_restore(g_snap, OV_SNAP_SIZE) < 0);   /* half an image: refused */
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_EQUAL(OV_EXT_MIN_LBA, overlay_ext_lba());
    memset(back, 0, sizeof(back));
    TEST_ASSERT_EQUAL((int)OV_FILE_MAX_EXT_BYTES, overlay_read("big64.bin", back, sizeof(back)));
    TEST_ASSERT_EQUAL_MEMORY(big, back, OV_FILE_MAX_EXT_BYTES);
    /* store bound: a second 30 KiB file does not fit beside it */
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("second.bin", big, 30720U));
    TEST_ASSERT_TRUE(overlay_read("second.bin", back, sizeof(back)) < 0);
    TEST_ASSERT_TRUE(overlay_snapshot_bytes() <= OV_SNAP_SIZE_MAX);
    /* a v4 header pointing into the FAT16 volume is refused */
    put_u32(g_snap + 16, 100U);
    TEST_ASSERT_TRUE(overlay_restore(g_snap, sz) < 0);
    TEST_ASSERT_EQUAL(0U, overlay_image_ext_lba(g_snap));
}

/* Lot 46 PR B: a v3 disk (Lot 44/45) still loads in a store that has the
 * extension, and the next snapshot is v4 with the same files. */
static void test_v3_image_loads_then_saves_v4(void) {
    char out[32];
    uint32_t sz = 0;
    setUp();
    TEST_ASSERT_EQUAL(9, overlay_write("v3.txt", "v3 kept !", 9));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(OV_SNAP_SIZE, sz);
    TEST_ASSERT_EQUAL(3, (int)g_snap[4]);
    TEST_ASSERT_EQUAL(64U, overlay_image_sectors(g_snap));
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_set_ext(5000U));
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_EQUAL(9, overlay_read("v3.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_MEMORY("v3 kept !", out, 9);
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    TEST_ASSERT_EQUAL(4, (int)g_snap[4]);
    TEST_ASSERT_EQUAL(5000U, overlay_image_ext_lba(g_snap));
    overlay_init();
    (void)overlay_set_ext(0U);
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_EQUAL(9, overlay_read("v3.txt", out, sizeof(out)));
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
    RUN_TEST(test_v4_extension_64k_file);
    RUN_TEST(test_v3_image_loads_then_saves_v4);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
