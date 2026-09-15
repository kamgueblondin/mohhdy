#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include "os_syscalls.h"

// Structure pour un en-tête TAR (format POSIX)
typedef struct {
    char name[100];      // Nom du fichier
    char mode[8];        // Permissions (octal)
    char uid[8];         // User ID (octal)
    char gid[8];         // Group ID (octal)
    char size[12];       // Taille en octets (octal)
    char mtime[12];      // Timestamp modification (octal)
    char checksum[8];    // Checksum de l'en-tête
    char typeflag;       // Type de fichier ('0' = fichier normal)
    char linkname[100];  // Nom du lien (si applicable)
    char magic[6];       // "ustar\0"
    char version[2];     // "00"
    char uname[32];      // Nom utilisateur
    char gname[32];      // Nom groupe
    char devmajor[8];    // Device major number
    char devminor[8];    // Device minor number
    char prefix[155];    // Préfixe du chemin
} __attribute__((packed)) tar_header_t;

// Structure pour représenter un fichier dans l'initrd
typedef struct {
    char name[256];      // Nom complet du fichier
    uint32_t size;       // Taille du fichier
    uint32_t offset;     // Offset dans l'initrd
    char* data;          // Pointeur vers les données
} initrd_file_t;

// Structure pour le système initrd
typedef struct {
    uint32_t location;   // Adresse de base de l'initrd
    uint32_t size;       // Taille totale de l'initrd
    uint32_t file_count; // Nombre de fichiers
    initrd_file_t* files; // Tableau des fichiers
} initrd_t;

// Fonctions publiques du système initrd
void initrd_init(uint32_t location, uint32_t size);
void initrd_list_files();
char* initrd_read_file(const char* filename);
uint32_t initrd_get_file_size(const char* filename);
int initrd_file_exists(const char* filename);
uint32_t initrd_get_file_count();
int initrd_listdir(const char* path, os_dirent_t* out, int max_n);
int initrd_listdir_page(const char* path, os_dirent_t* out, uint32_t start, int max_n);
int initrd_read_into(const char* path, char* buf, uint32_t max);
int initrd_is_dir(const char* path);
int initrd_is_file(const char* path);
int initrd_stat(const char* path, os_dirent_t* out);

// Fonctions utilitaires
int oct2bin(char *str, int size);
int tar_checksum_valid(tar_header_t* header);
void initrd_print_file_info(const char* filename);

// Variables globales
extern initrd_t* current_initrd;

#endif

