#include "../../framework/unity.h"
#include "../../../kernel/fs/fat32.h"
#include "../../../kernel/fs/lfn_utf8.h"

static uint8_t disk[2048U * 512U];
static int read_sector(uint32_t lba, void* buffer) {
    if (lba >= 2048U || !buffer) return -1;
    for (uint32_t i = 0U; i < 512U; i++) ((uint8_t*)buffer)[i] = disk[lba * 512U + i];
    return 0;
}
static int write_sector(uint32_t lba, const void* buffer) {
    if (lba >= 2048U || !buffer) return -1;
    for (uint32_t i = 0U; i < 512U; i++) disk[lba * 512U + i] = ((const uint8_t*)buffer)[i];
    return 0;
}
static void put16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8U); }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8U); p[2] = (uint8_t)(v >> 16U); p[3] = (uint8_t)(v >> 24U); }
void setUp(void) { for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U; }
void tearDown(void) {}

void test_fat32_mount_and_read_cluster(void) {
    fat32_volume_t volume; uint8_t cluster[1024]; uint32_t next, lba;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put16(disk + 19U, 0U); put16(disk + 22U, 0U); put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U); put16(disk + 510U, 0xaa55U);
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_TRUE(fat32_is_mounted(&volume)); TEST_ASSERT_EQUAL(2U, volume.root_cluster);
    TEST_ASSERT_EQUAL(0, fat32_cluster_lba(&volume, 2U, &lba)); TEST_ASSERT_EQUAL(2032U, lba);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU; disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_read_fat_entry(&volume, 2U, &next)); TEST_ASSERT_EQUAL(FAT32_EOC_MIN, next);
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_allocate_cluster(&volume, &next)); TEST_ASSERT_EQUAL(3U, next);
    TEST_ASSERT_EQUAL(0, fat32_read_fat_entry(&volume, 3U, &next)); TEST_ASSERT_EQUAL(FAT32_EOC_MIN, next);
    TEST_ASSERT_EQUAL(0, fat32_link_clusters(&volume, 2U, 3U));
    TEST_ASSERT_EQUAL(0, fat32_read_fat_entry(&volume, 2U, &next)); TEST_ASSERT_EQUAL(3U, next);
    { uint8_t data[1024U]; for (uint32_t i = 0U; i < sizeof(data); i++) data[i] = (uint8_t)(i & 0xffU);
      TEST_ASSERT_EQUAL(0, fat32_write_cluster(&volume, 3U, data));
      TEST_ASSERT_EQUAL(0, fat32_create_root_entry(&volume, "SESSION.BIN", 0x20U, 3U, 1024U));
      TEST_ASSERT_EQUAL('S', disk[2032U * 512U]); TEST_ASSERT_EQUAL('E', disk[2032U * 512U + 1U]);
      TEST_ASSERT_EQUAL('B', disk[2032U * 512U + 8U]); TEST_ASSERT_EQUAL('N', disk[2032U * 512U + 10U]);
      TEST_ASSERT_EQUAL(0x20U, disk[2032U * 512U + 11U]); TEST_ASSERT_EQUAL(0x00U, disk[2032U * 512U + 20U]);
      TEST_ASSERT_EQUAL(3U, disk[2032U * 512U + 26U]); TEST_ASSERT_EQUAL(0U, disk[2032U * 512U + 28U]); TEST_ASSERT_EQUAL(4U, disk[2032U * 512U + 29U]);
      { uint8_t file_data[1024U]; uint32_t first; for (uint32_t i = 0U; i < sizeof(file_data); i++) file_data[i] = (uint8_t)(0xa0U + (i & 0x0fU));
        TEST_ASSERT_EQUAL(0, fat32_create_file(&volume, "CHAT.BIN", 0x20U, file_data, sizeof(file_data), &first));
        TEST_ASSERT_EQUAL(4U, first); TEST_ASSERT_EQUAL(file_data[0], disk[2036U * 512U]); TEST_ASSERT_EQUAL(file_data[511U], disk[2036U * 512U + 511U]);
        TEST_ASSERT_EQUAL(file_data[512U], disk[2037U * 512U]); TEST_ASSERT_EQUAL('C', disk[2032U * 512U + 32U]); }
    }
}

void test_fat32_lists_root_page_after_first_entry(void) {
    fat32_volume_t volume;
    os_fat16_dirent_t entries[2];
    uint8_t data[1] = {'2'};
    uint32_t first = 0U;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U);
    put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU;
    disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_create_file(&volume, "FIRST.TXT", 0x20U,
                                           data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(0, fat32_create_file(&volume, "SECOND.TXT", 0x20U,
                                           data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(1, fat32_list_root_page(&volume, 1U, entries, 2U));
    TEST_ASSERT_EQUAL_STRING("SECOND.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(0, fat32_list_root_page(&volume, 2U, entries, 2U));
}

void test_fat32_rename_file_root_8_3(void) {
    fat32_volume_t volume;
    os_fat16_dirent_t entries[2];
    uint8_t data[8] = {'q', 'e', 'm', 'u', '-', 'o', 'k', '!'};
    uint8_t readback[8];
    uint32_t first = 0U;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U);
    put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU;
    disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_create_file(&volume, "NEW.TXT", 0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(0, fat32_create_file(&volume, "KEEP.TXT", 0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_rename_file(&volume, "NEW.TXT", "KEEP.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_rename_file(&volume, "NEW.TXT", "RENAMED.TXT"));
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_FOUND, fat32_read_file(&volume, "NEW.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL(8, fat32_read_file(&volume, "RENAMED.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL('q', readback[0]);
    TEST_ASSERT_EQUAL(2, fat32_list_root(&volume, entries, 2U));
    TEST_ASSERT_EQUAL(0, fat32_unlink_file(&volume, "RENAMED.TXT"));
    TEST_ASSERT_EQUAL(1, fat32_list_root(&volume, entries, 2U));
    TEST_ASSERT_EQUAL_STRING("KEEP.TXT", entries[0].name);
}

void test_fat32_extend_full_root(void) {
    fat32_volume_t volume; uint32_t next;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U); put16(disk + 510U, 0xaa55U);
    for (uint32_t i = 0U; i < 1024U; i++) disk[2032U * 512U + i] = 0x41U;
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU; disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U)); TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_extend_root_directory(&volume, &next)); TEST_ASSERT_EQUAL(3U, next);
    TEST_ASSERT_EQUAL(3U, disk[32U * 512U + 8U]); TEST_ASSERT_EQUAL(0U, disk[2034U * 512U]);
}

void test_lfn_utf8_bmp_conversion(void) {
    uint16_t units[8]; char output[16]; uint32_t count = 0U;
    TEST_ASSERT_EQUAL(0, lfn_utf8_to_utf16_bmp("caf\xC3\xA9", units, 8U, &count));
    TEST_ASSERT_EQUAL(4U, count); TEST_ASSERT_EQUAL(0x00E9U, units[3]);
    TEST_ASSERT_EQUAL(5, lfn_utf16_bmp_to_utf8(units, count, output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("caf\xC3\xA9", output);
    TEST_ASSERT_EQUAL(0, lfn_utf8_to_utf16_bmp("A\xF0\x9F\x98\x80", units, 8U, &count));
    TEST_ASSERT_EQUAL(3U, count); TEST_ASSERT_EQUAL(0xD83DU, units[1]); TEST_ASSERT_EQUAL(0xDE00U, units[2]);
    TEST_ASSERT_EQUAL(5, lfn_utf16_bmp_to_utf8(units, count, output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("A\xF0\x9F\x98\x80", output);
    units[0] = 0xD83DU; units[1] = 0U;
    TEST_ASSERT_TRUE(lfn_utf16_bmp_to_utf8(units, 2U, output, sizeof(output)) < 0);
    TEST_ASSERT_TRUE(lfn_utf8_to_utf16_bmp("\xC0\x80", units, 8U, &count) < 0);
}

void test_fat32_lfn_encoding(void) {
    uint8_t alias[11] = {'C','H','A','T',' ',' ',' ',' ','B','I','N'}, entry[32];
    TEST_ASSERT_EQUAL(0, fat32_encode_lfn_entry("Session-2026", 0x41U, fat32_lfn_checksum(alias), entry));
    TEST_ASSERT_EQUAL(0x41U, entry[0]); TEST_ASSERT_EQUAL(0x0fU, entry[11]); TEST_ASSERT_EQUAL(fat32_lfn_checksum(alias), entry[13]);
    TEST_ASSERT_EQUAL('S', entry[1]); TEST_ASSERT_EQUAL(0U, entry[2]); TEST_ASSERT_EQUAL('n', entry[16]); TEST_ASSERT_EQUAL(0U, entry[17]);
    TEST_ASSERT_EQUAL(0, fat32_encode_lfn_entry("abcdefghijklm", 1U, 0U, entry));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_encode_lfn_entry("abcdefghijklmn", 2U, 0U, entry));
    TEST_ASSERT_EQUAL(0, fat32_encode_lfn_entry("caf\xC3\xA9", 0x41U, 0U, entry));
    TEST_ASSERT_EQUAL(0xE9U, entry[7]); TEST_ASSERT_EQUAL(0x00U, entry[8]);
}

void test_fat32_lfn_file_and_list(void) {
    fat32_volume_t volume; os_fat16_dirent_t entries[4]; uint8_t data[16]; uint32_t first;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U); put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU; disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    for (uint32_t i = 0U; i < sizeof(data); i++) data[i] = (uint8_t)i;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U)); TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    { int rc = fat32_create_lfn_file(&volume, "Persistent-LLM-Session", "SESSION.BIN", 0x20U, data, sizeof(data), &first); TEST_ASSERT_EQUAL(0, rc); }
    TEST_ASSERT_EQUAL(3U, first); TEST_ASSERT_EQUAL(16, fat32_read_file(&volume, "SESSION.BIN", data + 0U, sizeof(data)));
    TEST_ASSERT_EQUAL(16, fat32_read_file(&volume, "persistent-llm-session", data + 0U, sizeof(data)));
    TEST_ASSERT_EQUAL(1, fat32_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("Persistent-LLM-Session", entries[0].name); TEST_ASSERT_EQUAL(sizeof(data), entries[0].size);
    TEST_ASSERT_EQUAL(0, fat32_rename_lfn_file(&volume, "persistent-llm-session",
                                                "Persistent-LLM-Record", "RECORD.BIN"));
    TEST_ASSERT_EQUAL(1, fat32_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("Persistent-LLM-Record", entries[0].name);
    TEST_ASSERT_EQUAL(sizeof(data), entries[0].size);
    TEST_ASSERT_EQUAL(0, fat32_unlink_file(&volume, "persistent-llm-record"));
    TEST_ASSERT_EQUAL(0xe5U, disk[2032U * 512U]);
    TEST_ASSERT_EQUAL(0xe5U, disk[2032U * 512U + 32U]);
    TEST_ASSERT_EQUAL(0xe5U, disk[2032U * 512U + 64U]);
    TEST_ASSERT_EQUAL(0xe5U, disk[2032U * 512U + 96U]);
    TEST_ASSERT_EQUAL(0U, disk[32U * 512U + 12U]);
    TEST_ASSERT_EQUAL(0, fat32_list_root(&volume, entries, 4U));
    TEST_ASSERT_EQUAL(OS_FAT16_NOT_FOUND, fat32_unlink_file(&volume, "Persistent-LLM-Session"));
}

void test_fat32_utf8_lfn_file_roundtrip(void) {
    fat32_volume_t volume;
    os_fat16_dirent_t entries[2];
    uint8_t data[4] = {'U', 'T', 'F', '8'};
    uint8_t readback[4] = {0U};
    uint32_t first = 0U;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U); put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU;
    disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_create_lfn_file(&volume, "caf\xC3\xA9-2026.txt", "CAFE26.TXT",
                                               0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(0xE9U, disk[2032U * 512U + 32U + 7U]);
    TEST_ASSERT_EQUAL(0x00U, disk[2032U * 512U + 32U + 8U]);
    TEST_ASSERT_EQUAL(4, fat32_read_file(&volume, "caf\xC3\xA9-2026.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(data, readback, sizeof(data));
    TEST_ASSERT_EQUAL(1, fat32_list_root(&volume, entries, 2U));
    TEST_ASSERT_EQUAL_STRING("caf\xC3\xA9-2026.txt", entries[0].name);
}

void test_fat32_utf8_lfn_non_bmp_roundtrip(void) {
    fat32_volume_t volume;
    os_fat16_dirent_t entries[2];
    uint8_t data[5] = {'A', 'S', 'T', 'R', 'A'};
    uint8_t readback[5] = {0U};
    uint32_t first = 0U;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U); put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU;
    disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_create_lfn_file(&volume, "rocket-\xF0\x9F\x98\x80.txt", "ROCKT1.TXT",
                                               0x20U, data, sizeof(data), &first));
    TEST_ASSERT_EQUAL(3U, first);
    TEST_ASSERT_EQUAL(0x3DU, disk[2032U * 512U + 32U + 18U]);
    TEST_ASSERT_EQUAL(0xD8U, disk[2032U * 512U + 32U + 19U]);
    TEST_ASSERT_EQUAL(0x00U, disk[2032U * 512U + 32U + 20U]);
    TEST_ASSERT_EQUAL(0xDEU, disk[2032U * 512U + 32U + 21U]);
    TEST_ASSERT_EQUAL(5, fat32_read_file(&volume, "rocket-\xF0\x9F\x98\x80.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(data, readback, sizeof(data));
    TEST_ASSERT_EQUAL(1, fat32_list_root(&volume, entries, 2U));
    TEST_ASSERT_EQUAL_STRING("rocket-\xF0\x9F\x98\x80.txt", entries[0].name);
}

void test_fat32_mutates_one_level_subdirectory_without_overwrite(void) {
    fat32_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t alpha[5] = {'a', 'l', 'p', 'h', 'a'};
    uint8_t beta[4] = {'b', 'e', 't', 'a'};
    uint8_t readback[5] = {0U};
    uint32_t first = 0U;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U);
    put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU;
    disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_create_directory(&volume, "DIR"));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_create_directory(&volume, "DIR"));
    TEST_ASSERT_EQUAL(0, fat32_create_path_file(&volume, "DIR/CHILD.TXT", alpha,
                                                sizeof(alpha), &first));
    TEST_ASSERT_TRUE(first >= 2U);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_create_path_file(&volume, "DIR/CHILD.TXT", beta,
                                                                 sizeof(beta), &first));
    TEST_ASSERT_EQUAL(1, fat32_list_path_page(&volume, "DIR/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("CHILD.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(5, fat32_read_path(&volume, "DIR/CHILD.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(alpha, readback, sizeof(alpha));
    TEST_ASSERT_EQUAL(0, fat32_rename_path_file(&volume, "DIR/CHILD.TXT", "DIR/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_create_path_file(&volume, "DIR/OTHER.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_rename_path_file(&volume, "DIR/RENAMED.TXT",
                                                                 "DIR/OTHER.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_create_path_file(&volume, "DIR/ONE.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(0, fat32_create_path_file(&volume, "DIR/TWO.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(0, fat32_create_path_file(&volume, "DIR/THREE.TXT", beta,
                                                sizeof(beta), &first));
    TEST_ASSERT_EQUAL(4, fat32_list_path_page(&volume, "DIR/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL(1, fat32_list_path_page(&volume, "DIR/", 4U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("THREE.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_remove_directory(&volume, "DIR"));
    TEST_ASSERT_EQUAL(0, fat32_unlink_path_file(&volume, "DIR/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_unlink_path_file(&volume, "DIR/OTHER.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_unlink_path_file(&volume, "DIR/ONE.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_unlink_path_file(&volume, "DIR/TWO.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_unlink_path_file(&volume, "DIR/THREE.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_remove_directory(&volume, "DIR"));
}

void test_fat32_mutates_multilevel_subdirectory(void) {
    fat32_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t payload[7] = {'N', 'E', 'S', 'T', 'E', 'D', '!'};
    uint8_t readback[8] = {0U};
    uint32_t first = 0U;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U);
    put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU;
    disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_create_directory(&volume, "SUB1"));
    TEST_ASSERT_EQUAL(0, fat32_create_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(0, fat32_create_path_file(&volume, "SUB1/SUB2/FILE.TXT", payload,
                                                sizeof(payload), &first));
    TEST_ASSERT_TRUE(first >= 2U);
    TEST_ASSERT_EQUAL(1, fat32_list_path_page(&volume, "SUB1/SUB2/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("FILE.TXT", entries[0].name);
    TEST_ASSERT_EQUAL(7, fat32_read_path(&volume, "SUB1/SUB2/FILE.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(payload, readback, sizeof(payload));
    TEST_ASSERT_EQUAL(0, fat32_rename_path_file(&volume, "SUB1/SUB2/FILE.TXT", "SUB1/SUB2/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(7, fat32_read_path(&volume, "SUB1/SUB2/RENAMED.TXT", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_remove_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(OS_FAT16_BAD_PATH, fat32_remove_directory(&volume, "SUB1"));
    TEST_ASSERT_EQUAL(0, fat32_unlink_path_file(&volume, "SUB1/SUB2/RENAMED.TXT"));
    TEST_ASSERT_EQUAL(0, fat32_remove_directory(&volume, "SUB1/SUB2"));
    TEST_ASSERT_EQUAL(0, fat32_remove_directory(&volume, "SUB1"));
}

void test_fat32_creates_lfn_in_subdirectory(void) {
    fat32_volume_t volume;
    os_fat16_dirent_t entries[4];
    uint8_t payload[11] = {'S', 'U', 'B', '-', 'L', 'F', 'N', '-', 'O', 'K', '!'};
    uint8_t readback[12] = {0U};
    uint32_t first = 0U;
    for (uint32_t i = 0U; i < sizeof(disk); i++) disk[i] = 0U;
    disk[13] = 2U; put16(disk + 11U, 512U); put16(disk + 14U, 32U); disk[16] = 2U;
    put32(disk + 32U, 200000U); put32(disk + 36U, 1000U); put32(disk + 44U, 2U);
    put16(disk + 510U, 0xaa55U);
    disk[32U * 512U + 8U] = 0xf8U; disk[32U * 512U + 9U] = 0xffU;
    disk[32U * 512U + 10U] = 0xffU; disk[32U * 512U + 11U] = 0x0fU;
    TEST_ASSERT_EQUAL(0, fat32_mount(&volume, read_sector, 0U));
    TEST_ASSERT_EQUAL(0, fat32_attach_writer(&volume, write_sector));
    TEST_ASSERT_EQUAL(0, fat32_create_directory(&volume, "DIR1"));
    TEST_ASSERT_EQUAL(0, fat32_create_path_file(&volume, "DIR1/long-subdirectory-file.txt", payload,
                                                sizeof(payload), &first));
    TEST_ASSERT_TRUE(first >= 2U);
    TEST_ASSERT_EQUAL(1, fat32_list_path_page(&volume, "DIR1/", 0U, entries, 4U));
    TEST_ASSERT_EQUAL_STRING("long-subdirectory-file.txt", entries[0].name);
    TEST_ASSERT_EQUAL(11, fat32_read_path(&volume, "DIR1/long-subdirectory-file.txt", readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(payload, readback, sizeof(payload));
    TEST_ASSERT_EQUAL(0, fat32_unlink_path_file(&volume, "DIR1/long-subdirectory-file.txt"));
    TEST_ASSERT_EQUAL(0, fat32_remove_directory(&volume, "DIR1"));
}

int main(void) { unity_init(); RUN_TEST(test_fat32_mount_and_read_cluster); RUN_TEST(test_fat32_lists_root_page_after_first_entry); RUN_TEST(test_fat32_rename_file_root_8_3); RUN_TEST(test_fat32_extend_full_root); RUN_TEST(test_lfn_utf8_bmp_conversion); RUN_TEST(test_fat32_lfn_encoding); RUN_TEST(test_fat32_lfn_file_and_list); RUN_TEST(test_fat32_utf8_lfn_file_roundtrip); RUN_TEST(test_fat32_utf8_lfn_non_bmp_roundtrip); RUN_TEST(test_fat32_mutates_one_level_subdirectory_without_overwrite); RUN_TEST(test_fat32_mutates_multilevel_subdirectory); RUN_TEST(test_fat32_creates_lfn_in_subdirectory); unity_print_results(); unity_cleanup(); return 0; }
