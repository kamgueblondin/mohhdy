#include "fat16.h"
#include "lfn_utf8.h"

#define FAT16_SECTOR_SIZE 512U
#define FAT16_ENTRY_SIZE 32U
#define FAT16_EOC_MIN 0xFFF8U
#define FAT16_BAD_CLUSTER 0xFFF7U
#define FAT16_MAX_ROOT_ENTRIES 512U
#define FAT16_MAX_CLUSTERS 65525U

static uint8_t sector[FAT16_SECTOR_SIZE];
static uint8_t sector2[FAT16_SECTOR_SIZE];

/* Forward declarations pour LFN */
static int fat16_write_root_slot(const fat16_volume_t* v, uint32_t index, const uint8_t* entry);
static uint8_t fat16_lfn_checksum(const uint8_t* short_name);
static void fat16_lfn_put(uint8_t* entry, uint32_t offset, uint32_t pos, const uint16_t* units, uint32_t length);
static void fat16_lfn_get(uint8_t* entry, uint32_t offset, uint32_t pos, uint16_t* units, uint32_t max);
static int fat16_lfn_query_valid(const char* name);
static int fat16_lfn_name_equals_folded(const uint16_t* units, const char* name);
static uint8_t fat_sector_cache[FAT16_SECTOR_SIZE];
static uint32_t fat_sector_cache_lba;
static uint8_t fat_sector_cache_valid;
static const char* status_text = "FAT16: non monte";
static fat16_volume_t root_volume;

static uint16_t le16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t* p) {
    return (uint32_t)le16(p) | ((uint32_t)le16(p + 2) << 16);
}

static void copy_name(char* out, const uint8_t* entry) {
    uint32_t i;
    uint32_t pos = 0U;
    for (i = 0U; i < 8U && entry[i] != ' '; i++) out[pos++] = (char)entry[i];
    if (entry[8] != ' ' && entry[8] != 0U) {
        out[pos++] = '.';
        for (i = 8U; i < 11U && entry[i] != ' '; i++) out[pos++] = (char)entry[i];
    }
    out[pos] = '\0';
}

static char upper_ascii(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
    return c;
}

static uint32_t fat16_text_length(const char* text) {
    uint32_t length = 0U;
    while (text && text[length] != '\0') length++;
    return length;
}

static int make_short_name(const char* name, uint8_t* out) {
    uint32_t i = 0U;
    uint32_t base = 0U;
    uint32_t ext = 0U;
    if (!name || !out || !name[0]) return OS_FAT16_BAD_PATH;
    for (i = 0U; i < 11U; i++) out[i] = ' ';
    i = 0U;
    while (name[i] && name[i] != '.' && base < 8U) {
        char c = upper_ascii(name[i++]);
        if (c < 0x21 || c == '/' || c == '\\') return OS_FAT16_BAD_PATH;
        out[base++] = (uint8_t)c;
    }
    if (!name[i]) return base ? 0 : OS_FAT16_BAD_PATH;
    if (name[i++] != '.') return OS_FAT16_BAD_PATH;
    while (name[i] && ext < 3U) {
        char c = upper_ascii(name[i++]);
        if (c < 0x21 || c == '.' || c == '/' || c == '\\') return OS_FAT16_BAD_PATH;
        out[8U + ext++] = (uint8_t)c;
    }
    return name[i] || ext == 0U ? OS_FAT16_BAD_PATH : 0;
}

static int entry_matches(const uint8_t* entry, const uint8_t* short_name) {
    uint32_t i;
    for (i = 0U; i < 11U; i++) {
        if (upper_ascii((char)entry[i]) != (char)short_name[i]) return 0;
    }
    return 1;
}

#define FAT16_READ_WINDOW_MAX_SECTORS 16U

static int read_metadata_at(const fat16_volume_t* v, uint32_t lba, void* out) {
    if (!v || !v->read_sector || !out || lba < v->base_lba ||
        lba - v->base_lba >= v->total_sectors) return OS_FAT16_CORRUPT;
    return v->read_sector(lba, out) == 0 ? 0 : OS_FAT16_CORRUPT;
}

static int read_at(const fat16_volume_t* v, uint32_t lba, void* out) {
    fat16_volume_t* mutable;
    uint32_t index;
    uint32_t count;
    uint32_t available;
    uint32_t i;
    if (!v || !v->read_sector || !out || lba < v->base_lba ||
        lba - v->base_lba >= v->total_sectors) return OS_FAT16_CORRUPT;
    mutable = (fat16_volume_t*)v;
    if (mutable->read_window_valid && lba >= mutable->read_window_lba &&
        lba - mutable->read_window_lba < mutable->read_window_sectors) {
        index = lba - mutable->read_window_lba;
        for (i = 0U; i < FAT16_SECTOR_SIZE; i++)
            ((uint8_t*)out)[i] = mutable->read_window[index * FAT16_SECTOR_SIZE + i];
        return 0;
    }
    if (!mutable->read_sectors || !mutable->read_window ||
        mutable->read_window_capacity < FAT16_SECTOR_SIZE) return read_metadata_at(v, lba, out);
    available = v->total_sectors - (lba - v->base_lba);
    count = mutable->read_window_capacity / FAT16_SECTOR_SIZE;
    if (count > FAT16_READ_WINDOW_MAX_SECTORS) count = FAT16_READ_WINDOW_MAX_SECTORS;
    if (count > available) count = available;
    if (count == 0U || mutable->read_sectors(lba, count, mutable->read_window) != 0) {
        mutable->read_window_valid = 0U;
        return OS_FAT16_CORRUPT;
    }
    mutable->read_window_lba = lba;
    mutable->read_window_sectors = (uint8_t)count;
    mutable->read_window_valid = 1U;
    for (i = 0U; i < FAT16_SECTOR_SIZE; i++) ((uint8_t*)out)[i] = mutable->read_window[i];
    return 0;
}

static int read_fat_entry(const fat16_volume_t* v, uint16_t cluster, uint16_t* next) {
    uint32_t byte_offset = (uint32_t)cluster * 2U;
    uint32_t lba = v->fat_lba + (byte_offset >> 9U);
    uint32_t offset = byte_offset & 511U;
    if (!next || offset > 510U || (lba - v->base_lba) >= v->total_sectors) return OS_FAT16_CORRUPT;
    if (!fat_sector_cache_valid || fat_sector_cache_lba != lba) {
        if (read_metadata_at(v, lba, fat_sector_cache) != 0) return OS_FAT16_CORRUPT;
        fat_sector_cache_lba = lba;
        fat_sector_cache_valid = 1U;
    }
    *next = le16(fat_sector_cache + offset);
    return 0;
}

static int read_root_entry(const fat16_volume_t* v, uint32_t index, uint8_t* entry) {
    uint32_t lba;
    uint32_t offset;
    if (!v || !entry || index >= v->root_entries) return OS_FAT16_NOT_FOUND;
    lba = v->root_lba + ((index * FAT16_ENTRY_SIZE) >> 9U);
    offset = (index * FAT16_ENTRY_SIZE) & 511U;
    if (read_at(v, lba, sector) != 0) return OS_FAT16_CORRUPT;
    for (uint32_t i = 0U; i < FAT16_ENTRY_SIZE; i++) entry[i] = sector[offset + i];
    return 0;
}

fat16_volume_t* fat16_root(void) {
    return &root_volume;
}

int fat16_mount(fat16_volume_t* v, fat16_read_sector_fn read_sector, uint32_t base_lba) {
    uint32_t total;
    uint32_t fat_lba;
    uint32_t root_lba;
    uint32_t data_lba;
    uint32_t data_sectors;
    uint32_t clusters;
    uint16_t reserved;
    uint16_t fat_sectors;
    uint16_t root_entries;
    uint8_t fats;
    uint8_t spc;
    if (!v || !read_sector) return OS_FAT16_CORRUPT;
    v->mounted = 0U;
    fat_sector_cache_valid = 0U;
    v->read_sector = read_sector;
    v->read_sectors = 0;
    v->write_sector = 0;
    v->read_window = 0;
    v->read_window_capacity = 0U;
    v->read_window_lba = 0U;
    v->read_window_sectors = 0U;
    v->read_window_valid = 0U;
    v->base_lba = base_lba;
    if (read_sector(base_lba, sector) != 0) {
        status_text = "FAT16: secteur boot illisible";
        return OS_FAT16_CORRUPT;
    }
    if (le16(sector + 11U) != FAT16_SECTOR_SIZE || sector[13] == 0U ||
        le16(sector + 14U) == 0U || sector[16] == 0U || sector[16] > 2U) {
        status_text = "FAT16: BPB invalide";
        return OS_FAT16_CORRUPT;
    }
    spc = sector[13];
    if ((spc & (uint8_t)(spc - 1U)) != 0U || spc > 128U) {
        status_text = "FAT16: clusters invalides";
        return OS_FAT16_CORRUPT;
    }
    reserved = le16(sector + 14U);
    fats = sector[16];
    root_entries = le16(sector + 17U);
    fat_sectors = le16(sector + 22U);
    total = le16(sector + 19U);
    if (total == 0U) total = le32(sector + 32U);
    if (total == 0U || fat_sectors == 0U || root_entries == 0U ||
        root_entries > FAT16_MAX_ROOT_ENTRIES) {
        status_text = "FAT16: tailles BPB invalides";
        return OS_FAT16_CORRUPT;
    }
    root_lba = base_lba + reserved + (uint32_t)fats * fat_sectors;
    if ((uint32_t)root_entries * FAT16_ENTRY_SIZE > 0xffffffffU - 511U) return OS_FAT16_CORRUPT;
    v->root_sectors = ((uint32_t)root_entries * FAT16_ENTRY_SIZE + 511U) >> 9U;
    data_lba = root_lba + v->root_sectors;
    if (data_lba < base_lba || data_lba - base_lba >= total) return OS_FAT16_CORRUPT;
    data_sectors = total - (data_lba - base_lba);
    clusters = data_sectors / spc;
    if (clusters < 4085U || clusters > FAT16_MAX_CLUSTERS) {
        status_text = "FAT16: nombre de clusters hors plage";
        return OS_FAT16_CORRUPT;
    }
    fat_lba = base_lba + reserved;
    v->total_sectors = total;
    v->fat_lba = fat_lba;
    v->fat_sectors = fat_sectors;
    v->root_lba = root_lba;
    v->data_lba = data_lba;
    v->cluster_count = clusters;
    v->bytes_per_sector = FAT16_SECTOR_SIZE;
    v->sectors_per_cluster = spc;
    v->fat_count = fats;
    v->root_entries = root_entries;
    v->mounted = 1U;
    status_text = "FAT16: volume lecture seule monte";
    return 0;
}

int fat16_is_mounted(const fat16_volume_t* v) {
    return v && v->mounted != 0U;
}

int fat16_attach_read_window(fat16_volume_t* v, fat16_read_sectors_fn read_sectors,
                             uint8_t* window, uint32_t window_capacity) {
    if (!v || !fat16_is_mounted(v) || !read_sectors || !window ||
        window_capacity < FAT16_SECTOR_SIZE) return OS_FAT16_CORRUPT;
    v->read_sectors = read_sectors;
    v->read_window = window;
    v->read_window_capacity = window_capacity;
    v->read_window_lba = 0U;
    v->read_window_sectors = 0U;
    v->read_window_valid = 0U;
    return 0;
}

int fat16_attach_writer(fat16_volume_t* v, fat16_write_sector_fn write_sector){if(!v||!fat16_is_mounted(v)||!write_sector)return OS_FAT16_CORRUPT;v->write_sector=write_sector;status_text="FAT16: volume lecture/ecriture monte";return 0;}
int fat16_write_sector(const fat16_volume_t* v,uint32_t lba,const uint8_t* buffer){if(!v||!buffer||!fat16_is_mounted(v)||!v->write_sector)return OS_FAT16_NOT_MOUNTED;if(lba<v->base_lba||lba-v->base_lba>=v->total_sectors)return OS_FAT16_CORRUPT;((fat16_volume_t*)v)->read_window_valid=0U;fat_sector_cache_valid=0U;return v->write_sector(lba,buffer)==0?0:OS_FAT16_CORRUPT;}
int fat16_write_cluster_range(const fat16_volume_t* v,uint16_t cluster,uint32_t offset,const uint8_t* buffer,uint32_t length){uint32_t cluster_bytes,absolute,lba,sector_offset,chunk,i;if(!v||!fat16_is_mounted(v)||!v->write_sector)return OS_FAT16_NOT_MOUNTED;if((uint32_t)cluster<2U||(uint32_t)cluster>(v->cluster_count+1U))return OS_FAT16_CORRUPT;if(length!=0U&&!buffer)return OS_FAT16_BAD_PATH;cluster_bytes=(uint32_t)v->bytes_per_sector*v->sectors_per_cluster;if(offset>cluster_bytes||length>cluster_bytes-offset)return OS_FAT16_BUFFER_SMALL;while(length){absolute=((uint32_t)cluster-2U)*cluster_bytes+offset;lba=v->data_lba+(absolute/v->bytes_per_sector);sector_offset=absolute%v->bytes_per_sector;chunk=(uint32_t)v->bytes_per_sector-sector_offset;if(chunk>length)chunk=length;if(read_at(v,lba,sector)!=0)return OS_FAT16_CORRUPT;for(i=0U;i<chunk;i++)sector[sector_offset+i]=buffer[i];if(fat16_write_sector(v,lba,sector)!=0)return OS_FAT16_CORRUPT;buffer+=chunk;offset+=chunk;length-=chunk;}return 0;}
int fat16_allocate_cluster(const fat16_volume_t* v,uint16_t* out_cluster){uint32_t cluster,fat,byte_offset,lba,offset;uint16_t value;if(!v||!out_cluster||!fat16_is_mounted(v)||!v->write_sector)return OS_FAT16_NOT_MOUNTED;for(cluster=2U;cluster<=v->cluster_count+1U;cluster++){if(read_fat_entry(v,(uint16_t)cluster,&value)!=0)return OS_FAT16_CORRUPT;if(value!=0U)continue;byte_offset=cluster*2U;offset=byte_offset&511U;for(fat=0U;fat<v->fat_count;fat++){lba=v->fat_lba+fat*v->fat_sectors+(byte_offset>>9U);if(read_at(v,lba,sector)!=0)return OS_FAT16_CORRUPT;for(value=0U;value<512U;value++)sector2[value]=sector[value];sector[offset]=(uint8_t)FAT16_EOC_MIN;sector[offset+1U]=(uint8_t)(FAT16_EOC_MIN>>8U);if(fat16_write_sector(v,lba,sector)!=0){(void)v->write_sector(lba,sector2);return OS_FAT16_CORRUPT;}}*out_cluster=(uint16_t)cluster;return 0;}return OS_FAT16_NOT_FOUND;}
int fat16_link_clusters(const fat16_volume_t* v,uint16_t source,uint16_t target){uint32_t fat,byte_offset,lba,offset,i;uint16_t next,target_next;if(!v||!fat16_is_mounted(v)||!v->write_sector)return OS_FAT16_NOT_MOUNTED;if(source<2U||target<2U||(uint32_t)source>v->cluster_count+1U||(uint32_t)target>v->cluster_count+1U||source==target)return OS_FAT16_CORRUPT;if(read_fat_entry(v,source,&next)!=0||read_fat_entry(v,target,&target_next)!=0)return OS_FAT16_CORRUPT;if(next<FAT16_EOC_MIN||next==FAT16_BAD_CLUSTER||target_next==0U||target_next==FAT16_BAD_CLUSTER)return OS_FAT16_CORRUPT;byte_offset=(uint32_t)source*2U;offset=byte_offset&511U;for(fat=0U;fat<v->fat_count;fat++){lba=v->fat_lba+fat*v->fat_sectors+(byte_offset>>9U);if(read_at(v,lba,sector)!=0)return OS_FAT16_CORRUPT;for(i=0U;i<512U;i++)sector2[i]=sector[i];sector[offset]=(uint8_t)target;sector[offset+1U]=(uint8_t)(target>>8U);if(fat16_write_sector(v,lba,sector)!=0){(void)v->write_sector(lba,sector2);return OS_FAT16_CORRUPT;}}return 0;}

int fat16_create_root_entry(const fat16_volume_t* v,const char* name,uint8_t attributes,uint16_t first_cluster,uint32_t size){uint8_t short_name[11];uint32_t index,byte_offset,lba,entry_offset,i,free_index;if(!v||!name||!fat16_is_mounted(v)||!v->write_sector)return OS_FAT16_NOT_MOUNTED;if(first_cluster<2U||(uint32_t)first_cluster>v->cluster_count+1U)return OS_FAT16_CORRUPT;if((attributes&0x0FU)==0x0FU)return OS_FAT16_BAD_PATH;if(make_short_name(name,short_name)!=0)return OS_FAT16_BAD_PATH;free_index=v->root_entries;for(index=0U;index<v->root_entries;index++){byte_offset=index*FAT16_ENTRY_SIZE;lba=v->root_lba+(byte_offset/FAT16_SECTOR_SIZE);entry_offset=byte_offset%FAT16_SECTOR_SIZE;if(read_at(v,lba,sector)!=0)return OS_FAT16_CORRUPT;if(sector[entry_offset]==0U){if(free_index==v->root_entries)free_index=index;break;}if(sector[entry_offset]==0xE5U){if(free_index==v->root_entries)free_index=index;continue;}if(entry_matches(sector+entry_offset,short_name))return OS_FAT16_BAD_PATH;}if(free_index==v->root_entries)return OS_FAT16_NOT_FOUND;byte_offset=free_index*FAT16_ENTRY_SIZE;lba=v->root_lba+(byte_offset/FAT16_SECTOR_SIZE);entry_offset=byte_offset%FAT16_SECTOR_SIZE;if(read_at(v,lba,sector)!=0)return OS_FAT16_CORRUPT;for(i=0U;i<FAT16_ENTRY_SIZE;i++)sector[entry_offset+i]=0U;for(i=0U;i<11U;i++)sector[entry_offset+i]=short_name[i];sector[entry_offset+11U]=attributes;sector[entry_offset+26U]=(uint8_t)first_cluster;sector[entry_offset+27U]=(uint8_t)(first_cluster>>8U);sector[entry_offset+28U]=(uint8_t)size;sector[entry_offset+29U]=(uint8_t)(size>>8U);sector[entry_offset+30U]=(uint8_t)(size>>16U);sector[entry_offset+31U]=(uint8_t)(size>>24U);if(fat16_write_sector(v,lba,sector)!=0)return OS_FAT16_CORRUPT;return 0;}
static int fat16_set_fat_entry(const fat16_volume_t* v, uint16_t cluster, uint16_t value) {
    uint32_t fat, byte_offset = (uint32_t)cluster * 2U, lba, offset = byte_offset & 511U, i;
    if (!v || cluster < 2U || (uint32_t)cluster > v->cluster_count + 1U || offset > 510U) return OS_FAT16_CORRUPT;
    for (fat = 0U; fat < v->fat_count; fat++) {
        lba = v->fat_lba + fat * v->fat_sectors + (byte_offset >> 9U);
        if (read_at(v, lba, sector) != 0) return OS_FAT16_CORRUPT;
        for (i = 0U; i < FAT16_SECTOR_SIZE; i++) sector2[i] = sector[i];
        sector[offset] = (uint8_t)value;
        sector[offset + 1U] = (uint8_t)(value >> 8U);
        if (fat16_write_sector(v, lba, sector) != 0) {
            (void)v->write_sector(lba, sector2);
            return OS_FAT16_CORRUPT;
        }
    }
    return 0;
}

static int fat16_release_chain(const fat16_volume_t* v, uint16_t first) {
    uint16_t current = first, next;
    uint32_t guard = 0U;
    while (current >= 2U && (uint32_t)current <= v->cluster_count + 1U && guard++ <= v->cluster_count) {
        if (read_fat_entry(v, current, &next) != 0) return OS_FAT16_CORRUPT;
        if (fat16_set_fat_entry(v, current, 0U) != 0) return OS_FAT16_CORRUPT;
        if (next >= FAT16_EOC_MIN) return 0;
        if (next == FAT16_BAD_CLUSTER || next < 2U || (uint32_t)next > v->cluster_count + 1U) {
            return OS_FAT16_CORRUPT;
        }
        current = next;
    }
    return OS_FAT16_CORRUPT;
}

/* Supprime uniquement une entrée classique 8.3 de la racine. Les séquences LFN,
 * labels et répertoires restent hors périmètre : les effacer partiellement
 * rendrait leur métadonnée incohérente. L’entrée est rendue invisible avant la
 * libération de chaîne afin de ne jamais conserver un fichier référençant des
 * clusters déjà libérés si une écriture ultérieure échoue. */
int fat16_unlink_file(const fat16_volume_t* v, const char* name) {
    uint8_t entry[FAT16_ENTRY_SIZE], short_name[11], lfn_sum = 0U, expected = 0U, valid = 0U;
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t lfn_start = 0U, i, j;
    uint16_t first;
    uint32_t size;
    int short_valid;
    if (!v || !name || !fat16_is_mounted(v) || !v->write_sector) return OS_FAT16_NOT_MOUNTED;
    short_valid = make_short_name(name, short_name) == 0;
    if (!fat16_lfn_query_valid(name)) return OS_FAT16_BAD_PATH;
    for (i = 0U; i < v->root_entries; i++) {
        uint8_t ord;
        if (read_root_entry(v, i, entry) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U) { valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ord = entry[0] & 0x1FU;
            if (entry[0] & 0x40U) {
                if (ord == 0U || ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; }
                for (j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; expected = ord; lfn_start = i; valid = 1U;
            }
            if (!valid || ord == 0U || ord != expected || entry[13] != lfn_sum) { valid = 0U; continue; }
            fat16_lfn_get(entry, 1U, (ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 14U, (ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 28U, (ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            expected--; continue;
        }
        if (entry[11] & 0x08U) { valid = 0U; continue; }
        { int same = short_valid; for (j = 0U; j < 11U && same; j++) if (entry[j] != short_name[j]) same = 0;
        if ((!same) &&
            !(valid && expected == 0U && fat16_lfn_checksum(entry) == lfn_sum && fat16_lfn_name_equals_folded(lfn_units, name))) { valid = 0U; continue; }
        }
        first = le16(entry + 26U);
        size = le32(entry + 28U);
        if (first == 0U && size != 0U) return OS_FAT16_CORRUPT;
        if (first != 0U && (first < 2U || (uint32_t)first > v->cluster_count + 1U)) {
            return OS_FAT16_CORRUPT;
        }
        for (j = valid && expected == 0U ? lfn_start : i; j <= i; j++) {
            uint8_t deleted[FAT16_ENTRY_SIZE];
            if (read_root_entry(v, j, deleted) != 0) return OS_FAT16_CORRUPT;
            deleted[0] = 0xE5U;
            if (fat16_write_root_slot(v, j, deleted) != 0) return OS_FAT16_CORRUPT;
        }
        return first == 0U ? 0 : fat16_release_chain(v, first);
    }
    return OS_FAT16_NOT_FOUND;
}

int fat16_rename_file(const fat16_volume_t* v, const char* old_name, const char* new_name) {
    uint8_t old_short[11], new_short[11];
    uint32_t index, old_index, byte_offset, lba, entry_offset, i;
    uint8_t lfn_pending = 0U;
    if (!v || !old_name || !new_name || !fat16_is_mounted(v) || !v->write_sector) {
        return OS_FAT16_NOT_MOUNTED;
    }
    if (make_short_name(old_name, old_short) != 0 || make_short_name(new_name, new_short) != 0) {
        return OS_FAT16_BAD_PATH;
    }
    old_index = v->root_entries;
    for (index = 0U; index < v->root_entries; index++) {
        byte_offset = index * FAT16_ENTRY_SIZE;
        lba = v->root_lba + (byte_offset / FAT16_SECTOR_SIZE);
        entry_offset = byte_offset % FAT16_SECTOR_SIZE;
        if (read_at(v, lba, sector) != 0) return OS_FAT16_CORRUPT;
        if (sector[entry_offset] == 0U) break;
        if (sector[entry_offset] == 0xE5U) { lfn_pending = 0U; continue; }
        if ((sector[entry_offset + 11U] & 0x0FU) == 0x0FU) { lfn_pending = 1U; continue; }
        if (entry_matches(sector + entry_offset, old_short)) {
            if (lfn_pending || (sector[entry_offset + 11U] & 0x18U) != 0U) return OS_FAT16_BAD_PATH;
            old_index = index;
        }
        if (entry_matches(sector + entry_offset, new_short) &&
            !entry_matches(sector + entry_offset, old_short)) return OS_FAT16_BAD_PATH;
        lfn_pending = 0U;
    }
    if (old_index == v->root_entries) return OS_FAT16_NOT_FOUND;
    byte_offset = old_index * FAT16_ENTRY_SIZE;
    lba = v->root_lba + (byte_offset / FAT16_SECTOR_SIZE);
    entry_offset = byte_offset % FAT16_SECTOR_SIZE;
    if (read_at(v, lba, sector) != 0) return OS_FAT16_CORRUPT;
    for (i = 0U; i < 11U; i++) sector[entry_offset + i] = new_short[i];
    return fat16_write_sector(v, lba, sector);
}

int fat16_rename_lfn_file(const fat16_volume_t* v, const char* old_name,
                          const char* new_long_name, const char* new_short_name) {
    uint8_t entry[FAT16_ENTRY_SIZE], old_short[11], new_short[11], sum = 0U, expected = 0U, valid = 0U;
    uint16_t lfn_units[OS_NAME_MAX], units[OS_NAME_MAX];
    uint32_t start = 0U, i, j, old_count = 0U, length = 0U, new_count;
    int old_short_valid;
    if (!v || !old_name || !new_long_name || !new_short_name || !fat16_is_mounted(v) || !v->write_sector) return OS_FAT16_NOT_MOUNTED;
    old_short_valid = make_short_name(old_name, old_short) == 0;
    if (make_short_name(new_short_name, new_short) != 0) return OS_FAT16_BAD_PATH;
    if (lfn_utf8_to_utf16_bmp(new_long_name, units, OS_NAME_MAX, &length) != 0) return OS_FAT16_BAD_PATH;
    new_count = (length + 12U) / 13U;
    for (i = 0U; i < v->root_entries; i++) {
        uint8_t ord;
        if (read_root_entry(v, i, entry) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U) { valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ord = entry[0] & 0x1FU;
            if (entry[0] & 0x40U) {
                if (ord == 0U || ord * 13U >= OS_NAME_MAX) { valid = 0U; continue; }
                for (j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                sum = entry[13]; expected = ord; start = i; old_count = ord; valid = 1U;
            }
            if (!valid || ord == 0U || ord != expected || entry[13] != sum) { valid = 0U; continue; }
            fat16_lfn_get(entry, 1U, (ord - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 14U, (ord - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 28U, (ord - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            expected--; continue;
        }
        if (entry[11] & 0x08U) { valid = 0U; continue; }
        { int same = old_short_valid; for (j = 0U; j < 11U && same; j++) if (entry[j] != old_short[j]) same = 0;
          if (!same && !(valid && expected == 0U && fat16_lfn_checksum(entry) == sum && fat16_lfn_name_equals_folded(lfn_units, old_name))) { valid = 0U; continue; } }
        if (!valid || expected != 0U || old_count != new_count) return OS_FAT16_BAD_PATH;
        for (j = 0U; j < new_count; j++) {
            uint32_t ordinal = new_count - j;
            uint32_t pos = (ordinal - 1U) * 13U;
            uint32_t k;
            for (k = 0U; k < 32U; k++) entry[k] = 0xFFU;
            entry[0] = (uint8_t)ordinal | (j == 0U ? 0x40U : 0U);
            entry[11] = 0x0FU; entry[12] = 0U; entry[13] = fat16_lfn_checksum(new_short);
            entry[26] = 0U; entry[27] = 0U;
            fat16_lfn_put(entry, 1U, pos, units, length);
            fat16_lfn_put(entry, 14U, pos + 5U, units, length);
            fat16_lfn_put(entry, 28U, pos + 11U, units, length);
            if (fat16_write_root_slot(v, start + j, entry) != 0) return OS_FAT16_CORRUPT;
        }
        if (read_root_entry(v, i, entry) != 0) return OS_FAT16_CORRUPT;
        for (j = 0U; j < 11U; j++) entry[j] = new_short[j];
        if (fat16_write_root_slot(v, i, entry) != 0) return OS_FAT16_CORRUPT;
        return 0;
    }
    return OS_FAT16_NOT_FOUND;
}

int fat16_create_file(const fat16_volume_t* v, const char* name, uint8_t attributes,
                      const uint8_t* data, uint32_t size, uint16_t* out_first_cluster) {
    uint32_t cluster_bytes, remaining, offset = 0U;
    uint16_t first = 0U, previous = 0U, current;
    int rc;
    if (!v || !name || !out_first_cluster || !fat16_is_mounted(v) || !v->write_sector) return OS_FAT16_NOT_MOUNTED;
    if ((attributes & 0x0FU) == 0x0FU || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    cluster_bytes = (uint32_t)v->bytes_per_sector * v->sectors_per_cluster;
    remaining = size == 0U ? 1U : size;
    while (remaining != 0U) {
        rc = fat16_allocate_cluster(v, &current);
        if (rc != 0) { if (first != 0U) fat16_release_chain(v, first); return rc; }
        if (first == 0U) first = current;
        if (previous != 0U) {
            rc = fat16_link_clusters(v, previous, current);
            if (rc != 0) { fat16_release_chain(v, first); return rc; }
        }
        if (size != 0U) {
            uint32_t chunk = remaining > cluster_bytes ? cluster_bytes : remaining;
            rc = fat16_write_cluster_range(v, current, 0U, data + offset, chunk);
            if (rc != 0) { fat16_release_chain(v, first); return rc; }
            offset += chunk;
            remaining -= chunk;
        } else remaining = 0U;
        previous = current;
    }
    rc = fat16_create_root_entry(v, name, attributes, first, size);
    if (rc != 0) { fat16_release_chain(v, first); return rc; }
    *out_first_cluster = first;
    return 0;
}

static uint8_t fat16_lfn_checksum(const uint8_t* short_name) {
    uint32_t i;
    uint8_t sum = 0U;
    for (i = 0U; i < 11U; i++) sum = (uint8_t)(((sum & 1U) ? 0x80U : 0U) + (sum >> 1U) + short_name[i]);
    return sum;
}

static int fat16_write_root_slot(const fat16_volume_t* v, uint32_t index, const uint8_t* entry) {
    uint32_t byte_offset = index * FAT16_ENTRY_SIZE;
    uint32_t lba = v->root_lba + (byte_offset / FAT16_SECTOR_SIZE);
    uint32_t offset = byte_offset % FAT16_SECTOR_SIZE, i;
    if (index >= v->root_entries || read_at(v, lba, sector) != 0) return OS_FAT16_CORRUPT;
    for (i = 0U; i < FAT16_ENTRY_SIZE; i++) sector[offset + i] = entry[i];
    return fat16_write_sector(v, lba, sector);
}

static void fat16_lfn_put(uint8_t* entry, uint32_t offset, uint32_t pos,
                           const uint16_t* units, uint32_t length) {
    uint32_t i;
    uint32_t limit = offset == 1U ? 5U : (offset == 14U ? 6U : 2U);
    for (i = 0U; i < limit; i++) {
        uint32_t at = pos + i;
        uint16_t value = at < length ? units[at] : (at == length ? 0U : 0xFFFFU);
        entry[offset + i * 2U] = (uint8_t)value;
        entry[offset + i * 2U + 1U] = (uint8_t)(value >> 8U);
    }
}

int fat16_create_lfn_file(const fat16_volume_t* v, const char* long_name,
                          const char* short_name, uint8_t attributes,
                          const uint8_t* data, uint32_t size, uint16_t* out_first_cluster) {
    uint8_t alias[11], entry[FAT16_ENTRY_SIZE];
    uint16_t units[OS_NAME_MAX];
    uint32_t length = 0U, i, alias_index = v ? v->root_entries : 0U, count, start;
    uint16_t first = 0U;
    int rc;
    if (!v || !long_name || !short_name || !out_first_cluster) return OS_FAT16_BAD_PATH;
    if (lfn_utf8_to_utf16_bmp(long_name, units, OS_NAME_MAX, &length) != 0 ||
        make_short_name(short_name, alias) != 0) return OS_FAT16_BAD_PATH;
    count = (length + 12U) / 13U;
    if (count == 0U || count > 20U) return OS_FAT16_BAD_PATH;
    rc = fat16_create_file(v, short_name, attributes, data, size, &first);
    if (rc != 0) return rc;
    for (i = 0U; i < v->root_entries; i++) {
        if (read_root_entry(v, i, entry) != 0) return OS_FAT16_CORRUPT;
        if (entry_matches(entry, alias)) { alias_index = i; break; }
    }
    if (alias_index >= v->root_entries || alias_index + count + 1U >= v->root_entries) return OS_FAT16_NOT_FOUND;
    start = alias_index + 1U;
    for (i = 0U; i < count + 1U; i++) {
        if (read_root_entry(v, start + i, entry) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] != 0U && entry[0] != 0xE5U) return OS_FAT16_NOT_FOUND;
    }
    entry[0] = 0xE5U;
    rc = fat16_write_root_slot(v, alias_index, entry);
    if (rc != 0) return rc;
    for (i = 0U; i < count; i++) {
        uint32_t ordinal = count - i;
        uint32_t pos = (ordinal - 1U) * 13U;
        uint32_t j;
        for (j = 0U; j < 32U; j++) entry[j] = 0xFFU;
        entry[0] = (uint8_t)ordinal | (i == 0U ? 0x40U : 0U);
        entry[11] = 0x0FU; entry[12] = 0U; entry[13] = fat16_lfn_checksum(alias);
        entry[26] = 0U; entry[27] = 0U;
        fat16_lfn_put(entry, 1U, pos, units, length);
        fat16_lfn_put(entry, 14U, pos + 5U, units, length);
        fat16_lfn_put(entry, 28U, pos + 11U, units, length);
        rc = fat16_write_root_slot(v, start + i, entry);
        if (rc != 0) return rc;
    }
    for (i = 0U; i < 32U; i++) entry[i] = 0U;
    for (i = 0U; i < 11U; i++) entry[i] = alias[i];
    entry[11] = attributes; entry[26] = (uint8_t)first; entry[27] = (uint8_t)(first >> 8U);
    entry[28] = (uint8_t)size; entry[29] = (uint8_t)(size >> 8U);
    entry[30] = (uint8_t)(size >> 16U); entry[31] = (uint8_t)(size >> 24U);
    rc = fat16_write_root_slot(v, start + count, entry);
    if (rc != 0) return rc;
    *out_first_cluster = first;
    return 0;
}

static void fat16_lfn_get(uint8_t* entry, uint32_t offset, uint32_t pos,
                           uint16_t* units, uint32_t max) {
    uint32_t i;
    uint32_t limit = offset == 1U ? 5U : (offset == 14U ? 6U : 2U);
    for (i = 0U; i < limit; i++) {
        uint32_t at = pos + i;
        uint16_t value = (uint16_t)entry[offset + i * 2U] |
                         ((uint16_t)entry[offset + i * 2U + 1U] << 8U);
        if (at >= max || value == 0U || value == 0xFFFFU) continue;
        units[at] = value;
    }
}

int fat16_list_root_page(const fat16_volume_t* v, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity) {
    uint32_t i;
    uint32_t count = 0U;
    uint32_t seen = 0U;
    uint8_t entry[FAT16_ENTRY_SIZE];
    char lfn[OS_NAME_MAX];
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t lfn_length = 0U;
    uint8_t lfn_sum = 0U, lfn_expected = 0U, lfn_valid = 0U;
    if (!fat16_is_mounted(v)) return OS_FAT16_NOT_MOUNTED;
    if (!out || capacity == 0U) return OS_FAT16_BAD_PATH;
    for (i = 0U; i < v->root_entries; i++) {
        uint8_t ordinal;
        if (read_root_entry(v, i, entry) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0x00U) break;
        if (entry[0] == 0xE5U) { lfn_valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ordinal = entry[0] & 0x1FU;
            if ((entry[0] & 0x40U) != 0U) {
                lfn_length = ordinal * 13U;
                if (lfn_length >= OS_NAME_MAX) { lfn_valid = 0U; continue; }
                for (uint32_t j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; lfn_expected = ordinal; lfn_valid = 1U;
            }
            if (!lfn_valid || ordinal == 0U || ordinal != lfn_expected || entry[13] != lfn_sum) { lfn_valid = 0U; continue; }
            fat16_lfn_get(entry, 1U, (uint32_t)(ordinal - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 14U, (uint32_t)(ordinal - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 28U, (uint32_t)(ordinal - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            lfn_expected--;
            continue;
        }
        if ((entry[11] & 0x08U) != 0U) { lfn_valid = 0U; continue; }
        if (seen++ < start) { lfn_valid = 0U; continue; }
        if (count >= capacity) return (int)count;
        if (!(lfn_valid && lfn_expected == 0U && fat16_lfn_checksum(entry) == lfn_sum &&
              lfn_utf16_bmp_to_utf8(lfn_units, OS_NAME_MAX, lfn, OS_NAME_MAX) >= 0)) {
            copy_name(out[count].name, entry);
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

static int fat16_lfn_query_valid(const char* name) {
    uint16_t units[OS_NAME_MAX];
    uint32_t length;
    return lfn_utf8_to_utf16_bmp(name, units, OS_NAME_MAX, &length) == 0;
}

static int fat16_name_equals_folded(const char* left, const char* right) {
    uint32_t i;
    if (!left || !right) return 0;
    for (i = 0U; i < OS_NAME_MAX; i++) {
        char a = left[i];
        char b = right[i];
        if (upper_ascii(a) != upper_ascii(b)) return 0;
        if (a == '\0' || b == '\0') return a == b;
    }
    return 0;
}

static int fat16_lfn_name_equals_folded(const uint16_t* units, const char* name) {
    char decoded[OS_NAME_MAX];
    return lfn_utf16_bmp_to_utf8(units, OS_NAME_MAX, decoded, OS_NAME_MAX) >= 0 &&
           fat16_name_equals_folded(name, decoded);
}

/* Recherche une entrée de racine à la fois par alias 8.3 et par LFN ASCII
 * validé. Le résultat reste l’entrée courte FAT16 : aucun état ni allocation
 * supplémentaire n’est nécessaire aux lecteurs et curseurs existants. */
static int fat16_find_root_entry(const fat16_volume_t* v, const char* name,
                                 uint8_t* out) {
    uint8_t short_name[11];
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t i;
    uint32_t lfn_length = 0U;
    uint8_t lfn_sum = 0U;
    uint8_t lfn_expected = 0U;
    uint8_t lfn_valid = 0U;
    int short_valid;
    if (!fat16_is_mounted(v)) return OS_FAT16_NOT_MOUNTED;
    if (!out || !fat16_lfn_query_valid(name)) return OS_FAT16_BAD_PATH;
    short_valid = make_short_name(name, short_name) == 0;
    for (i = 0U; i < v->root_entries; i++) {
        uint8_t ordinal;
        if (read_root_entry(v, i, entry) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0x00U) break;
        if (entry[0] == 0xE5U) { lfn_valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ordinal = entry[0] & 0x1FU;
            if ((entry[0] & 0x40U) != 0U) {
                lfn_length = (uint32_t)ordinal * 13U;
                if (lfn_length >= OS_NAME_MAX) { lfn_valid = 0U; continue; }
                for (uint32_t j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13];
                lfn_expected = ordinal;
                lfn_valid = 1U;
            }
            if (!lfn_valid || ordinal == 0U || ordinal != lfn_expected ||
                entry[13] != lfn_sum) { lfn_valid = 0U; continue; }
            fat16_lfn_get(entry, 1U, (uint32_t)(ordinal - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 14U, (uint32_t)(ordinal - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 28U, (uint32_t)(ordinal - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            lfn_expected--;
            continue;
        }
        if ((entry[11] & 0x08U) != 0U) { lfn_valid = 0U; continue; }
        if ((short_valid && entry_matches(entry, short_name)) ||
            (lfn_valid && lfn_expected == 0U && fat16_lfn_checksum(entry) == lfn_sum &&
             fat16_lfn_name_equals_folded(lfn_units, name))) {
            for (uint32_t j = 0U; j < FAT16_ENTRY_SIZE; j++) out[j] = entry[j];
            return 0;
        }
        lfn_valid = 0U;
    }
    return OS_FAT16_NOT_FOUND;
}

int fat16_read_file(const fat16_volume_t* v, const char* name, char* buffer, uint32_t max) {
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint32_t i;
    uint32_t size;
    uint32_t copied = 0U;
    uint16_t cluster;
    uint32_t guard = 0U;
    int status;
    if (!fat16_is_mounted(v)) return OS_FAT16_NOT_MOUNTED;
    if (!buffer || max == 0U) return OS_FAT16_BUFFER_SMALL;
    status = fat16_find_root_entry(v, name, entry);
    if (status != 0) return status;
    if (entry[11] & 0x10U) return OS_FAT16_BAD_PATH;
    size = le32(entry + 28U);
    if (size > max) return OS_FAT16_BUFFER_SMALL;
    cluster = le16(entry + 26U);
    while (copied < size) {
        uint32_t s;
        uint32_t take;
        if (cluster < 2U || cluster >= FAT16_EOC_MIN || cluster - 2U >= v->cluster_count || guard++ > v->cluster_count) return OS_FAT16_CORRUPT;
        for (s = 0U; s < v->sectors_per_cluster && copied < size; s++) {
            uint32_t lba = v->data_lba + (uint32_t)(cluster - 2U) * v->sectors_per_cluster + s;
            if (read_at(v, lba, sector2) != 0) return OS_FAT16_CORRUPT;
            take = size - copied;
            if (take > FAT16_SECTOR_SIZE) take = FAT16_SECTOR_SIZE;
            for (i = 0U; i < take; i++) buffer[copied + i] = (char)sector2[i];
            copied += take;
        }
        if (copied < size && read_fat_entry(v, cluster, &cluster) != 0) return OS_FAT16_CORRUPT;
    }
    return (int)copied;
}

int fat16_list_root(const fat16_volume_t* v, os_fat16_dirent_t* out, uint32_t capacity) {
    return fat16_list_root_page(v, 0U, out, capacity);
}

const char* fat16_status(void) {
    return status_text;
}

int fat16_read_file_range(const fat16_volume_t* v, const char* name,
                          uint32_t offset, uint8_t* buffer, uint32_t max,
                          uint32_t* out_read) {
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint32_t i;
    uint32_t size;
    uint32_t cluster_bytes;
    uint32_t skip_clusters;
    uint32_t intra;
    uint32_t copied = 0U;
    uint16_t cluster;
    uint32_t guard = 0U;
    int status;
    if (out_read) *out_read = 0U;
    if (!fat16_is_mounted(v)) return OS_FAT16_NOT_MOUNTED;
    if (!buffer || max == 0U || !out_read) return OS_FAT16_BUFFER_SMALL;
    status = fat16_find_root_entry(v, name, entry);
    if (status != 0) return status;
    if (entry[11] & 0x10U) return OS_FAT16_BAD_PATH;
    size = le32(entry + 28U);
    if (offset > size) return OS_FAT16_BAD_PATH;
    cluster = le16(entry + 26U);
    cluster_bytes = (uint32_t)v->sectors_per_cluster * FAT16_SECTOR_SIZE;
    if (cluster_bytes == 0U) return OS_FAT16_CORRUPT;
    skip_clusters = offset / cluster_bytes;
    intra = offset % cluster_bytes;
    while (skip_clusters-- > 0U) {
        if (cluster < 2U || cluster >= FAT16_EOC_MIN ||
            cluster - 2U >= v->cluster_count || guard++ > v->cluster_count ||
            read_fat_entry(v, cluster, &cluster) != 0) return OS_FAT16_CORRUPT;
    }
    while (copied < max && offset + copied < size) {
        uint32_t sector_in_cluster = intra / FAT16_SECTOR_SIZE;
        uint32_t sector_offset = intra % FAT16_SECTOR_SIZE;
        uint32_t lba;
        uint32_t take = FAT16_SECTOR_SIZE - sector_offset;
        if (cluster < 2U || cluster >= FAT16_EOC_MIN ||
            cluster - 2U >= v->cluster_count || guard > v->cluster_count) return OS_FAT16_CORRUPT;
        lba = v->data_lba + (uint32_t)(cluster - 2U) * v->sectors_per_cluster + sector_in_cluster;
        if (read_at(v, lba, sector2) != 0) return OS_FAT16_CORRUPT;
        if (take > max - copied) take = max - copied;
        if (take > size - offset - copied) take = size - offset - copied;
        for (i = 0U; i < take; i++) buffer[copied + i] = sector2[sector_offset + i];
        copied += take;
        intra += take;
        if (intra >= cluster_bytes && copied < max && offset + copied < size) {
            if (guard++ >= v->cluster_count ||
                read_fat_entry(v, cluster, &cluster) != 0) return OS_FAT16_CORRUPT;
            intra = 0U;
        }
    }
    *out_read = copied;
    return 0;
}


int fat16_open_file(const fat16_volume_t* v, const char* name, fat16_file_t* out) {
    uint8_t entry[FAT16_ENTRY_SIZE];
    int status;
    if (!fat16_is_mounted(v) || !out) return OS_FAT16_NOT_MOUNTED;
    out->open = 0U;
    status = fat16_find_root_entry(v, name, entry);
    if (status != 0) return status;
    if (entry[11] & 0x10U) return OS_FAT16_BAD_PATH;
    out->volume = v;
    out->first_cluster = le16(entry + 26U);
    out->cluster = out->first_cluster;
    out->size = le32(entry + 28U);
    out->position = 0U;
    out->cluster_offset = 0U;
    out->guard = 0U;
    out->cached_lba = 0U;
    out->cache_valid = 0U;
    out->open = 1U;
    return 0;
}

int fat16_file_seek(fat16_file_t* file, uint32_t offset) {
    const fat16_volume_t* v;
    uint32_t cluster_bytes;
    uint32_t steps;
    uint32_t i;
    if (!file || !file->open || !file->volume || offset > file->size) return OS_FAT16_BAD_PATH;
    v = file->volume;
    cluster_bytes = (uint32_t)v->sectors_per_cluster * FAT16_SECTOR_SIZE;
    if (cluster_bytes == 0U) return OS_FAT16_CORRUPT;
    file->cluster = file->first_cluster;
    file->position = 0U;
    file->cluster_offset = 0U;
    file->guard = 0U;
    file->cache_valid = 0U;
    if (offset == file->size) {
        file->position = offset;
        return 0;
    }
    steps = offset / cluster_bytes;
    if (steps > v->cluster_count) return OS_FAT16_CORRUPT;
    for (i = 0U; i < steps; i++) {
        if (file->cluster < 2U || file->cluster >= FAT16_EOC_MIN ||
            file->cluster - 2U >= v->cluster_count ||
            read_fat_entry(v, file->cluster, &file->cluster) != 0) return OS_FAT16_CORRUPT;
    }
    if (file->cluster < 2U || file->cluster >= FAT16_EOC_MIN ||
        file->cluster - 2U >= v->cluster_count) return OS_FAT16_CORRUPT;
    file->position = offset;
    file->cluster_offset = offset % cluster_bytes;
    file->guard = steps;
    return 0;
}

int fat16_file_read(fat16_file_t* file, uint8_t* buffer, uint32_t max,
                    uint32_t* out_read) {
    const fat16_volume_t* v;
    uint32_t cluster_bytes;
    uint32_t copied = 0U;
    if (out_read) *out_read = 0U;
    if (!file || !file->open || !file->volume || !buffer || max == 0U || !out_read) return OS_FAT16_BUFFER_SMALL;
    v = file->volume;
    cluster_bytes = (uint32_t)v->sectors_per_cluster * FAT16_SECTOR_SIZE;
    if (cluster_bytes == 0U || file->position > file->size) return OS_FAT16_CORRUPT;
    while (copied < max && file->position < file->size) {
        uint32_t sector_in_cluster = file->cluster_offset / FAT16_SECTOR_SIZE;
        uint32_t sector_offset = file->cluster_offset % FAT16_SECTOR_SIZE;
        uint32_t take = FAT16_SECTOR_SIZE - sector_offset;
        uint32_t lba;
        if (file->cluster < 2U || file->cluster >= FAT16_EOC_MIN ||
            file->cluster - 2U >= v->cluster_count || file->guard > v->cluster_count) return OS_FAT16_CORRUPT;
        lba = v->data_lba + (uint32_t)(file->cluster - 2U) * v->sectors_per_cluster + sector_in_cluster;
        if (!file->cache_valid || file->cached_lba != lba) {
            if (read_at(v, lba, file->sector_cache) != 0) return OS_FAT16_CORRUPT;
            file->cached_lba = lba;
            file->cache_valid = 1U;
        }
        if (take > max - copied) take = max - copied;
        if (take > file->size - file->position) take = file->size - file->position;
        for (uint32_t i = 0U; i < take; i++) buffer[copied + i] = file->sector_cache[sector_offset + i];
        copied += take;
        file->position += take;
        file->cluster_offset += take;
        if (file->cluster_offset >= cluster_bytes && file->position < file->size) {
            if (file->guard++ >= v->cluster_count ||
                read_fat_entry(v, file->cluster, &file->cluster) != 0) return OS_FAT16_CORRUPT;
            file->cluster_offset = 0U;
        }
    }
    *out_read = copied;
    return 0;
}

/* Support des sous-répertoires FAT16 VFS multi-niveaux. */

static int fat16_directory_slot(const fat16_volume_t* v, uint16_t cluster,
                                uint32_t index, uint8_t* entry, int write);

static int fat16_directory_find_short(const fat16_volume_t* v, uint16_t cluster,
                                      const char* name, uint8_t* entry_out,
                                      uint32_t* index_out);

static void make_short_alias(const char* name, char* out_alias);

static int fat16_directory_create_lfn(const fat16_volume_t* v, uint16_t cluster,
                                      const char* long_name, const char* short_name,
                                      uint8_t attributes, uint16_t first_cluster,
                                      uint32_t size);

static int fat16_root_find_short(const fat16_volume_t* v, const char* name,
                                 uint8_t* entry_out, uint32_t* index_out) {
    uint8_t short_name[11];
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint32_t index;
    if (!v || !name || !entry_out || !index_out || make_short_name(name, short_name) != 0) {
        return OS_FAT16_BAD_PATH;
    }
    for (index = 0U; index < v->root_entries; index++) {
        if (read_root_entry(v, index, entry) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U || entry[11] == 0x0FU) continue;
        if (entry_matches(entry, short_name)) {
            uint32_t i;
            for (i = 0U; i < FAT16_ENTRY_SIZE; i++) entry_out[i] = entry[i];
            *index_out = index;
            return 0;
        }
    }
    return OS_FAT16_NOT_FOUND;
}

/* Résolution multi-niveaux de sous-répertoire (ex: "DIR1" ou "DIR1/DIR2") */
static int fat16_directory_cluster(const fat16_volume_t* v, const char* name,
                                   uint16_t* cluster_out) {
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint32_t index, i = 0U;
    uint16_t current_cluster = 0U;
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
            rc = fat16_root_find_short(v, segment, entry, &index);
        } else {
            rc = fat16_directory_find_short(v, current_cluster, segment, entry, &index);
        }
        if (rc != 0) return rc;
        if ((entry[11] & 0x10U) == 0U) return OS_FAT16_BAD_PATH;

        current_cluster = le16(entry + 26U);
        if (current_cluster < 2U || (uint32_t)current_cluster > v->cluster_count + 1U) {
            return OS_FAT16_CORRUPT;
        }

        if (name[i] == '\0') break;
        i++; // Sauter '/'
        if (name[i] == '\0') break; // Trailing slash admis
    }

    *cluster_out = current_cluster;
    return 0;
}

static int fat16_split_subpath(const char* path, char* directory, char* leaf) {
    uint32_t i = 0U;
    uint32_t last_slash = 0xffffffffU;
    uint32_t directory_size = 0U;
    uint32_t leaf_size = 0U;
    if (!path || !directory || !leaf || path[0] == '\0' || path[0] == '/') return OS_FAT16_BAD_PATH;
    while (path[i] != '\0') {
        if (i + 1U >= OS_NAME_MAX * 2U) return OS_FAT16_BAD_PATH;
        if (path[i] == '/') {
            if (i == 0U || path[i + 1U] == '\0') return OS_FAT16_BAD_PATH;
            last_slash = i;
        }
        i++;
    }
    if (last_slash == 0xffffffffU) return OS_FAT16_NOT_FOUND;
    directory_size = last_slash;
    leaf_size = i - last_slash - 1U;
    if (directory_size >= OS_NAME_MAX * 2U || leaf_size >= OS_NAME_MAX) return OS_FAT16_BAD_PATH;
    for (i = 0U; i < directory_size; i++) directory[i] = path[i];
    directory[directory_size] = '\0';
    for (i = 0U; i < leaf_size; i++) leaf[i] = path[last_slash + 1U + i];
    leaf[leaf_size] = '\0';
    return 0;
}

static int fat16_directory_slot(const fat16_volume_t* v, uint16_t cluster,
                                uint32_t index, uint8_t* entry, int write) {
    uint32_t entries_per_cluster;
    uint32_t lba;
    uint32_t sector_index;
    uint32_t entry_index;
    uint32_t i;
    if (!v || !entry || cluster < 2U || (uint32_t)cluster > v->cluster_count + 1U) {
        return OS_FAT16_CORRUPT;
    }
    entries_per_cluster = (uint32_t)v->sectors_per_cluster * 16U;
    if (index >= entries_per_cluster) return OS_FAT16_NOT_FOUND;
    sector_index = index / 16U;
    entry_index = index % 16U;
    lba = v->data_lba + (uint32_t)(cluster - 2U) * v->sectors_per_cluster + sector_index;
    if (read_at(v, lba, sector) != 0) return OS_FAT16_CORRUPT;
    if (!write) {
        for (i = 0U; i < FAT16_ENTRY_SIZE; i++) entry[i] = sector[entry_index * FAT16_ENTRY_SIZE + i];
        return 0;
    }
    for (i = 0U; i < FAT16_ENTRY_SIZE; i++) sector[entry_index * FAT16_ENTRY_SIZE + i] = entry[i];
    return fat16_write_sector(v, lba, sector);
}

static int fat16_directory_find_short(const fat16_volume_t* v, uint16_t cluster,
                                      const char* name, uint8_t* entry_out,
                                      uint32_t* index_out) {
    uint8_t short_name[11];
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t index, limit, lfn_length = 0U;
    uint8_t lfn_sum = 0U, lfn_expected = 0U, lfn_valid = 0U;
    int short_valid;
    if (!v || !name || !entry_out || !index_out || !fat16_lfn_query_valid(name)) {
        return OS_FAT16_BAD_PATH;
    }
    short_valid = make_short_name(name, short_name) == 0;
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        uint8_t ordinal;
        if (fat16_directory_slot(v, cluster, index, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U) { lfn_valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ordinal = entry[0] & 0x1FU;
            if ((entry[0] & 0x40U) != 0U) {
                lfn_length = (uint32_t)ordinal * 13U;
                if (lfn_length >= OS_NAME_MAX) { lfn_valid = 0U; continue; }
                for (uint32_t j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; lfn_expected = ordinal; lfn_valid = 1U;
            }
            if (!lfn_valid || ordinal == 0U || ordinal != lfn_expected || entry[13] != lfn_sum) {
                lfn_valid = 0U; continue;
            }
            fat16_lfn_get(entry, 1U, (uint32_t)(ordinal - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 14U, (uint32_t)(ordinal - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 28U, (uint32_t)(ordinal - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            lfn_expected--;
            continue;
        }
        if ((entry[11] & 0x08U) != 0U) { lfn_valid = 0U; continue; }
        if ((short_valid && entry_matches(entry, short_name)) ||
            (lfn_valid && lfn_expected == 0U && fat16_lfn_checksum(entry) == lfn_sum &&
             fat16_lfn_name_equals_folded(lfn_units, name))) {
            uint32_t i;
            for (i = 0U; i < FAT16_ENTRY_SIZE; i++) entry_out[i] = entry[i];
            *index_out = index;
            return 0;
        }
        lfn_valid = 0U;
    }
    return OS_FAT16_NOT_FOUND;
}

static void make_short_alias(const char* name, char* out_alias) {
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

static int fat16_directory_create_short(const fat16_volume_t* v, uint16_t cluster,
                                        const char* name, uint8_t attributes,
                                        uint16_t first_cluster, uint32_t size) {
    uint8_t short_name[11];
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint32_t index;
    uint32_t free_index = 0xffffffffU;
    uint32_t limit;
    if (!v || !name || (attributes & 0x0FU) == 0x0FU ||
        (first_cluster != 0U && (first_cluster < 2U || (uint32_t)first_cluster > v->cluster_count + 1U)) ||
        make_short_name(name, short_name) != 0) return OS_FAT16_BAD_PATH;
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        if (fat16_directory_slot(v, cluster, index, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) { free_index = index; break; }
        if (entry[0] == 0xE5U) { if (free_index == 0xffffffffU) free_index = index; continue; }
        if (entry[11] != 0x0FU && entry_matches(entry, short_name)) return OS_FAT16_BAD_PATH;
    }
    if (free_index == 0xffffffffU) return OS_FAT16_NOT_FOUND;
    for (index = 0U; index < FAT16_ENTRY_SIZE; index++) entry[index] = 0U;
    for (index = 0U; index < 11U; index++) entry[index] = short_name[index];
    entry[11] = attributes;
    entry[26] = (uint8_t)first_cluster;
    entry[27] = (uint8_t)(first_cluster >> 8U);
    entry[28] = (uint8_t)size;
    entry[29] = (uint8_t)(size >> 8U);
    entry[30] = (uint8_t)(size >> 16U);
    entry[31] = (uint8_t)(size >> 24U);
    return fat16_directory_slot(v, cluster, free_index, entry, 1);
}

static int fat16_directory_create_lfn(const fat16_volume_t* v, uint16_t cluster,
                                      const char* long_name, const char* short_name,
                                      uint8_t attributes, uint16_t first_cluster,
                                      uint32_t size) {
    uint8_t alias[11], entry[FAT16_ENTRY_SIZE];
    uint16_t units[OS_NAME_MAX];
    uint32_t length = 0U, i, count, start_slot = 0U, consecutive = 0U;
    uint32_t limit = (uint32_t)v->sectors_per_cluster * 16U;
    int rc;

    if (!v || !long_name || !short_name) return OS_FAT16_BAD_PATH;
    if (lfn_utf8_to_utf16_bmp(long_name, units, OS_NAME_MAX, &length) != 0 ||
        make_short_name(short_name, alias) != 0) return OS_FAT16_BAD_PATH;
    count = (length + 12U) / 13U;
    if (count == 0U || count > 20U) return OS_FAT16_BAD_PATH;

    for (i = 0U; i < limit; i++) {
        if (fat16_directory_slot(v, cluster, i, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] != 0U && entry[0] != 0xE5U) {
            if (entry_matches(entry, alias)) return OS_FAT16_BAD_PATH;
            consecutive = 0U;
        } else {
            if (consecutive == 0U) start_slot = i;
            consecutive++;
            if (consecutive == count + 1U) break;
        }
    }
    if (consecutive < count + 1U) return OS_FAT16_NOT_FOUND;

    for (i = 0U; i < count; i++) {
        uint32_t ordinal = count - i;
        uint32_t pos = (ordinal - 1U) * 13U;
        uint32_t j;
        for (j = 0U; j < 32U; j++) entry[j] = 0xFFU;
        entry[0] = (uint8_t)ordinal | (i == 0U ? 0x40U : 0U);
        entry[11] = 0x0FU; entry[12] = 0U; entry[13] = fat16_lfn_checksum(alias);
        entry[26] = 0U; entry[27] = 0U;
        fat16_lfn_put(entry, 1U, pos, units, length);
        fat16_lfn_put(entry, 14U, pos + 5U, units, length);
        fat16_lfn_put(entry, 28U, pos + 11U, units, length);
        rc = fat16_directory_slot(v, cluster, start_slot + i, entry, 1);
        if (rc != 0) return rc;
    }

    for (i = 0U; i < 32U; i++) entry[i] = 0U;
    for (i = 0U; i < 11U; i++) entry[i] = alias[i];
    entry[11] = attributes; entry[26] = (uint8_t)first_cluster; entry[27] = (uint8_t)(first_cluster >> 8U);
    entry[28] = (uint8_t)size; entry[29] = (uint8_t)(size >> 8U);
    entry[30] = (uint8_t)(size >> 16U); entry[31] = (uint8_t)(size >> 24U);
    return fat16_directory_slot(v, cluster, start_slot + count, entry, 1);
}

static int fat16_read_entry_data(const fat16_volume_t* v, const uint8_t* entry,
                                 char* buffer, uint32_t max) {
    uint32_t copied = 0U;
    uint32_t size;
    uint16_t cluster;
    uint32_t guard = 0U;
    if (!v || !entry || !buffer || max == 0U) return OS_FAT16_BUFFER_SMALL;
    if (entry[11] & 0x10U) return OS_FAT16_BAD_PATH;
    size = le32(entry + 28U);
    if (size > max) return OS_FAT16_BUFFER_SMALL;
    cluster = le16(entry + 26U);
    while (copied < size) {
        uint32_t sector_index;
        if (cluster < 2U || cluster >= FAT16_EOC_MIN || (uint32_t)cluster > v->cluster_count + 1U ||
            guard++ > v->cluster_count) return OS_FAT16_CORRUPT;
        for (sector_index = 0U; sector_index < v->sectors_per_cluster && copied < size; sector_index++) {
            uint32_t take = size - copied;
            uint32_t i;
            uint32_t lba = v->data_lba + (uint32_t)(cluster - 2U) * v->sectors_per_cluster + sector_index;
            if (read_at(v, lba, sector2) != 0) return OS_FAT16_CORRUPT;
            if (take > FAT16_SECTOR_SIZE) take = FAT16_SECTOR_SIZE;
            for (i = 0U; i < take; i++) buffer[copied + i] = (char)sector2[i];
            copied += take;
        }
        if (copied < size && read_fat_entry(v, cluster, &cluster) != 0) return OS_FAT16_CORRUPT;
    }
    return (int)copied;
}

static int fat16_create_directory_file(const fat16_volume_t* v, uint16_t directory,
                                       const char* name, const uint8_t* data, uint32_t size,
                                       uint16_t* out_first_cluster) {
    uint32_t cluster_bytes;
    uint32_t remaining;
    uint32_t offset = 0U;
    uint16_t first = 0U;
    uint16_t previous = 0U;
    uint16_t current;
    uint8_t dummy_short[11];
    int rc;
    if (!v || !name || !out_first_cluster || (size != 0U && !data)) return OS_FAT16_BAD_PATH;
    cluster_bytes = (uint32_t)v->bytes_per_sector * v->sectors_per_cluster;
    remaining = size == 0U ? 1U : size;
    while (remaining != 0U) {
        rc = fat16_allocate_cluster(v, &current);
        if (rc != 0) { if (first != 0U) (void)fat16_release_chain(v, first); return rc; }
        if (first == 0U) first = current;
        if (previous != 0U && fat16_link_clusters(v, previous, current) != 0) {
            (void)fat16_release_chain(v, first); return OS_FAT16_CORRUPT;
        }
        if (size != 0U) {
            uint32_t chunk = remaining > cluster_bytes ? cluster_bytes : remaining;
            rc = fat16_write_cluster_range(v, current, 0U, data + offset, chunk);
            if (rc != 0) { (void)fat16_release_chain(v, first); return rc; }
            offset += chunk;
            remaining -= chunk;
        } else {
            remaining = 0U;
        }
        previous = current;
    }
    if (make_short_name(name, dummy_short) == 0) {
        rc = fat16_directory_create_short(v, directory, name, 0x20U, first, size);
    } else {
        char short_alias[16];
        make_short_alias(name, short_alias);
        rc = fat16_directory_create_lfn(v, directory, name, short_alias, 0x20U, first, size);
    }
    if (rc != 0) { (void)fat16_release_chain(v, first); return rc; }
    *out_first_cluster = first;
    return 0;
}

int fat16_list_path_page(const fat16_volume_t* v, const char* path, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    char lfn[OS_NAME_MAX];
    uint16_t lfn_units[OS_NAME_MAX];
    uint32_t lfn_length = 0U;
    uint8_t lfn_sum = 0U, lfn_expected = 0U, lfn_valid = 0U;
    uint16_t cluster;
    uint32_t index;
    uint32_t seen = 0U;
    uint32_t count = 0U;
    uint32_t limit;
    uint8_t entry[FAT16_ENTRY_SIZE];
    int rc;
    if (!v || !path || !out || capacity == 0U) return OS_FAT16_BAD_PATH;
    if (path[0] == '/' && path[1] == '\0') return fat16_list_root_page(v, start, out, capacity);
    if (path[0] != '\0' && path[fat16_text_length(path) - 1U] == '/') {
        uint32_t n = fat16_text_length(path) - 1U;
        if (n == 0U || n >= OS_NAME_MAX) return OS_FAT16_BAD_PATH;
        for (index = 0U; index < n; index++) directory[index] = path[index];
        directory[n] = '\0';
        rc = fat16_directory_cluster(v, directory, &cluster);
    } else {
        rc = fat16_split_subpath(path, directory, leaf);
        if (rc == OS_FAT16_NOT_FOUND) return OS_FAT16_BAD_PATH;
        return rc;
    }
    if (rc != 0) return rc;
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        uint8_t ordinal;
        if (fat16_directory_slot(v, cluster, index, entry, 0) != 0) return OS_FAT16_CORRUPT;
        if (entry[0] == 0U) break;
        if (entry[0] == 0xE5U) { lfn_valid = 0U; continue; }
        if (entry[11] == 0x0FU) {
            ordinal = entry[0] & 0x1FU;
            if ((entry[0] & 0x40U) != 0U) {
                lfn_length = ordinal * 13U;
                if (lfn_length >= OS_NAME_MAX) { lfn_valid = 0U; continue; }
                for (uint32_t j = 0U; j < OS_NAME_MAX; j++) lfn_units[j] = 0U;
                lfn_sum = entry[13]; lfn_expected = ordinal; lfn_valid = 1U;
            }
            if (!lfn_valid || ordinal == 0U || ordinal != lfn_expected || entry[13] != lfn_sum) {
                lfn_valid = 0U; continue;
            }
            fat16_lfn_get(entry, 1U, (uint32_t)(ordinal - 1U) * 13U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 14U, (uint32_t)(ordinal - 1U) * 13U + 5U, lfn_units, OS_NAME_MAX);
            fat16_lfn_get(entry, 28U, (uint32_t)(ordinal - 1U) * 13U + 11U, lfn_units, OS_NAME_MAX);
            lfn_expected--;
            continue;
        }
        if (entry[11] & 0x08U || entry[0] == '.') { lfn_valid = 0U; continue; }
        if (seen++ < start) { lfn_valid = 0U; continue; }
        if (count >= capacity) return (int)count;
        if (!(lfn_valid && lfn_expected == 0U && fat16_lfn_checksum(entry) == lfn_sum &&
              lfn_utf16_bmp_to_utf8(lfn_units, OS_NAME_MAX, lfn, OS_NAME_MAX) >= 0)) {
            copy_name(out[count].name, entry);
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

int fat16_read_path(const fat16_volume_t* v, const char* path, char* buffer, uint32_t max) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint32_t index;
    uint16_t cluster;
    int rc = fat16_split_subpath(path, directory, leaf);
    if (rc == OS_FAT16_NOT_FOUND) return fat16_read_file(v, path, buffer, max);
    if (rc != 0) return rc;
    rc = fat16_directory_cluster(v, directory, &cluster);
    if (rc != 0) return rc;
    rc = fat16_directory_find_short(v, cluster, leaf, entry, &index);
    if (rc != 0) return rc;
    return fat16_read_entry_data(v, entry, buffer, max);
}

int fat16_create_path_file(const fat16_volume_t* v, const char* path,
                           const uint8_t* data, uint32_t size, uint16_t* out_first_cluster) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    uint16_t cluster;
    int rc = fat16_split_subpath(path, directory, leaf);
    if (rc == OS_FAT16_NOT_FOUND) return fat16_create_file(v, path, 0x20U, data, size, out_first_cluster);
    if (rc != 0) return rc;
    if ((rc = fat16_directory_cluster(v, directory, &cluster)) != 0) return rc;
    return fat16_create_directory_file(v, cluster, leaf, data, size, out_first_cluster);
}

int fat16_unlink_path_file(const fat16_volume_t* v, const char* path) {
    char directory[OS_NAME_MAX];
    char leaf[OS_NAME_MAX];
    uint16_t cluster;
    uint32_t index;
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint16_t first;
    uint8_t lfn_sum;
    int32_t start_index;
    int rc = fat16_split_subpath(path, directory, leaf);
    if (rc == OS_FAT16_NOT_FOUND) return fat16_unlink_file(v, path);
    if (rc != 0 || (rc = fat16_directory_cluster(v, directory, &cluster)) != 0) return rc;
    if ((rc = fat16_directory_find_short(v, cluster, leaf, entry, &index)) != 0) return rc;
    if (entry[11] & 0x10U) return OS_FAT16_BAD_PATH;
    first = le16(entry + 26U);
    lfn_sum = fat16_lfn_checksum(entry);
    start_index = (int32_t)index;
    while (start_index > 0) {
        uint8_t prev[FAT16_ENTRY_SIZE];
        if (fat16_directory_slot(v, cluster, (uint32_t)(start_index - 1), prev, 0) != 0) break;
        if (prev[11] == 0x0FU && prev[13] == lfn_sum && prev[0] != 0xE5U) {
            start_index--;
            if ((prev[0] & 0x40U) != 0U) break;
        } else {
            break;
        }
    }
    for (int32_t j = start_index; j <= (int32_t)index; j++) {
        uint8_t del[FAT16_ENTRY_SIZE];
        if (fat16_directory_slot(v, cluster, (uint32_t)j, del, 0) == 0) {
            del[0] = 0xE5U;
            (void)fat16_directory_slot(v, cluster, (uint32_t)j, del, 1);
        }
    }
    return first == 0U ? 0 : fat16_release_chain(v, first);
}

int fat16_rename_path_file(const fat16_volume_t* v, const char* old_path, const char* new_path) {
    char old_directory[OS_NAME_MAX];
    char old_leaf[OS_NAME_MAX];
    char new_directory[OS_NAME_MAX];
    char new_leaf[OS_NAME_MAX];
    uint16_t cluster;
    uint32_t old_index;
    uint32_t ignored_index;
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint8_t ignored[FAT16_ENTRY_SIZE];
    uint8_t short_name[11];
    uint32_t i;
    int old_rc = fat16_split_subpath(old_path, old_directory, old_leaf);
    int new_rc = fat16_split_subpath(new_path, new_directory, new_leaf);
    if (old_rc == OS_FAT16_NOT_FOUND && new_rc == OS_FAT16_NOT_FOUND) {
        return fat16_rename_file(v, old_path, new_path);
    }
    if (old_rc != 0 || new_rc != 0 || !fat16_name_equals_folded(old_directory, new_directory) ||
        (old_rc = fat16_directory_cluster(v, old_directory, &cluster)) != 0 ||
        fat16_directory_find_short(v, cluster, old_leaf, entry, &old_index) != 0 ||
        entry[11] & 0x10U || make_short_name(new_leaf, short_name) != 0) return OS_FAT16_BAD_PATH;
    if (fat16_directory_find_short(v, cluster, new_leaf, ignored, &ignored_index) == 0 && ignored_index != old_index) {
        return OS_FAT16_BAD_PATH;
    }
    for (i = 0U; i < 11U; i++) entry[i] = short_name[i];
    return fat16_directory_slot(v, cluster, old_index, entry, 1);
}

int fat16_create_directory(const fat16_volume_t* v, const char* name) {
    char directory[OS_NAME_MAX * 2U];
    char leaf[OS_NAME_MAX];
    uint16_t parent_cluster = 0U;
    uint16_t cluster;
    uint8_t dot[FAT16_ENTRY_SIZE];
    uint32_t sector_index;
    int rc = fat16_split_subpath(name, directory, leaf);

    if (rc == OS_FAT16_NOT_FOUND) {
        if (!v || !name || make_short_name(name, sector2) != 0) return OS_FAT16_BAD_PATH;
        if (fat16_root_find_short(v, name, dot, &sector_index) == 0) return OS_FAT16_BAD_PATH;
        if ((rc = fat16_allocate_cluster(v, &cluster)) != 0) return rc;
        for (sector_index = 0U; sector_index < FAT16_SECTOR_SIZE; sector_index++) sector2[sector_index] = 0U;
        for (sector_index = 0U; sector_index < v->sectors_per_cluster; sector_index++) {
            uint32_t lba = v->data_lba + (uint32_t)(cluster - 2U) * v->sectors_per_cluster + sector_index;
            if (fat16_write_sector(v, lba, sector2) != 0) { (void)fat16_release_chain(v, cluster); return OS_FAT16_CORRUPT; }
        }
        for (sector_index = 0U; sector_index < FAT16_ENTRY_SIZE; sector_index++) dot[sector_index] = 0U;
        for (sector_index = 0U; sector_index < 11U; sector_index++) dot[sector_index] = ' ';
        dot[0] = '.'; dot[11] = 0x10U; dot[26] = (uint8_t)cluster; dot[27] = (uint8_t)(cluster >> 8U);
        if ((rc = fat16_directory_slot(v, cluster, 0U, dot, 1)) != 0) { (void)fat16_release_chain(v, cluster); return rc; }
        dot[1] = '.'; dot[26] = 0U; dot[27] = 0U;
        if ((rc = fat16_directory_slot(v, cluster, 1U, dot, 1)) != 0 ||
            (rc = fat16_create_root_entry(v, name, 0x10U, cluster, 0U)) != 0) {
            (void)fat16_release_chain(v, cluster); return rc;
        }
        return 0;
    }
    if (rc != 0) return rc;

    rc = fat16_directory_cluster(v, directory, &parent_cluster);
    if (rc != 0) return rc;
    if (make_short_name(leaf, sector2) != 0) return OS_FAT16_BAD_PATH;
    if (fat16_directory_find_short(v, parent_cluster, leaf, dot, &sector_index) == 0) return OS_FAT16_BAD_PATH;

    if ((rc = fat16_allocate_cluster(v, &cluster)) != 0) return rc;
    for (sector_index = 0U; sector_index < FAT16_SECTOR_SIZE; sector_index++) sector2[sector_index] = 0U;
    for (sector_index = 0U; sector_index < v->sectors_per_cluster; sector_index++) {
        uint32_t lba = v->data_lba + (uint32_t)(cluster - 2U) * v->sectors_per_cluster + sector_index;
        if (fat16_write_sector(v, lba, sector2) != 0) { (void)fat16_release_chain(v, cluster); return OS_FAT16_CORRUPT; }
    }
    for (sector_index = 0U; sector_index < FAT16_ENTRY_SIZE; sector_index++) dot[sector_index] = 0U;
    for (sector_index = 0U; sector_index < 11U; sector_index++) dot[sector_index] = ' ';
    dot[0] = '.'; dot[11] = 0x10U; dot[26] = (uint8_t)cluster; dot[27] = (uint8_t)(cluster >> 8U);
    if ((rc = fat16_directory_slot(v, cluster, 0U, dot, 1)) != 0) { (void)fat16_release_chain(v, cluster); return rc; }
    dot[1] = '.'; dot[26] = (uint8_t)parent_cluster; dot[27] = (uint8_t)(parent_cluster >> 8U);
    if ((rc = fat16_directory_slot(v, cluster, 1U, dot, 1)) != 0 ||
        (rc = fat16_directory_create_short(v, parent_cluster, leaf, 0x10U, cluster, 0U)) != 0) {
        (void)fat16_release_chain(v, cluster); return rc;
    }
    return 0;
}

int fat16_remove_directory(const fat16_volume_t* v, const char* name) {
    char directory[OS_NAME_MAX * 2U];
    char leaf[OS_NAME_MAX];
    uint8_t entry[FAT16_ENTRY_SIZE];
    uint32_t dir_index;
    uint16_t parent_cluster = 0U;
    uint16_t cluster;
    uint32_t index;
    uint32_t limit;
    int rc = fat16_split_subpath(name, directory, leaf);

    if (rc == OS_FAT16_NOT_FOUND) {
        rc = fat16_root_find_short(v, name, entry, &dir_index);
        if (rc != 0) return rc;
        if ((entry[11] & 0x10U) == 0U) return OS_FAT16_BAD_PATH;
        cluster = le16(entry + 26U);
        limit = (uint32_t)v->sectors_per_cluster * 16U;
        for (index = 0U; index < limit; index++) {
            uint8_t child[FAT16_ENTRY_SIZE];
            if (fat16_directory_slot(v, cluster, index, child, 0) != 0) return OS_FAT16_CORRUPT;
            if (child[0] == 0U) break;
            if (child[0] == 0xE5U || child[0] == '.') continue;
            return OS_FAT16_BAD_PATH;
        }
        entry[0] = 0xE5U;
        if ((rc = fat16_write_root_slot(v, dir_index, entry)) != 0) return rc;
        return fat16_release_chain(v, cluster);
    }
    if (rc != 0) return rc;

    rc = fat16_directory_cluster(v, directory, &parent_cluster);
    if (rc != 0) return rc;
    rc = fat16_directory_find_short(v, parent_cluster, leaf, entry, &dir_index);
    if (rc != 0) return rc;
    if ((entry[11] & 0x10U) == 0U) return OS_FAT16_BAD_PATH;
    cluster = le16(entry + 26U);
    limit = (uint32_t)v->sectors_per_cluster * 16U;
    for (index = 0U; index < limit; index++) {
        uint8_t child[FAT16_ENTRY_SIZE];
        if (fat16_directory_slot(v, cluster, index, child, 0) != 0) return OS_FAT16_CORRUPT;
        if (child[0] == 0U) break;
        if (child[0] == 0xE5U || child[0] == '.') continue;
        return OS_FAT16_BAD_PATH;
    }
    entry[0] = 0xE5U;
    if ((rc = fat16_directory_slot(v, parent_cluster, dir_index, entry, 1)) != 0) return rc;
    return fat16_release_chain(v, cluster);
}
