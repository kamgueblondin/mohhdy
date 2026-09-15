# MOHHDY Foundation — Incrément 68 : volume FAT16 lecture seule

**Statut :** livré dans la branche `manus/mohhdy-foundation-fat16-volume`, en attente de pull request.

## Objectif

L’incrément 68 livre le premier volume disque structuré de MOHHDY. Il s’agit d’un volume **FAT16 en lecture seule**, présenté sur le disque IDE primaire à partir du **LBA 64**. Les 64 premiers secteurs restent réservés au snapshot overlay AIOV V2 ; le volume FAT ne les recouvre donc pas.

Le jalon ne transforme pas MOHHDY en système de fichiers généraliste. Il fournit une lecture déterministe du BPB, de la table d’allocation, du répertoire racine 8.3 et des fichiers chaînés sur plusieurs clusters.

## Contrat technique

| Élément | Contrat livré |
|---|---|
| Support | ATA PIO primaire, même image brute que l’overlay |
| Placement | FAT16 relatif au LBA 64 ; AIOV conserve les LBA 0–63 |
| BPB | 512 octets par secteur, clusters puissance de deux, 1 ou 2 FAT, racine bornée |
| Répertoire | Racine uniquement, entrées 8.3 ; entrées supprimées, LFN et volume ignorés |
| Allocation | Chaîne FAT16 suivie jusqu’à une sentinelle EOC avec garde anti-boucle |
| Lecture | Fichier borné par la capacité utilisateur, y compris plusieurs clusters |
| ABI | `SYS_FAT16_READ=87`, `SYS_FAT16_LIST=88`, `MAX_SYSCALLS=89` |
| Shell | `fat16-list`, `fat16-cat <8.3>` |

Le backend refuse les BPB invalides, les tailles hors plage FAT16, les offsets hors volume, les chemins non 8.3, les répertoires et les chaînes de clusters corrompues. Il ne réalise aucune écriture, création, suppression ou allocation.

## Image de test et isolation

`tests/scripts/make_fat16_image.py` fabrique une image brute de 4224 secteurs. Le fixture contient `FATOK.TXT` et `BIGFILE.BIN`, utilise deux copies de FAT et réserve explicitement la zone AIOV avant le volume. Le smoke core QEMU recrée son disque de test à chaque exécution, puis vérifie `fat16-list` et `fat16-cat FATOK.TXT` avant les tests initrd et overlay.

> Le volume FAT16 est préparé par l’hôte de build. MOHHDY ne contient pas d’outil de formatage et ne modifie pas la FAT.

## Validation

La livraison est vérifiée par les tests Unity du parseur et du backend FAT16, par les ABI dans le miroir syscall, par la reconstruction i386 et par les scénarios QEMU. La suite complète atteint **261 tests verts**. `make qemu-smoke` valide core, extras, persistance overlay, supervision spawn et exec ; le contrat core vérifie en plus la liste et le contenu FAT16.

## Limites et prochaine tranche

Les noms longs, FAT32, sous-répertoires, écriture, journalisation, droits et exécution depuis FAT restent hors périmètre. Le volume repose aussi sur le disque primaire et son placement fixe à LBA 64 ; une future tranche pourra isoler FAT sur un second disque ou ajouter un médiateur VFS `fat/`, mais elle devra préserver l’overlay AIOV et ses 64 secteurs réservés.
