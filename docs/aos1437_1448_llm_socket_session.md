# AOS-1437 à AOS-1448 — Session LLM unifiée sur API socket

## Objectif

Ce macro-lot introduit un contexte caller-owned de session LLM fondé sur un identifiant de socket. Il rassemble les transitions depuis un SYN socket déjà transmis vers le ClientHello, le polling TLS authentifié, l’émission LLM, le polling HTTP/SSE et le réarmement d’un nouveau tour. La session ne conserve aucune copie de connexion TCP, de clé, de secret, de hostname ni de buffer applicatif.

> `ne2k_llm_socket_session_t` contient uniquement une adresse IPv4 distante, une phase et un identifiant de slot. Le registre socket statique reste l’unique autorité sur l’état et les séquences TCP.

## Contrat livré

| Primitive | Précondition | Publication après succès |
|---|---|---|
| `ne2k_llm_socket_session_init` | Pointeur valide. | Session `IDLE`, identifiant socket à `-1`. |
| `ne2k_llm_socket_session_attach` | Slot en `SYN_SENT`, session `IDLE`. | IPv4 distante, slot et phase `SYN_SENT`. |
| `ne2k_llm_socket_session_poll_tls_start` | `SYN_SENT`. | Après SYN-ACK valide et ClientHello transmis, phase `TLS_STARTED`. |
| `ne2k_llm_socket_session_poll_tls` | `TLS_STARTED`. | Phase `TLS_COMPLETE` seulement après Finished serveur effectivement validé. |
| `ne2k_llm_socket_session_request` | `TLS_COMPLETE`. | Phase `REQUEST_SENT` après transmission LLM réussie. |
| `ne2k_llm_socket_session_poll_response` | `REQUEST_SENT`. | Phase `RESPONSE_READY` seulement pour une réponse HTTP complète. |
| `ne2k_llm_socket_session_poll_sse` | `REQUEST_SENT` ou `STREAMING`. | `STREAMING` pendant le flux, puis `RESPONSE_READY` à sa terminaison. |
| `ne2k_llm_socket_session_reset_for_request` | `RESPONSE_READY`. | Phase `TLS_COMPLETE`, socket et session AEAD inchangés. |

## Comportement transactionnel

Chaque façade vérifie d’abord sa phase et l’existence de l’identifiant socket. Les transitions n’écrivent l’état de session qu’après le retour non négatif de la primitive inférieure. Pour le démarrage TLS et le polling TLS, la session et le client TLS sont snapshottés avant l’appel ; une erreur restaure les deux objets. Les façades HTTP et SSE actives maintiennent leurs propres snapshots du slot TCP, de la session AEAD et des accumulateurs, puis la couche de session refuse de publier toute nouvelle phase si elles échouent.

Une réception NE2000 vide conserve les phases `SYN_SENT`, `TLS_STARTED` ou `REQUEST_SENT` quand le chemin inférieur retourne `1`. Dans le cas SSE, un progrès non terminal publie explicitement `STREAMING`, sans masquer le caractère non bloquant de l’opération.

## Périmètre de l’unification

La session commence volontairement après l’émission du SYN. L’acquisition DHCP, la résolution DNS, le choix du prochain saut et la création socket restent actuellement dans les façades réseau précédentes, qui demeurent compatibles. Cette frontière évite de dupliquer les contrats DHCP/DNS déjà validés tout en supprimant, du SYN-ACK à la réponse applicative, l’exposition d’une connexion TCP caller-owned.

## Validation

Le vecteur Unity associé contrôle l’initialisation, le rejet d’un attachement invalide, l’attachement au slot `SYN_SENT`, la garde de requête avant TLS, l’émission réussie depuis `TLS_COMPLETE`, le polling HTTP vide, le passage SSE à `STREAMING` et le réarmement `RESPONSE_READY → TLS_COMPLETE`.

| Contrôle | Résultat |
|---|---|
| Compilation i386 `make all` | Réussie |
| Suite complète `make test-all` | 442/442 tests verts |
| `git diff --check` | Propre |
| Recherche d’allocation dynamique dans les chemins modifiés | Aucune occurrence |

## Limites restantes

Le prochain incrément doit déplacer également le bootstrap DHCP/DNS/ARP/SYN vers un contexte socket unique, en créant le slot et le SYN après la résolution d’hôte puis en l’attachant transactionnellement à cette session. La planification périodique du renouvellement DHCP, les timeouts/backoff, la fermeture TLS `close_notify`, le provisionnement sécurisé d’identifiants OpenAI et la mise à disposition de l’ensemble au shell restent à compléter.

## Références

[1]: aos1425_1436_socket_llm_orchestrator.md "Orchestrateur LLM HTTP/SSE actif sur socket"
[2]: aos1413_1424_socket_tls_poll.md "Polling TLS authentifié sur API socket"
[3]: aos1401_1412_socket_tls_clienthello.md "ClientHello TLS actif sur socket"

[1] [2] [3]
