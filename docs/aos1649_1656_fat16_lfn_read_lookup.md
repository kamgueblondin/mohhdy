# AOS-1649 à AOS-1656 — recherche FAT16 par nom long pour la lecture

## Objet

Ce macro-lot rend les opérations FAT16 de lecture capables de sélectionner un fichier par son **alias 8.3** ou par son **nom long LFN ASCII validé**. Il complète le lot de création et de listage LFN sans modifier l’ABI publique `fat16_*`, sans allocation dynamique et sans dupliquer la logique de parcours de clusters.

## Contrat de recherche

La nouvelle recherche interne `fat16_find_root_entry` parcourt la racine FAT16 une seule fois. Elle conserve un état LFN borné dans un tableau automatique de taille `OS_NAME_MAX`, exactement comme le listage déjà existant. Une séquence ne peut produire une correspondance longue que lorsque toutes les conditions suivantes sont respectées.

| Condition | Effet |
|---|---|
| Nom demandé non vide, borné et ASCII imprimable | Autorise la recherche LFN ; les séparateurs de chemin et les octets non ASCII sont refusés. |
| Alias 8.3 valide | Conserve le repli historique, sans sensibilité à la casse ASCII. |
| Entrées LFN ordonnées, séquence initiale marquée et checksum cohérent | Reconstruit le nom long dans le buffer local. |
| Entrée courte suivante avec checksum correspondant | Lie le LFN à son entrée FAT16 effective. |
| Séquence supprimée, volume ou entrée de volume | Invalide l’état LFN et n’est jamais sélectionnée. |

> La recherche retourne toujours l’entrée courte FAT16 associée. Les lecteurs réemploient donc les champs existants `first_cluster`, `size` et `attributes` sans nouvelle structure, ni copie persistante, ni changement du format disque.

## Intégration

`fat16_read_file`, `fat16_read_file_range` et `fat16_open_file` appellent désormais cette primitive commune. Le curseur `fat16_file_read` ne change pas : après l’ouverture par LFN, il conserve son cache de secteur, sa fenêtre FAT16 caller-owned et ses gardes de chaîne FAT habituels.

| API | Prise en charge après le lot |
|---|---|
| `fat16_read_file` | Alias 8.3 et LFN ASCII validé. |
| `fat16_read_file_range` | Alias 8.3 et LFN ASCII validé, y compris un offset non nul. |
| `fat16_open_file` | Alias 8.3 et LFN ASCII validé. |
| `fat16_file_read` | Inchangé ; fonctionne sur le curseur ouvert par LFN. |
| `fat16_list_root` | Inchangé ; demeure la source de restitution des LFN validés. |

## Validation

Le scénario Unity de création `Session-2026-A` exerce désormais les quatre usages de lecture du même volume en mémoire. Il vérifie la lecture totale avec un nom long en casse différente, la lecture bornée à offset, l’ouverture puis la lecture cursorisée, ainsi que le refus d’un LFN absent. Les seize scénarios FAT16 restent verts, y compris les chemins GGUF, les fenêtres multi-secteurs, les chaînes profondes et l’écriture persistante.

La suite complète doit être exécutée avant publication afin de confirmer l’absence de régression dans les autres lecteurs FAT16, le VFS et le runtime GGUF.

## Limites

Le format reste volontairement limité aux LFN ASCII de `OS_NAME_MAX - 1` caractères. L’UTF-8 complet, les caractères non BMP, la normalisation et les opérations de suppression ou renommage LFN restent hors de ce macro-lot. Le contrat n’introduit aucune allocation implicite.

## Références internes

- [Fondations LFN FAT16](aos1193_fat16_lfn.md)
- [Contrats FAT16 publics](../kernel/fs/fat16.h)
- [Clôture de la latence GGUF](aos1641_1648_gguf_latency_closure.md)
