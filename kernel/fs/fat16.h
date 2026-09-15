#ifndef MOHHDY_FAT16_H
#define MOHHDY_FAT16_H

#include <stdint.h>
#include "../../include/os_syscalls.h"

typedef int (*fat16_read_sector_fn)(uint32_t lba, void* buffer);
typedef int (*fat16_read_sectors_fn)(uint32_t lba, uint32_t count, void* buffer);
typedef int (*fat16_write_sector_fn)(uint32_t lba, const void* buffer);

typedef struct {
    fat16_read_sector_fn read_sector;
    fat16_read_sectors_fn read_sectors;
    fat16_write_sector_fn write_sector;
    uint8_t* read_window;
    uint32_t read_window_capacity;
    uint32_t read_window_lba;
    uint8_t read_window_sectors;
    uint8_t read_window_valid;
    uint32_t base_lba;
    uint32_t total_sectors;
    uint32_t fat_lba;
    uint32_t fat_sectors;
    uint32_t root_lba;
    uint32_t root_sectors;
    uint32_t data_lba;
    uint32_t cluster_count;
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint8_t fat_count;
    uint16_t root_entries;
    uint8_t mounted;
} fat16_volume_t;

typedef struct {
    const fat16_volume_t* volume;
    uint16_t first_cluster;
    uint16_t cluster;
    uint32_t size;
    uint32_t position;
    uint32_t cluster_offset;
    uint32_t guard;
    uint32_t cached_lba;
    uint8_t sector_cache[512];
    uint8_t cache_valid;
    uint8_t open;
} fat16_file_t;

fat16_volume_t* fat16_root(void);
int fat16_mount(fat16_volume_t* volume, fat16_read_sector_fn read_sector,
                uint32_t base_lba);
int fat16_is_mounted(const fat16_volume_t* volume);
/* Attache un lecteur multi-secteurs et sa fenêtre caller-owned, sans cache implicite. */
int fat16_attach_read_window(fat16_volume_t* volume, fat16_read_sectors_fn read_sectors,
                             uint8_t* window, uint32_t window_capacity);
/* Attache explicitement un writer caller-owned ; aucun writer implicite n’est créé au montage. */
int fat16_attach_writer(fat16_volume_t* volume, fat16_write_sector_fn write_sector);
int fat16_write_sector(const fat16_volume_t* volume, uint32_t lba, const uint8_t* buffer);
/* Écrit une plage dans un cluster existant ; n’alloue aucun cluster et reste caller-owned. */
int fat16_write_cluster_range(const fat16_volume_t* volume, uint16_t cluster,
                              uint32_t offset, const uint8_t* buffer, uint32_t length);
/* Réserve le premier cluster FAT libre et le marque EOC dans toutes les FAT. */
int fat16_allocate_cluster(const fat16_volume_t* volume, uint16_t* out_cluster);
/* Relie un cluster source EOC à une cible déjà allouée, sans allocation implicite. */
int fat16_link_clusters(const fat16_volume_t* volume, uint16_t source, uint16_t target);
/* Crée une entrée 8.3 dans la racine ; le cluster et les buffers sont caller-owned. */
int fat16_create_root_entry(const fat16_volume_t* volume, const char* name,
                            uint8_t attributes, uint16_t first_cluster, uint32_t size);
/* Crée un fichier 8.3 persistant depuis un buffer caller-owned, sans kmalloc. */
int fat16_create_file(const fat16_volume_t* volume, const char* name,
                      uint8_t attributes, const uint8_t* data, uint32_t size,
                      uint16_t* out_first_cluster);
/* Supprime un fichier 8.3 classique de la racine et libère sa chaîne, sans LFN ni répertoire. */
int fat16_unlink_file(const fat16_volume_t* volume, const char* name);
/* Renomme un fichier 8.3 classique de la racine sans déplacer sa chaîne, sans LFN ni répertoire. */
int fat16_rename_file(const fat16_volume_t* volume, const char* old_name, const char* new_name);
int fat16_rename_lfn_file(const fat16_volume_t* volume, const char* old_name,
                          const char* new_long_name, const char* new_short_name);
/* Crée un fichier avec une séquence LFN ASCII bornée et un alias 8.3 explicite. */
int fat16_create_lfn_file(const fat16_volume_t* volume, const char* long_name,
                          const char* short_name, uint8_t attributes,
                          const uint8_t* data, uint32_t size,
                          uint16_t* out_first_cluster);
int fat16_list_root(const fat16_volume_t* volume, os_fat16_dirent_t* out,
                    uint32_t capacity);
/* Retourne une page de racine à partir d’un index logique, sans allocation. */
int fat16_list_root_page(const fat16_volume_t* volume, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity);
/* Sous-répertoires FAT16 bornés à un niveau : les noms de répertoire et de
 * fichier enfant sont 8.3. Les LFN racine existants restent inchangés. */
int fat16_list_path_page(const fat16_volume_t* volume, const char* path, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity);
int fat16_read_path(const fat16_volume_t* volume, const char* path,
                    char* buffer, uint32_t max);
int fat16_create_path_file(const fat16_volume_t* volume, const char* path,
                           const uint8_t* data, uint32_t size, uint16_t* out_first_cluster);
int fat16_unlink_path_file(const fat16_volume_t* volume, const char* path);
int fat16_rename_path_file(const fat16_volume_t* volume, const char* old_path,
                           const char* new_path);
int fat16_create_directory(const fat16_volume_t* volume, const char* name);
int fat16_remove_directory(const fat16_volume_t* volume, const char* name);
int fat16_read_file(const fat16_volume_t* volume, const char* name,
                    char* buffer, uint32_t max);
/* Lit au plus max octets à partir d’un offset sans charger tout le fichier. */
int fat16_read_file_range(const fat16_volume_t* volume, const char* name,
                          uint32_t offset, uint8_t* buffer, uint32_t max,
                          uint32_t* out_read);
int fat16_open_file(const fat16_volume_t* volume, const char* name,
                    fat16_file_t* out);
int fat16_file_read(fat16_file_t* file, uint8_t* buffer, uint32_t max,
                    uint32_t* out_read);
/* Positionne un curseur ouvert sur un offset, en parcourant seulement la FAT. */
int fat16_file_seek(fat16_file_t* file, uint32_t offset);
const char* fat16_status(void);

#endif
