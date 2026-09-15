# AOS-1333 à AOS-1344 — fondation TCP d’écoute passive

## Objectif

Ce macro-lot ajoute la fondation déterministe de l’écoute TCP passive sans allocation dynamique. Une connexion caller-owned peut être placée dans l’état `LISTEN`, accepter un SYN entrant correspondant à son port local, produire un segment SYN-ACK, puis passer à `ESTABLISHED` après l’ACK final.

## Contrat livré

| Primitive | Contrat |
|---|---|
| `net_tcp_connection_listen` | Initialise une connexion avec port local, séquence initiale caller-owned et état `LISTEN`. |
| `net_tcp_connection_accept_syn` | Accepte uniquement un SYN entrant sans ACK, mémorise le quadruplet distant et passe à `SYN_RECEIVED`. |
| `net_tcp_connection_build_syn_ack` | Construit un segment SYN-ACK de 20 octets minimum avec les séquences validées. |
| `net_tcp_connection_accept_ack` | Accepte l’ACK final exact et passe de `SYN_RECEIVED` à `ESTABLISHED`. |

Les états `LISTEN` et `SYN_RECEIVED` sont ajoutés sans modifier la représentation existante de `net_tcp_connection_t`. Les contrôles rejettent les ports nuls, les ports de destination incohérents, les drapeaux SYN+ACK reçus au premier stade et les séquences ou acquittements incorrects.

## Limites

Le registre `net_socket` expose désormais `net_socket_listen`, `net_socket_accept_syn`, `net_socket_build_syn_ack` et `net_socket_accept_ack`. `net_socket_feed` route aussi un segment SYN vers un socket `LISTEN` et l’ACK final vers `SYN_RECEIVED`, ce qui permet au chemin `ne2k_rx_poll_tcp` de faire progresser le cycle passif. Les syscalls `SYS_SOCKET_LISTEN` (105), `SYS_SOCKET_ACCEPT_SYN` (106), `SYS_SOCKET_BUILD_SYN_ACK` (107) et `SYS_SOCKET_ACCEPT_ACK` (108) valident les vues et buffers Ring 3 par page avant d’appeler le registre. `net_tcp_build_syn_ack_ipv4` et `ne2k_tcp_syn_ack_via` construisent puis émettent le paquet IPv4 avec résolution ARP bornée, adresses Ethernet et TX NE2000.

## Validation

Le runner noyau passe **35/35 tests** et la suite complète passe **428/428 tests**. Le builder IPv4 SYN-ACK et l’émission NE2000 via passerelle sont couverts par les tests TCP/NE2000. Les tests vérifient aussi le routage d’un SYN puis d’un ACK final via `net_socket_feed`, chemin compatible avec les vues produites par `ne2k_rx_poll_tcp`. Les tests vérifient la séquence `LISTEN → SYN_RECEIVED → ESTABLISHED`, le décodage du SYN-ACK généré, le rejet d’un SYN accompagné à tort d’un ACK et le cycle équivalent dans le registre socket. Les wrappers syscall réutilisent la validation VMM page-par-page déjà appliquée aux six syscalls socket actifs.

## Mémoire

Aucune allocation dynamique n’est introduite. Les buffers de segment sont fournis par l’appelant et les transitions modifient uniquement la structure de connexion passée en argument.

## Auteur

Manus AI

## Références

Aucune source externe n’est nécessaire : ce document décrit les contrats et tests présents dans le dépôt MOHHDY.

[1]: ../kernel/net_tcp.h
[2]: ../kernel/net_tcp.c
[3]: ../tests/unit/kernel/test_net_tcp.c
