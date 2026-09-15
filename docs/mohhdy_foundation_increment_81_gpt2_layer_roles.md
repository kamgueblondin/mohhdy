# MOHHDY Foundation — Incrément 81 : rôles GPT-2 par couche

**État :** implémenté sur la branche de travail du lot 81.

## Objectif

Les lots précédents associaient les cinq tenseurs globaux GPT-2 aux rôles `token_embd`, `position_embd`, `output_norm` et `output`. Le lot 81 étend cette abstraction aux familles répétées de chaque bloc Transformer, sans charger le checkpoint complet et sans allouer de mémoire dans le noyau.

La nouvelle API `gpt2_gguf_map_layer_role` reçoit un index GGUF caller-owned, un numéro de couche, un rôle, un buffer de nom caller-owned et sa capacité. Elle construit le nom `blk.<layer>.<suffix>` dans ce buffer, puis réutilise `gpt2_gguf_index_find` pour résoudre le descripteur déjà indexé.

## Rôles couverts

| Famille | Rôles GGUF |
| --- | --- |
| normalisation attention | `attn_norm.weight`, `attn_norm.bias` |
| projection attention | `attn_qkv.weight`, `attn_qkv.bias`, `attn_output.weight`, `attn_output.bias` |
| normalisation MLP | `ffn_norm.weight`, `ffn_norm.bias` |
| MLP | `ffn_up.weight`, `ffn_down.weight` |

Les cinq rôles globaux historiques restent inchangés et `gpt2_gguf_map_role` continue de rejeter les rôles qui ne sont pas globaux. La conversion du numéro de couche est bornée à dix chiffres et le nom refuse explicitement un buffer trop petit.

## Tests

La fixture GGUF caller-owned contient désormais sept tenseurs: les cinq tenseurs globaux historiques, `blk.0.attn_norm.weight` et `blk.0.attn_qkv.weight`. Les tests vérifient la construction exacte des deux noms, leur résolution dans l’index, le rejet de `blk.1` absent et le rejet d’une capacité de huit octets.

La suite `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**. Les taux de couverture rapportés restent de 85 % pour le noyau, 72 % pour l’espace utilisateur et 78 % global; l’avertissement de cible 70–79 % est inchangé.

## Limites et suite

Ce lot établit le mapping sémantique des familles par couche, mais ne parcourt pas encore les `n_layer` couches d’un checkpoint réel, ne valide pas les formes propres à chaque matrice et ne branche pas encore ces descripteurs au forward quantifié. Les biais ne disposent pas encore d’une primitive de lecture/dot dédiée, et le forward autoregressif complet reste hors périmètre.
