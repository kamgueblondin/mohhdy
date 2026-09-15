# MOHHDY Foundation — Incrément 82 : descripteur complet de couche GPT-2

**État :** implémenté sur la branche de travail du lot 82.

## Objectif

Le lot 81 savait résoudre individuellement les dix rôles d’un bloc Transformer, mais le futur forward devait encore répéter ces résolutions et gérer lui-même leur présence. Le lot 82 introduit `gpt2_gguf_layer_t`, une structure caller-owned qui regroupe les dix descripteurs de tenseurs d’une couche, son index de couche et un masque de présence.

`gpt2_gguf_map_layer` réutilise le buffer de nom fourni par l’appelant, appelle la résolution bornée de chaque rôle et remplit le descripteur sans allocation dynamique. Une résolution réussie positionne les dix bits du masque `present_mask` à `0x3FF`; une couche absente ou un buffer invalide est rejeté sans exposer de pointeur non vérifié.

## Contrat

| Élément | Contrat |
| --- | --- |
| descripteurs | 10 tenseurs par bloc |
| familles | attention, normalisations et MLP |
| stockage | structure caller-owned |
| buffer de noms | caller-owned, réutilisé séquentiellement |
| masque complet | `0x3FF` |
| allocation noyau | aucune |
| erreur couche absente | `-8` via l’index |

Les cinq rôles globaux historiques demeurent inchangés. Les rôles de couche restent basés sur les noms GGUF `blk.<layer>.*` et conservent les contrôles de capacité du lot 81.

## Tests

La fixture GGUF contient les cinq tenseurs globaux et les dix noms de couche du bloc zéro. Le test vérifie que `gpt2_gguf_map_layer` remplit les dix entrées et le masque `0x3FF`, que l’index de couche vaut zéro et qu’une couche un absente est rejetée. Les tests de nom exact et de buffer trop petit du lot 81 restent actifs.

La suite `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**. Les kernels FAT16, GGUF, Q-K et les scénarios de bornes restent verts.

## Limites et suite

Le descripteur ne valide pas encore les dimensions sémantiques propres à chaque matrice, le nombre de couches du modèle ou la compatibilité des types entre matrices. Il ne charge pas le checkpoint complet et n’exécute pas le forward. La prochaine étape peut utiliser ce descripteur pour construire un contexte de forward caller-owned, en conservant la lecture paginée FAT16 et les contrôles de débordement.
