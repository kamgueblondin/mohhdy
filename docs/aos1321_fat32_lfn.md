# AOS-1321 à AOS-1332 — Fondations LFN FAT32 sans allocation dynamique

## Objectif

Ce macro-lot ajoute les primitives déterministes nécessaires à la génération d’une entrée **Long File Name (LFN)** FAT32. Le contrat demeure borné : l’appelant fournit le buffer de 32 octets, aucun `kmalloc` n’est utilisé et une entrée représente au plus 13 unités UTF-16LE issues d’un nom UTF-8 validé.

## Contrat technique

| Primitive | Contrat |
|---|---|
| `fat32_lfn_checksum` | Calcule le checksum FAT sur les 11 octets de l’alias 8.3. |
| `fat32_encode_lfn_entry` | Initialise et encode une entrée LFN de 32 octets dans un buffer fourni par l’appelant. |
| Mémoire | Zéro allocation dynamique ; buffers statiques ou caller-owned uniquement. |
| Encodage | UTF-8 validé, conversion UTF-16LE y compris paires substituts, 13 unités UTF-16 maximum par entrée. |
| Métadonnées | Attribut `0x0F`, type nul, checksum alias, cluster initial nul. |

L’algorithme de checksum applique la rotation droite d’un bit puis l’addition de chaque octet de l’alias. Les unités UTF-16LE sont écrites aux offsets FAT LFN standards `1,3,5,7,9`, `14,16,18,20,22,24`, puis `28,30`; les paires substituts sont produites et validées pour les points de code non-BMP. Les emplacements après le terminateur sont initialisés à `0xFFFF` et le terminateur est écrit lorsqu’il reste une position dans l’entrée.

## Publication et reconstruction livrées

`fat32_create_lfn_file` réserve d’abord les données via le créateur FAT32 transactionnel, localise l’alias 8.3, libère son slot logique puis publie les ordinals LFN décroissants dans les slots suivants avant de réécrire l’alias court. Les slots traversent la chaîne du répertoire racine et peuvent provoquer son extension automatique ; les buffers restent caller-owned et aucune allocation dynamique n’est introduite.

`fat32_list_root` parcourt la chaîne racine, ignore les entrées supprimées et les volumes, reconstitue les fragments UTF-16LE en UTF-8 dans un buffer `os_fat16_dirent_t` fourni par l’appelant, puis n’accepte le nom long que si la séquence d’ordinals et le checksum de l’alias sont cohérents. En cas de séquence invalide, le listage retombe sur l’alias 8.3 plutôt que de publier un nom non vérifié. La lecture reconnaît le nom long, tandis que `fat32_rename_lfn_file` et `fat32_unlink_file` assurent respectivement son renommage et sa suppression transactionnels sans allocation dynamique.

## Limites explicites

Le contrat reste borné à `OS_NAME_MAX - 1` octets de sortie UTF-8, à 13 unités UTF-16 par entrée et à 20 entrées LFN par fichier. Les entrées sont limitées aux séquences UTF-8 valides et excluent les séparateurs de chemin ainsi que les contrôles. L’intégration du volume FAT32 aux opérations VFS génériques reste hors de ce périmètre.

## Validation

Le test `test_lfn_utf8_bmp_conversion` vérifie les conversions UTF-8/UTF-16LE, y compris les paires substituts. `test_fat32_lfn_encoding` vérifie checksum, ordinal, attribut, encodage UTF-16LE et borne d’entrée. `test_fat32_lfn_file_and_list` couvre création, lecture par noms court et long, listage, renommage et suppression ; les tests de round-trip vérifient ensuite un nom BMP et un nom non-BMP. La suite complète `make test-all` est le contrôle de non-régression de référence.
