# AOS-537 à AOS-544 — Orchestrateur de connexion LLM DNS vers ClientHello

**Auteur :** Manus AI  
**Statut :** implémenté et validé localement  
**Périmètre :** DNS A, ARP, SYN, SYN-ACK, ClientHello TLS, NE2000

## Objectif

Ce macro-lot rassemble les deux transitions de préconnexion déjà livrées dans un contexte léger, explicite et fourni par l’appelant. Le nouveau contexte garde uniquement l’IPv4 résolue et une phase de progression ; il ne stocke aucun hostname, secret, token ou buffer réseau.

> L’établissement TCP s’appuie sur le three-way handshake défini par TCP ; ce lot rend explicite la progression locale entre le SYN émis, le SYN-ACK reçu et le premier segment ClientHello [1].

## Contexte et API

```c
typedef struct {
    uint8_t remote_ip[4];
    uint8_t phase;
} ne2k_llm_connection_state_t;
```

| Phase | Valeur | Signification |
|---|---:|---|
| `NE2K_LLM_CONNECTION_IDLE` | 0 | Aucun bootstrap n’a été publié. |
| `NE2K_LLM_CONNECTION_SYN_SENT` | 1 | DNS A, ARP distant et SYN ont réussi ; l’IPv4 est publiée. |
| `NE2K_LLM_CONNECTION_TLS_STARTED` | 2 | SYN-ACK validé et ClientHello TLS émis. |

| API | Rôle |
|---|---|
| `ne2k_llm_connection_state_init` | Réinitialise l’IPv4 et la phase à `IDLE`. |
| `ne2k_llm_connection_start` | Délègue au bootstrap DNS→ARP→SYN et publie `SYN_SENT` seulement au succès. |
| `ne2k_llm_connection_poll_tls_start` | Délègue au polling SYN-ACK→ClientHello et publie `TLS_STARTED` seulement au succès. |

## Garanties transactionnelles

Les deux appels manipulent des copies locales du contexte de phase, de la connexion TCP et, pour le polling TLS, du client TLS. Une erreur de DNS, ARP, SYN, réception TCP, validation SYN-ACK ou émission ClientHello ne publie aucun état partiel. Un polling RX vide retourne `1`, sans progression de phase ni émission.

| Opération | Précondition | Succès | Échec |
|---|---|---|---|
| `connection_start` | Phase `IDLE` | IPv4 + TCP publiés, phase `SYN_SENT` | Phase, IPv4 et TCP inchangés. |
| `poll_tls_start` | Phase `SYN_SENT` | TCP + TLS publiés, phase `TLS_STARTED` | Phase, TCP et TLS inchangés. |
| Réentrée | Phase incompatible | Rejet | Aucun effet de bord. |

Aucune allocation dynamique n’est introduite. Les buffers ARP, Ethernet, RX, TX, ClientHello, les contextes TCP/TLS et le cache ARP restent caller-owned.

## Tests et validation locale

Le test NE2000 vérifie l’initialisation, la remise à zéro de l’IPv4, le rejet d’un démarrage invalide sans mutation de TCP, le rejet de polling hors phase, le rejet d’une réentrée et le refus d’un pointeur de contexte nul. Les lots précédents continuent de couvrir les transitions SYN-ACK→ClientHello, la construction DNS, ARP et SYN.

| Vérification | Résultat |
|---|---|
| Suite noyau | **32/32** exécutables réussis. |
| Suite complète | **374/374** tests réussis. |
| Compilation freestanding i386 | Réussie. |
| Smoke QEMU `qemu-ai-provider` | Réussi. |
| Smoke QEMU `qemu-ne2k-status` | Réussi. |

## Limites connues

L’orchestrateur ne rend pas DNS asynchrone et ne possède pas de timer, timeout, retransmission SYN, backoff, DNS AAAA/CNAME/DNSSEC, cache DNS, sélection multi-adresses, DHCP ou reprise de connexion. Il ne traite pas le reste du handshake TLS : l’appelant poursuit avec les pollings TLS authentifiés existants dès la phase `TLS_STARTED`.

## Références

[1] [RFC 793 — Transmission Control Protocol](https://datatracker.ietf.org/doc/html/rfc793)  
[2] [AOS-521 à AOS-528 — bootstrap DNS, ARP et SYN](aos521_528_llm_dns_syn_bootstrap.md)  
[3] [AOS-529 à AOS-536 — SYN-ACK et ClientHello](aos529_536_llm_synack_tls_bootstrap.md)
