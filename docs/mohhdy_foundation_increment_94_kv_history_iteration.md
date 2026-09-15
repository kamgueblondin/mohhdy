# MOHHDY Foundation — Incrément 94 : itération historique du cache KV

**État :** implémenté sur la branche de travail du lot 94.

## Objectif

Le lot 94 ajoute `gpt2_gguf_kv_cache_copy_history`, qui copie un intervalle de positions historiques d’une couche du cache KV vers deux buffers caller-owned, l’un pour les clés et l’autre pour les valeurs.

La copie est bornée par `cache->count`: une position non encore écrite ou un intervalle dépassant l’historique disponible est rejeté. Les couches restent isolées par le calcul d’offset déjà utilisé par `get`, et la fonction ne crée aucune mémoire interne.

## Contrat

| Condition | Résultat |
| --- | --- |
| intervalle `[start, start + count)` disponible | K/V copiés dans l’ordre |
| `position_count == 0` | succès avec `out_count = 0` |
| capacité K ou V insuffisante | `-6` |
| couche ou intervalle hors bornes | `-9` |
| pointeur de sortie ou compteur absent | `-1` |

Les sorties sont compactes: la position `start + i` est écrite à l’index `i` dans les buffers K/V. Cette disposition est directement exploitable par une boucle de score d’attention causale.

## Tests

La fixture écrit trois positions distinctes dans la couche 1, relit l’historique complet, vérifie l’ordre des valeurs et rejette une lecture sur la couche 0 non alimentée, une capacité K insuffisante et les pointeurs invalides. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain incrément pourra calculer les scores query-key sur l’historique copié, puis accumuler les valeurs pondérées pour l’attention autoregressive.
