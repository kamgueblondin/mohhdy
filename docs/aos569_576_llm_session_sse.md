# AOS-569 à AOS-576 — Façade SSE du contexte LLM

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** SSE HTTP chunked, TLS AES-GCM, NE2000, contexte LLM caller-owned

## Objectif

Ce macro-lot ajoute le chemin de streaming au contexte `ne2k_llm_connection_state_t`. Après l’envoi d’une requête LLM configurée pour le streaming, `ne2k_llm_connection_poll_sse` effectue un polling borné de la carte NE2000, déchiffre les records TLS, délègue le décodage HTTP chunked/SSE existant et remet au demandeur le texte extrait à chaque progression.

> Le flux SSE est non bloquant : l’absence de trame retourne `1`, publie des longueurs de sortie nulles et permet à l’appelant de reprendre la main sans attendre le fournisseur.

## API et phases

| Élément | Valeur ou contrat |
|---|---|
| `NE2K_LLM_CONNECTION_STREAMING` | `6` ; flux SSE activé et non terminé. |
| `NE2K_LLM_CONNECTION_RESPONSE_READY` | `5` ; le décodeur SSE a reconnu la terminaison du flux. |
| `ne2k_llm_connection_poll_sse` | Accepte `REQUEST_SENT` ou `STREAMING`, délègue à `ne2k_https_llm_poll_sse` et publie les résultats caller-owned. |

La transition de `REQUEST_SENT` vers `STREAMING` intervient à la première invocation SSE réussie, y compris lorsqu’aucune trame n’est encore disponible. Cette information exprime le mode de réception sélectionné, plutôt qu’une promesse de contenu déjà reçu. Tout appel depuis `IDLE`, `TLS_COMPLETE` ou `RESPONSE_READY` est rejeté avant toute opération réseau.

## Publication transactionnelle

La façade travaille sur des copies locales du contexte de phase, de la connexion TCP, du client TLS et de l’accumulateur SSE. Ainsi, un échec de réception, de déchiffrement, de parsing HTTP/SSE ou d’ACK ne modifie pas les états que possède l’appelant.

| Retour du délégué | Publication |
|---|---|
| Négatif | Aucun contexte TCP/TLS/SSE ni phase n’est publié ; `text_length` et `consumed` deviennent zéro. |
| `1` | Les éventuels progrès de transport/décodage sont publiés ; la phase devient ou reste `STREAMING`. |
| `0` | Le dernier fragment est publié ; la phase devient `RESPONSE_READY`. |

Aucun buffer interne persistant ni allocation dynamique n’est introduit. Les trames RX/TX, plaintext, accumulateur HTTP/SSE, texte extrait, connexion TCP, client TLS et cache ARP restent fournis et possédés par l’appelant.

## Tests et validation locale

Le test NE2000 dédié vérifie les trois propriétés de contrat : le rejet hors phase conserve les sentinelles de sortie ; un polling sans trame retourne immédiatement `1` ; ce polling non bloquant passe de `REQUEST_SENT` à `STREAMING` avec `text_length = 0` et `consumed = 0`. Il vérifie aussi que `RESPONSE_READY` ne peut pas relancer un flux.

| Vérification | Résultat |
|---|---|
| Test NE2000 ciblé | **14/14** réussis. |
| Suite complète | **377/377** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

Le lot constitue une façade de réception. Le builder de requête de session conserve le paramètre JSON de streaming fourni par les couches existantes. La réinitialisation de session pour un nouveau tour, les timeouts, le retry automatique, la fermeture TCP/TLS explicite, la conservation d’historique conversationnel, les tool calls, le multimodal et le support Unicode complet restent hors périmètre.

## Références

[1] [AOS-513 à AOS-520 — streaming LLM SSE sur HTTPS](aos513_520_llm_sse_streaming.md)  
[2] [AOS-553 à AOS-560 — émission LLM depuis une session TLS complète](aos553_560_llm_session_request.md)  
[3] [AOS-561 à AOS-568 — polling et extraction de réponse LLM de session](aos561_568_llm_session_response.md)
