#ifndef MOHHDY_FAT32_H
#define MOHHDY_FAT32_H

#include <stdint.h>
#include "fat16.h"

#define FAT32_EOC_MIN 0x0ffffff8U
#define FAT32_BAD_CLUSTER 0x0ffffff7U
#define FAT32_MAX_CLUSTER 0x0fffffffU

typedef struct {
    fat16_read_sector_fn read_sector;
    fat16_write_sector_fn write_sector;
    uint32_t base_lba;
    uint32_t total_sectors;
    uint32_t fat_lba;
    uint32_t fat_sectors;
    uint32_t data_lba;
    uint32_t cluster_count;
    uint32_t root_cluster;
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint8_t fat_count;
    uint8_t mounted;
} fat32_volume_t;

fat32_volume_t* fat32_root(void);
int fat32_mount(fat32_volume_t* volume, fat16_read_sector_fn read_sector, uint32_t base_lba);
int fat32_is_mounted(const fat32_volume_t* volume);
int fat32_attach_writer(fat32_volume_t* volume, fat16_write_sector_fn write_sector);
int fat32_write_fat_entry(const fat32_volume_t* volume, uint32_t cluster, uint32_t next);
int fat32_allocate_cluster(const fat32_volume_t* volume, uint32_t* out_cluster);
int fat32_link_clusters(const fat32_volume_t* volume, uint32_t source, uint32_t target);
int fat32_read_fat_entry(const fat32_volume_t* volume, uint32_t cluster, uint32_t* out_next);
int fat32_cluster_lba(const fat32_volume_t* volume, uint32_t cluster, uint32_t* out_lba);
int fat32_read_cluster(const fat32_volume_t* volume, uint32_t cluster, uint8_t* buffer);
int fat32_write_cluster(const fat32_volume_t* volume, uint32_t cluster, const uint8_t* buffer);
int fat32_create_root_entry(const fat32_volume_t* volume, const char* name, uint8_t attributes, uint32_t first_cluster, uint32_t size);
int fat32_extend_root_directory(const fat32_volume_t* volume, uint32_t* out_cluster);
uint8_t fat32_lfn_checksum(const uint8_t short_name[11]);
int fat32_encode_lfn_entry(const char* name, uint8_t ordinal, uint8_t checksum, uint8_t entry[32]);
int fat32_create_file(const fat32_volume_t* volume, const char* name, uint8_t attributes, const uint8_t* data, uint32_t size, uint32_t* out_first_cluster);
int fat32_create_lfn_file(const fat32_volume_t* volume, const char* long_name, const char* short_name, uint8_t attributes, const uint8_t* data, uint32_t size, uint32_t* out_first_cluster);
int fat32_list_root(const fat32_volume_t* volume, os_fat16_dirent_t* out, uint32_t capacity);
/* Retourne une page de racine à partir d’un index logique, sans allocation. */
int fat32_list_root_page(const fat32_volume_t* volume, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity);
/* Sous-répertoires FAT32 bornés à un niveau : noms 8.3 pour le répertoire
 * et son enfant ; les LFN racine restent compatibles et inchangées. */
int fat32_list_path_page(const fat32_volume_t* volume, const char* path, uint32_t start,
                         os_fat16_dirent_t* out, uint32_t capacity);
int fat32_read_path(const fat32_volume_t* volume, const char* path,
                    uint8_t* buffer, uint32_t max);
int fat32_create_path_file(const fat32_volume_t* volume, const char* path,
                           const uint8_t* data, uint32_t size, uint32_t* out_first_cluster);
int fat32_unlink_path_file(const fat32_volume_t* volume, const char* path);
int fat32_rename_path_file(const fat32_volume_t* volume, const char* old_path,
                           const char* new_path);
int fat32_create_directory(const fat32_volume_t* volume, const char* name);
int fat32_remove_directory(const fat32_volume_t* volume, const char* name);
/* Lit un fichier FAT32 8.3 dans un buffer caller-owned, sans allocation dynamique. */
int fat32_read_file(const fat32_volume_t* volume, const char* name, uint8_t* buffer, uint32_t max);
/* Lit au plus max octets à partir d’un offset dans un fichier FAT32. */
int fat32_read_file_range(const fat32_volume_t* volume, const char* name,
                          uint32_t offset, uint8_t* buffer, uint32_t max,
                          uint32_t* out_read);
/* Supprime un alias 8.3 ou une séquence LFN ASCII validée et libère sa chaîne. */
int fat32_unlink_file(const fat32_volume_t* volume, const char* name);
/* Renomme un fichier 8.3 classique de la racine sans deplacer sa chaine, sans LFN ni repertoire. */
int fat32_rename_file(const fat32_volume_t* volume, const char* old_name, const char* new_name);
/* Renomme une séquence LFN validée sans déplacer ni recopier sa chaîne de données. */
int fat32_rename_lfn_file(const fat32_volume_t* volume, const char* old_name,
                          const char* new_long_name, const char* new_short_name);

#endif
