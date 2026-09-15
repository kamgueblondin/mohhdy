# AOS-1461 à AOS-1472 — Orchestrateur noyau LLM sur session socket

## Objectif

Ce macro-lot raccorde enfin la surface noyau LLM au bootstrap et à la session socket statique. Les appels de contrôle passent du contexte historique contenant une `net_tcp_connection_t` privée à un bail DHCP et à `ne2k_llm_socket_session_t`. Le registre socket est désormais l’unique propriétaire persistant de l’état TCP du parcours DHCP/DNS/ARP/SYN, handshake TLS, requête HTTP, streaming SSE, réarmement et fermeture.

## Changements livrés

| Surface noyau | Comportement socket livré |
|---|---|
| `kernel_llm_acquire_start` | Utilise l’acquisition DHCP et le bootstrap DNS/ARP/SYN transactionnels vers un slot socket. |
| `kernel_llm_poll_tls` | Accepte le SYN-ACK, transmet ClientHello et exécute le polling TLS authentifié via l’identifiant de slot. |
| `kernel_llm_request` | Construit et transmet les requêtes Ollama/OpenAI normales ou streaming sur la session TLS du socket. |
| `kernel_llm_poll_text` | Alimente la réponse HTTP socket puis conserve les extracteurs Ollama/OpenAI et le contrôle du statut HTTP. |
| `kernel_llm_poll_sse` | Publie les deltas SSE par la session socket, avec les phases `REQUEST_SENT` et `STREAMING`. |
| `kernel_llm_reset_for_request` | Réarme `RESPONSE_READY → TLS_COMPLETE` sans toucher au slot établi. |
| `kernel_llm_close` | Émet un FIN+ACK socket lorsque la connexion est établie, restaure le slot si TX échoue, puis libère le slot. |

## Fermeture socket

`net_socket_begin_close` encapsule la primitive TCP `net_tcp_connection_begin_close` et publie `FIN_WAIT_1`. `ne2k_socket_fin` prend un snapshot de ce slot, encapsule le segment FIN+ACK dans Ethernet/IPv4 et le transmet. Une erreur d’encapsulation ou de transmission restaure le snapshot avant que la fermeture noyau ne libère le slot.

> Le noyau libère toujours le slot après l’effacement de session ; l’échec du FIN est signalé à l’appelant par `OS_LLM_CLOSE_FIN_FAILED`, sans conserver une capacité socket bloquée.

## Invariants

Le bail DHCP reste séparé des buffers et secrets TLS. La session socket ne conserve qu’un identifiant de slot, une IPv4 distante et une phase. Les clés TLS, les espaces de travail cryptographiques, les buffers HTTP/SSE et les identifiants OpenAI restent des buffers fixes du noyau. Aucune allocation dynamique n’est ajoutée.

Le polling HTTP conserve la réponse accumulée sur fragments, vérifie les codes 2xx puis appelle l’extracteur correspondant au fournisseur. Les séquences, nonces AEAD, accumulateurs et états SSE restent protégés par les rollbacks des façades socket/NE2000 sous-jacentes.

## Validation

Le vecteur `test_socket_begin_close_is_bounded` valide le rejet de la fermeture hors état établi, le paquet `FIN|ACK` et la transition vers `FIN_WAIT_1`. Le build i386 a aussi vérifié l’intégration de `kernel.c` avec les nouvelles interfaces.

| Contrôle | Résultat |
|---|---|
| `make all` | Réussi |
| `make test-all` | 444/444 tests verts |
| `git diff --check` | Propre |
| Recherche `kmalloc|malloc|calloc|realloc` | Aucune occurrence dans les chemins modifiés |

## Limites restantes

Le chemin est maintenant connecté dans le noyau mais attend encore une campagne QEMU avec un serveur TLS/LLM réellement joint au backend NE2000. Il reste aussi à planifier le renouvellement DHCP périodique, la réacquisition après expiration, les délais/backoff de session, la fermeture TLS `close_notify` et le provisioning opérationnel des identifiants OpenAI.

## Références

[1]: aos1449_1460_socket_bootstrap.md "Bootstrap DHCP/DNS/ARP/SYN vers session LLM socket"
[2]: aos1437_1448_llm_socket_session.md "Session LLM unifiée sur API socket"
[3]: aos1425_1436_socket_llm_orchestrator.md "Orchestrateur LLM HTTP/SSE actif sur socket"

[1] [2] [3]
