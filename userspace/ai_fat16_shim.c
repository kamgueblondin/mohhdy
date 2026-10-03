/* aiworker GGUF slice: the GGUF loader/runtime (kernel/llm/gpt2_gguf*.c)
 * reads weights only through fat16_read_file_range()/fat16_open_file().
 * In Ring 3 those calls are served from the worker's own copy of GPT2.GGU,
 * loaded once with the worker-only bulk read SYS_AI_ENGINE GGUF_READ. No
 * FAT, no disk and no kernel memory are touched here. FAT32 is reported
 * unmounted (the kernel only exposes the FAT16 GGUF to the worker). */
#include <stdint.h>
#include "../kernel/fs/fat16.h"
#include "../kernel/fs/fat32.h"
#include "ai_fat16_shim.h"

static const uint8_t* shim_data;
static uint32_t shim_size;
static char shim_name[13];

static int shim_name_is(const char* name) {
    uint32_t i = 0U;
    if (!name || !shim_data) return 0;
    while (shim_name[i] != '\0' && name[i] != '\0') {
        if (shim_name[i] != name[i]) return 0;
        i++;
    }
    return shim_name[i] == '\0' && name[i] == '\0';
}

int ai_fat16_shim_attach(fat16_volume_t* volume, const char* name,
                         const uint8_t* data, uint32_t size) {
    uint32_t i = 0U;
    uint8_t* raw = (uint8_t*)volume;
    if (!volume || !name || !data || size == 0U) return -1;
    while (name[i] != '\0') {
        if (i + 1U >= sizeof(shim_name)) return -1;
        shim_name[i] = name[i];
        i++;
    }
    shim_name[i] = '\0';
    for (i = 0U; i < (uint32_t)sizeof(*volume); i++) raw[i] = 0U;
    volume->mounted = 1U;
    shim_data = data;
    shim_size = size;
    return 0;
}

int fat16_is_mounted(const fat16_volume_t* volume) {
    return volume && volume->mounted && shim_data ? 1 : 0;
}

int fat16_read_file_range(const fat16_volume_t* volume, const char* name,
                          uint32_t offset, uint8_t* buffer, uint32_t max,
                          uint32_t* out_read) {
    uint32_t n = max;
    uint32_t i;
    if (out_read) *out_read = 0U;
    if (!fat16_is_mounted(volume)) return OS_FAT16_NOT_MOUNTED;
    if (!buffer || !out_read) return OS_FAT16_BAD_PATH;
    if (!shim_name_is(name)) return OS_FAT16_NOT_FOUND;
    if (offset > shim_size) return OS_FAT16_BAD_PATH;
    if (n > shim_size - offset) n = shim_size - offset;
    for (i = 0U; i < n; i++) buffer[i] = shim_data[offset + i];
    *out_read = n;
    return 0;
}

int fat16_read_file(const fat16_volume_t* volume, const char* name, char* buffer, uint32_t max) {
    uint32_t read = 0U;
    int status = fat16_read_file_range(volume, name, 0U, (uint8_t*)buffer, max, &read);
    return status != 0 ? status : (int)read;
}

int fat16_open_file(const fat16_volume_t* volume, const char* name, fat16_file_t* out) {
    uint32_t i;
    if (!fat16_is_mounted(volume)) return OS_FAT16_NOT_MOUNTED;
    if (!out) return OS_FAT16_BAD_PATH;
    if (!shim_name_is(name)) return OS_FAT16_NOT_FOUND;
    for (i = 0U; i < (uint32_t)sizeof(*out); i++) ((uint8_t*)out)[i] = 0U;
    out->volume = volume;
    out->size = shim_size;
    out->open = 1U;
    return 0;
}

int fat16_file_seek(fat16_file_t* file, uint32_t offset) {
    if (!file || !file->open || !shim_data) return OS_FAT16_BAD_PATH;
    if (offset > file->size) return OS_FAT16_BAD_PATH;
    file->position = offset;
    return 0;
}

int fat16_file_read(fat16_file_t* file, uint8_t* buffer, uint32_t max, uint32_t* out_read) {
    uint32_t n = max;
    uint32_t i;
    if (out_read) *out_read = 0U;
    if (!file || !file->open || !shim_data || !buffer || !out_read) return OS_FAT16_BAD_PATH;
    if (file->position > file->size) return OS_FAT16_BAD_PATH;
    if (n > file->size - file->position) n = file->size - file->position;
    for (i = 0U; i < n; i++) buffer[i] = shim_data[file->position + i];
    file->position += n;
    *out_read = n;
    return 0;
}

int fat32_is_mounted(const fat32_volume_t* volume) {
    (void)volume;
    return 0;
}

int fat32_list_root_page(const fat32_volume_t* volume, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t max) {
    (void)volume; (void)start; (void)out; (void)max;
    return OS_FAT16_NOT_MOUNTED;
}

int fat32_read_file_range(const fat32_volume_t* volume, const char* name,
                          uint32_t offset, uint8_t* buffer, uint32_t max,
                          uint32_t* out_read) {
    (void)volume; (void)name; (void)offset; (void)buffer; (void)max;
    if (out_read) *out_read = 0U;
    return OS_FAT16_NOT_MOUNTED;
}
