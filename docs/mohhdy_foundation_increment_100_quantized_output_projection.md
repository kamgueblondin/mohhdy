# MOHHDY Foundation — Incrément 100 : projection de sortie quantifiée

**État :** implémenté et testé.

## Objectif

Le lot 100 ajoute `gpt2_gguf_project_matrix_fat16`, une projection matricielle générique pour les tenseurs GGUF quantifiés 2D orientés `[input_channels, output_channels]`. La fonction lit chaque ligne de sortie depuis FAT16 dans le scratch caller-owned, calcule son produit avec le vecteur d’entrée et écrit le résultat dans la sortie caller-owned.

Cette primitive généralise le chemin QKV sans imposer la forme `[C,3C]`. Elle prépare directement l’application de `attn_output.weight` au vecteur concaténé multi-têtes.

## Contrat caller-owned

| Élément | Contrat |
| --- | --- |
| tenseur | forme GGUF 2D `[input_channels, output_channels]`, type Q3_K/Q4_K/Q6_K |
| `input` | `input_channels` activations en lecture seule |
| `row_buffer` | scratch d’une ligne quantifiée, réutilisé pour chaque sortie |
| `output` | `output_channels` résultats float |
| capacités float | exprimées en octets, comme les APIs QKV existantes |
| allocation | aucune allocation dynamique |

La fonction contrôle les dimensions, les pointeurs, la capacité de sortie et délègue les contrôles de taille de ligne au lecteur quantifié. Elle n’ajoute pas de biais et ne modifie ni le tenseur ni le vecteur d’entrée.

## Validation

La fixture Q4_K `[256,2]` est projetée avec une entrée nulle. Les deux sorties restent nulles, puis les tests vérifient une destination trop courte et une forme d’entrée incohérente. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

La projection de sortie est maintenant disponible après concaténation des têtes. Le prochain incrément peut intégrer la boucle de bloc d’attention, puis enchaîner sur la normalisation et les projections MLP quantifiées.
