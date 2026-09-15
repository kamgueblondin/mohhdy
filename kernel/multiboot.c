#include "multiboot.h"

// Fonctions externes
extern void print_string_serial(const char* str);

// Obtient la taille totale de la mémoire depuis les informations Multiboot
uint32_t multiboot_get_memory_size(multiboot_info_t* mbi) {
    if (mbi->flags & MULTIBOOT_FLAG_MEM) {
        // mem_lower est en Ko, mem_upper aussi
        // mem_lower = mémoire de 0 à 640Ko
        // mem_upper = mémoire de 1Mo à la fin
        return (mbi->mem_lower + mbi->mem_upper + 1024) * 1024; // Convertit en octets
    }
    
    // Valeur par défaut si pas d'info mémoire
    return 16 * 1024 * 1024; // 16 MB par défaut
}

// Obtient un module spécifique
multiboot_module_t* multiboot_get_module(multiboot_info_t* mbi, uint32_t index) {
    if (!(mbi->flags & MULTIBOOT_FLAG_MODS) || index >= mbi->mods_count) {
        return 0;
    }
    
    multiboot_module_t* modules = (multiboot_module_t*)mbi->mods_addr;
    return &modules[index];
}

// Obtient le nombre de modules
uint32_t multiboot_get_module_count(multiboot_info_t* mbi) {
    if (mbi->flags & MULTIBOOT_FLAG_MODS) {
        return mbi->mods_count;
    }
    return 0;
}

// Affiche les informations Multiboot pour debug
void multiboot_print_info(multiboot_info_t* mbi) {
    print_string_serial("=== Multiboot Information ===\n");
    
    if (mbi->flags & MULTIBOOT_FLAG_MEM) {
        print_string_serial("Memory info available:\n");
        print_string_serial("  Lower memory: ");
        
        // Conversion simple pour affichage
        uint32_t lower = mbi->mem_lower;
        char str[16];
        int i = 0;
        if (lower == 0) {
            str[i++] = '0';
        } else {
            while (lower > 0) {
                str[i++] = '0' + (lower % 10);
                lower /= 10;
            }
        }
        str[i] = '\0';
        
        // Inverse la chaîne
        for (int j = 0; j < i / 2; j++) {
            char tmp = str[j];
            str[j] = str[i - 1 - j];
            str[i - 1 - j] = tmp;
        }
        
        print_string_serial(str);
        print_string_serial(" KB\n");
        
        print_string_serial("  Upper memory: ");
        uint32_t upper = mbi->mem_upper;
        i = 0;
        if (upper == 0) {
            str[i++] = '0';
        } else {
            while (upper > 0) {
                str[i++] = '0' + (upper % 10);
                upper /= 10;
            }
        }
        str[i] = '\0';
        
        // Inverse la chaîne
        for (int j = 0; j < i / 2; j++) {
            char tmp = str[j];
            str[j] = str[i - 1 - j];
            str[i - 1 - j] = tmp;
        }
        
        print_string_serial(str);
        print_string_serial(" KB\n");
    }
    
    if (mbi->flags & MULTIBOOT_FLAG_MODS) {
        print_string_serial("Modules loaded: ");
        
        uint32_t count = mbi->mods_count;
        char str[16];
        int i = 0;
        if (count == 0) {
            str[i++] = '0';
        } else {
            while (count > 0) {
                str[i++] = '0' + (count % 10);
                count /= 10;
            }
        }
        str[i] = '\0';
        
        // Inverse la chaîne
        for (int j = 0; j < i / 2; j++) {
            char tmp = str[j];
            str[j] = str[i - 1 - j];
            str[i - 1 - j] = tmp;
        }
        
        print_string_serial(str);
        print_string_serial("\n");
        
        // Affiche les détails des modules
        for (uint32_t mod_idx = 0; mod_idx < mbi->mods_count; mod_idx++) {
            multiboot_module_t* mod = multiboot_get_module(mbi, mod_idx);
            if (mod) {
                print_string_serial("  Module ");
                
                // Affiche l'index
                char idx_str[8];
                int k = 0;
                uint32_t idx = mod_idx;
                if (idx == 0) {
                    idx_str[k++] = '0';
                } else {
                    while (idx > 0) {
                        idx_str[k++] = '0' + (idx % 10);
                        idx /= 10;
                    }
                }
                idx_str[k] = '\0';
                
                // Inverse la chaîne
                for (int l = 0; l < k / 2; l++) {
                    char tmp = idx_str[l];
                    idx_str[l] = idx_str[k - 1 - l];
                    idx_str[k - 1 - l] = tmp;
                }
                
                print_string_serial(idx_str);
                print_string_serial(": 0x");
                
                // Affiche l'adresse de début en hexadécimal (simplifié)
                print_string_serial("????????");
                print_string_serial(" - 0x");
                print_string_serial("????????");
                print_string_serial("\n");
            }
        }
    }
    
    print_string_serial("=============================\n");
}

