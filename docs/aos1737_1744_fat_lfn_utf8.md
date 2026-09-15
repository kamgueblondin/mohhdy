# AOS-1737…1744 — Noms longs FAT en UTF-8 BMP

## Objet du macro-lot

Ce macro-lot étend les noms de fichier longs, ou **LFN**, des pilotes FAT16 et FAT32 afin que l’interface interne accepte et restitue des chaînes **UTF-8** représentables dans le plan multilingue de base (**BMP**). Les entrées de répertoire FAT restent conformes à leur représentation UTF-16LE ; la conversion est effectuée aux frontières des pilotes, sans allocation dynamique.

> Le périmètre couvre les caractères BMP valides encodés en UTF-8. Les paires de surrogates, les séquences UTF-8 mal formées, les encodages non minimaux, les caractères de contrôle et les séparateurs de chemin sont refusés.

| Élément | Avant le macro-lot | Après le macro-lot |
|---|---|---|
| Création LFN FAT16 | ASCII uniquement | UTF-8 BMP encodé en UTF-16LE |
| Création et renommage LFN FAT32 | ASCII uniquement | UTF-8 BMP encodé en UTF-16LE |
| Lecture, recherche et suppression FAT32 | LFN ASCII uniquement | Requêtes et noms LFN UTF-8 BMP |
| Lecture et listage FAT16 | Caractères non ASCII rendus par `?` | Décodage UTF-16LE vers UTF-8 BMP |
| Gestion mémoire | Sans allocation dynamique | Inchangée : tableaux automatiques bornés |

## Conception

Le nouveau composant `kernel/fs/lfn_utf8.h` fournit deux fonctions statiques et freestanding. `lfn_utf8_to_utf16_bmp()` décode une chaîne UTF-8 dans un tableau d’unités UTF-16LE logiques. Il limite l’entrée à trois octets par unité de sortie, ce qui borne les parcours tout en couvrant le maximum de trois octets du BMP UTF-8. `lfn_utf16_bmp_to_utf8()` effectue l’opération inverse dans un tampon appartenant à l’appelant.

| Fonction | Entrée | Sortie | Rejets intentionnels |
|---|---|---|---|
| `lfn_utf8_to_utf16_bmp` | UTF-8 NUL-terminé | unités UTF-16 BMP | séquences invalides, surlongues, surrogates, contrôle, `/`, `\\` |
| `lfn_utf16_bmp_to_utf8` | unités UTF-16LE FAT | UTF-8 NUL-terminé | surrogates et dépassement de capacité |

Les fragments LFN FAT sont dorénavant reconstruits dans des tableaux de `uint16_t` avant tout décodage UTF-8. Cette étape est indispensable : une unité UTF-16LE peut représenter deux ou trois octets UTF-8, et l’écriture directe de caractères dans un tableau `char` par ordinal FAT aurait fragmenté les séquences multioctets. Les comparaisons de noms reconstruisent donc une chaîne UTF-8 bornée avant d’appliquer le repli de casse ASCII historique.

## Intégration FAT32

Les opérations `fat32_create_lfn_file()` et `fat32_rename_lfn_file()` transforment d’abord le nom UTF-8 en unités BMP. `fat32_lfn_segment()` sérialise ensuite les treize unités de chaque entrée LFN avec les positions FAT prescrites. Les parcours de lecture, suppression et listage recueillent les unités via `fat32_lfn_get()` puis restituent l’UTF-8 avec une capacité explicite.

Les requêtes de lecture et suppression passent désormais par une validation UTF-8 BMP commune. Un nom long contenant `é`, comme `café-2026.txt`, peut donc être créé, retrouvé et listé sans que l’octet UTF-8 soit confondu avec une unité de répertoire FAT.

## Intégration FAT16

Le pilote FAT16 adopte le même contrat. `fat16_create_lfn_file()` convertit le nom avant de calculer le nombre d’entrées LFN. `fat16_lfn_put()` écrit les unités UTF-16LE et `fat16_lfn_get()` les réassemble dans un tableau d’unités. Le listage et la recherche par nom long produisent ensuite une chaîne UTF-8 complète avant copie ou comparaison.

Aucune modification n’est apportée au format 8.3, aux fenêtres de lecture FAT16, aux caches sectoriels ou aux API de fichiers. Les fichiers dont le nom court reste représentable sont toujours accessibles via leur alias historique.

## Garanties de sûreté

| Garantie | Mécanisme appliqué |
|---|---|
| Zéro allocation dynamique | aucun appel à `kmalloc`, `malloc`, `calloc`, `realloc` ou `free` dans le périmètre UTF-8 LFN |
| Bornes de nom | capacités explicites, maximum `OS_NAME_MAX`, maximum FAT de vingt entrées LFN |
| Intégrité UTF-8 | rejet des octets de continuation isolés, surlongueurs et séquences tronquées |
| Intégrité UTF-16 | rejet des surrogates ; arrêt sur `0x0000` ou remplissage `0xFFFF` |
| Chemins sûrs | rejet des contrôles, de `/` et de `\\` |
| Compatibilité | comportement ASCII et alias 8.3 préservés |

Le périmètre volontairement exclu est le support des caractères hors BMP ; ceux-ci exigeraient la gestion de paires de surrogates UTF-16. Les transformations de casse Unicode ne sont pas introduites : seul le repli de casse ASCII préexistant est appliqué, de sorte que les comparaisons restent déterministes et freestanding.

## Validation

Les tests unitaires FAT16 et FAT32 ajoutent chacun un scénario complet autour de `café-2026.txt`. Ils vérifient l’écriture de l’unité `U+00E9` sous la forme `E9 00` dans une entrée LFN, la lecture du contenu par le nom UTF-8 et le nom UTF-8 restitué par le listage. Le socle de conversion vérifie également le rejet de l’encodage UTF-8 surlong `C0 80`.

| Vérification | Résultat |
|---|---|
| Conversion UTF-8 ↔ UTF-16 BMP bornée | Réussie |
| Création, lecture et listage FAT16 avec `café-2026.txt` | Réussis |
| Création, lecture et listage FAT32 avec `café-2026.txt` | Réussis |
| Recherche/suppression FAT32 avec requête UTF-8 BMP validée | Couverte par le nouveau chemin de validation |
| Recherche d’allocations dynamiques dans le périmètre | Aucune occurrence |
| `make -s test-all` | **460/460 tests réussis** |

## Fichiers principaux

| Fichier | Rôle dans le macro-lot |
|---|---|
| `kernel/fs/lfn_utf8.h` | conversion UTF-8/UTF-16 BMP freestanding et bornée |
| `kernel/fs/fat16.c` | création, recherche et listage LFN FAT16 Unicode |
| `kernel/fs/fat32.c` | création, renommage, lecture, suppression et listage LFN FAT32 Unicode |
| `tests/unit/kernel/test_fat16.c` | scénario FAT16 `café-2026.txt` |
| `tests/unit/kernel/test_fat32.c` | conversion, encodage et scénario FAT32 `café-2026.txt` |

## Références

[1] [Microsoft — Long File Names on FAT volumes](https://download.microsoft.com/download/1/2/7/127443f9-8bb5-40a6-8d72-a3c6e5bd12d5/FAT32_WhitePaper.pdf)  
[2] [Unicode Consortium — UTF-8, UTF-16 et BMP](https://www.unicode.org/standard/standard.html)
