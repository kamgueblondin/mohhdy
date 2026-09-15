# AOS-1545 à AOS-1552 — forward d’un bloc transformeur GPT-2 GGUF

## Objectif

Les kernels GGUF et les helpers d’attention existaient isolément, mais aucun chemin ne reliait la projection QKV, l’écriture dans le cache KV, l’attention autoregressive, la projection de sortie et le MLP d’un même bloc GPT-2. Ce lot fournit cette orchestration dans `gpt2_gguf_block_forward_fat16`.

La fonction travaille exclusivement avec des tenseurs indexés et des buffers fournis par l’appelant. Elle ne réserve aucune mémoire et ne conserve aucun état implicite hors du cache KV caller-owned.

## Séquence d’exécution

| Étape | Entrée | Sortie | Primitive réutilisée |
|---|---|---|---|
| Normalisation attention | résiduel | `workspace.norm` | `gpt2_gguf_layernorm` |
| Projection QKV | état normalisé | query, key, value | `gpt2_gguf_project_qkv_fat16` |
| Mémoire temporelle | key, value | cache KV couche/position | `gpt2_gguf_kv_cache_put` |
| Attention causale | query + cache | vecteur concaténé | `gpt2_gguf_kv_cache_attention_multi_head` |
| Projection attention | concaténation | résiduel mis à jour | `gpt2_gguf_attention_output_add_residual_fat16` |
| Normalisation MLP | résiduel | `workspace.norm` | `gpt2_gguf_layernorm` |
| MLP | état normalisé | résiduel mis à jour | `gpt2_gguf_mlp_forward_fat16` puis ajout résiduel |

Le cache KV est global aux couches : son compteur représente la profondeur temporelle commune. Une même position peut donc être écrite par chacune des couches. Une position future absente du cache est refusée.

## Workspace et sûreté

`gpt2_gguf_block_workspace_t` porte chaque vecteur temporaire et sa capacité explicite. Les capacités de vecteurs sont exprimées en éléments `float`; le runtime les convertit en octets uniquement à la frontière des primitives de lecture/projection qui l’exigent.

> Cette conversion corrige également une incohérence antérieure : les projections MLP et d’attention recevaient des capacités en éléments alors que leur contrat aval attendait des octets.

Le runtime vérifie avant toute lecture FAT16 les dimensions `[C, 3C]`, `[C, C]`, `[C, H]` et `[H, C]`, la divisibilité des têtes, la position, les capacités de chaque buffer et les produits susceptibles de déborder.

## Validation

La suite FAT16 comprend désormais un modèle GGUF Q4_K synthétique contenant les quatre matrices d’un bloc de 256 canaux et un MLP de largeur 512. Le test :

| Scénario | Résultat |
|---|---|
| Position 0 | QKV, cache, attention, projection, MLP et résiduel réussissent |
| Position 1 | lecture de l’historique causal et extension du cache réussissent |
| Workspace avec un score insuffisant | rejet contrôlé `-6` |

Le modèle de test est réparti sur une chaîne FAT16 de plusieurs clusters ; la validation couvre donc aussi les lectures de lignes quantifiées au-delà du premier cluster.

## Limites restantes

Le forward d’un bloc est disponible, mais la génération GGUF de bout en bout reste à raccorder. Les travaux suivants devront préparer les couches d’un modèle réel, lire les embeddings/positions et les paramètres de normalisation depuis les tenseurs denses, itérer sur les blocs, appliquer la normalisation finale, calculer les logits et choisir le token dans l’interface locale.
