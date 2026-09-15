# MOHHDY Foundation — Incrément 88 : barrière de stockage du forward

**État :** implémenté sur la branche de travail du lot 88.

## Objectif

Le lot 87 vérifiait individuellement la cohérence de `byte_size` avec les axes et le type d’un tenseur. Le lot 88 compose cette vérification avec la validation GPT-2 d’une couche complète via `gpt2_gguf_validate_gpt2_layer_storage`.

Le contexte `gpt2_gguf_forward_context_init` appelle désormais cette barrière après la résolution des dix rôles. Une couche ne peut donc pas être retenue dans le contexte de forward si ses formes `[C]`, `[C,3C]`, `[C,C]`, `[C,4C]`, `[4C,C]` ou ses tailles physiques quantifiées sont incohérentes.

## Tailles par rang

Pour `C = 256` et Q4_K, un vecteur `[C]` vaut un super-bloc, `[3C]` trois super-blocs, `[C,C]` `C` super-blocs, et `[C,3C]` ou `[C,4C]` respectivement `3C` ou `4C` super-blocs. Cette distinction est importante: le produit des axes, et non le seul facteur de sortie, détermine la taille physique.

| Forme | Nombre de super-blocs |
| --- | ---: |
| `[C]` | `1` |
| `[3C]` | `3` |
| `[C,C]` | `C` |
| `[C,3C]` | `3C` |
| `[C,4C]` / `[4C,C]` | `4C` |

La logique reste sans allocation dynamique et compatible avec le linker i386 freestanding: les divisions 64 bits ont été évitées au profit de contrôles par multiplication, masques et décalage pour les blocs de 256 valeurs.

## Tests

La fixture synthétique vérifie une couche Q4_K complète, la taille correcte de chaque vecteur et matrice, puis rejette une seule taille décrémentée. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain lot pourra utiliser la barrière validée avant les lectures FAT16 de lignes et brancher progressivement les tenseurs QKV et MLP sur les primitives quantifiées existantes.
