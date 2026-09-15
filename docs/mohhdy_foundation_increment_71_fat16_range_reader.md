# MOHHDY Foundation — Incrément 71 : lecture par plages FAT16

**État :** implémenté sur la branche de travail du lot 71.

## Objectif

Cet incrément ajoute `fat16_read_file_range`, une primitive de lecture partielle adaptée aux profils GGUF qui ne doivent pas être copiés intégralement en mémoire noyau. L’appelant fournit un offset, un buffer et une capacité; le backend parcourt la chaîne FAT16 jusqu’au cluster concerné, lit uniquement les secteurs nécessaires et retourne le nombre exact d’octets copiés.

La primitive reste compatible avec le volume lecture seule et le format 8.3 existant. Elle ne modifie ni la FAT ni le répertoire racine, ne crée aucune allocation et conserve les contrôles de borne sur les clusters, les secteurs, les offsets et les tailles. Un offset strictement supérieur à la taille du fichier est refusé; une lecture en fin de fichier peut retourner zéro octet sans dépasser le volume.

## Contrat pour GGUF

Le loader complet du lot 70 continue d’être utile pour les petits profils de validation. La lecture par plages prépare désormais un chargeur GGUF paginé: l’en-tête et les descripteurs pourront être lus dans une petite fenêtre, puis les données d’un tenseur pourront être demandées à la position exacte sans charger le checkpoint entier.

| Paramètre | Garantie |
| --- | --- |
| `offset` | doit être inférieur ou égal à la taille du fichier |
| `buffer` / `max` | mémoire fournie par l’appelant et taille strictement contrôlée |
| chaîne FAT | saut de clusters borné par `cluster_count` |
| retour | `out_read` contient le nombre réellement copié |
| erreurs | volume, chemin, corruption et capacité restent distingués |

## Validation

Le test FAT16 lit `hello` à partir du deuxième octet de `FATOK.TXT`, vérifie la copie de `ell` et rejette un offset situé au-delà de la taille. La suite globale `make test-all` atteint **264 tests réussis, 0 échec et 0 test ignoré**. Les tests GGUF, le loader FAT16→GGUF du lot 70, la robustesse et les modules historiques restent verts.

## Limites et suite

La lecture par plages ne fournit pas encore un descripteur de fichier persistant ni une lecture sectorielle asynchrone. Chaque appel retrouve l’entrée racine et reparcourt la chaîne depuis le début; cette simplicité est volontaire pour le premier contrat sûr. Le prochain groupe pourra ajouter un curseur de fichier ou une API de lecture de tenseur directement adossée à l’index GGUF, tout en conservant les limites i386 et l’absence d’allocation non contrôlée.
