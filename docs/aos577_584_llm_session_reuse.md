# AOS-577 à AOS-584 — Réutilisation d’une session TLS LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** machine d’état LLM, réemploi de TCP/TLS, HTTP/SSE caller-owned

## Objectif

Ce macro-lot permet d’enchaîner une nouvelle requête LLM après la réception complète de la précédente, sans recréer la connexion TCP ni renégocier TLS 1.2. La façade `ne2k_llm_connection_reset_for_request` ramène le contexte terminal à `TLS_COMPLETE`, état déjà exigé par la façade d’émission chiffrée.

> Le réarmement est volontairement limité à la machine d’état LLM. La session TLS AES-GCM active, les compteurs de séquence, la connexion TCP et l’adresse IPv4 résolue restent inchangés.

## API et contrat

| API | Précondition | Effet au succès | Refus |
|---|---|---|---|
| `ne2k_llm_connection_reset_for_request` | `state->phase == RESPONSE_READY` | `state->phase = TLS_COMPLETE` | `NULL` ou toute autre phase. |

La transition n’est admise qu’après une réponse non-streaming complète ou la fermeture normale d’un flux SSE. Elle refuse donc explicitement une session `REQUEST_SENT` ou `STREAMING`, où une réponse reste en cours. Une seconde invocation immédiatement après succès est également rejetée : la session est déjà prête à émettre et non plus terminale.

## Propriété de non-destruction

L’implémentation travaille sur une copie locale du seul contexte LLM et la publie en une affectation. Aucun argument TCP, TLS, HTTP ou SSE n’est nécessaire ; aucune allocation dynamique n’est réalisée.

| État | Traitement |
|---|---|
| IPv4 distante | Conservée. |
| Connexion TCP et numéros de séquence | Non modifiés. |
| Session AES-GCM, clés et transcript | Non modifiés. |
| Accumulateur HTTP `Content-Length` | Géré par l’appelant : réinitialisation explicite avant le polling suivant. |
| Accumulateur HTTP chunked/SSE | Géré par l’appelant : réinitialisation explicite avec ses buffers existants avant le polling suivant. |

Cette séparation évite de cacher des buffers ou de supposer une topologie mémoire. L’appelant contrôle le cycle de vie des accumulations de réponse et peut sélectionner, à chaque nouveau tour, la voie non-streaming ou SSE adéquate.

## Tests et validation locale

Le test NE2000 dédié initialise une IPv4 de session, vérifie que la transition est refusée depuis `IDLE`, puis confirme que `RESPONSE_READY → TLS_COMPLETE` conserve tous les octets de l’IPv4. Il vérifie enfin le refus de la seconde transition et du pointeur nul.

| Vérification | Résultat |
|---|---|
| Test NE2000 ciblé | **15/15** réussis. |
| Suite complète | **378/378** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

Le réarmement n’implémente ni stratégie de connexion persistante HTTP/1.1 au niveau du header, ni pipeline de requêtes, ni timeout, ni retry/backoff automatique, ni fermeture propre TCP/TLS, ni historique conversationnel, ni tool calls, ni multimodal, ni support Unicode complet. Il offre uniquement le point de reprise sûr de la machine d’état pour la prochaine émission LLM sur la session cryptographique conservée.

## Références

[1] [AOS-553 à AOS-560 — émission LLM depuis une session TLS complète](aos553_560_llm_session_request.md)  
[2] [AOS-561 à AOS-568 — polling et extraction de réponse LLM de session](aos561_568_llm_session_response.md)  
[3] [AOS-569 à AOS-576 — façade SSE du contexte LLM](aos569_576_llm_session_sse.md)
