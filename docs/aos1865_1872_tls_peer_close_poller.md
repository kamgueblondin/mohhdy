# AOS-1865…1872 — Poller SSE et `close_notify` TLS distant

## Objet

Ce macro-lot raccorde une alerte TLS distante `warning/close_notify` au polling HTTP/SSE de l’agent LLM. Un pair qui ferme proprement le canal chiffré n’est plus traité comme une erreur de protocole ni comme une cause de reconnexion SSE : le record est acquitté, la séquence de lecture AES-GCM est publiée, puis la session LLM passe à l’état `RESPONSE_READY`.

| Composant | Responsabilité livrée |
|---|---|
| `net_http_tls.h` | Expose `NET_HTTP_TLS_STATUS_CLOSE_NOTIFY` (`2`) comme statut de polling explicite. |
| `net_http_tls.c` | Déchiffre le record puis reconnaît strictement l’alerte avec `net_tls_close_notify_parse()`. |
| `ne2k.c` | Acquitte le segment TCP, isole la transition de fermeture et place la session en `RESPONSE_READY`. |
| `ne2k.h` | Déclare la transition déterministe `ne2k_llm_connection_sse_peer_close_notify()`. |

## Sémantique transactionnelle

Après un déchiffrement AES-GCM valide, un record Alert contenant exclusivement `warning/close_notify` retourne le statut `2`. Les snapshots de connexion, de session AES-GCM et d’accumulateur ne sont pas restaurés dans ce cas : le numéro de séquence de lecture et l’acquittement TCP restent cohérents avec le record effectivement reçu. Toute autre alerte, tout type de contenu inattendu ou toute erreur de déchiffrement emprunte le rollback existant.

> La fermeture TLS propre est une information terminale du transport chiffré, pas un fragment HTTP/SSE ni un échec retryable.

| Situation après déchiffrement | Valeur de retour | Effet sur le contexte |
|---|---:|---|
| Données applicatives valides | `0` ou `1` | Le parseur HTTP/SSE poursuit son travail. |
| Alert `warning/close_notify` valide | `2` | Le segment est acquitté ; le tick SSE termine en `RESPONSE_READY`. |
| Alert différente ou record invalide | négatif | Rollback de l’état local et signalement d’erreur. |

La primitive `ne2k_llm_connection_sse_peer_close_notify()` ne touche ni à l’IP distante ni au budget de reprise. Elle effectue exclusivement la transition vers `NE2K_LLM_CONNECTION_RESPONSE_READY`, ce qui garantit l’absence de reconnexion automatique après une fermeture TLS distante ordonnée.

## Validation

Les vecteurs unitaires construisent côté serveur un record AES-GCM `close_notify`, l’encapsulent dans TCP, puis le soumettent aux deux décodeurs de flux. Ils contrôlent le statut `2`, la consommation des 31 octets TLS, l’avancement de la séquence de lecture et l’absence de mutation des accumulateurs HTTP/SSE. Le test NE2000 contrôle en outre la transition terminale et la conservation de l’adresse du pair.

| Suite | Résultat |
|---|---:|
| `make -s test-kernel` | 38/38 suites noyau réussies |
| Vecteurs ajoutés | HTTP stream, SSE stream, transition NE2000 |

Aucune allocation dynamique n’est introduite : tous les plaintexts, segments TCP, records TLS et états sont fournis par l’appelant.

## Références

[1] [RFC 5246 — TLS 1.2, §7.2.1](https://www.rfc-editor.org/rfc/rfc5246#section-7.2.1)
[2] [RFC 5246 — TLS 1.2, §6.2.1](https://www.rfc-editor.org/rfc/rfc5246#section-6.2.1)
