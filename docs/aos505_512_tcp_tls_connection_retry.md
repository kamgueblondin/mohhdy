# AOS-505 à AOS-512 — Récupération TCP/TLS avec retry borné

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** connexion TCP, état TLS NE2000, reprise caller-owned

## Objectif

Ce macro-lot introduit un mécanisme explicite et borné de reprise de connexion pour les erreurs TCP/TLS. Il sépare trois notions auparavant voisines mais distinctes : la retransmission d’un payload TCP en attente d’ACK, la réémission HTTP guidée par un code de statut, et la **réouverture complète d’une connexion** après un échec de transport ou de handshake.

La reprise ne réalise aucune allocation dynamique. L’appelant possède le budget, la connexion, le client TLS et tous les buffers de réassemblage ou de cryptographie.

## API et sémantique

| API | Rôle | Valeur de retour |
|---|---|---|
| `net_tcp_connection_retry_init` | Initialise un budget de tentatives caller-owned. | `0` si succès. |
| `net_tcp_connection_retry_consume` | Consomme une tentative sans agir sur le transport. | `1` autorisée, `0` épuisée, négatif invalide. |
| `net_tcp_connection_retry_reopen` | Consomme une tentative puis replace la connexion en `SYN_SENT`. | `1` réouverte, `0` épuisée, négatif erreur. |
| `ne2k_tls_client_retry_reset` | Réouvre TCP et purge l’état TLS dérivé avant un nouveau handshake. | `1` réinitialisé, `0` épuisé, négatif erreur. |

Une réouverture conserve les ports locaux et distants, mais reçoit une nouvelle valeur de séquence initiale fournie par l’appelant. `net_tcp_connection_open` reconstruit alors un état `SYN_SENT`, efface les données en attente, les métadonnées de retransmission, la séquence distante et remet la fenêtre de réception au contrat initial.

> Un budget épuisé n’est pas une erreur silencieuse : la primitive retourne `0` et laisse la connexion ainsi que le client TLS intacts. L’appelant conserve la responsabilité de déclarer l’échec final ou de changer de stratégie.

## Reset transactionnel TLS

La façade NE2000 travaille sur des copies locales de `net_tcp_connection_t` et `ne2k_tls_client_t`. Après une reprise TCP autorisée, elle replace le handshake dans l’état `IDLE`, vide les accumulateurs de record et de handshake, remet la longueur du transcript à zéro, efface le secret maître, le bloc de clés AES-GCM, le contexte X25519 et les séquences AEAD. Elle annule également les indicateurs `peer_identity_validated` et `complete`.

| Élément réinitialisé | Justification |
|---|---|
| État TCP et payload pending | Aucun octet de la connexion échouée ne doit être retransmis sur la nouvelle connexion. |
| Accumulateurs TLS et transcript | Les messages serveur partiels ne peuvent jamais être mélangés avec un nouveau `ClientHello`. |
| Secret maître, key block, X25519 | Toute matière cryptographique dérivée de l’échange interrompu est invalidée. |
| Session AES-GCM et compteurs | Les clés et numéros de séquence ne survivent pas à un nouveau handshake. |
| Drapeaux d’identité et complétion | Une identité ne reste pas validée après rupture du canal qui l’a authentifiée. |

Les pointeurs et capacités des buffers caller-owned ne sont pas modifiés. La routine n’envoie pas elle-même le SYN ; après le retour `1`, l’orchestrateur peut émettre une nouvelle ouverture TCP puis repartir avec `ne2k_tls_client_start` lorsque le SYN-ACK est accepté.

## Tests et validation locale

Le test TCP couvre l’initialisation du budget, la consommation, la réouverture avec une nouvelle séquence, la suppression du payload pending et l’absence de mutation après épuisement. Le test NE2000 prépare un client TLS partiellement dérivé, déclenche le reset, puis vérifie l’effacement des accumulateurs, secret, X25519, session et indicateurs. Il confirme ensuite qu’une seconde reprise est refusée sans modifier la nouvelle séquence.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **369/369** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

Ce lot ne programme pas de temporisation réelle, backoff exponentiel, jitter, DNS/ARP/SYN automatisé, basculement de fournisseur LLM, conservation de session TLS ou reprise 0-RTT. Il ne déclenche pas de SYN directement et ne classe pas les causes réseau ; l’orchestrateur reste responsable de décider quand appeler le reset et avec quelle nouvelle séquence. Les retransmissions TCP de payload et le budget HTTP LLM restent des mécanismes séparés.

## Références

[1] [AOS-337 à AOS-344 — retransmission TCP bornée](aos337_344_tcp_retransmission.md)  
[2] [AOS-393 à AOS-400 — erreurs HTTP LLM et retry borné](aos393_400_llm_http_retry.md)
