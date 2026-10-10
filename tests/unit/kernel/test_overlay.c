/* test_overlay.c - snapshot / restore of the RAM overlay (no ATA ports) */

#include <string.h>
#include "../../framework/unity.h"
#include "../../../fs/overlay.h"

static uint8_t g_snap[OV_SNAP_SIZE + 64];

static void put_u32(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

void setUp(void) {
    overlay_init();
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

/* Multi-block files: write/read/append/copy/rename/unlink across extents,
 * persistence round trip, NOSPACE keeps the old content, corrupt chains
 * rejected. */
static char g_big[OV_FILE_MAX + 8];
static char g_out2[OV_FILE_MAX + 8];
static void fill(char* b, uint32_t n, char seed) { uint32_t i; for (i = 0U; i < n; i++) b[i] = (char)('a' + ((i * 7U + (uint32_t)seed) % 26U)); }
static void test_multiblock_files(void) {
    uint32_t sz = 0U;
    os_dirent_t st, ents[8];
    int n, i, files;
    setUp();
    fill(g_big, 3000U, 1);
    TEST_ASSERT_EQUAL(3000, overlay_write("big.txt", g_big, 3000U));
    TEST_ASSERT_EQUAL(0, overlay_stat("big.txt", &st));
    TEST_ASSERT_EQUAL(3000, (int)st.size);
    memset(g_out2, 0, sizeof(g_out2));
    TEST_ASSERT_EQUAL(3000, overlay_read("big.txt", g_out2, sizeof(g_out2)));
    TEST_ASSERT_EQUAL(0, memcmp(g_big, g_out2, 3000));
    TEST_ASSERT_EQUAL(100, overlay_read("big.txt", g_out2, 100U));
    /* extents are invisible in listings */
    files = overlay_listdir("/", ents, 0, 8);
    TEST_ASSERT_EQUAL(1, files);
    /* append crosses a block boundary */
    fill(g_big + 3000, 500U, 2);
    TEST_ASSERT_EQUAL(500, overlay_append("big.txt", g_big + 3000, 500U));
    TEST_ASSERT_EQUAL(3500, overlay_read("big.txt", g_out2, sizeof(g_out2)));
    TEST_ASSERT_EQUAL(0, memcmp(g_big, g_out2, 3500));
    /* survives a snapshot/restore (two boots on one disk) */
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    overlay_init();
    TEST_ASSERT_EQUAL(0, overlay_restore(g_snap, sz));
    TEST_ASSERT_EQUAL(3500, overlay_read("big.txt", g_out2, sizeof(g_out2)));
    TEST_ASSERT_EQUAL(0, memcmp(g_big, g_out2, 3500));
    /* copy + rename keep every block; unlink frees the extents */
    TEST_ASSERT_EQUAL(0, overlay_copy("big.txt", "big2.txt"));
    TEST_ASSERT_EQUAL(0, overlay_rename("big2.txt", "moved.txt"));
    TEST_ASSERT_EQUAL(3500, overlay_read("moved.txt", g_out2, sizeof(g_out2)));
    TEST_ASSERT_EQUAL(0, memcmp(g_big, g_out2, 3500));
    TEST_ASSERT_EQUAL(0, overlay_unlink("moved.txt"));
    /* shrink in place */
    TEST_ASSERT_EQUAL(10, overlay_write("big.txt", "0123456789", 10U));
    TEST_ASSERT_EQUAL(10, overlay_read("big.txt", g_out2, sizeof(g_out2)));
    /* over the per-file cap: refused, old content kept */
    TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, overlay_write("big.txt", g_big, OV_FILE_MAX + 1U));
    TEST_ASSERT_EQUAL(10, overlay_read("big.txt", g_out2, sizeof(g_out2)));
    /* fill the node table: a large write without room fails cleanly */
    for (i = 0; i < 14; i++) {
        char name[16] = "f00.txt";
        name[1] = (char)('0' + i / 10); name[2] = (char)('0' + i % 10);
        n = overlay_write(name, g_big, 4000U);
        if (n < 0) { TEST_ASSERT_EQUAL(OV_ERR_NOSPACE, n); TEST_ASSERT_TRUE(overlay_read(name, g_out2, 8U) < 0); break; }
    }
    TEST_ASSERT_TRUE(i < 14);
    TEST_ASSERT_EQUAL(10, overlay_read("big.txt", g_out2, sizeof(g_out2)));
}
static void test_restore_rejects_broken_chain(void) {
    uint32_t sz = 0U;
    setUp();
    fill(g_big, 1000U, 3);
    TEST_ASSERT_EQUAL(1000, overlay_write("c.txt", g_big, 1000U));
    TEST_ASSERT_EQUAL(0, overlay_snapshot(g_snap, sizeof(g_snap), &sz));
    /* head 0 points to extent 1: turn node 1 into a non-extent */
    TEST_ASSERT_EQUAL(2, g_snap[16 + 3]); /* head 0 -> extent 1 */
    TEST_ASSERT_EQUAL(1, g_snap[16 + OV_SNAP_NODE + 2]);
    g_snap[16 + 3] = 0; /* oversized head without a chain */
    TEST_ASSERT_TRUE(overlay_restore(g_snap, sz) < 0);
}

static void test_snapshot_rejects_small_buffer(void) {
    uint8_t tiny[16];
    uint32_t sz = 99;

    setUp();
    TEST_ASSERT_TRUE(overlay_snapshot(tiny, sizeof(tiny), &sz) < 0);
    TEST_ASSERT_TRUE(overlay_restore(tiny, sizeof(tiny)) < 0);
    TEST_ASSERT_EQUAL(99, sz);
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
    RUN_TEST(test_multiblock_files);
    RUN_TEST(test_restore_rejects_broken_chain);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
