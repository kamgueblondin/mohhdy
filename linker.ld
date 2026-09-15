ENTRY(_start) /* Le point d'entrée de notre OS */

SECTIONS
{
    /* Commence à placer le code à 1MB */
    . = 1M;

    /* Section pour le code exécutable (.text) - Lecture + Exécution uniquement */
    .text ALIGN(4K) : {
        *(.multiboot) /* Place l'en-tête multiboot au tout début */
        *(.text)      /* Place tout le reste du code (.text) après */
    }

    /* Section pour les données en lecture seule (.rodata) - Lecture uniquement */
    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    /* Section pour les données initialisées (.data) - Lecture + Écriture */
    .data ALIGN(4K) : {
        *(.data)
    }

    /* Section pour les données non initialisées (.bss) - Lecture + Écriture */
    .bss ALIGN(4K) : {
        *(.bss)
        *(COMMON)
    }

    end = .; /* Symbole marquant la fin du noyau */

    /* Section GNU stack (non-exécutable) */
    .note.GNU-stack (NOLOAD) : {
        *(.note.GNU-stack)
    }
}

