#include "initrd.h"
#include "kernel/mem/string.h"

// Fonctions externes (définies dans kernel.c)
extern void print_string_serial(const char* str);
extern void print_string_vga(const char* str, char color);

// Variables globales
initrd_t* current_initrd = 0;
#define INITRD_FILE_MAX 64U
static initrd_file_t files_array[INITRD_FILE_MAX]; // Support jusqu'à 64 fichiers
static os_dirent_t initrd_page_entries[INITRD_FILE_MAX];

// Fonctions utilitaires pour les chaînes de caractères
int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

// Convertit une chaîne octale en entier
int oct2bin(char *str, int size) {
    int n = 0;
    char *c = str;
    while (size-- > 0 && *c >= '0' && *c <= '7') {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

// Vérifie le checksum d'un en-tête TAR
int tar_checksum_valid(tar_header_t* header) {
    unsigned int sum = 0;
    char* ptr = (char*)header;
    
    // Calcule la somme de tous les octets de l'en-tête
    for (int i = 0; i < 512; i++) {
        if (i >= 148 && i < 156) {
            // Pour le champ checksum, utilise des espaces
            sum += ' ';
        } else {
            sum += (unsigned char)ptr[i];
        }
    }
    
    // Compare avec le checksum stocké
    int stored_checksum = oct2bin(header->checksum, 8);
    return (sum == (unsigned int)stored_checksum);
}

// Initialise le système initrd
void initrd_init(uint32_t location, uint32_t size) {
    if (!location || !size) {
        print_string_serial("Erreur: initrd invalide\n");
        return;
    }
    
    // Alloue la structure initrd (statiquement pour simplifier)
    static initrd_t initrd_struct;
    current_initrd = &initrd_struct;
    
    current_initrd->location = location;
    current_initrd->size = size;
    current_initrd->file_count = 0;
    current_initrd->files = files_array;
    
    // Parse l'archive TAR
    uint32_t current_pos = location;
    uint32_t file_index = 0;
    
    print_string_serial("Parsing initrd TAR archive...\n");
    
    while (current_pos < location + size && file_index < 64) {
        tar_header_t* header = (tar_header_t*)current_pos;
        
        // Vérifie si on a atteint la fin (deux blocs de zéros)
        if (header->name[0] == '\0') {
            break;
        }
        
        // Vérifie la signature TAR
        if (header->magic[0] != 'u' || header->magic[1] != 's' || 
            header->magic[2] != 't' || header->magic[3] != 'a' || 
            header->magic[4] != 'r') {
            print_string_serial("Warning: Invalid TAR magic, skipping\n");
            current_pos += 512;
            continue;
        }
        
        // Vérifie le checksum
        if (!tar_checksum_valid(header)) {
            print_string_serial("Warning: Invalid TAR checksum, skipping\n");
            current_pos += 512;
            continue;
        }
        
        // Traite seulement les fichiers normaux
        if (header->typeflag == '0' || header->typeflag == '\0') {
            int file_size = oct2bin(header->size, 11);
            
            // Copie les informations du fichier
            strcpy(files_array[file_index].name, header->name);
            files_array[file_index].size = file_size;
            files_array[file_index].offset = current_pos + 512;
            files_array[file_index].data = (char*)(current_pos + 512);
            
            file_index++;
            
            // Calcule la position du prochain en-tête (aligné sur 512 octets)
            uint32_t data_blocks = (file_size + 511) / 512;
            current_pos += 512 + (data_blocks * 512);
        } else {
            // Ignore les autres types de fichiers (répertoires, liens, etc.)
            current_pos += 512;
        }
    }
    
    current_initrd->file_count = file_index;
    
    print_string_serial("Initrd initialized: ");
    // Conversion simple d'entier en chaîne
    char count_str[16];
    int temp = file_index;
    int i = 0;
    if (temp == 0) {
        count_str[i++] = '0';
    } else {
        while (temp > 0) {
            count_str[i++] = '0' + (temp % 10);
            temp /= 10;
        }
    }
    count_str[i] = '\0';
    
    // Inverse la chaîne
    for (int j = 0; j < i / 2; j++) {
        char tmp = count_str[j];
        count_str[j] = count_str[i - 1 - j];
        count_str[i - 1 - j] = tmp;
    }
    
    print_string_serial(count_str);
    print_string_serial(" files found\n");
}

// Liste tous les fichiers dans l'initrd
void initrd_list_files() {
    if (!current_initrd) {
        print_string_serial("Initrd not initialized\n");
        return;
    }
    
    print_string_serial("Files in initrd:\n");
    for (uint32_t i = 0; i < current_initrd->file_count; i++) {
        print_string_serial("  ");
        print_string_serial(current_initrd->files[i].name);
        print_string_serial(" (");
        
        // Affiche la taille (conversion simple)
        uint32_t size = current_initrd->files[i].size;
        char size_str[16];
        int j = 0;
        if (size == 0) {
            size_str[j++] = '0';
        } else {
            while (size > 0) {
                size_str[j++] = '0' + (size % 10);
                size /= 10;
            }
        }
        size_str[j] = '\0';
        
        // Inverse la chaîne
        for (int k = 0; k < j / 2; k++) {
            char tmp = size_str[k];
            size_str[k] = size_str[j - 1 - k];
            size_str[j - 1 - k] = tmp;
        }
        
        print_string_serial(size_str);
        print_string_serial(" bytes)\n");
    }
}

// Lit le contenu d'un fichier
char* initrd_read_file(const char* filename) {
    if (!current_initrd) {
        return 0;
    }
    
    for (uint32_t i = 0; i < current_initrd->file_count; i++) {
        const char* stored_name = current_initrd->files[i].name;
        
        // Ignore le préfixe "./" si présent
        if (stored_name[0] == '.' && stored_name[1] == '/') {
            stored_name += 2;
        }
        
        if (strcmp(stored_name, filename) == 0) {
            return current_initrd->files[i].data;
        }
    }
    
    return 0; // Fichier non trouvé
}

// Obtient la taille d'un fichier
uint32_t initrd_get_file_size(const char* filename) {
    if (!current_initrd) {
        return 0;
    }
    
    for (uint32_t i = 0; i < current_initrd->file_count; i++) {
        const char* stored_name = current_initrd->files[i].name;
        
        // Ignore le préfixe "./" si présent
        if (stored_name[0] == '.' && stored_name[1] == '/') {
            stored_name += 2;
        }
        
        if (strcmp(stored_name, filename) == 0) {
            return current_initrd->files[i].size;
        }
    }
    
    return 0; // Fichier non trouvé
}

// Vérifie si un fichier existe
int initrd_file_exists(const char* filename) {
    return (initrd_read_file(filename) != 0);
}

// Obtient le nombre de fichiers
uint32_t initrd_get_file_count() {
    if (!current_initrd) {
        return 0;
    }
    return current_initrd->file_count;
}

// Affiche les informations d'un fichier
void initrd_print_file_info(const char* filename) {
    if (!current_initrd) {
        print_string_serial("Initrd not initialized\n");
        return;
    }
    
    for (uint32_t i = 0; i < current_initrd->file_count; i++) {
        if (strcmp(current_initrd->files[i].name, filename) == 0) {
            print_string_serial("File: ");
            print_string_serial(filename);
            print_string_serial("\nSize: ");
            
            // Affiche la taille
            uint32_t size = current_initrd->files[i].size;
            char size_str[16];
            int j = 0;
            if (size == 0) {
                size_str[j++] = '0';
            } else {
                while (size > 0) {
                    size_str[j++] = '0' + (size % 10);
                    size /= 10;
                }
            }
            size_str[j] = '\0';
            
            // Inverse la chaîne
            for (int k = 0; k < j / 2; k++) {
                char tmp = size_str[k];
                size_str[k] = size_str[j - 1 - k];
                size_str[j - 1 - k] = tmp;
            }
            
            print_string_serial(size_str);
            print_string_serial(" bytes\n");
            return;
        }
    }
    
    print_string_serial("File not found: ");
    print_string_serial(filename);
    print_string_serial("\n");
}

static void ird_copy_name(char* dest, const char* src, int max) {
    int i = 0;
    if (!src) src = "";
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void ird_normalize(const char* in, char* out, int max) {
    if (!in) {
        out[0] = '\0';
        return;
    }
    if (in[0] == '.' && in[1] == '/') in += 2;
    while (*in == '/') in++;
    ird_copy_name(out, in, max);
    {
        int n = 0;
        while (out[n]) n++;
        while (n > 0 && out[n - 1] == '/') {
            out[n - 1] = '\0';
            n--;
        }
    }
}

static const initrd_file_t* ird_find(const char* filename) {
    char want[256];
    char have[256];
    if (!current_initrd || !filename) return 0;
    ird_normalize(filename, want, 256);
    for (uint32_t i = 0; i < current_initrd->file_count; i++) {
        ird_normalize(current_initrd->files[i].name, have, 256);
        if (strcmp(have, want) == 0) {
            return &current_initrd->files[i];
        }
    }
    return 0;
}

int initrd_read_into(const char* path, char* buf, uint32_t max) {
    const initrd_file_t* f = ird_find(path);
    uint32_t n;
    if (!f || !buf || max == 0) return -1;
    n = f->size;
    if (n > max) n = max;
    memcpy(buf, f->data, n);
    return (int)n;
}

int initrd_listdir(const char* path, os_dirent_t* out, int max_n) {
    char prefix[256];
    int plen;
    int count = 0;
    if (!current_initrd || !out || max_n <= 0) return -1;
    ird_normalize(path ? path : "/", prefix, 256);
    plen = 0;
    while (prefix[plen]) plen++;

    for (uint32_t i = 0; i < current_initrd->file_count && count < max_n; i++) {
        char have[256];
        const char* rest;
        ird_normalize(current_initrd->files[i].name, have, 256);
        if (plen == 0) {
            rest = have;
        } else {
            int j = 0;
            while (j < plen && have[j] == prefix[j]) j++;
            if (j != plen || have[j] != '/') continue;
            rest = have + plen + 1;
        }
        if (rest[0] == '\0') continue;

        {
            int slash = -1;
            int k = 0;
            while (rest[k]) {
                if (rest[k] == '/') {
                    slash = k;
                    break;
                }
                k++;
            }
            if (slash >= 0) {
                char dname[OS_NAME_MAX];
                int dup = 0;
                int nlen = slash;
                if (nlen >= OS_NAME_MAX) nlen = OS_NAME_MAX - 1;
                for (int t = 0; t < nlen; t++) dname[t] = rest[t];
                dname[nlen] = '\0';
                for (int e = 0; e < count; e++) {
                    if (out[e].flags == OS_DIRENT_DIR && strcmp(out[e].name, dname) == 0) {
                        dup = 1;
                        break;
                    }
                }
                if (!dup) {
                    ird_copy_name(out[count].name, dname, OS_NAME_MAX);
                    out[count].size = 0;
                    out[count].flags = OS_DIRENT_DIR;
                    count++;
                }
            } else {
                ird_copy_name(out[count].name, rest, OS_NAME_MAX);
                out[count].size = current_initrd->files[i].size;
                out[count].flags = OS_DIRENT_FILE;
                count++;
            }
        }
    }
    return count;
}

int initrd_listdir_page(const char* path, os_dirent_t* out, uint32_t start, int max_n) {
    int total;
    int emitted;
    int i;
    if (!out || max_n <= 0) return -1;
    total = initrd_listdir(path, initrd_page_entries, (int)INITRD_FILE_MAX);
    if (total < 0) return total;
    if (start >= (uint32_t)total) return 0;
    emitted = total - (int)start;
    if (emitted > max_n) emitted = max_n;
    for (i = 0; i < emitted; i++) {
        uint32_t j;
        for (j = 0U; j < OS_NAME_MAX; j++) {
            out[i].name[j] = initrd_page_entries[start + (uint32_t)i].name[j];
        }
        out[i].size = initrd_page_entries[start + (uint32_t)i].size;
        out[i].flags = initrd_page_entries[start + (uint32_t)i].flags;
    }
    return emitted;
}

int initrd_is_file(const char* path) {
    return ird_find(path) != 0;
}

int initrd_is_dir(const char* path) {
    char prefix[256];
    int plen;
    if (!current_initrd || !path) return 0;
    ird_normalize(path, prefix, 256);
    if (!prefix[0]) return 1;
    plen = 0;
    while (prefix[plen]) plen++;
    for (uint32_t i = 0; i < current_initrd->file_count; i++) {
        char have[256];
        int j = 0;
        ird_normalize(current_initrd->files[i].name, have, 256);
        if (strcmp(have, prefix) == 0) {
            /* TAR directory entries are not stored as files; a file
             * with the exact directory name is not a directory. */
            continue;
        }
        while (j < plen && have[j] == prefix[j]) j++;
        if (j == plen && have[j] == '/') return 1;
    }
    return 0;
}

int initrd_stat(const char* path, os_dirent_t* out) {
    const initrd_file_t* f;
    char want[256];
    const char* base;
    int i;
    if (!path || !out) return -1;
    ird_normalize(path, want, 256);
    if (!want[0]) {
        out->name[0] = '/';
        out->name[1] = '\0';
        out->size = 0;
        out->flags = OS_DIRENT_DIR;
        return 0;
    }
    if (initrd_is_dir(want)) {
        base = want;
        for (i = 0; want[i]; i++) {
            if (want[i] == '/') base = want + i + 1;
        }
        ird_copy_name(out->name, base, OS_NAME_MAX);
        out->size = 0;
        out->flags = OS_DIRENT_DIR;
        return 0;
    }
    f = ird_find(want);
    if (!f) return -1;
    base = want;
    for (i = 0; want[i]; i++) {
        if (want[i] == '/') base = want + i + 1;
    }
    ird_copy_name(out->name, base, OS_NAME_MAX);
    out->size = f->size;
    out->flags = OS_DIRENT_FILE;
    return 0;
}

