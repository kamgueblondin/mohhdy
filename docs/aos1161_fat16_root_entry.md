# AOS-1161 à AOS-1176 — Création d’une entrée racine FAT16

> **État :** implémenté et validé. **Validation : 415/415 tests verts.**

## Objectif

Ce macro-lot ajoute la création bornée d’une entrée de répertoire dans la racine FAT16. L’opération complète les primitives précédentes d’écriture sectorielle, d’allocation de cluster et de liaison de chaînes, tout en conservant la contrainte stricte de l’OS : **aucune allocation dynamique et aucun `kmalloc`**.

L’appelant fournit le volume monté, le nom, les attributs, le premier cluster et la taille logique du fichier. Le noyau convertit le nom en format court 8.3, recherche un emplacement libre ou supprimé dans la racine, construit l’entrée de 32 octets et écrit le secteur concerné via le writer explicitement configuré.

## Contrat d’API

```c
int fat16_create_root_entry(const fat16_volume_t* volume,
                            const char* name,
                            uint8_t attributes,
                            uint16_t first_cluster,
                            uint32_t size);
```

| Paramètre | Contrat |
|---|---|
| `volume` | Volume FAT16 monté et doté d’un writer sectoriel. |
| `name` | Nom court 8.3, sans chemin, avec base de 1 à 8 caractères et extension facultative de 1 à 3 caractères. |
| `attributes` | Attribut FAT16 normal ; la combinaison LFN `0x0F` est refusée. |
| `first_cluster` | Cluster FAT16 valide, compris dans la plage du volume ; son allocation préalable relève de l’appelant. |
| `size` | Taille logique du fichier, stockée en little-endian dans l’entrée. |

La fonction retourne `0` en cas de succès. Elle retourne `OS_FAT16_NOT_MOUNTED` lorsque le volume n’est pas monté ou qu’aucun writer n’est disponible, `OS_FAT16_BAD_PATH` pour un nom invalide ou un attribut LFN, `OS_FAT16_CORRUPT` pour un cluster invalide ou une erreur d’I/O, et `OS_FAT16_NOT_FOUND` lorsque la racine ne contient aucun slot libre.

## Conversion 8.3

La conversion est volontairement limitée au format court. Le nom est séparé autour du point unique ; les caractères sont normalisés en majuscules et les espaces de remplissage sont ajoutés jusqu’aux champs de huit et trois octets. Les noms de chemin, les points multiples, les caractères interdits, une base vide, une base de plus de huit caractères ou une extension de plus de trois caractères sont rejetés.

Cette politique ne fabrique aucune entrée LFN et ne prétend pas représenter un nom long sous un alias implicite. Le support LFN constitue donc un lot ultérieur distinct, avec ses propres entrées, sommes de contrôle et règles de rollback.

## Recherche et écriture du slot

La racine est parcourue entrée par entrée. Un slot dont le premier octet vaut `0x00` est libre et termine également l’usage courant de la racine ; un slot dont le premier octet vaut `0xE5` est supprimé et peut être réutilisé. Les autres slots sont conservés.

Lorsqu’un emplacement est retenu, ses 32 octets sont remis à zéro avant l’écriture du nom court, de l’attribut, du cluster initial et de la taille. Les champs multi-octets sont écrits en little-endian. Le secteur est relu avant modification, puis soumis au writer explicite ; une défaillance d’écriture est propagée sous forme d’erreur et ne signale pas une création réussie.

| Offset dans l’entrée | Taille | Contenu |
|---:|---:|---|
| `0` | 11 | Nom court 8.3, base puis extension |
| `11` | 1 | Attributs FAT16 |
| `26` | 2 | Premier cluster |
| `28` | 4 | Taille du fichier |

## Invariants de sécurité

Le volume et le writer restent caller-owned. La fonction n’alloue pas de cluster, ne modifie pas la FAT, ne crée pas de chaîne implicitement et ne copie pas le contenu du fichier. L’appelant doit donc effectuer, dans l’ordre approprié, l’allocation, la liaison éventuelle, l’écriture des données et enfin la publication de l’entrée de répertoire.

Cette séparation rend possible une orchestration transactionnelle ultérieure : en cas d’échec avant publication de l’entrée, les clusters réservés pourront être libérés par une primitive de rollback dédiée. Le présent lot ne prétend pas fournir ce rollback complet.

## Tests et non-régression

Le test FAT16 couvre notamment la création d’une entrée `NOTE.TXT`, le remplissage des champs 8.3, l’attribut, le cluster et la taille, ainsi que le rejet d’un véritable nom long (`TOOLONG12.TXT`). Les chemins d’erreur existants vérifient également le montage, le writer, les limites de cluster et les buffers.

La suite globale exécutée avec `make test-all` produit le résultat suivant :

| Total | Réussis | Échecs | Ignorés |
|---:|---:|---:|---:|
| 415 | 415 | 0 | 0 |

## Suite logique

Le prochain macro-lot recommandé est l’orchestration d’un fichier FAT16 persistant : allouer un ou plusieurs clusters, les chaîner, écrire les données par plages, puis publier l’entrée racine avec une gestion explicite des échecs. Après cette fondation, le backlog pourra traiter les entrées LFN et les transitions FAT32 sans mélanger les contrats d’allocation.

**Auteur :** Manus AI
