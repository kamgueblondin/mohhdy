# MOHHDY Foundation — Incrément 95 : scores query-key

**État :** implémenté sur la branche de travail du lot 95.

## Objectif

Le lot 95 ajoute `gpt2_gguf_kv_cache_query_scores`, qui calcule les produits scalaires entre une query caller-owned et les clés historiques d’une couche du cache KV.

La primitive retourne les scores bruts, dans l’ordre compact de l’intervalle demandé. Elle ne réalise volontairement ni mise à l’échelle par `sqrt(head_size)`, ni softmax: ces opérations appartiennent à l’étape d’attention suivante. Le segment K est lu directement depuis la disposition contrôlée du cache, avec un scratch caller-owned réutilisé à chaque position.

## Contrat

| Condition | Résultat |
| --- | --- |
| query et historique valides | scores `dot(query, key[position])` |
| scratch K ou tableau scores insuffisant | `-6` |
| couche ou intervalle hors historique | `-9` |
| pointeur requis absent | `-1` |
| intervalle vide | succès avec `out_count = 0` |

Les scores sont calculés en float et écrits dans `scores[0 ... position_count-1]`. La fonction respecte le cache caller-owned et ne modifie ni les clés ni les valeurs stockées.

## Tests

La fixture contient les clés `[1,2,3,4]`, `[9,2,3,4]` et `[1,2,3,4]`. Une query de quatre composantes unitaires produit les scores `10`, `18` et `10`. Les tests couvrent également le scratch trop petit et un intervalle dépassant `cache->count`. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le prochain incrément pourra appliquer la normalisation de tête puis accumuler les valeurs historiques pondérées par les probabilités d’attention.
