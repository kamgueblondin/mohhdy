# AOS-921 à AOS-928 — adaptateur temporel du scheduler SSE NE2000

Le contexte de connexion NE2000 expose désormais deux primitives caller-owned. `ne2k_llm_connection_schedule_sse_retry` valide la phase active, délègue le statut HTTP et le hint `retry` au scheduler SSE borné, copie l’état uniquement après succès et replace la session en `TLS_COMPLETE`. Cette phase permet ensuite d’émettre le GET de reprise avec `Last-Event-ID`.

`ne2k_llm_connection_sse_retry_ready` est non bloquant : il rejette les phases qui ne sont pas prêtes et retourne le résultat du scheduler à partir du tick fourni par l’appelant. Aucun IRQ, sommeil, timer global ou `kmalloc` n’est utilisé.

Le test couvre le délai de 1 500 ms, le tick avant et après échéance, ainsi que le rejet transactionnel d’une phase `IDLE`. Le prochain axe pourra brancher ces primitives dans la boucle d’événements matérielle qui fournit le tick système.

Auteur : **Manus AI**
