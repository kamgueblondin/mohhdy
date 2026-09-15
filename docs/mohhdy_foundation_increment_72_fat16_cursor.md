# MOHHDY Foundation — Incrément 72 : curseur FAT16 réutilisable

**État :** implémenté sur la branche de travail du lot 72.

## Objectif

Cet incrément ajoute `fat16_open_file` et `fat16_file_read`. Le premier résout une entrée 8.3 et initialise un état caller-owned; le second lit séquentiellement des fenêtres successives en conservant le cluster courant, la position dans le fichier et la garde de parcours FAT16. Une suite de petites lectures ne rescane donc plus le répertoire ni la chaîne depuis le début.

Le curseur ne possède aucune mémoire dynamique et reste invalide après une ouverture échouée. Les contrôles vérifient le montage, le type fichier, la position, la taille des secteurs, le nombre de clusters et les marqueurs de fin FAT16 avant chaque accès. Lorsque la fin du fichier est atteinte, la lecture retourne zéro octet avec un statut de succès; un appel avec buffer nul ou capacité nulle est refusé.

## Usage GGUF

Le curseur constitue la base d’une lecture progressive des métadonnées et des données de tenseurs GGUF depuis FAT16. Le loader complet du lot 70 reste adapté aux petits profils, tandis que le futur runtime pourra conserver un curseur par fichier et demander les fenêtres correspondant aux offsets des tenseurs indexés.

| Élément | Garantie |
| --- | --- |
| état | entièrement fourni par l’appelant dans `fat16_file_t` |
| progression | `position` et `cluster_offset` avancent après chaque lecture |
| FAT | lecture du prochain cluster uniquement à la frontière nécessaire |
| mémoire | aucune allocation ni copie du fichier complet |
| fin | `out_read == 0` quand la position atteint la taille |

## Validation

Le test ouvre `FATOK.TXT`, lit successivement `hel`, puis `lo`, puis vérifie la fin de fichier. Les tests précédents de lecture par plage, de loader GGUF depuis FAT16, de bornes et de corruption restent actifs. La suite `make test-all` atteint **265 tests réussis, 0 échec et 0 test ignoré**.

## Limites et suite

Le curseur est séquentiel et ne propose pas encore de seek; un accès aléatoire continue d’utiliser `fat16_read_file_range`. La prochaine étape pourra associer un curseur à un descripteur de tenseur GGUF et exposer une lecture vérifiée par type, offset et taille de bloc quantifié, avant tout branchement au forward GPT-2.
