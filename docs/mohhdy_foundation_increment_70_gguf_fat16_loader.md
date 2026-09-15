# MOHHDY Foundation — Incrément 70 : loader GGUF depuis FAT16

**État :** implémenté sur la branche de travail du lot 70.

## Objectif

Cet incrément relie le volume FAT16 lecture seule au parseur GGUF déjà validé. `gpt2_gguf_load_fat16` lit un fichier dans un buffer fourni par l’appelant, vérifie le retour du backend FAT16, puis construit immédiatement l’index GGUF et le mapping des rôles GPT-2. Le chemin ne fait aucune allocation dynamique, ne conserve aucun pointeur vers un buffer temporaire et ne modifie pas le contenu du volume.

La capacité du buffer est contrôlée avant lecture. Un fichier absent, un nom invalide, un volume non monté, une capacité insuffisante ou un fichier GGUF structurellement incorrect renvoie une erreur explicite au lieu de lancer une interprétation partielle. Les erreurs FAT16 restent distinctes des erreurs de validation GGUF, ce qui permet au shell ou au futur runtime de produire un diagnostic utile.

## Convention de stockage

Le backend FAT16 actuel traite les noms courts 8.3. Le nom logique `gpt2.gguf` ne peut donc pas être utilisé tel quel dans cette première tranche, car son extension contient quatre caractères. Le profil disque de test est nommé `GPT2.GGU`; cette contrainte devra être levée dans un lot ultérieur avec une résolution de chemins ou des noms longs dédiés.

| Étape | Contrat |
| --- | --- |
| Montage | `fat16_mount` valide le BPB et les limites du volume |
| Lecture | `fat16_read_file` copie dans un buffer fourni, sans allocation |
| Parse | `gpt2_gguf_build_index` vérifie GGUF v3, GPT-2, alignement et tailles |
| Sélection | `gpt2_gguf_map_role` retrouve les rôles structurants |

## Validation

La fixture Unity construit un petit fichier GGUF Q4_K dans un cluster FAT16 séparé, le lit sous le nom `gpt2.ggu`, vérifie les 320 octets chargés et retrouve le rôle `output.weight`. Le test couvre ainsi le montage, la résolution 8.3, la chaîne FAT, la copie bornée, l’index GGUF et le mapping GPT-2 dans une seule exécution i386.

La suite `make test-all` est verte avec **263 tests réussis, 0 échec et 0 test ignoré**. Le test de chargement rejoint les suites FAT16, GGUF, quantification, robustesse et les autres modules sans régression; aucun forward GPT-2 quantifié complet n’est annoncé par ce lot.

## Limites et suite

Le loader ne charge pas encore les centaines de mégaoctets d’un checkpoint GPT-2 réel dans une représentation paginée et ne lit pas les tenseurs à la demande depuis FAT16. Il fournit la première jonction sûre entre stockage persistant et métadonnées de modèle. La suite cohérente consiste à ajouter une lecture par plages ou par secteurs, puis à connecter progressivement les tenseurs de configuration, d’embedding et de blocs au runtime quantifié sans dépasser les bornes mémoire du noyau i386.
