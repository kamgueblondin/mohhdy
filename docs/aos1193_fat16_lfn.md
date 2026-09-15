# AOS-1193 à AOS-1208 — Sous-lot LFN FAT16 borné

> **État :** implémenté localement. **Validation noyau : 33/33 tests verts.**

## Périmètre

Ce sous-lot ajoute la publication d’un fichier FAT16 accompagné d’entrées **Long File Name** et la reconstruction de son nom dans le listage de la racine. L’implémentation utilise uniquement des buffers automatiques de taille fixe et des données fournies par l’appelant ; elle n’introduit aucun `kmalloc`.

Le nom long accepté est constitué de caractères ASCII imprimables, sans séparateur de chemin, et est limité à `OS_NAME_MAX - 1` octets. Chaque caractère est stocké sous forme de code UTF-16LE sur un octet utile et un octet nul. Ce choix constitue un sous-ensemble sûr avant l’ajout de la conversion UTF-8/UTF-16 complète.

## Publication

L’API est la suivante :

```c
int fat16_create_lfn_file(const fat16_volume_t* volume,
                          const char* long_name,
                          const char* short_name,
                          uint8_t attributes,
                          const uint8_t* data,
                          uint32_t size,
                          uint16_t* out_first_cluster);
```

L’orchestrateur crée d’abord le contenu et l’alias 8.3 par `fat16_create_file`. Il réserve ensuite les slots contigus suivants, marque l’ancien slot court comme supprimé, écrit les entrées LFN dans l’ordre FAT16 requis, puis écrit l’entrée courte finale. L’octet ordinal du premier slot porte le marqueur `0x40`, tandis que les ordinals suivants sont décroissants jusqu’à `1`.

Chaque entrée LFN contient l’attribut `0x0F`, un type nul, le checksum de l’alias 8.3 et les trois champs UTF-16LE aux offsets FAT16 standards. Le checksum est recalculé lors du listage pour empêcher l’association d’un nom long à un alias différent.

## Reconstruction

`fat16_list_root` conserve une vue caller-owned de la séquence LFN courante. Il accepte une séquence uniquement si les ordinals sont continus, si le checksum reste identique et si le checksum final correspond à l’entrée courte suivante. Les entrées invalides ou interrompues sont ignorées et l’alias court est alors utilisé comme représentation de repli.

Le résultat est copié dans `os_fat16_dirent_t.name`, dont la capacité est `OS_NAME_MAX`. Les valeurs UTF-16 hors ASCII sont remplacées par `?` dans ce sous-lot ; aucune conversion Unicode complète n’est prétendue.

| Élément | Statut |
|---|---|
| Écriture LFN ASCII bornée | Implémentée |
| Encodage UTF-16LE des caractères ASCII | Implémenté |
| Checksum alias 8.3 | Implémenté |
| Reconstruction dans `fat16_list_root` | Implémentée |
| Recherche directe par nom long dans `read`/`open` | Prochain incrément |
| UTF-8 complet, caractères non BMP et normalisation | Prochain incrément |
| FAT32 | Hors périmètre de ce lot |

## Validation

Le test LFN crée `Session-2026-A` avec l’alias `SESS01.TXT`, vérifie la suppression de l’ancien slot, les ordinals `0x42` et `0x01`, l’attribut LFN, l’encodage des caractères et la restitution du nom long par `fat16_list_root`. La suite noyau est verte à **33/33** et la suite complète est verte à **417/417**.

## Contraintes et suite

Le lot ne modifie pas l’ABI de `os_fat16_dirent_t`, ne dépend d’aucune allocation dynamique et conserve l’alias court pour les APIs de lecture existantes. Le prochain incrément doit factoriser la recherche d’entrée afin d’accepter soit l’alias 8.3, soit le nom long validé, puis ajouter les tests de `read_file`, `read_file_range`, `open_file` et du curseur.

**Auteur :** Manus AI
