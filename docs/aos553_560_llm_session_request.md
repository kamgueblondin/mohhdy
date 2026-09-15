# AOS-553 à AOS-560 — Émission LLM depuis une session TLS complète

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** session LLM, HTTP POST JSON, TLS AES-GCM, NE2000

## Objectif

Ce macro-lot raccorde l’émission d’une requête Ollama ou OpenAI au contexte de session LLM. La façade vérifie explicitement que le handshake authentifié est complet avant de déléguer au wrapper HTTPS déjà responsable du JSON fournisseur, du header Bearer OpenAI, du POST HTTP/1.1 et du chiffrement AES-GCM.

> La session ne passe à l’état applicatif qu’après la validation TLS. Le nouveau wrapper ne réduit pas les contrôles cryptographiques existants ; il les transforme en précondition de l’appel LLM.

## Phase et API ajoutées

| Phase | Valeur | Signification |
|---|---:|---|
| `NE2K_LLM_CONNECTION_TLS_COMPLETE` | 3 | Handshake terminé ; une requête applicative peut être émise. |
| `NE2K_LLM_CONNECTION_REQUEST_SENT` | 4 | Requête LLM chiffrée et transmise avec succès ; une réponse est attendue. |

| API | Rôle |
|---|---|
| `ne2k_llm_connection_request` | Émet une requête LLM HTTPS depuis l’IPv4 de session et publie `REQUEST_SENT` seulement au succès. |

## Contrat transactionnel

La façade vérifie la phase `TLS_COMPLETE`, puis travaille sur des copies locales du contexte de phase, de `net_tcp_connection_t` et de `ne2k_tls_client_t`. Elle délègue à `ne2k_https_llm_request`, qui construit le JSON spécifique au fournisseur, le POST HTTP et le record TLS chiffré.

| Situation | Résultat |
|---|---|
| Phase différente de `TLS_COMPLETE` | Rejet avant appel réseau, sans mutation. |
| Erreur JSON, Bearer, framing HTTP, AES-GCM, TCP ou TX | État de session, TCP et TLS externe inchangé. |
| Transmission réussie | Séquences TCP/TLS, contextes et phase publiés ; phase `REQUEST_SENT`. |

Aucune allocation dynamique n’est introduite. L’IPv4 résolue, les buffers TX, JSON, HTTP, record TLS, la connexion TCP, la session TLS et le cache ARP restent caller-owned. Les secrets Bearer restent passés directement par l’appelant et ne sont pas copiés dans le contexte de session.

## Tests et validation locale

Le harnais NE2000 étend la garde de phase de session. Il appelle la façade d’émission avec une session `IDLE` et des dépendances nulles, puis vérifie le rejet avant toute dérférence réseau ainsi que la conservation de la phase et de la séquence TCP. Le wrapper HTTPS sous-jacent conserve ses tests Ollama/OpenAI, JSON, Bearer, chiffrement AES-GCM et rollback.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **375/375** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

Le lot n’ouvre pas encore la réponse HTTP dans le contexte de session ; le polling de réponse, le texte provider, les statuts HTTP retryables et SSE restent délégués aux APIs existantes et à un prochain adaptateur de session. Les requêtes multi-tours, tool calls, multimodal, Unicode complet, annulation, temporisations, retries automatiques et secrets persistés restent hors périmètre.

## Références

[1] [AOS-385 à AOS-392 — requête LLM NE2000 unifiée](aos385_392_ne2k_llm_request.md)  
[2] [AOS-545 à AOS-552 — progression TLS authentifiée de session LLM](aos545_552_llm_tls_session_progress.md)  
[3] [AOS-321 à AOS-328 — Authorization Bearer caller-owned](aos321_328_http_authorization.md)
