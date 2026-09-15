# MOHHDY Foundation — Incrément 93 : cache KV caller-owned

**État :** implémenté sur la branche de travail du lot 93.

## Objectif

Le lot 93 ajoute un cache KV GGUF séquentiel, fourni et dimensionné par l’appelant. Il stocke les clés et valeurs par couche et position selon la disposition `[layer][position][K puis V]`.

`gpt2_gguf_kv_cache_init` vérifie la capacité totale avant d’initialiser le descripteur. `gpt2_gguf_kv_cache_put` écrit une paire K/V à une position, tandis que `gpt2_gguf_kv_cache_get` la relit dans des buffers fournis par l’appelant. Le champ `count` suit la plus grande position écrite plus un, ce qui prépare le parcours autoregressif.

## Contrat

| Élément | Garantie |
| --- | --- |
| mémoire | aucun `kmalloc`, aucun buffer interne |
| disposition | `[couche][position][K][V]` |
| bornes | couche et position contrôlées |
| capacité | `layers × max_positions × 2 × channels` floats exigés |
| progression | `count = max(count, position + 1)` |

Les calculs d’offsets utilisent des produits 64 bits contrôlés puis un index 32 bits compatible avec le noyau i386 freestanding.

## Tests

La fixture alloue un cache `2 × 3 × 2 × 4`, écrit et relit les K/V de la couche 1 à la position 2, vérifie `count == 3`, puis rejette une couche hors bornes, une capacité de stockage insuffisante et un pointeur nul. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

Le build i386 et le smoke QEMU complet (core, extras, persist, spawn, exec) sont également verts. Un premier passage QEMU avait expiré sur un événement de supervision du runner lent; la relance propre a validé tous les scénarios.

## Suite

Le prochain incrément pourra connecter l’accumulation QKV aux écritures du cache et ajouter une primitive d’itération des positions historiques pour l’attention autoregressive.
