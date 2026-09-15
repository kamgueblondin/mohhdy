# AOS-881 à AOS-888 — scheduler de reconnexion SSE

## Objectif

Ce macro-lot fournit un état de reconnexion SSE caller-owned. Il conserve le nombre de tentatives, le plafond autorisé, le prochain instant d’essai et l’état planifié. La planification délègue au backoff HTTP borné existant, réinitialise transactionnellement les accumulateurs HTTP chunked et SSE, puis expose une deadline abstraite que l’appelant peut comparer à son horloge ou à ses ticks IRQ.

Le scheduler ne bloque jamais, ne démarre aucun timer global et ne réémet pas automatiquement de segment TCP. Les statuts non-retryables et l’épuisement du budget sont retournés sans modifier les accumulateurs. La reconnexion effective reste donc sous le contrôle du poller réseau, ce qui évite de mélanger l’état TLS, TCP et applicatif.

| Élément | Garantie |
|---|---|
| État | Structure caller-owned, sans état global |
| Budget | Réutilise le plafond retry existant |
| Délai | Backoff borné et deadline monotone fournie par l’appelant |
| Reset | Buffers conservés, longueurs HTTP/SSE remises à zéro |
| Blocage | Aucun sleep, spin ou timer implicite |
| Mémoire | Aucun pointeur caché ni `kmalloc` |

## Validation

Le test vérifie une première reprise à `now + 10`, l’état non prêt à 109 ticks, l’état prêt à 110 ticks, une deuxième reprise à 130 ticks et l’épuisement du budget. La suite HTTP/TLS ciblée passe à **21/21 tests**.

## Limites restantes

L’appelant doit encore relancer explicitement la connexion TCP/TLS lorsque `net_llm_sse_reconnect_ready` retourne vrai. La persistance de `Last-Event-ID`, le jitter, les timers IRQ intégrés au poller NE2000, la reprise au milieu d’un événement et la reconnexion automatique multi-fournisseur restent des lots distincts.
