# Spécifications Détaillées - Étapes 3 et 4

## Vue d'Ensemble

Les étapes 3 et 4 représentent une évolution majeure du système MOHHDY, ajoutant des capacités essentielles pour la gestion de la mémoire et l'accès aux fichiers. Ces fonctionnalités sont cruciales pour préparer le système à exécuter une intelligence artificielle.

## Étape 3 : Gestion de la Mémoire (Memory Management)

### Objectifs

Transformer le noyau d'un simple afficheur de messages en un véritable gestionnaire de ressources capable d'allouer et de libérer de la mémoire dynamiquement.

### Concepts Techniques

#### Physical Memory Manager (PMM)

Le PMM agit comme un comptable de la RAM physique, utilisant un bitmap pour tracker l'état de chaque cadre de page de 4 Ko :

- **Bitmap** : Chaque bit représente un cadre de page (0 = libre, 1 = utilisé)
- **Granularité** : Pages de 4096 octets (standard x86)
- **Localisation** : Bitmap placé à l'adresse 0x10000 (64 Ko)

#### Virtual Memory Manager (VMM)

Le VMM implémente le paging, créant une couche d'abstraction entre les adresses logiques et physiques :

- **Page Directory** : Table principale de 1024 entrées
- **Page Tables** : Tables secondaires de 1024 entrées chacune
- **Mapping initial** : Identité 1:1 pour les premiers 4 Mo
- **Sécurité** : Isolation mémoire entre processus futurs

### Architecture Mémoire

```
Adresses Virtuelles    Adresses Physiques
0x00000000-0x003FFFFF → 0x00000000-0x003FFFFF (4 Mo, mapping 1:1)
0x00400000+           → Gestion dynamique future
```

### Registres CPU Utilisés

- **CR0** : Activation du paging (bit 31)
- **CR3** : Adresse du Page Directory
- **CR2** : Adresse causant une page fault (futur)

## Étape 4 : Accès Disque et Système de Fichiers (Initrd)

### Objectifs

Implémenter un système de fichiers basique sans la complexité d'un pilote de disque complet, en utilisant un initrd (Initial RAM Disk).

### Concepts Techniques

#### Format TAR

Le format TAR a été choisi pour sa simplicité :

- **Structure** : Succession de blocs de 512 octets
- **En-tête** : 512 octets contenant métadonnées (nom, taille, permissions)
- **Données** : Contenu du fichier, padded à 512 octets
- **Fin** : Deux blocs de zéros consécutifs

#### Structure d'En-tête TAR

```c
typedef struct {
    char name[100];      // Nom du fichier
    char mode[8];        // Permissions (octal)
    char uid[8];         // User ID (octal)
    char gid[8];         // Group ID (octal)
    char size[12];       // Taille en octets (octal)
    char mtime[12];      // Timestamp modification (octal)
    char checksum[8];    // Checksum de l'en-tête
    char typeflag;       // Type de fichier
    char linkname[100];  // Nom du lien (si applicable)
    char magic[6];       // "ustar\0"
    char version[2];     // "00"
    char uname[32];      // Nom utilisateur
    char gname[32];      // Nom groupe
    char devmajor[8];    // Device major number
    char devminor[8];    // Device minor number
    char prefix[155];    // Préfixe du chemin
} tar_header_t;
```

### Intégration avec Multiboot

Le bootloader (QEMU/GRUB) charge l'initrd en mémoire et fournit son adresse via la structure Multiboot :

```c
typedef struct {
    uint32_t mod_start;  // Adresse de début du module
    uint32_t mod_end;    // Adresse de fin du module
    uint32_t cmdline;    // Ligne de commande du module
    uint32_t pad;        // Padding
} multiboot_module_t;
```

## Implémentation Technique

### Fichiers à Créer

#### Gestion Mémoire
- `kernel/mem/pmm.h` - Interface du Physical Memory Manager
- `kernel/mem/pmm.c` - Implémentation du PMM
- `kernel/mem/vmm.h` - Interface du Virtual Memory Manager  
- `kernel/mem/vmm.c` - Implémentation du VMM
- `boot/paging.s` - Fonctions assembleur pour le paging

#### Système de Fichiers
- `fs/initrd.h` - Interface du parser initrd
- `fs/initrd.c` - Implémentation du parser TAR

### Modifications Requises

#### kernel/kernel.c
- Ajout du support Multiboot complet
- Initialisation des gestionnaires de mémoire
- Initialisation du système de fichiers initrd
- Gestion des paramètres Multiboot

#### Makefile
- Ajout des nouveaux fichiers objets
- Règle pour créer l'initrd de test
- Modification de la commande QEMU pour inclure l'initrd

#### boot/boot.s
- Mise à jour pour passer les informations Multiboot au kernel

## Flux d'Exécution

1. **Démarrage** : GRUB/QEMU charge le kernel et l'initrd
2. **Multiboot** : Récupération des informations mémoire et modules
3. **PMM Init** : Initialisation du gestionnaire de mémoire physique
4. **VMM Init** : Activation du paging et mapping initial
5. **Initrd Init** : Localisation et parsing de l'initrd
6. **Listing** : Affichage des fichiers disponibles
7. **Prêt** : Système opérationnel pour les prochaines étapes

## Tests et Validation

### Tests Mémoire
- Allocation/libération de pages
- Vérification du bitmap
- Test du mapping virtuel/physique

### Tests Initrd
- Création d'un initrd de test avec plusieurs fichiers
- Parsing correct des en-têtes TAR
- Lecture du contenu des fichiers

### Commandes de Test

```bash
# Création manuelle d'un initrd de test
mkdir -p initrd_content
echo "Test content from initrd!" > initrd_content/test.txt
echo "Another file content." > initrd_content/hello.txt
echo "Configuration data" > initrd_content/config.cfg
tar -cf my_initrd.tar -C initrd_content .

# Compilation et test
make clean && make
make run
```

## Résultats Attendus

Après implémentation complète, le système devrait afficher :

```
Bienvenue dans MOHHDY !
Entrez du texte :
Interruptions initialisees. Clavier pret.
Gestionnaire de memoire initialise.
Initrd trouve ! Fichiers:
test.txt
hello.txt
config.cfg
Systeme pret pour execution de programmes.
```

## Préparation pour l'IA

Ces étapes préparent spécifiquement le terrain pour l'intégration d'une IA :

1. **Mémoire dynamique** : Nécessaire pour charger les modèles et données
2. **Système de fichiers** : Stockage des poids du modèle et configurations
3. **Isolation** : Sécurité pour exécuter l'IA dans un environnement contrôlé
4. **Modularité** : Architecture extensible pour ajouter de nouveaux composants

La prochaine étape logique sera l'implémentation d'un scheduler pour exécuter l'IA comme une tâche de fond, complétant ainsi l'infrastructure nécessaire pour un système d'exploitation capable d'héberger une intelligence artificielle.

