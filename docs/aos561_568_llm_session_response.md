# AOS-561 à AOS-568 — Polling et extraction de réponse LLM de session

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** session LLM, HTTP Content-Length, texte Ollama/OpenAI, TLS AES-GCM, NE2000

## Objectif

Ce macro-lot ferme le cycle non-streaming du contexte LLM. Après l’émission d’une requête, la nouvelle façade poll les records TLS, alimente l’accumulateur HTTP, reconnaît les bodies incomplets et extrait le texte Ollama ou OpenAI au moment où la réponse complète est disponible.

> Une réponse partielle est un progrès de transport et non un résultat LLM. Le contexte conserve donc `REQUEST_SENT` jusqu’à l’extraction fournisseur réussie.

## Phase et API ajoutées

| Phase | Valeur | Signification |
|---|---:|---|
| `NE2K_LLM_CONNECTION_REQUEST_SENT` | 4 | POST chiffré transmis ; la réponse HTTP est attendue. |
| `NE2K_LLM_CONNECTION_RESPONSE_READY` | 5 | Body HTTP complet et texte fournisseur extrait. |

| API | Rôle |
|---|---|
| `ne2k_llm_connection_poll_text` | Polling de réponse non-streaming Ollama/OpenAI depuis l’IPv4 de session. |

## Contrat transactionnel

La façade exige `REQUEST_SENT`. Elle crée des copies locales du contexte de phase, de la connexion TCP, du client TLS, de l’accumulateur HTTP et de la vue de réponse. Elle délègue ensuite à `ne2k_https_llm_poll_text`.

| Retour sous-jacent | Effet publié |
|---|---|
| Négatif | Aucun état TCP/TLS/HTTP/phase n’est publié ; les longueurs texte et consommation de sortie sont remises à zéro. |
| `1` | Les progrès du transport et de l’accumulateur HTTP sont publiés ; la phase reste `REQUEST_SENT`. |
| `0` | Le texte fournisseur est publié et la phase devient `RESPONSE_READY`. |

Les buffers RX/TX, plaintext, texte, accumulateur, vue HTTP, connexion TCP, client TLS et cache ARP restent caller-owned. Aucune allocation dynamique, copie cachée de token ou stockage global n’est ajouté.

## Tests et validation locale

Le test NE2000 ajouté appelle le polling avec une session `IDLE`. Il confirme le rejet avant toute dépendance réseau, HTTP ou cryptographique, et vérifie que les longueurs de texte et de consommation fournies par l’appelant sont préservées sur ce rejet de phase. Les tests existants couvrent l’accumulation `Content-Length`, le chiffrement TLS, les statuts HTTP et l’extraction texte des deux fournisseurs.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **376/376** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

Le lot couvre le chemin HTTP non-streaming. Le contexte ne possède pas encore de façade SSE dédiée, de boucle de conversation, de réutilisation de connexion, de timeout, de retry automatique, de stratégie `Retry-After`, de pagination, de tool calls, de multimodal ou de support Unicode complet. Les API SSE et retry existantes restent disponibles séparément.

## Références

[1] [AOS-385 à AOS-392 — requête LLM NE2000 unifiée](aos385_392_ne2k_llm_request.md)  
[2] [AOS-513 à AOS-520 — streaming LLM SSE sur HTTPS](aos513_520_llm_sse_streaming.md)  
[3] [AOS-553 à AOS-560 — émission LLM depuis une session TLS complète](aos553_560_llm_session_request.md)
