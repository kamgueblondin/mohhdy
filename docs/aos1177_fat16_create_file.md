# AOS-1177 à AOS-1192 — Orchestration de création d’un fichier FAT16

> **État :** implémenté localement. **Validation noyau : 33/33 tests verts.**

## Objectif

Ce macro-lot compose les primitives FAT16 précédemment livrées afin de créer un fichier persistant à partir d’un buffer fourni par l’appelant. L’opération réserve les clusters nécessaires, construit une chaîne FAT, écrit les données cluster par cluster et publie enfin une entrée 8.3 dans le répertoire racine.

L’implémentation respecte la contrainte bare-metal de l’OS : aucun `kmalloc`, aucune liste dynamique de clusters et aucun buffer persistant interne. Les données restent dans le buffer caller-owned pendant toute l’opération.

## API

```c
int fat16_create_file(const fat16_volume_t* volume,
                      const char* name,
                      uint8_t attributes,
                      const uint8_t* data,
                      uint32_t size,
                      uint16_t* out_first_cluster);
```

| Élément | Contrat |
|---|---|
| `volume` | Volume FAT16 monté avec writer sectoriel explicite. |
| `name` | Nom court 8.3 accepté par `fat16_create_root_entry`. Les LFN restent exclus. |
| `attributes` | Attributs FAT16 normaux ; la combinaison LFN `0x0F` est refusée. |
| `data` / `size` | Buffer caller-owned et taille logique. `data` doit être non nul si `size` est non nul. |
| `out_first_cluster` | Sortie caller-owned recevant le premier cluster publié. |

Le retour `0` signifie que les données, la chaîne FAT et l’entrée de racine ont été publiées. Toute erreur d’allocation, de chaînage, d’écriture ou de répertoire est propagée après tentative de libération de la chaîne réservée.

## Séquence transactionnelle

La taille d’un cluster est calculée à partir du BPB monté. Pour un fichier non vide, l’orchestrateur réserve autant de clusters que nécessaire, relie chaque nouveau cluster au précédent, puis écrit la portion correspondante du buffer avec `fat16_write_cluster_range`. La publication du répertoire est volontairement la dernière étape : un fichier n’est pas visible tant que ses données et sa chaîne ne sont pas prêtes.

Pour un fichier vide, un cluster initial est tout de même réservé afin de respecter le contrat actuel de `fat16_create_root_entry`, qui exige un cluster valide. La taille inscrite dans l’entrée reste nulle.

> **Invariant de publication :** aucune entrée de racine n’est créée avant que tous les clusters nécessaires aient été réservés, chaînés et écrits.

## Rollback

Lorsqu’une étape échoue avant la publication, `fat16_release_chain` parcourt la chaîne depuis le premier cluster, lit le successeur avant chaque libération et remet l’entrée FAT à zéro dans toutes les copies. La traversée est bornée par le nombre de clusters du volume afin d’éviter une boucle sur une FAT corrompue.

Le rollback ne peut pas annuler une panne physique persistante qui empêcherait une écriture FAT ultérieure. Dans ce cas, l’erreur est retournée à l’appelant et une vérification du volume est nécessaire. Aucun état partiellement publié n’est cependant annoncé comme succès par l’API.

## Tests

Le test `test_creates_persistent_file` crée un fichier `PERSIST.BIN` de 600 octets dans un volume de test à clusters de 512 octets. Il vérifie que le premier cluster est alloué, que la chaîne `3 → 4 → EOC` est répliquée dans les deux FAT, puis relit les 600 octets afin de comparer chaque valeur au buffer source.

| Périmètre | Résultat |
|---|---:|
| Tests unitaires noyau | 33/33 verts |
| Tests FAT16 dans le runner noyau | Vert |
| Suite globale | 416/416 verts |

## Limites conservées

Le lot ne génère pas d’entrées LFN, ne remplace pas l’algorithme FAT16 par FAT32 et ne fournit pas encore la suppression ou le remplacement atomique d’un fichier existant. Les noms longs, la gestion des sous-répertoires et la libération complète après panne seront traités dans des lots séparés.

**Auteur :** Manus AI
