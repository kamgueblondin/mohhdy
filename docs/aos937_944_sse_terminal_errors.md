# AOS-937 à AOS-944 — classification des fins SSE/TCP et reprise retryable

Le chemin NE2000 expose une classification stable des résultats SSE : progression, fin normale, statut HTTP retryable, erreur de transport et erreur terminale. Cette distinction évite de relancer un flux après `[DONE]` ou après une erreur d’authentification, tout en permettant au scheduler de réutiliser `retry:` pour les statuts 408, 425, 429, 500, 502, 503 et 504.

`ne2k_llm_connection_handle_sse_terminal` conserve le modèle caller-owned. Une panne de transport est représentée par le statut synthétique 503 afin de réutiliser le même budget et le même backoff. Une planification réussie passe la connexion en `TLS_COMPLETE`; une fin normale passe en `RESPONSE_READY`; une erreur terminale retourne un rejet sans mutation de la reconnexion.

Les tests couvrent les cinq classes de résultat. Les timers IRQ et le jitter restent des extensions indépendantes : l’instant `now` est toujours fourni par l’appelant et aucune attente active n’est réalisée.

Auteur : **Manus AI**
