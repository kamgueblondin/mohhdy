# MOHHDY Foundation — Incrément 90 : calcul sur ligne quantifiée

**État :** implémenté sur la branche de travail du lot 90.

## Objectif

Le lot 89 séparait la lecture brute d’une ligne quantifiée. Le lot 90 ajoute `gpt2_gguf_dot_quant_row_buffer`, qui consomme directement cette ligne dans un buffer caller-owned et accumule les super-blocs Q3_K, Q4_K ou Q6_K avec les kernels quantifiés existants.

La primitive ne relit pas FAT16, ne conserve pas de cache implicite et n’alloue aucune mémoire. L’appelant contrôle la durée de vie du buffer, de l’entrée float et du résultat.

## Contrat

La largeur de la ligne doit être `shape[0]` et un multiple de 256. La capacité doit couvrir `shape[0] / 256` super-blocs avec la taille propre au type. Le calcul retourne `0` pour Q3_K, Q4_K et Q6_K valides; `-4` pour un type non supporté, `-6` pour une capacité insuffisante et `-7` pour une largeur incompatible.

| Type | Kernel appelé | Taille par super-bloc |
| --- | --- | ---: |
| Q3_K | `gpt2_q3_k_dot_f32` | 110 octets |
| Q4_K | `gpt2_q4_k_dot_f32` | 144 octets |
| Q6_K | `gpt2_q6_k_dot_f32` | 210 octets |

L’accumulation est effectuée en float, bloc par bloc, avec une adresse calculée depuis le buffer fourni par l’appelant. Le chemin est donc compatible avec la contrainte noyau sans allocation dynamique.

## Tests

La fixture FAT16 lit d’abord une ligne Q4_K, puis la calcule avec une entrée nulle. La même ligne est testée via les chemins Q3_K et Q6_K, et les rejets de capacité insuffisante et de largeur de 32 valeurs sont vérifiés. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain incrément peut intégrer la séquence lecture-calcul au contexte de forward et commencer à exposer une opération de projection QKV sur une ligne à la fois.
