# AOS-1801…1808 — Jitter SSE dans le contexte réseau LLM

## Objet

Ce macro-lot expose `ne2k_llm_network_context_sse_schedule_jittered()`, une façade qui programme un retry SSE jitteré directement dans le contexte réseau LLM. Elle réutilise le scheduler borné existant et publie ensemble la phase de session, le scheduler, la réponse SSE, le checkpoint de reprise et la graine pseudo-aléatoire.

| Élément | Garantie |
|---|---|
| Fenêtre de jitter | Délai strictement borné autour du backoff de base |
| Scheduler | Une seule tentative est consommée après statut retryable |
| Contexte | Passe à `TLS_COMPLETE` uniquement après programmation réussie |
| Checkpoint | Conserve fournisseur, budget et `Last-Event-ID` |
| Graine | Avancée seulement après succès |
| Échec | Aucun objet appelant n’est publié |

## Transactionnalité

La façade utilise des copies locales du contexte, du scheduler, de la réponse SSE et de la graine. Elle délègue le calcul à `net_llm_sse_reconnect_schedule_jittered()`, qui applique les bornes du délai et réinitialise l’accumulateur SSE tout en préservant l’identifiant de reprise. Le checkpoint est sauvegardé avant toute publication.

> Le jitter désynchronise les reconnexions de clients sans introduire de stockage dynamique, de thread, de secret ou de nouvel état réseau caché.

## Tests

Le vecteur NE2000 vérifie un retry HTTP 503 avec une base de 10 ticks, une fenêtre de jitter de 5 ticks et un instant courant de 100. Il impose une échéance comprise entre 105 et 115, la transition vers `TLS_COMPLETE`, la persistance de `evt`, l’avancement de la graine et le rejet transactionnel d’une base nulle.

| Vérification | Résultat |
|---|---|
| Fenêtre `[105, 115]` | Validée |
| Transition de session | Validée |
| Checkpoint du fournisseur et de `Last-Event-ID` | Validé |
| Avancement de la graine | Validé |
| Base de délai nulle | Rejet transactionnel |
| Test NE2000 ciblé | **40/40 réussis** |

## Références

[1] [AOS-945 à AOS-952 — jitter SSE borné](aos945_952_sse_bounded_jitter.md)  
[2] [AOS-1785 à AOS-1792 — tick SSE actif de contexte](aos1785_1792_sse_context_event_tick.md)  
[3] [AOS-1793 à AOS-1800 — rotation de fournisseur](aos1793_1800_sse_provider_rotation_context.md)  
[4] [WHATWG — Server-Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html)
