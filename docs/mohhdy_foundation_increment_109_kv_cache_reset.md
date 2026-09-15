# MOHHDY Foundation — Incrément 109 : reset O(1) du cache KV

**État :** implémenté et testé.

## Objectif

Le lot 109 ajoute `gpt2_gguf_kv_cache_reset`. Le cache KV peut être réarmé en remettant uniquement son compteur de positions à zéro, sans effacer le stockage caller-owned. Cette opération évite un coût proportionnel à la taille du cache lors du changement de séquence autoregressive.

## Contrat

La primitive exige un pointeur de cache non nul, remet `count` à zéro et renvoie `0`. Elle renvoie `-1` pour un pointeur nul. Les valeurs résiduelles dans le stockage ne sont pas promises comme effacées; elles deviennent simplement inaccessibles tant qu’une nouvelle séquence ne les réécrit pas.

## Validation

Le test remet à zéro le cache d’intégration, vérifie `count == 0`, puis contrôle le rejet du pointeur nul. `make test-all` passe avec **265 tests réussis, 0 échec et 0 test ignoré**.

## Suite

Le reset peut être utilisé par le runtime GGUF pour réutiliser un cache entre prompts sans allocation ni memset coûteux.
