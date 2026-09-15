# MOHHDY Foundation — Incrément 89 : lecture de ligne quantifiée FAT16

**État :** implémenté sur la branche de travail du lot 89.

## Objectif

Le lot 89 ajoute `gpt2_gguf_read_quant_row_fat16`, une primitive de lecture brute d’une ligne quantifiée dans un buffer fourni par l’appelant. Elle complète les lectures de tenseur et de super-bloc sans charger une matrice complète en mémoire.

La fonction accepte les tenseurs 1D ou 2D, interprète `shape[0]` comme largeur de ligne et `shape[1]` comme nombre de lignes lorsqu’il est présent. Elle calcule la taille d’une ligne selon le type Q3_K, Q4_K ou Q6_K, vérifie l’alignement sur 256 valeurs, contrôle l’index de ligne et la capacité caller-owned, puis délègue à `gpt2_gguf_read_tensor_fat16` pour la plage physique exacte.

## Contrat

| Condition | Résultat |
| --- | --- |
| Q3_K, Q4_K ou Q6_K valide | lecture exacte de la ligne |
| largeur non multiple de 256 | `-7` |
| ligne hors bornes ou overflow | `-9` |
| buffer trop petit | `-6` |
| type non quantifié supporté | `-4` |
| lecture physique partielle | `-8` |

Aucune allocation dynamique n’est utilisée. Le résultat est écrit uniquement dans le buffer de l’appelant et `out_read` est remis à zéro sur les erreurs après validation du pointeur.

## Tests

La fixture FAT16 vérifie les deux lignes Q4_K avec les marqueurs physiques `0x11` et `0x77`, puis couvre les variantes Q3_K et Q6_K ainsi qu’un buffer insuffisant. La non-régression `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain lot peut utiliser cette primitive pour découpler la lecture d’une ligne de son calcul, puis connecter les lignes QKV et MLP aux kernels de produit quantifié déjà présents.
