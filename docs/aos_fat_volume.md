# Volume FAT sur disque IDE - conception

**Statut :** jalon AOS-026 lecture seule livré, puis mutations VFS FAT16/FAT32 : LFN à la racine et un seul sous-répertoire 8.3. FAT32 reste un second disque IDE.

**Date :** 26 août 2026.

**Source de verite runtime :** [ETAT_REEL.md](ETAT_REEL.md). Vocabulaire : [vocabulaire.md](vocabulaire.md). Backlog : [../US/mohhdy_us.md](../US/mohhdy_us.md) (AOS-026).

## Decision

Le premier systeme de fichiers **sur disque**, hors overlay AIOV, est un **volume FAT** (FAT16 en premier jalon, FAT12 acceptable pour une image minuscule). **ext2, ext3, ext4 et tout systeme a inodes sont hors perimetre.**

MOHHDY n'est pas un clone Unix. Un volume a table d'allocation (clusters, repertoire d'entrees fixes) suffit pour persister des fichiers plus grands que l'overlay, sans importer le modele inode / superbloc / liens physiques.

## Ce qui existe deja (a ne pas confondre)

| Support | Format | Role | Limite |
|---|---|---|---|
| Initrd | Archive TAR (ustar) en RAM | Programmes, modeles, lecture seule | Pas un volume disque |
| Overlay | Snapshot **AIOV** V2 sur ATA PIO | Petits fichiers persistants du shell | 64 noeuds, 80 octets de chemin, 384 octets de contenu, 64 secteurs (32 Kio) a LBA 0 |
| Disque IDE principal | Image brute `build/overlay.img` | Snapshot AIOV aux LBA 0-63, volume FAT16 à partir du LBA 64 | LFN à la racine ; un répertoire et des enfants 8.3 via VFS |
| Second disque IDE | Image FAT32 de fixture | Même contrat VFS via `fat32/` | Pas de LFN enfant, de second niveau ni de prétention FS généraliste |

L'overlay occupe les **64 premiers secteurs** (LBA 0-63). Un volume FAT ne doit **pas** les ecraser.

1. **Second disque IDE** dedie (`-drive ...,if=ide`) pour FAT32.
2. **Meme disque**, volume FAT16 a partir du LBA 64, overlay AIOV intact.

## Commandes observables

- `fat16-list` et `fat16-cat <8.3>` : lecture directe du volume FAT16.
- `vfs-list fat16/`, `vfs-read fat16/<8.3>`, `vfs-stat fat16/<8.3>` : lecture mediee.
- `vfs-write fat16/<nom> <texte>` : crée un fichier racine 8.3 ou LFN, refuse l’écrasement.
- `vfs-mkdir fat16/<dir>` puis `vfs-write fat16/<dir>/<8.3> <texte>` : crée un répertoire et son enfant 8.3 unique ; le worker Ring 3 reçoit seulement `mutate`.
- `vfs-stat`, `vfs-list` et `vfs-list-page` couvrent l’enfant ; `vfs-rename` et `vfs-remove` refusent une cible existante et libèrent la chaîne.
- `vfs-rmdir fat16/<dir>` refuse un répertoire non vide ; après envoi d’une mutation, disparition ou expiration du worker renvoie un échec sans rejeu local.
- `vfs-mount-add media32/ fat32` applique le même contrat FAT32.

Hors contrat actuel : écrasement, remplacement transactionnel, second niveau, LFN enfant et renommage entre répertoires.

## Pourquoi FAT, et pas un FS Unix

- **Modele simple :** secteur d'amorcage (BPB), une ou deux tables d'allocation, repertoire racine, chaines de clusters. Pas d'inodes, pas de groupes de blocs, pas de journal.
- **Taille adaptee a ATA PIO LBA28 :** une image de quelques Mio se parcourt secteur par secteur sans cache de blocs sophistique.
- **Preparation hors de l'OS :** une image FAT se fabrique sur la machine de build, puis QEMU la presente comme disque IDE.
- **Distance volontaire :** FAT vient des volumes DOS/Windows. Ce n'est pas une invitation a recopier Linux.

## Contrat du premier jalon (AOS-026) puis suite

**En tant qu'**utilisateur, **je veux** lire (puis ecrire de facon bornee) des fichiers sur un volume FAT branche en IDE, **afin de** depasser le snapshot AIOV sans changer d'identite de systeme.

| Livrable | Etat |
|---|---|
| Lecture du BPB FAT16 | Livre |
| Table d'allocation et racine 8.3 | Livre |
| Lecture de fichier chainee | Livre |
| Isolation overlay LBA 0-63 | Livre |
| Création / suppression / renommage FAT16/FAT32 8.3 ou LFN à la racine | Livré sous capacité `mutate` |
| Sous-répertoire FAT16/FAT32 unique 8.3 : `mkdir`, lecture, statut, pagination, écriture, renommage, suppression, `rmdir` vide | Livré sous capacité `mutate`, via worker Ring 3 sans rejeu incertain |
| Écrasement, second niveau, LFN enfant, renommage inter-répertoire, remplacement transactionnel | Hors périmètre |
| FAT32 lecture / listage / statut VFS | Livré sur le second disque, racine et sous-répertoire 8.3 |
| ext2 et assimilés | Hors perimetre |

## Place dans la suite du prototype

| Priorite proche | Rapport avec FAT |
|---|---|
| Kernels GGUF Q3_K / Q4_K / Q6_K | Un GGUF trop gros pour l'initrd vit deja sur FAT16 (`make gguf-disk`). |
| Latence GPT-2 < 1 s | Independante. QEMU TCG reste ~48 s / ~23 s. |
| Externaliser le backend du mediateur | FAT16/FAT32 sont des backends VFS ; ils ne retirent pas encore initrd/overlay du noyau. |
| TLS authentifie / HTTP / OpenAI | Orthogonal. Voir [aos025_network_stub.md](aos025_network_stub.md). |

Ce n'est **pas** le moment d'introduire ext2 "pour faire comme un Unix", ni de pretendre qu'MOHHDY a un systeme de fichiers generaliste.

## Verification

```text
make test-all              # 505 tests : BPB, chaînes, mutations et pagination FAT16/FAT32
make qemu-smoke            # overlay AIOV inchangé
make qemu-vfs-service      # cycles FAT16/FAT32 racine LFN et sous-répertoire 8.3
make integration-qemu      # sept contrats QEMU, 23 min 30 s lors de la validation locale
```

Aucun secret, aucun modele et aucune cle API ne doivent etre committes sur l'image FAT de test.
