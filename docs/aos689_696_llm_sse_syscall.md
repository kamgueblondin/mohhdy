# AOS-689 à AOS-696 — Syscall de streaming SSE LLM contrôlé

**Auteur :** Manus AI

**Statut :** implémenté et validé localement

**Périmètre :** émission LLM `stream:true` et restitution de deltas SSE par syscall, sans exposition de buffers ni de secrets

## Objectif

Ce macro-lot complète la chaîne de contrôle LLM par le mode streaming. Une requête `os_llm_request_t` porte désormais le booléen borné `streaming`. Lorsqu’il vaut `1`, le noyau construit le JSON `stream:true`, initialise ses accumulateurs HTTP chunked/SSE fixes et émet la requête chiffrée via `ne2k_llm_connection_stream_request`.

`SYS_LLM_POLL_SSE` lit ensuite les fragments HTTPS reçus, déchiffre, décode HTTP chunked et SSE dans le noyau, puis recopie seulement un delta texte borné et le statut HTTP dans `os_llm_text_result_t`.

| Élément | Contrat |
|---|---|
| `SYS_LLM_REQUEST` | Le champ `streaming` sélectionne le POST normal (`0`) ou SSE (`1`). |
| `SYS_LLM_POLL_SSE` (`95`) | `EBX` pointe vers une sortie texte bornée, sans vue sur les buffers noyau. |
| Phases acceptées | `REQUEST_SENT` ou `STREAMING`, uniquement si la requête précédente était streaming. |
| Retour `1` | Aucun delta complet actuellement disponible ; le flux reste actif. |
| Retour `0` | Delta disponible ou terminaison SSE ; l’état devient `RESPONSE_READY` à la clôture. |

## Façade de session et publication transactionnelle

La nouvelle façade `ne2k_llm_connection_stream_request` reprend le contrat de la requête non streaming : elle exige `TLS_COMPLETE`, travaille avec des copies du contexte LLM, de la connexion TCP et du client TLS, puis publie `REQUEST_SENT` seulement après transmission réussie. Elle délègue à l’émetteur HTTPS existant, qui construit le corps JSON avec `stream:true`.

`kernel_llm_poll_sse` conserve le fournisseur, les accumulateurs HTTP/SSE et les buffers de plaintext dans le noyau. Il refuse toute invocation si le mode non streaming a été choisi ou si aucune requête n’est active. Les erreurs ne livrent aucun texte ni état TLS, et la façade réseau sous-jacente conserve son rollback transactionnel.

| Garde | Code | Effet |
|---|---:|---|
| Sortie nulle | `OS_LLM_SSE_BAD_ARGUMENT` | Aucune lecture. |
| Requête non streaming ou phase invalide | `OS_LLM_SSE_BAD_PHASE` | Aucun état de flux publié. |
| RX, TLS, HTTP ou SSE invalide | `OS_LLM_SSE_FAILED` | Aucun delta livré et contexte conservé. |
| Flux valide sans delta | `1` | Phase `STREAMING`, sortie vide. |
| Delta ou clôture valide | `0` | Texte extrait copié par valeur. |

## Mémoire et sécurité

Deux buffers statiques de 8 Kio sont ajoutés pour le décodage HTTP chunked et les événements SSE. Ils complètent les buffers JSON, HTTP, TLS et plaintext déjà privés au noyau. Aucun chemin n’alloue dynamiquement de mémoire ; l’ABI ne publie ni adresse, ni clé TLS, ni record, ni token, ni prompt persistant, ni identifiant fournisseur.

OpenAI reste refusé tant qu’un credential est absent du noyau. Le SSE ne contourne pas les prérequis TLS précédents : il demeure inaccessible tant que l’entropie cryptographique et l’ancre X.509 de production ne sont pas provisionnées pour terminer le handshake.

## Interface shell et tests

Le shell expose `ai-stream-request <ollama|openai> <modele> <chemin> <prompt>` et `ai-sse-poll`. Les commandes réutilisent les structures POD bornées ; elles ne demandent aucun secret. Le smoke QEMU vérifie qu’avant TLS authentifié la requête streaming est rejetée, que le polling SSE sans flux est rejeté et que la session demeure `IDLE` avec bail DHCP absent.

| Vérification | Résultat |
|---|---|
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU fournisseur | Réussi ; gardes streaming/SSE et état conservé. |
| Smoke QEMU NE2000 | Réussi ; carte prête, `IDLE`, bail absent. |
| Suite complète | Réussie : **384/384** tests. |
| `git diff --check` | Propre avant commit. |

## Limites connues

Le protocole SSE est contrôlable mais reste dépendant de l’achèvement sécurisé de TLS. Le provisionnement d’entropie, d’ancre X.509 et de credential OpenAI est toujours requis avant un appel réel. Le reset de session par syscall, la fermeture TCP/TLS, timeout/retry, historique de conversation, outils, multimodal et Unicode complet restent hors périmètre.

## Références

[1] [AOS-569 à AOS-576 — façade SSE du contexte LLM](aos569_576_llm_session_sse.md)

[2] [AOS-681 à AOS-688 — syscalls HTTP LLM et polling texte contrôlés](aos681_688_llm_http_text_syscalls.md)

[3] [AOS-673 à AOS-680 — syscall de polling TLS LLM contrôlé](aos673_680_llm_poll_tls_syscall.md)
