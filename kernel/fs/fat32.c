#include "fat32.h"
#include "lfn_utf8.h"

static fat32_volume_t fat32_root_volume;

fat32_volume_t* fat32_root(void) { return &fat32_root_volume; }

static uint8_t fat32_sector[512];

static uint16_t le16(const uint8_t* p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8U); }
static uint32_t le32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U); }
static void put32(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8U); p[2] = (uint8_t)(v >> 16U); p[3] = (uint8_t)(v >> 24U); }
static int power_of_two(uint8_t value) { return value != 0U && (value & (uint8_t)(value - 1U)) == 0U; }

int fat32_mount(fat32_volume_t* volume, fat16_read_sector_fn read_sector, uint32_t base_lba) {
    uint32_t total, fat_sectors, data_sectors, clusters;
    uint16_t reserved, root_entries, fat16_sectors;
    uint8_t spc, fats;
    if (!volume || !read_sector) return OS_FAT16_CORRUPT;
    volume->mounted = 0U;
    volume->read_sector = read_sector;
    volume->write_sector = 0;

    volume->base_lba = base_lba;
    if (read_sector(base_lba, fat32_sector) != 0) return OS_FAT16_CORRUPT;
    if (le16(fat32_sector + 11U) != 512U || !power_of_two(fat32_sector[13]) || fat32_sector[13] > 128U) return OS_FAT16_CORRUPT;
    reserved = le16(fat32_sector + 14U); fats = fat32_sector[16]; root_entries = le16(fat32_sector + 17U);
    fat16_sectors = le16(fat32_sector + 22U); total = le16(fat32_sector + 19U);
    if (total == 0U) total = le32(fat32_sector + 32U);
    fat_sectors = le32(fat32_sector + 36U);
    if (reserved == 0U || fats == 0U || fats > 2U || root_entries != 0U || fat16_sectors != 0U || total == 0U || fat_sectors == 0U) return OS_FAT16_CORRUPT;
    if (le16(fat32_sector + 510U) != 0xaa55U || le32(fat32_sector + 44U) < 2U) return OS_FAT16_CORRUPT;
    data_sectors = total - (uint32_t)reserved - (uint32_t)fats * fat_sectors;
    spc = fat32_sector[13];
    if (data_sectors == 0U || data_sectors < spc) return OS_FAT16_CORRUPT;
    clusters = data_sectors / spc;
    if (clusters < 65525U || clusters > FAT32_MAX_CLUSTER - 1U) return OS_FAT16_CORRUPT;
    volume->total_sectors = total; volume->fat_lba = base_lba + reserved; volume->fat_sectors = fat_sectors;
    volume->data_lba = volume->fat_lba + (uint32_t)fats * fat_sectors; volume->cluster_count = clusters;
    volume->root_cluster = le32(fat32_sector + 44U) & FAT32_MAX_CLUSTER; volume->bytes_per_sector = 512U;
    volume->sectors_per_cluster = spc; volume->fat_count = fats; volume->mounted = 1U;
    return 0;
}

int fat32_is_mounted(const fat32_volume_t* volume) { return volume && volume->mounted && volume->read_sector; }

int fat32_attach_writer(fat32_volume_t* volume, fat16_write_sector_fn write_sector) {
    if (!volume || !fat32_is_mounted(volume) || !write_sector) return OS_FAT16_NOT_MOUNTED;
    volume->write_sector = write_sector;
    return 0;
}

int fat32_write_fat_entry(const fat32_volume_t* volume, uint32_t cluster, uint32_t next) {
    uint32_t byte_offset, fat, lba, offset, current, updated;
    if (!volume || !fat32_is_mounted(volume) || !volume->write_sector || cluster < 2U || cluster > volume->cluster_count + 1U || next > FAT32_MAX_CLUSTER) return OS_FAT16_CORRUPT;
    byte_offset = cluster * 4U; offset = byte_offset & 511U;
    if (offset > 508U) return OS_FAT16_CORRUPT;
    for (fat = 0U; fat < volume->fat_count; fat++) {
        lba = volume->fat_lba + fat * volume->fat_sectors + (byte_offset >> 9U);
        if (volume->read_sector(lba, fat32_sector) != 0) return OS_FAT16_CORRUPT;
        current = le32(fat32_sector + offset); updated = (current & 0xf0000000U) | next;
        put32(fat32_sector + offset, updated);
        if (volume->write_sector(lba, fat32_sector) != 0) return OS_FAT16_CORRUPT;
    }
    return 0;
}

int fat32_allocate_cluster(const fat32_volume_t* volume, uint32_t* out_cluster) {
    uint32_t cluster, value;
    if (!volume || !out_cluster || !fat32_is_mounted(volume) || !volume->write_sector) return OS_FAT16_NOT_MOUNTED;
    for (cluster = 2U; cluster <= volume->cluster_count + 1U; cluster++) {
        if (fat32_read_fat_entry(volume, cluster, &value) != 0) return OS_FAT16_CORRUPT;
        if (value == 0U && fat32_write_fat_entry(volume, cluster, FAT32_EOC_MIN) == 0) { *out_cluster = cluster; return 0; }
    }
    return OS_FAT16_NOT_FOUND;
}

int fat32_link_clusters(const fat32_volume_t* volume, uint32_t source, uint32_t target) {
    uint32_t source_value, target_value;
    if (!volume || source < 2U || target < 2U || source == target) return OS_FAT16_CORRUPT;
    if (fat32_read_fat_entry(volume, source, &source_value) != 0 || fat32_read_fat_entry(volume, target, &target_value) != 0) return OS_FAT16_CORRUPT;
    if (source_value < FAT32_EOC_MIN || source_value == FAT32_BAD_CLUSTER || target_value == 0U || target_value == FAT32_BAD_CLUSTER) return OS_FAT16_CORRUPT;
    return fat32_write_fat_entry(volume, source, target);
}

int fat32_cluster_lba(const fat32_volume_t* volume, uint32_t cluster, uint32_t* out_lba) {
    if (!fat32_is_mounted(volume) || !out_lba || cluster < 2U || cluster > volume->cluster_count + 1U) return OS_FAT16_CORRUPT;
    *out_lba = volume->data_lba + (cluster - 2U) * volume->sectors_per_cluster;
    return 0;
}

int fat32_read_fat_entry(const fat32_volume_t* volume, uint32_t cluster, uint32_t* out_next) {
    uint32_t byte_offset, lba, offset, value;
    if (!fat32_is_mounted(volume) || !out_next || cluster < 2U || cluster > volume->cluster_count + 1U) return OS_FAT16_CORRUPT;
    byte_offset = cluster * 4U; lba = volume->fat_lba + (byte_offset >> 9U); offset = byte_offset & 511U;
    if (offset > 508U || volume->read_sector(lba, fat32_sector) != 0) return OS_FAT16_CORRUPT;
    value = le32(fat32_sector + offset) & FAT32_MAX_CLUSTER;
    *out_next = value;
    return 0;
}

int fat32_read_cluster(const fat32_volume_t* volume, uint32_t cluster, uint8_t* buffer) {
    uint32_t lba, i;
    if (!buffer || fat32_cluster_lba(volume, cluster, &lba) != 0) return OS_FAT16_CORRUPT;
    for (i = 0U; i < volume->sectors_per_cluster; i++) if (volume->read_sector(lba + i, buffer + i * 512U) != 0) return OS_FAT16_CORRUPT;
    return 0;
}

int fat32_write_cluster(const fat32_volume_t* volume, uint32_t cluster, const uint8_t* buffer) {
    uint32_t lba, i;
    if (!volume || !buffer || !volume->write_sector || fat32_cluster_lba(volume, cluster, &lba) != 0) return OS_FAT16_NOT_MOUNTED;
    for (i = 0U; i < volume->sectors_per_cluster; i++) if (volume->write_sector(lba + i, buffer + i * 512U) != 0) return OS_FAT16_CORRUPT;
    return 0;
}

static int fat32_short_name(const char* input, uint8_t out[11]) {
    uint32_t i = 0U, base = 0U, ext = 0U, dot = 0U;
    if (!input || !input[0]) return OS_FAT16_BAD_PATH;
    for (i = 0U; input[i]; i++) {
        char c = input[i];
        if (c == '/') return OS_FAT16_BAD_PATH;
        if (c == '.') { if (dot || base == 0U) return OS_FAT16_BAD_PATH; dot = 1U; continue; }
        if (c <= ' ' || c == '"' || c == '*' || c == '+' || c == ',' || c == ':' || c == ';' || c == '<' || c == '=' || c == '>' || c == '?' || c == '[' || c == '\\' || c == ']' || c == '|') return OS_FAT16_BAD_PATH;
        if (!dot) { if (base >= 8U) return OS_FAT16_BAD_PATH; out[base++] = (uint8_t)(c >= 'a' && c <= 'z' ? c - 32 : c); }
        else { if (ext >= 3U) return OS_FAT16_BAD_PATH; out[8U + ext++] = (uint8_t)(c >= 'a' && c <= 'z' ? c - 32 : c); }
    }
    if (base == 0U) return OS_FAT16_BAD_PATH;
    while (base < 8U) out[base++] = ' ';
    while (ext < 3U) out[8U + ext++] = ' ';
    return 0;
}

int fat32_create_root_entry(const fat32_volume_t* volume, const char* name, uint8_t attributes, uint32_t first_cluster, uint32_t size) {
    uint8_t short_name[11]; uint32_t cluster, next, lba, sector_index, entry_index, i;
    if (!volume || !fat32_is_mounted(volume) || !volume->write_sector) return OS_FAT16_NOT_MOUNTED;
    if (first_cluster < 2U || first_cluster > volume->cluster_count + 1U) return OS_FAT16_CORRUPT;
    if (fat32_short_name(name, short_name) != 0) return OS_FAT16_BAD_PATH;
    cluster = volume->root_cluster;
    for (i = 0U; i <= volume->cluster_count; i++) {
        if (fat32_cluster_lba(volume, cluster, &lba) != 0) return OS_FAT16_CORRUPT;
        for (sector_index = 0U; sector_index < volume->sectors_per_cluster; sector_index++) {
            if (volume->read_sector(lba + sector_index, fat32_sector) != 0) return OS_FAT16_CORRUPT;
            for (entry_index = 0U; entry_index < 16U; entry_index++) {
                uint8_t* entry = fat32_sector + entry_index * 32U;
                if (entry[0] != 0U && entry[0] != 0xe5U) continue;
                for (uint32_t j = 0U; j < 32U; j++) entry[j] = 0U;
                for (uint32_t j = 0U; j < 11U; j++) entry[j] = short_name[j];
                entry[11] = attributes; entry[20] = (uint8_t)(first_cluster >> 24U); entry[21] = (uint8_t)(first_cluster >> 16U);
                entry[26] = (uint8_t)first_cluster; entry[27] = (uint8_t)(first_cluster >> 8U);
                entry[28] = (uint8_t)size; entry[29] = (uint8_t)(size >> 8U); entry[30] = (uint8_t)(size >> 16U); entry[31] = (uint8_t)(size >> 24U);
                return volume->write_sector(lba + sector_index, fat32_sector) == 0 ? 0 : OS_FAT16_CORRUPT;
            }
        }
        if (fat32_read_fat_entry(volume, cluster, &next) != 0 || next >= FAT32_EOC_MIN) return OS_FAT16_NOT_FOUND;
        cluster = next;
    }
    return OS_FAT16_NOT_FOUND;
}

static uint8_t fat32_file_cluster[128U * 512U];

static void fat32_release_chain(const fat32_volume_t* volume, uint32_t first) {
    uint32_t current = first, next, guard = 0U;
    while (current >= 2U && current <= volume->cluster_count + 1U && guard++ <= volume->cluster_count) {
        if (fat32_read_fat_entry(volume, current, &next) != 0) break;
        (void)fat32_write_fat_entry(volume, current, 0U);
        if (next >= FAT32_EOC_MIN || next == FAT32_BAD_CLUSTER || next == 0U) break;
        current = next;
    }
}

int fat32_create_file(const fat32_volume_t* volume, const char* name, uint8_t attributes, const uint8_t* data, uint32_t size, uint32_t* out_first_cluster) {
    uint32_t first = 0U, previous = 0U, current = 0U, offset = 0U, cluster_bytes, needed, i, take;
    if (!volume || !out_first_cluster || (size != 0U && !data) ||
        !fat32_is_mounted(volume) || !volume->write_sector) return OS_FAT16_NOT_MOUNTED;
    if (fat32_short_name(name, fat32_file_cluster) != 0) return OS_FAT16_BAD_PATH;
    cluster_bytes = (uint32_t)volume->sectors_per_cluster * 512U;
    needed = size == 0U ? 1U : (size + cluster_bytes - 1U) / cluster_bytes;
    if (needed > volume->cluster_count) return OS_FAT16_NOT_FOUND;
    for (i = 0U; i < needed; i++) {
        if (fat32_allocate_cluster(volume, &current) != 0) { if (first) fat32_release_chain(volume, first); return OS_FAT16_NOT_FOUND; }
        if (!first) first = current;
        if (previous && fat32_link_clusters(volume, previous, current) != 0) { fat32_release_chain(volume, first); return OS_FAT16_CORRUPT; }
        previous = current;
        for (uint32_t j = 0U; j < cluster_bytes; j++) fat32_file_cluster[j] = 0U;
        take = size - offset; if (take > cluster_bytes) take = cluster_bytes;
        for (uint32_t j = 0U; j < take; j++) fat32_file_cluster[j] = data[offset + j];
        if (fat32_write_cluster(volume, current, fat32_file_cluster) != 0) { fat32_release_chain(volume, first); return OS_FAT16_CORRUPT; }
        offset += take;
    }
    if (fat32_create_root_entry(volume, name, attributes, first, size) != 0) { fat32_release_chain(volume, first); return OS_FAT16_CORRUPT; }
    *out_first_cluster = first;
    return 0;
}

int fat32_extend_root_directory(const fat32_volume_t* volume, uint32_t* out_cluster) {
    uint32_t last, next, allocated, guard = 0U;
    if (!volume || !out_cluster || !fat32_is_mounted(volume) || !volume->write_sector) return OS_FAT16_NOT_MOUNTED;
    last = volume->root_cluster;
    while (guard++ <= volume->cluster_count) {
        if (fat32_read_fat_entry(volume, last, &next) != 0) return OS_FAT16_CORRUPT;
        if (next >= FAT32_EOC_MIN) break;
        if (next < 2U || next > volume->cluster_count + 1U || next == FAT32_BAD_CLUSTER) return OS_FAT16_CORRUPT;
        last = next;
    }
    if (fat32_allocate_cluster(volume, &allocated) != 0) return OS_FAT16_NOT_FOUND;
    for (uint32_t i = 0U; i < (uint32_t)volume->sectors_per_cluster * 512U; i++) fat32_file_cluster[i] = 0U;
    if (fat32_write_cluster(volume, allocated, fat32_file_cluster) != 0 || fat32_link_clusters(volume, last, allocated) != 0) {
        (void)fat32_write_fat_entry(volume, allocated, 0U);
        return OS_FAT16_CORRUPT;
    }
    *out_cluster = allocated;
    return 0;
}

uint8_t fat32_lfn_checksum(const uint8_t short_name[11]) {
    uint8_t sum = 0U;
    uint32_t i;
    if (!short_name) return 0U;
    for (i = 0U; i < 11U; i++) sum = (uint8_t)(((sum & 1U) ? 0x80U : 0U) + (sum >> 1U) + short_name[i]);
    return sum;
}

int fat32_encode_lfn_entry(const char* name, uint8_t ordinal, uint8_t checksum, uint8_t entry[32]) {
    uint16_t units[13];
    uint32_t length = 0U, i, pos;
    static const uint8_t offsets[13] = {1U,3U,5U,7U,9U,14U,16U,18U,20U,22U,24U,28U,30U};
    if (!name || !entry || ordinal == 0U || (ordinal & 0x1fU) == 0U) return OS_FAT16_BAD_PATH;
    if (lfn_utf8_to_utf16_bmp(name, units, 13U, &length) != 0) return OS_FAT16_BAD_PATH;
    for (i = 0U; i < 32U; i++) entry[i] = 0xffU;
    entry[0] = ordinal; entry[11] = 0x0fU; entry[12] = 0U; entry[13] = checksum; entry[26] = 0U; entry[27] = 0U;
    for (i = 0U; i < 13U; i++) {
        pos = (uint32_t)(ordinal & 0x1fU) * 13U - 13U + i;
        if (pos < length) { entry[offsets[i]] = (uint8_t)units[pos]; entry[offsets[i] + 1U] = (uint8_t)(units[pos] >> 8U); }
        else if (pos == length) { entry[offsets[i]] = 0U; entry[offsets[i] + 1U] = 0U; }
    }
    return 0;
}

static int fat32_dir_slot(const fat32_volume_t* v, uint32_t index, uint8_t entry[32], int write, int extend) {
    uint32_t cluster = v ? v->root_cluster : 0U, next, per_cluster, guard = 0U;
    uint32_t sector_index, entry_index, lba;
    if (!v || !entry || !fat32_is_mounted(v)) return OS_FAT16_NOT_MOUNTED;
    per_cluster = (uint32_t)v->sectors_per_cluster * 16U;
    while (index >= per_cluster) {
        index -= per_cluster;
        if (fat32_read_fat_entry(v, cluster, &next) != 0) return OS_FAT16_CORRUPT;
        if (next >= FAT32_EOC_MIN) {
            if (!extend || fat32_extend_root_directory(v, &next) != 0) return OS_FAT16_NOT_FOUND;
        } else if (next < 2U || next > v->cluster_count + 1U || next == FAT32_BAD_CLUSTER) return OS_FAT16_CORRUPT;
        cluster = next; if (++guard > v->cluster_count) return OS_FAT16_CORRUPT;
    }
    sector_index = index / 16U; entry_index = index % 16U;
    if (fat32_cluster_lba(v, cluster, &lba) != 0 || v->read_sector(lba + sector_index, fat32_sector) != 0) return OS_FAT16_CORRUPT;
    if (write) { for (uint32_t i = 0U; i < 32U; i++) fat32_sector[entry_index * 32U + i] = entry[i]; if (v->write_sector(lba + sector_index, fat32_sector) != 0) return OS_FAT16_CORRUPT; }
    else for (uint32_t i = 0U; i < 32U; i++) entry[i] = fat32_sector[entry_index * 32U + i];
    return 0;
}

static void fat32_lfn_get(const uint8_t entry[32], uint32_t offset, uint32_t pos,
                            uint16_t* units, uint32_t max) {
    uint32_t limit = offset == 1U ? 5U : (offset == 14U ? 6U : 2U);
    for (uint32_t i = 0U; i < limit; i++) {
        uint32_t at = pos + i;
        uint16_t value = (uint16_t)entry[offset + i * 2U] |
                         ((uint16_t)entry[offset + i * 2U + 1U] << 8U);
        if (at >= max || value == 0U || value == 0xffffU) continue;
        units[at] = value;
    }
}

static int fat32_lfn_segment(const uint16_t* units, uint32_t length, uint32_t count,
                             uint32_t ordinal, uint8_t checksum, uint8_t entry[32]) {
    static const uint8_t offsets[13] = {1U,3U,5U,7U,9U,14U,16U,18U,20U,22U,24U,28U,30U};
    uint32_t start = (ordinal - 1U) * 13U;
    if (!units || !entry || ordinal == 0U || ordinal > count) return OS_FAT16_BAD_PATH;
    for (uint32_t i = 0U; i < 32U; i++) entry[i] = 0xffU;
    entry[0] = (uint8_t)ordinal | (ordinal == count ? 0x40U : 0U);
    entry[11] = 0x0fU; entry[12] = 0U; entry[13] = checksum; entry[26] = 0U; entry[27] = 0U;
    for (uint32_t i = 0U; i < 13U; i++) {
        uint32_t at = start + i;
        uint16_t value = at < length ? units[at] : (at == length ? 0U : 0xffffU);
        entry[offsets[i]] = (uint8_t)value;
        entry[offsets[i] + 1U] = (uint8_t)(value >> 8U);
    }
    return 0;
}

int fat32_create_lfn_file(const fat32_volume_t* v, const char* long_name, const char* short_name, uint8_t attributes, const uint8_t* data, uint32_t size, uint32_t* out_first_cluster) {
    uint8_t alias[11], entry[32];
    uint16_t units[OS_NAME_MAX];
    uint32_t length = 0U, count, alias_index = 0xffffffffU, first = 0U, i;
    int rc;
    if (!v || !long_name || !short_name || !out_first_cluster) return OS_FAT16_BAD_PATH;
    if (lfn_utf8_to_utf16_bmp(long_name, units, OS_NAME_MAX, &length) != 0 ||
        length > 13U * 20U || fat32_short_name(short_name, alias) != 0) return OS_FAT16_BAD_PATH;
    count = (length + 12U) / 13U;
    rc = fat32_create_file(v, short_name, attributes, data, size, &first);
    if (rc != 0) return rc;
    for (i = 0U; i < v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U; i++) { if (fat32_dir_slot(v, i, entry, 0, 0) != 0 || entry[0] == 0U) break; int match = 1; for (uint32_t j = 0U; j < 11U; j++) if (entry[j] != alias[j]) { match = 0; break; } if (match) { alias_index = i; break; } }
    if (alias_index == 0xffffffffU) return OS_FAT16_CORRUPT;
    for (i = 0U; i < count + 1U; i++) if (fat32_dir_slot(v, alias_index + 1U + i, entry, 0, 1) != 0 || (entry[0] != 0U && entry[0] != 0xe5U)) return OS_FAT16_NOT_FOUND;
    entry[0] = 0xe5U; rc = fat32_dir_slot(v, alias_index, entry, 1, 0); if (rc != 0) return rc;
    for (i = 0U; i < count; i++) { rc = fat32_lfn_segment(units, length, count, count - i, fat32_lfn_checksum(alias), entry); if (rc != 0) return rc; rc = fat32_dir_slot(v, alias_index + 1U + i, entry, 1, 1); if (rc != 0) return rc; }
    for (i = 0U; i < 32U; i++) entry[i] = 0U;
    for (i = 0U; i < 11U; i++) entry[i] = alias[i];
    entry[11] = attributes; entry[20] = (uint8_t)(first >> 24U); entry[21] = (uint8_t)(first >> 16U);
    entry[26] = (uint8_t)first; entry[27] = (uint8_t)(first >> 8U); put32(entry + 28U, size);
    rc = fat32_dir_slot(v, alias_index + 1U + count, entry, 1, 1); if (rc != 0) return rc; *out_first_cluster = first; return 0;
}


static int fat32_name_equal_folded(const char* left, const char* right) {
    uint32_t i;
    if (!left || !right) return 0;
    for (i = 0U; i < OS_NAME_MAX; i++) {
        char a = left[i], b = right[i];
        if (a >= 'a' && a <= 'z') a = (char)(a - 32);
        if (b >= 'a' && b <= 'z') b = (char)(b - 32);
        if (a != b) return 0;
        if (a == '\0') return 1;
    }
    return 0;
}

static int fat32_lfn_name_equal_folded(const uint16_t* units, const char* name) {
    char decoded[OS_NAME_MAX];
    return lfn_utf16_bmp_to_utf8(units, OS_NAME_MAX, decoded, OS_NAME_MAX) >= 0 &&
           fat32_name_equal_folded(name, decoded);
}

static int fat32_lfn_query_valid(const char* name) {
    uint16_t units[OS_NAME_MAX];
    uint32_t length;
    return lfn_utf8_to_utf16_bmp(name, units, OS_NAME_MAX, &length) == 0;
}

int fat32_unlink_file(const fat32_volume_t* v, const char* name) {
    uint8_t entry[32], short_name[11], lfn_sum = 0U, expected = 0U, valid = 0U;
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t lfn_start = 0U, i, j, limit;
    uint32_t first;
    int short_valid;
    if (!v || !name || !fat32_is_mounted(v) || !v->write_sector) return OS_FAT16_NOT_MOUNTED;
    short_valid = fat32_short_name(name, short_name) == 0;
    if (!fat32_lfn_query_valid(name)) return OS_FAT16_BAD_PATH;
    limit = v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U;
    for (i = 0U; i < limit; i++) {
        uint8_t ord;
        if (fat32_dir_slot(v, i, entry, 0, 0) != 0 || entry[0] == 0U) break;
        if (entry[0] == 0xe5U) { valid = 0U; continue; }
        if (entry[11] == 0x0fU) {
            ord = entry[0] & 0x1fU;
            if (entry[0] & 0x40U) {
                if (ord == 0U || ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; }
                for (j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; expected = ord; lfn_start = i; valid = 1U;
            }
            if (!valid || ord == 0U || ord != expected || entry[13] != lfn_sum) { valid = 0U; continue; }
            fat32_lfn_get(entry, 1U, (ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 14U, (ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 28U, (ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            expected--; continue;
        }
        if (entry[11] & 0x08U) { valid = 0U; continue; }
        { int same = short_valid; for (j = 0U; j < 11U && same; j++) if (entry[j] != short_name[j]) same = 0;
        if ((!same) &&
            !(valid && expected == 0U && fat32_lfn_checksum(entry) == lfn_sum && fat32_lfn_name_equal_folded(lfn_units, name))) { valid = 0U; continue; }
        }
        first = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
        for (j = valid && expected == 0U ? lfn_start : i; j <= i; j++) {
            uint8_t deleted[32];
            if (fat32_dir_slot(v, j, deleted, 0, 0) != 0) return OS_FAT16_CORRUPT;
            deleted[0] = 0xe5U;
            if (fat32_dir_slot(v, j, deleted, 1, 0) != 0) return OS_FAT16_CORRUPT;
        }
        fat32_release_chain(v, first);
        return 0;
    }
    return OS_FAT16_NOT_FOUND;
}

/* Renommage limite a une entree 8.3 classique. La recherche de collision
 * parcourt la racine entiere avant d'ecrire : un emplacement supprime ou une
 * fin logique ne peut donc pas masquer une cible deja presente plus loin. */
int fat32_rename_file(const fat32_volume_t* v, const char* old_name, const char* new_name) {
    uint8_t entry[32], old_short[11], new_short[11];
    uint32_t i, j, limit, old_index;
    uint8_t lfn_pending = 0U;
    if (!v || !old_name || !new_name || !fat32_is_mounted(v) || !v->write_sector) {
        return OS_FAT16_NOT_MOUNTED;
    }
    if (fat32_short_name(old_name, old_short) != 0 || fat32_short_name(new_name, new_short) != 0) {
        return OS_FAT16_BAD_PATH;
    }
    limit = v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U;
    old_index = limit;
    for (i = 0U; i < limit; i++) {
        int same_old = 1, same_new = 1;
        if (fat32_dir_slot(v, i, entry, 0, 0) != 0 || entry[0] == 0U) break;
        if (entry[0] == 0xe5U) { lfn_pending = 0U; continue; }
        if (entry[11] == 0x0fU) { lfn_pending = 1U; continue; }
        if ((entry[11] & 0x18U) != 0U) { lfn_pending = 0U; continue; }
        for (j = 0U; j < 11U; j++) {
            if (entry[j] != old_short[j]) same_old = 0;
            if (entry[j] != new_short[j]) same_new = 0;
        }
        if (same_old) {
            if (lfn_pending) return OS_FAT16_BAD_PATH;
            old_index = i;
        }
        if (same_new && !same_old) return OS_FAT16_BAD_PATH;
        lfn_pending = 0U;
    }
    if (old_index == limit) return OS_FAT16_NOT_FOUND;
    if (fat32_dir_slot(v, old_index, entry, 0, 0) != 0) return OS_FAT16_CORRUPT;
    for (j = 0U; j < 11U; j++) entry[j] = new_short[j];
    if (fat32_dir_slot(v, old_index, entry, 1, 0) != 0) return OS_FAT16_CORRUPT;
    return 0;
}

int fat32_rename_lfn_file(const fat32_volume_t* v, const char* old_name,
                          const char* new_long_name, const char* new_short_name) {
    uint8_t entry[32], old_short[11], new_short[11], sum = 0U, expected = 0U, valid = 0U;
    uint16_t lfn_units[OS_NAME_MAX], units[OS_NAME_MAX];
    uint32_t start = 0U, i, j, limit, old_count = 0U, length = 0U, new_count;
    int old_short_valid;
    if (!v || !old_name || !new_long_name || !new_short_name || !fat32_is_mounted(v) || !v->write_sector) return OS_FAT16_NOT_MOUNTED;
    old_short_valid = fat32_short_name(old_name, old_short) == 0;
    if (fat32_short_name(new_short_name, new_short) != 0) return OS_FAT16_BAD_PATH;
    if (lfn_utf8_to_utf16_bmp(new_long_name, units, OS_NAME_MAX, &length) != 0) return OS_FAT16_BAD_PATH;
    new_count = (length + 12U) / 13U;
    limit = v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U;
    for (i = 0U; i < limit; i++) {
        uint8_t ord;
        if (fat32_dir_slot(v, i, entry, 0, 0) != 0 || entry[0] == 0U) break;
        if (entry[0] == 0xe5U) { valid = 0U; continue; }
        if (entry[11] == 0x0fU) {
            ord = entry[0] & 0x1fU;
            if (entry[0] & 0x40U) { if (ord == 0U || ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; }
                for (j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                sum = entry[13]; expected = ord; start = i; old_count = ord; valid = 1U; }
            if (!valid || ord == 0U || ord != expected || entry[13] != sum) { valid = 0U; continue; }
            fat32_lfn_get(entry, 1U, (ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 14U, (ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 28U, (ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            expected--; continue;
        }
        if (entry[11] & 0x08U) { valid = 0U; continue; }
        { int same = old_short_valid; for (j = 0U; j < 11U && same; j++) if (entry[j] != old_short[j]) same = 0;
          if (!same && !(valid && expected == 0U && fat32_lfn_checksum(entry) == sum && fat32_lfn_name_equal_folded(lfn_units, old_name))) { valid = 0U; continue; } }
        if (!valid || expected != 0U || old_count != new_count) return OS_FAT16_BAD_PATH;
        for (j = 0U; j < new_count; j++) {
            if (fat32_lfn_segment(units, length, new_count, new_count - j, fat32_lfn_checksum(new_short), entry) != 0 ||
                fat32_dir_slot(v, start + j, entry, 1, 0) != 0) return OS_FAT16_CORRUPT;
        }
        if (fat32_dir_slot(v, i, entry, 0, 0) != 0) return OS_FAT16_CORRUPT;
        for (j = 0U; j < 11U; j++) entry[j] = new_short[j];
        if (fat32_dir_slot(v, i, entry, 1, 0) != 0) return OS_FAT16_CORRUPT;
        return 0;
    }
    return OS_FAT16_NOT_FOUND;
}

int fat32_read_file_range(const fat32_volume_t* v, const char* name,
                          uint32_t offset, uint8_t* buffer, uint32_t max,
                          uint32_t* out_read) {
    uint8_t entry[32], short_name[11], lfn_sum = 0U, expected = 0U, valid = 0U;
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t i, j, limit, size, cluster_bytes, skip_clusters, intra, copied = 0U, guard = 0U;
    uint32_t cluster;
    int short_valid;
    if (out_read) *out_read = 0U;
    if (!v || !name || !buffer || max == 0U || !out_read || !fat32_is_mounted(v)) return OS_FAT16_BAD_PATH;
    short_valid = fat32_short_name(name, short_name) == 0;
    if (!fat32_lfn_query_valid(name)) return OS_FAT16_BAD_PATH;
    limit = v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U;
    for (i = 0U; i < limit; i++) {
        uint8_t ord;
        if (fat32_dir_slot(v, i, entry, 0, 0) != 0 || entry[0] == 0U) break;
        if (entry[0] == 0xe5U) { valid = 0U; continue; }
        if (entry[11] == 0x0fU) {
            ord = entry[0] & 0x1fU;
            if (entry[0] & 0x40U) {
                if (ord == 0U || ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; }
                for (j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; expected = ord; valid = 1U;
            }
            if (!valid || ord == 0U || ord != expected || entry[13] != lfn_sum) { valid = 0U; continue; }
            fat32_lfn_get(entry, 1U, (ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 14U, (ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 28U, (ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            expected--; continue;
        }
        if (entry[11] & 0x18U) { valid = 0U; continue; }
        { int match = short_valid; for (j = 0U; j < 11U && match; j++) if (entry[j] != short_name[j]) match = 0;
          if (!match && !(valid && expected == 0U && fat32_lfn_checksum(entry) == lfn_sum && fat32_lfn_name_equal_folded(lfn_units, name))) { valid = 0U; continue; } }
        size = le32(entry + 28U);
        if (offset > size) return OS_FAT16_BAD_PATH;
        cluster = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
        cluster_bytes = (uint32_t)v->sectors_per_cluster * 512U;
        if (cluster_bytes == 0U || cluster_bytes > sizeof(fat32_file_cluster)) return OS_FAT16_CORRUPT;
        skip_clusters = offset / cluster_bytes;
        intra = offset % cluster_bytes;
        while (skip_clusters-- > 0U) {
            if (cluster < 2U || cluster > v->cluster_count + 1U || cluster == FAT32_BAD_CLUSTER ||
                guard++ > v->cluster_count || fat32_read_fat_entry(v, cluster, &cluster) != 0) return OS_FAT16_CORRUPT;
        }
        while (copied < max && offset + copied < size) {
            uint32_t take = cluster_bytes - intra;
            if (cluster < 2U || cluster > v->cluster_count + 1U || cluster == FAT32_BAD_CLUSTER || guard > v->cluster_count) return OS_FAT16_CORRUPT;
            if (fat32_read_cluster(v, cluster, fat32_file_cluster) != 0) return OS_FAT16_CORRUPT;
            if (take > max - copied) take = max - copied;
            if (take > size - offset - copied) take = size - offset - copied;
            for (j = 0U; j < take; j++) buffer[copied + j] = fat32_file_cluster[intra + j];
            copied += take;
            intra += take;
            if (intra >= cluster_bytes && copied < max && offset + copied < size) {
                if (guard++ >= v->cluster_count ||
                    fat32_read_fat_entry(v, cluster, &cluster) != 0) return OS_FAT16_CORRUPT;
                intra = 0U;
            }
        }
        *out_read = copied;
        return 0;
    }
    return OS_FAT16_NOT_FOUND;
}

int fat32_read_file(const fat32_volume_t* v, const char* name, uint8_t* buffer, uint32_t max) {
    uint8_t entry[32], short_name[11], lfn_sum = 0U, expected = 0U, valid = 0U;
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t i, j, limit, size, copied = 0U, cluster_bytes, guard = 0U;
    uint32_t cluster, next;
    int short_valid;
    if (!v || !name || !buffer || max == 0U || !fat32_is_mounted(v)) return OS_FAT16_BAD_PATH;
    short_valid = fat32_short_name(name, short_name) == 0;
    if (!fat32_lfn_query_valid(name)) return OS_FAT16_BAD_PATH;
    limit = v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U;
    for (i = 0U; i < limit; i++) {
        uint8_t ord;
        if (fat32_dir_slot(v, i, entry, 0, 0) != 0 || entry[0] == 0U) break;
        if (entry[0] == 0xe5U) { valid = 0U; continue; }
        if (entry[11] == 0x0fU) {
            ord = entry[0] & 0x1fU;
            if (entry[0] & 0x40U) {
                if (ord == 0U || ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; }
                for (j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; expected = ord; valid = 1U;
            }
            if (!valid || ord == 0U || ord != expected || entry[13] != lfn_sum) { valid = 0U; continue; }
            fat32_lfn_get(entry, 1U, (ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 14U, (ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 28U, (ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            expected--; continue;
        }
        if (entry[11] & 0x18U) { valid = 0U; continue; }
        { int match = short_valid; for (j = 0U; j < 11U && match; j++) if (entry[j] != short_name[j]) match = 0;
          if (!match && !(valid && expected == 0U && fat32_lfn_checksum(entry) == lfn_sum && fat32_lfn_name_equal_folded(lfn_units, name))) { valid = 0U; continue; } }
        size = le32(entry + 28U);
        if (size > max) return OS_FAT16_BUFFER_SMALL;
        cluster = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
        cluster_bytes = (uint32_t)v->sectors_per_cluster * 512U;
        if (cluster_bytes == 0U || cluster_bytes > sizeof(fat32_file_cluster)) return OS_FAT16_CORRUPT;
        while (copied < size) {
            uint32_t take = size - copied;
            if (cluster < 2U || cluster > v->cluster_count + 1U || cluster == FAT32_BAD_CLUSTER || guard++ > v->cluster_count) return OS_FAT16_CORRUPT;
            if (fat32_read_cluster(v, cluster, fat32_file_cluster) != 0) return OS_FAT16_CORRUPT;
            if (take > cluster_bytes) take = cluster_bytes;
            for (j = 0U; j < take; j++) buffer[copied + j] = fat32_file_cluster[j];
            copied += take;
            if (copied < size) {
                if (fat32_read_fat_entry(v, cluster, &next) != 0 || next < 2U || next >= FAT32_EOC_MIN || next == FAT32_BAD_CLUSTER) return OS_FAT16_CORRUPT;
                cluster = next;
            }
        }
        return (int)copied;
    }
    return OS_FAT16_NOT_FOUND;
}

int fat32_list_root_page(const fat32_volume_t* v, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity) {
    uint8_t entry[32], lfn_sum = 0U, expected = 0U, valid = 0U;
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t count = 0U;
    uint32_t seen = 0U;
    if (!v || !out || capacity == 0U || !fat32_is_mounted(v)) return OS_FAT16_BAD_PATH;
    for (uint32_t i = 0U; i < v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U; i++) {
        if (fat32_dir_slot(v, i, entry, 0, 0) != 0 || entry[0] == 0U) break;
        if (entry[0] == 0xe5U) { valid = 0U; continue; }
        if (entry[11] == 0x0fU) { uint8_t ord = entry[0] & 0x1fU; if (entry[0] & 0x40U) { if (ord == 0U || ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; } for (uint32_t j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U; lfn_sum = entry[13]; expected = ord; valid = 1U; } if (!valid || ord == 0U || ord != expected || entry[13] != lfn_sum) { valid = 0U; continue; } fat32_lfn_get(entry, 1U, (ord - 1U) * 13U, lfn_units, OS_NAME_MAX); fat32_lfn_get(entry, 14U, (ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX); fat32_lfn_get(entry, 28U, (ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX); expected--; continue; }
        if (entry[11] & 0x08U) { valid = 0U; continue; }
        if (seen++ < start) { valid = 0U; continue; }
        if (count >= capacity) return (int)count;
        if (!(valid && expected == 0U && fat32_lfn_checksum(entry) == lfn_sum &&
              lfn_utf16_bmp_to_utf8(lfn_units, OS_NAME_MAX, out[count].name, OS_NAME_MAX) >= 0)) {
            uint32_t p = 0U;
            for (uint32_t j = 0U; j < 8U; j++) if (entry[j] != ' ') out[count].name[p++] = (char)entry[j];
            if (entry[8] != ' ') {
                out[count].name[p++] = '.';
                for (uint32_t j = 8U; j < 11U; j++) if (entry[j] != ' ') out[count].name[p++] = (char)entry[j];
            }
            out[count].name[p] = 0;
        }
        out[count].size = le32(entry + 28U); out[count].flags = (entry[11] & 0x10U) ? OS_DIRENT_DIR : OS_DIRENT_FILE; count++; valid = 0U;
    }
    return (int)count;
}

int fat32_list_root(const fat32_volume_t* v, os_fat16_dirent_t* out, uint32_t capacity) {
    return fat32_list_root_page(v, 0U, out, capacity);
}

/* Sous-répertoires FAT32 VFS : un niveau 8.3 est publié. Les LFN restent
 * volontairement limitées aux opérations racine déjà validées. */
static uint32_t fat32_text_length(const char* text) {
    uint32_t length = 0U;
    while (text && text[length] != '\0') length++;
    return length;
}

static int fat32_entry_matches(const uint8_t entry[32], const uint8_t short_name[11]) {
    uint32_t i;
    for (i = 0U; i < 11U; i++) {
        if (entry[i] != short_name[i]) return 0;
    }
    return 1;
}

static int fat32_child_find_short(const fat32_volume_t* v, uint32_t cluster,
                                  const char* name, uint8_t entry_out[32],
                                  uint32_t* index_out);

static void make_short_alias_fat32(const char* name, char* out_alias);

static int fat32_child_create_lfn(const fat32_volume_t* v, uint32_t cluster,
                                  const char* long_name, const char* short_name,
                                  uint8_t attributes, uint32_t first_cluster,
                                  uint32_t size);

static int fat32_root_find_short(const fat32_volume_t* v, const char* name,
                                 uint8_t entry_out[32], uint32_t* index_out) {
    uint8_t short_name[11];
    uint8_t entry[32];
    uint32_t limit;
    uint32_t index;
    if (!v || !name || !entry_out || !index_out || fat32_short_name(name, short_name) != 0) {
        return OS_FAT16_BAD_PATH;
    }
    limit = v->cluster_count * (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        if (fat32_dir_slot(v, index, entry, 0, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U || entry[11] == 0x0FU) continue;
        if (fat32_entry_matches(entry, short_name)) {
            uint32_t i;
            for (i = 0U; i < 32U; i++) entry_out[i] = entry[i];
            *index_out = index;
            return 0;
        }
    }
    return OS_FAT16_NOT_FOUND;
}

static int fat32_directory_cluster(const fat32_volume_t* v, const char* name,
                                   uint32_t* cluster_out) {
    uint8_t entry[32];
    uint32_t index, i = 0U;
    uint32_t current_cluster = 0U;
    char segment[OS_NAME_MAX];
    int rc;

    if (!v || !name || name[0] == '\0' || name[0] == '/') return OS_FAT16_BAD_PATH;

    while (1) {
        uint32_t seg_len = 0U;
        while (name[i] != '\0' && name[i] != '/') {
            if (seg_len + 1U >= OS_NAME_MAX) return OS_FAT16_BAD_PATH;
            segment[seg_len++] = name[i++];
        }
        segment[seg_len] = '\0';
        if (seg_len == 0U) return OS_FAT16_BAD_PATH;

        if (current_cluster == 0U) {
            rc = fat32_root_find_short(v, segment, entry, &index);
        } else {
            rc = fat32_child_find_short(v, current_cluster, segment, entry, &index);
        }
        if (rc != 0) return rc;
        if ((entry[11] & 0x10U) == 0U) return OS_FAT16_BAD_PATH;

        current_cluster = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
        if (current_cluster < 2U || current_cluster > v->cluster_count + 1U) {
            return OS_FAT16_CORRUPT;
        }

        if (name[i] == '\0') break;
        i++; // Sauter '/'
        if (name[i] == '\0') break; // Trailing slash admis
    }

    *cluster_out = current_cluster;
    return 0;
}

static int fat32_split_subpath(const char* path, char* directory, char* leaf) {
    uint32_t i = 0U;
    uint32_t last_slash = 0xffffffffU;
    uint32_t length;
    if (!path || !directory || !leaf || path[0] == '\0' || path[0] == '/') return OS_FAT16_BAD_PATH;
    while (path[i] != '\0') {
        if (i + 1U >= OS_NAME_MAX * 2U) return OS_FAT16_BAD_PATH;
        if (path[i] == '/') {
            if (i == 0U || path[i + 1U] == '\0') return OS_FAT16_BAD_PATH;
            last_slash = i;
        }
        i++;
    }
    length = i;
    if (last_slash == 0xffffffffU) return OS_FAT16_NOT_FOUND;
    if (last_slash >= OS_NAME_MAX * 2U || length - last_slash - 1U >= OS_NAME_MAX) return OS_FAT16_BAD_PATH;
    for (i = 0U; i < last_slash; i++) directory[i] = path[i];
    directory[last_slash] = '\0';
    for (i = 0U; i < length - last_slash - 1U; i++) leaf[i] = path[last_slash + 1U + i];
    leaf[length - last_slash - 1U] = '\0';
    return 0;
}

static int fat32_child_slot(const fat32_volume_t* v, uint32_t cluster,
                            uint32_t index, uint8_t entry[32], int write) {
    uint32_t per_cluster;
    uint32_t lba;
    uint32_t sector_index;
    uint32_t entry_index;
    uint32_t i;
    if (!v || !entry || cluster < 2U || cluster > v->cluster_count + 1U) return OS_FAT16_CORRUPT;
    per_cluster = (uint32_t)v->sectors_per_cluster * 16U;
    if (index >= per_cluster) return OS_FAT16_NOT_FOUND;
    sector_index = index / 16U;
    entry_index = index % 16U;
    if (fat32_cluster_lba(v, cluster, &lba) != 0 || v->read_sector(lba + sector_index, fat32_sector) != 0) {
        return OS_FAT16_CORRUPT;
    }
    if (!write) {
        for (i = 0U; i < 32U; i++) entry[i] = fat32_sector[entry_index * 32U + i];
        return 0;
    }
    for (i = 0U; i < 32U; i++) fat32_sector[entry_index * 32U + i] = entry[i];
    return v->write_sector(lba + sector_index, fat32_sector) == 0 ? 0 : OS_FAT16_CORRUPT;
}

static int fat32_child_find_short(const fat32_volume_t* v, uint32_t cluster,
                                  const char* name, uint8_t entry_out[32],
                                  uint32_t* index_out) {
    uint8_t short_name[11];
    uint8_t entry[32];
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t index, limit;
    uint8_t lfn_sum = 0U, expected = 0U, valid = 0U;
    int short_valid;
    if (!v || !name || !entry_out || !index_out || !fat32_lfn_query_valid(name)) {
        return OS_FAT16_BAD_PATH;
    }
    short_valid = fat32_short_name(name, short_name) == 0;
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        uint8_t ord;
        if (fat32_child_slot(v, cluster, index, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U) { valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ord = entry[0] & 0x1FU;
            if ((entry[0] & 0x40U) != 0U) {
                if (ord == 0U || (uint32_t)ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; }
                for (uint32_t j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; expected = ord; valid = 1U;
            }
            if (!valid || ord == 0U || ord != expected || entry[13] != lfn_sum) { valid = 0U; continue; }
            fat32_lfn_get(entry, 1U, (uint32_t)(ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 14U, (uint32_t)(ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 28U, (uint32_t)(ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            expected--;
            continue;
        }
        if ((entry[11] & 0x08U) != 0U) { valid = 0U; continue; }
        if ((short_valid && fat32_entry_matches(entry, short_name)) ||
            (valid && expected == 0U && fat32_lfn_checksum(entry) == lfn_sum &&
             fat32_lfn_name_equal_folded(lfn_units, name))) {
            uint32_t i;
            for (i = 0U; i < 32U; i++) entry_out[i] = entry[i];
            *index_out = index;
            return 0;
        }
        valid = 0U;
    }
    return OS_FAT16_NOT_FOUND;
}

static void make_short_alias_fat32(const char* name, char* out_alias) {
    uint32_t i = 0U, base = 0U, ext = 0U;
    while (name[i] != '\0' && name[i] != '.') {
        char c = name[i++];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            if (base < 6U) out_alias[base++] = c;
        }
    }
    if (base == 0U) {
        out_alias[base++] = 'F'; out_alias[base++] = 'I'; out_alias[base++] = 'L'; out_alias[base++] = 'E';
    }
    out_alias[base++] = '~'; out_alias[base++] = '1';
    if (name[i] == '.') {
        i++;
        while (name[i] != '\0' && ext < 3U) {
            char c = name[i++];
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                if (ext == 0U) out_alias[base++] = '.';
                out_alias[base++] = c;
                ext++;
            }
        }
    }
    if (ext == 0U) {
        out_alias[base++] = '.'; out_alias[base++] = 'T'; out_alias[base++] = 'X'; out_alias[base++] = 'T';
    }
    out_alias[base] = '\0';
}

static int fat32_child_create_short(const fat32_volume_t* v, uint32_t cluster,
                                    const char* name, uint8_t attributes,
                                    uint32_t first_cluster, uint32_t size) {
    uint8_t short_name[11];
    uint8_t entry[32];
    uint32_t index;
    uint32_t free_index = 0xffffffffU;
    uint32_t limit;
    if (!v || !name || (attributes & 0x0FU) == 0x0FU ||
        (first_cluster != 0U && (first_cluster < 2U || first_cluster > v->cluster_count + 1U)) ||
        fat32_short_name(name, short_name) != 0) {
        return OS_FAT16_BAD_PATH;
    }
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        if (fat32_child_slot(v, cluster, index, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) { free_index = index; break; }
        if (entry[0] == 0xE5U) { if (free_index == 0xffffffffU) free_index = index; continue; }
        if (entry[11] != 0x0FU && fat32_entry_matches(entry, short_name)) return OS_FAT16_BAD_PATH;
    }
    if (free_index == 0xffffffffU) return OS_FAT16_NOT_FOUND;
    for (index = 0U; index < 32U; index++) entry[index] = 0U;
    for (index = 0U; index < 11U; index++) entry[index] = short_name[index];
    entry[11] = attributes;
    entry[20] = (uint8_t)(first_cluster >> 24U);
    entry[21] = (uint8_t)(first_cluster >> 16U);
    entry[26] = (uint8_t)first_cluster;
    entry[27] = (uint8_t)(first_cluster >> 8U);
    put32(entry + 28U, size);
    return fat32_child_slot(v, cluster, free_index, entry, 1);
}

static int fat32_child_create_lfn(const fat32_volume_t* v, uint32_t cluster,
                                  const char* long_name, const char* short_name,
                                  uint8_t attributes, uint32_t first_cluster,
                                  uint32_t size) {
    uint8_t alias[11], entry[32];
    uint16_t units[OS_NAME_MAX];
    uint32_t length = 0U, i, count, start_slot = 0U, consecutive = 0U;
    uint32_t limit = (uint32_t)v->sectors_per_cluster * 16U;
    int rc;

    if (!v || !long_name || !short_name) return OS_FAT16_BAD_PATH;
    if (lfn_utf8_to_utf16_bmp(long_name, units, OS_NAME_MAX, &length) != 0 ||
        fat32_short_name(short_name, alias) != 0) return OS_FAT16_BAD_PATH;
    count = (length + 12U) / 13U;
    if (count == 0U || count > 20U) return OS_FAT16_BAD_PATH;

    for (i = 0U; i < limit; i++) {
        if (fat32_child_slot(v, cluster, i, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] != 0U && entry[0] != 0xE5U) {
            if (fat32_entry_matches(entry, alias)) return OS_FAT16_BAD_PATH;
            consecutive = 0U;
        } else {
            if (consecutive == 0U) start_slot = i;
            consecutive++;
            if (consecutive == count + 1U) break;
        }
    }
    if (consecutive < count + 1U) return OS_FAT16_NOT_FOUND;

    for (i = 0U; i < count; i++) {
        rc = fat32_lfn_segment(units, length, count, count - i, fat32_lfn_checksum(alias), entry);
        if (rc != 0) return rc;
        rc = fat32_child_slot(v, cluster, start_slot + i, entry, 1);
        if (rc != 0) return rc;
    }

    for (i = 0U; i < 32U; i++) entry[i] = 0U;
    for (i = 0U; i < 11U; i++) entry[i] = alias[i];
    entry[11] = attributes;
    entry[20] = (uint8_t)(first_cluster >> 24U);
    entry[21] = (uint8_t)(first_cluster >> 16U);
    entry[26] = (uint8_t)first_cluster;
    entry[27] = (uint8_t)(first_cluster >> 8U);
    put32(entry + 28U, size);
    return fat32_child_slot(v, cluster, start_slot + count, entry, 1);
}

static int fat32_read_entry_data(const fat32_volume_t* v, const uint8_t entry[32],
                                 uint8_t* buffer, uint32_t max) {
    uint32_t size;
    uint32_t copied = 0U;
    uint32_t cluster;
    uint32_t next;
    uint32_t cluster_bytes;
    uint32_t guard = 0U;
    if (!v || !entry || !buffer || max == 0U) return OS_FAT16_BAD_PATH;
    if (entry[11] & 0x10U) return OS_FAT16_BAD_PATH;
    size = le32(entry + 28U);
    if (size > max) return OS_FAT16_BUFFER_SMALL;
    cluster = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
    cluster_bytes = (uint32_t)v->sectors_per_cluster * 512U;
    if (cluster_bytes == 0U || cluster_bytes > sizeof(fat32_file_cluster)) return OS_FAT16_CORRUPT;
    while (copied < size) {
        uint32_t take = size - copied;
        uint32_t i;
        if (cluster < 2U || cluster > v->cluster_count + 1U || cluster == FAT32_BAD_CLUSTER ||
            guard++ > v->cluster_count || fat32_read_cluster(v, cluster, fat32_file_cluster) != 0) {
            return OS_FAT16_CORRUPT;
        }
        if (take > cluster_bytes) take = cluster_bytes;
        for (i = 0U; i < take; i++) buffer[copied + i] = fat32_file_cluster[i];
        copied += take;
        if (copied < size) {
            if (fat32_read_fat_entry(v, cluster, &next) != 0 || next < 2U ||
                next >= FAT32_EOC_MIN || next == FAT32_BAD_CLUSTER) return OS_FAT16_CORRUPT;
            cluster = next;
        }
    }
    return (int)copied;
}

static int fat32_create_child_file(const fat32_volume_t* v, uint32_t directory,
                                   const char* name, const uint8_t* data, uint32_t size,
                                   uint32_t* out_first_cluster) {
    uint32_t first = 0U;
    uint32_t previous = 0U;
    uint32_t current = 0U;
    uint32_t offset = 0U;
    uint32_t cluster_bytes;
    uint32_t needed;
    uint32_t i;
    uint8_t dummy_short[11];
    if (!v || !name || !out_first_cluster || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    cluster_bytes = (uint32_t)v->sectors_per_cluster * 512U;
    if (cluster_bytes == 0U || cluster_bytes > sizeof(fat32_file_cluster)) return OS_FAT16_CORRUPT;
    needed = size == 0U ? 1U : (size + cluster_bytes - 1U) / cluster_bytes;
    for (i = 0U; i < needed; i++) {
        uint32_t j;
        uint32_t take;
        if (fat32_allocate_cluster(v, &current) != 0) { if (first) fat32_release_chain(v, first); return OS_FAT16_NOT_FOUND; }
        if (!first) first = current;
        if (previous && fat32_link_clusters(v, previous, current) != 0) {
            fat32_release_chain(v, first); return OS_FAT16_CORRUPT;
        }
        for (j = 0U; j < cluster_bytes; j++) fat32_file_cluster[j] = 0U;
        take = size - offset;
        if (take > cluster_bytes) take = cluster_bytes;
        for (j = 0U; j < take; j++) fat32_file_cluster[j] = data[offset + j];
        if (fat32_write_cluster(v, current, fat32_file_cluster) != 0) {
            fat32_release_chain(v, first); return OS_FAT16_CORRUPT;
        }
        offset += take;
        previous = current;
    }
    if (fat32_short_name(name, dummy_short) == 0) {
        if (fat32_child_create_short(v, directory, name, 0x20U, first, size) != 0) {
            fat32_release_chain(v, first); return OS_FAT16_BAD_PATH;
        }
    } else {
        char short_alias[16];
        make_short_alias_fat32(name, short_alias);
        if (fat32_child_create_lfn(v, directory, name, short_alias, 0x20U, first, size) != 0) {
            fat32_release_chain(v, first); return OS_FAT16_BAD_PATH;
        }
    }
    *out_first_cluster = first;
    return 0;
}

int fat32_list_path_page(const fat32_volume_t* v, const char* path, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    char lfn[OS_NAME_MAX];
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t lfn_length = 0U;
    uint8_t lfn_sum = 0U, lfn_expected = 0U, lfn_valid = 0U;
    uint32_t cluster;
    uint32_t index;
    uint32_t seen = 0U;
    uint32_t count = 0U;
    uint32_t limit;
    uint8_t entry[32];
    int rc;
    if (!v || !path || !out || capacity == 0U) return OS_FAT16_BAD_PATH;
    if (path[0] == '/' && path[1] == '\0') return fat32_list_root_page(v, start, out, capacity);
    if (path[0] == '\0' || path[fat32_text_length(path) - 1U] != '/') return OS_FAT16_BAD_PATH;
    index = fat32_text_length(path) - 1U;
    if (index == 0U || index >= OS_NAME_MAX) return OS_FAT16_BAD_PATH;
    for (uint32_t i = 0U; i < index; i++) directory[i] = path[i];
    directory[index] = '\0';
    (void)leaf;
    if ((rc = fat32_directory_cluster(v, directory, &cluster)) != 0) return rc;
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        uint8_t ord;
        if (fat32_child_slot(v, cluster, index, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U) { lfn_valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ord = entry[0] & 0x1FU;
            if ((entry[0] & 0x40U) != 0U) {
                lfn_length = (uint32_t)ord * 13U;
                if (lfn_length >= OS_NAME_MAX) { lfn_valid = 0U; continue; }
                for (uint32_t j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; lfn_expected = ord; lfn_valid = 1U;
            }
            if (!lfn_valid || ord == 0U || ord != lfn_expected || entry[13] != lfn_sum) {
                lfn_valid = 0U; continue;
            }
            fat32_lfn_get(entry, 1U, (uint32_t)(ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 14U, (uint32_t)(ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat32_lfn_get(entry, 28U, (uint32_t)(ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            lfn_expected--;
            continue;
        }
        if (entry[11] & 0x08U || entry[0] == '.') { lfn_valid = 0U; continue; }
        if (seen++ < start) { lfn_valid = 0U; continue; }
        if (count >= capacity) return (int)count;
        if (!(lfn_valid && lfn_expected == 0U && fat32_lfn_checksum(entry) == lfn_sum &&
              lfn_utf16_bmp_to_utf8(lfn_units, OS_NAME_MAX, lfn, OS_NAME_MAX) >= 0)) {
            uint32_t p = 0U;
            for (uint32_t j = 0U; j < 8U; j++) if (entry[j] != ' ') out[count].name[p++] = (char)entry[j];
            if (entry[8] != ' ') { out[count].name[p++] = '.'; for (uint32_t j = 8U; j < 11U; j++) if (entry[j] != ' ') out[count].name[p++] = (char)entry[j]; }
            out[count].name[p] = '\0';
        } else {
            for (uint32_t j = 0U; j < OS_NAME_MAX; j++) out[count].name[j] = lfn[j];
        }
        out[count].size = le32(entry + 28U);
        out[count].flags = (entry[11] & 0x10U) ? OS_DIRENT_DIR : OS_DIRENT_FILE;
        count++;
        lfn_valid = 0U;
    }
    return (int)count;
}

int fat32_read_path(const fat32_volume_t* v, const char* path, uint8_t* buffer, uint32_t max) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    uint8_t entry[32];
    uint32_t cluster;
    uint32_t index;
    int rc = fat32_split_subpath(path, directory, leaf);
    if (rc == OS_FAT16_NOT_FOUND) return fat32_read_file(v, path, buffer, max);
    if (rc != 0 || (rc = fat32_directory_cluster(v, directory, &cluster)) != 0) return rc;
    if ((rc = fat32_child_find_short(v, cluster, leaf, entry, &index)) != 0) return rc;
    return fat32_read_entry_data(v, entry, buffer, max);
}

int fat32_create_path_file(const fat32_volume_t* v, const char* path,
                           const uint8_t* data, uint32_t size, uint32_t* out_first_cluster) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    uint32_t cluster;
    int rc = fat32_split_subpath(path, directory, leaf);
    if (rc == OS_FAT16_NOT_FOUND) return fat32_create_file(v, path, 0x20U, data, size, out_first_cluster);
    if (rc != 0 || (rc = fat32_directory_cluster(v, directory, &cluster)) != 0) return rc;
    return fat32_create_child_file(v, cluster, leaf, data, size, out_first_cluster);
}

int fat32_unlink_path_file(const fat32_volume_t* v, const char* path) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    uint8_t entry[32];
    uint32_t cluster;
    uint32_t index;
    uint32_t first;
    uint8_t lfn_sum;
    int32_t start_index;
    int rc = fat32_split_subpath(path, directory, leaf);
    if (rc == OS_FAT16_NOT_FOUND) return fat32_unlink_file(v, path);
    if (rc != 0 || (rc = fat32_directory_cluster(v, directory, &cluster)) != 0) return rc;
    if ((rc = fat32_child_find_short(v, cluster, leaf, entry, &index)) != 0) return rc;
    if (entry[11] & 0x10U) return OS_FAT16_BAD_PATH;
    first = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
    lfn_sum = fat32_lfn_checksum(entry);
    start_index = (int32_t)index;
    while (start_index > 0) {
        uint8_t prev[32];
        if (fat32_child_slot(v, cluster, (uint32_t)(start_index - 1), prev, 0) != 0) break;
        if (prev[11] == 0x0FU && prev[13] == lfn_sum && prev[0] != 0xE5U) {
            start_index--;
            if ((prev[0] & 0x40U) != 0U) break;
        } else {
            break;
        }
    }
    for (int32_t j = start_index; j <= (int32_t)index; j++) {
        uint8_t del[32];
        if (fat32_child_slot(v, cluster, (uint32_t)j, del, 0) == 0) {
            del[0] = 0xE5U;
            (void)fat32_child_slot(v, cluster, (uint32_t)j, del, 1);
        }
    }
    if (first != 0U) fat32_release_chain(v, first);
    return 0;
}

int fat32_rename_path_file(const fat32_volume_t* v, const char* old_path, const char* new_path) {
    char old_directory[OS_NAME_MAX];
    char old_leaf[OS_NAME_MAX];
    char new_directory[OS_NAME_MAX];
    char new_leaf[OS_NAME_MAX];
    uint32_t cluster;
    uint32_t old_index;
    uint32_t new_index;
    uint8_t entry[32];
    uint8_t existing[32];
    uint8_t short_name[11];
    int old_rc = fat32_split_subpath(old_path, old_directory, old_leaf);
    int new_rc = fat32_split_subpath(new_path, new_directory, new_leaf);
    if (old_rc == OS_FAT16_NOT_FOUND && new_rc == OS_FAT16_NOT_FOUND) return fat32_rename_file(v, old_path, new_path);
    if (old_rc != 0 || new_rc != 0 || !fat32_name_equal_folded(old_directory, new_directory) ||
        (old_rc = fat32_directory_cluster(v, old_directory, &cluster)) != 0 ||
        fat32_child_find_short(v, cluster, old_leaf, entry, &old_index) != 0 || entry[11] & 0x10U ||
        fat32_short_name(new_leaf, short_name) != 0) return OS_FAT16_BAD_PATH;
    if (fat32_child_find_short(v, cluster, new_leaf, existing, &new_index) == 0 && new_index != old_index) {
        return OS_FAT16_BAD_PATH;
    }
    for (uint32_t i = 0U; i < 11U; i++) entry[i] = short_name[i];
    return fat32_child_slot(v, cluster, old_index, entry, 1);
}

int fat32_create_directory(const fat32_volume_t* v, const char* name) {
    char directory[OS_NAME_MAX * 2U];
    char leaf[OS_NAME_MAX];
    uint32_t parent_cluster = 0U;
    uint32_t cluster;
    uint32_t index;
    uint8_t entry[32];
    int rc = fat32_split_subpath(name, directory, leaf);

    if (rc == OS_FAT16_NOT_FOUND) {
        if (!v || !name || fat32_short_name(name, fat32_file_cluster) != 0) return OS_FAT16_BAD_PATH;
        if (fat32_root_find_short(v, name, entry, &index) == 0) return OS_FAT16_BAD_PATH;
        if ((rc = fat32_allocate_cluster(v, &cluster)) != 0) return rc;
        for (index = 0U; index < (uint32_t)v->sectors_per_cluster * 512U; index++) fat32_file_cluster[index] = 0U;
        if (fat32_write_cluster(v, cluster, fat32_file_cluster) != 0) { fat32_release_chain(v, cluster); return OS_FAT16_CORRUPT; }
        for (index = 0U; index < 32U; index++) entry[index] = 0U;
        for (index = 0U; index < 11U; index++) entry[index] = ' ';
        entry[0] = '.'; entry[11] = 0x10U; entry[20] = (uint8_t)(cluster >> 24U); entry[21] = (uint8_t)(cluster >> 16U);
        entry[26] = (uint8_t)cluster; entry[27] = (uint8_t)(cluster >> 8U);
        if ((rc = fat32_child_slot(v, cluster, 0U, entry, 1)) != 0) { fat32_release_chain(v, cluster); return rc; }
        entry[1] = '.'; entry[20] = (uint8_t)(v->root_cluster >> 24U); entry[21] = (uint8_t)(v->root_cluster >> 16U);
        entry[26] = (uint8_t)v->root_cluster; entry[27] = (uint8_t)(v->root_cluster >> 8U);
        if ((rc = fat32_child_slot(v, cluster, 1U, entry, 1)) != 0 ||
            (rc = fat32_create_root_entry(v, name, 0x10U, cluster, 0U)) != 0) {
            fat32_release_chain(v, cluster); return rc;
        }
        return 0;
    }
    if (rc != 0) return rc;

    rc = fat32_directory_cluster(v, directory, &parent_cluster);
    if (rc != 0) return rc;
    if (fat32_short_name(leaf, fat32_file_cluster) != 0) return OS_FAT16_BAD_PATH;
    if (fat32_child_find_short(v, parent_cluster, leaf, entry, &index) == 0) return OS_FAT16_BAD_PATH;

    if ((rc = fat32_allocate_cluster(v, &cluster)) != 0) return rc;
    for (index = 0U; index < (uint32_t)v->sectors_per_cluster * 512U; index++) fat32_file_cluster[index] = 0U;
    if (fat32_write_cluster(v, cluster, fat32_file_cluster) != 0) { fat32_release_chain(v, cluster); return OS_FAT16_CORRUPT; }
    for (index = 0U; index < 32U; index++) entry[index] = 0U;
    for (index = 0U; index < 11U; index++) entry[index] = ' ';
    entry[0] = '.'; entry[11] = 0x10U; entry[20] = (uint8_t)(cluster >> 24U); entry[21] = (uint8_t)(cluster >> 16U);
    entry[26] = (uint8_t)cluster; entry[27] = (uint8_t)(cluster >> 8U);
    if ((rc = fat32_child_slot(v, cluster, 0U, entry, 1)) != 0) { fat32_release_chain(v, cluster); return rc; }
    entry[1] = '.'; entry[20] = (uint8_t)(parent_cluster >> 24U); entry[21] = (uint8_t)(parent_cluster >> 16U);
    entry[26] = (uint8_t)parent_cluster; entry[27] = (uint8_t)(parent_cluster >> 8U);
    if ((rc = fat32_child_slot(v, cluster, 1U, entry, 1)) != 0 ||
        (rc = fat32_child_create_short(v, parent_cluster, leaf, 0x10U, cluster, 0U)) != 0) {
        fat32_release_chain(v, cluster); return rc;
    }
    return 0;
}

int fat32_remove_directory(const fat32_volume_t* v, const char* name) {
    char directory[OS_NAME_MAX * 2U];
    char leaf[OS_NAME_MAX];
    uint8_t entry[32];
    uint32_t dir_index;
    uint32_t parent_cluster = 0U;
    uint32_t cluster;
    uint32_t index;
    uint32_t limit;
    int rc = fat32_split_subpath(name, directory, leaf);

    if (rc == OS_FAT16_NOT_FOUND) {
        rc = fat32_root_find_short(v, name, entry, &dir_index);
        if (rc != 0) return rc;
        if ((entry[11] & 0x10U) == 0U) return OS_FAT16_BAD_PATH;
        cluster = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
        limit = (uint32_t)v->sectors_per_cluster * 16U;
        for (index = 0U; index < limit; index++) {
            uint8_t child[32];
            if (fat32_child_slot(v, cluster, index, child, 0) != 0) return OS_FAT16_CORRUPT;
            if (child[0] == 0U) break;
            if (child[0] == 0xE5U || child[0] == '.') continue;
            return OS_FAT16_BAD_PATH;
        }
        entry[0] = 0xE5U;
        if ((rc = fat32_dir_slot(v, dir_index, entry, 1, 0)) != 0) return rc;
        fat32_release_chain(v, cluster);
        return 0;
    }
    if (rc != 0) return rc;

    rc = fat32_directory_cluster(v, directory, &parent_cluster);
    if (rc != 0) return rc;
    rc = fat32_child_find_short(v, parent_cluster, leaf, entry, &dir_index);
    if (rc != 0) return rc;
    if ((entry[11] & 0x10U) == 0U) return OS_FAT16_BAD_PATH;
    cluster = ((uint32_t)entry[20] << 24U) | ((uint32_t)entry[21] << 16U) | le16(entry + 26U);
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        uint8_t child[32];
        if (fat32_child_slot(v, cluster, index, child, 0) != 0) return OS_FAT16_CORRUPT;
        if (child[0] == 0U) break;
        if (child[0] == 0xE5U || child[0] == '.') continue;
        return OS_FAT16_BAD_PATH;
    }
    entry[0] = 0xE5U;
    if ((rc = fat32_child_slot(v, parent_cluster, dir_index, entry, 1)) != 0) return rc;
    fat32_release_chain(v, cluster);
    return 0;
}
