#ifndef ELF_H
#define ELF_H

#include <stdint.h>

// Magic number ELF
#define ELF_MAGIC 0x464C457F

// Types de fichiers ELF
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4

// Types de segments
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

// En-tête ELF 32-bit
typedef struct {
    unsigned char e_ident[16]; // Magic number et autres informations
    uint16_t e_type;      // Type de fichier
    uint16_t e_machine;   // Architecture cible
    uint32_t e_version;   // Version ELF
    uint32_t e_entry;     // Point d'entrée
    uint32_t e_phoff;     // Offset des program headers
    uint32_t e_shoff;     // Offset des section headers
    uint32_t e_flags;     // Flags spécifiques au processeur
    uint16_t e_ehsize;    // Taille de cet en-tête
    uint16_t e_phentsize; // Taille d'une entrée program header
    uint16_t e_phnum;     // Nombre d'entrées program header
    uint16_t e_shentsize; // Taille d'une entrée section header
    uint16_t e_shnum;     // Nombre d'entrées section header
    uint16_t e_shstrndx;  // Index de la section string table
} __attribute__((packed)) elf32_ehdr_t;

// Section Header 32-bit
typedef struct {
    uint32_t sh_name;       // Nom de la section (index dans la table de chaînes)
    uint32_t sh_type;       // Type de la section
    uint32_t sh_flags;      // Attributs de la section
    uint32_t sh_addr;       // Adresse virtuelle de la section en mémoire
    uint32_t sh_offset;     // Offset de la section dans le fichier
    uint32_t sh_size;       // Taille de la section
    uint32_t sh_link;       // Index de lien (dépend du type de section)
    uint32_t sh_info;       // Informations supplémentaires
    uint32_t sh_addralign;  // Alignement requis
    uint32_t sh_entsize;    // Taille des entrées si la section en contient
} __attribute__((packed)) elf32_shdr_t;

// Types de sections (sh_type)
#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_HASH     5
#define SHT_DYNAMIC  6
#define SHT_NOTE     7
#define SHT_NOBITS   8
#define SHT_REL      9
#define SHT_SHLIB    10
#define SHT_DYNSYM   11

// Program Header 32-bit
typedef struct {
    uint32_t p_type;   // Type de segment
    uint32_t p_offset; // Offset dans le fichier
    uint32_t p_vaddr;  // Adresse virtuelle
    uint32_t p_paddr;  // Adresse physique
    uint32_t p_filesz; // Taille dans le fichier
    uint32_t p_memsz;  // Taille en mémoire
    uint32_t p_flags;  // Flags
    uint32_t p_align;  // Alignement
} __attribute__((packed)) elf32_phdr_t;

// Flags pour les segments
#define PF_X 0x1  // Exécutable
#define PF_W 0x2  // Writable
#define PF_R 0x4  // Readable

#include "mem/vmm.h" // Pour vmm_directory_t

// Fonctions publiques
uint32_t elf_load(uint8_t* elf_data, vmm_directory_t* vmm_dir);
int elf_validate(uint8_t* elf_data);
void elf_print_info(uint8_t* elf_data);

#endif

