# MOHHDY Foundation — Incrément 73 : lecture de tenseur GGUF depuis FAT16

**État :** implémenté sur la branche de travail du lot 73.

## Objectif

Le lot 73 relie l’index GGUF et le backend FAT16 par `gpt2_gguf_read_tensor_fat16`. L’appelant fournit un modèle déjà chargé, un descripteur de tenseur, un offset relatif et un buffer. L’API calcule l’offset absolu dans la zone `tensor_data`, vérifie les additions 32-bit et délègue la lecture au chemin FAT16 borné.

La capacité demandée est automatiquement plafonnée à la taille restante du tenseur. Les arguments invalides, les offsets hors descripteurs et les débordements d’offset sont refusés avant tout accès disque. La fonction ne décode pas les valeurs quantifiées et ne prétend pas encore exécuter le forward; elle fournit uniquement une fenêtre binaire sûre pour le futur kernel Q4_K/Q6_K.

## Chaîne d’accès

| Étape | Contrôle |
| --- | --- |
| index GGUF | modèle marqué valide et tenseur déjà décrit |
| adresse | `tensor_data_offset + data_offset + tensor_offset` sans overflow |
| capacité | limitée à `byte_size - tensor_offset` |
| stockage | `fat16_read_file_range` et chaîne FAT bornée |
| résultat | `out_read` donne les octets réellement copiés |

## Validation

La fixture disque GGUF Q4_K du lot 70 est réutilisée. Après chargement et mapping de `output.weight`, le test lit quatre octets à l’offset du tenseur depuis `GPT2.GGU` et vérifie le contenu retourné. La suite `make test-all` reste à **265 tests réussis, 0 échec et 0 test ignoré**, avec le build des modules GGUF, FAT16, quantification et robustesse.

## Limites et suite

Le descripteur doit encore être fourni par un index GGUF conservé en mémoire; l’API ne possède pas de handle persistant et ne valide pas la cohérence sémantique du rôle au-delà du descripteur fourni. Un prochain incrément pourra introduire un handle modèle compact et une lecture de blocs quantifiés directement vers les kernels, sans charger l’intégralité du checkpoint.
