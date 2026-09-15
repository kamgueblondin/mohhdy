# AOS-1393 à AOS-1400 — SYN actif socket vers NE2000

## Objectif

Ce macro-lot complète le chemin actif TCP au-dessus de l’API socket statique. Un appelant peut ouvrir un socket avec ses ports et sa séquence initiale, faire construire son segment SYN caller-owned, puis confier exclusivement ce segment au pont NE2000 Ethernet/IPv4/ARP.

## Contrat livré

| Primitive | Responsabilité | État TCP privé exposé |
|---|---|---|
| `net_socket_open` | Réserve un slot statique et place son TCP interne en `SYN_SENT`. | Non |
| `net_socket_build_syn` | Construit le SYN avec la séquence initiale, sans copie ni mutation de buffers RX. | Non |
| `ne2k_socket_syn` | Encapsule et émet ce SYN par `ne2k_tcp_segment`. | Non |
| `ne2k_socket_poll_tcp` | Réinjecte ensuite le SYN-ACK entrant dans `net_socket_feed`. | Non |

La séquence initiale du SYN est reconstruite à partir de l’état post-ouverture, alors que le socket conserve la séquence suivante attendue pour vérifier le SYN-ACK. L’émission réutilise la résolution ARP et les checksums du pont TCP NE2000 existant. Toutes les trames et tous les segments sont des buffers caller-owned.

> Aucune structure `net_tcp_connection_t` n’est remise à l’appelant du nouveau chemin. Aucun `kmalloc`, `malloc`, `calloc` ou `realloc` n’est introduit.

## Validation

Les tests Unity vérifient les ports, la séquence initiale, le drapeau SYN et la garde de capacité de `net_socket_build_syn`. Un vecteur NE2000 couvre l’ouverture socket, l’ARP prérempli, l’encapsulation Ethernet/IPv4/TCP et le SYN effectivement transmis. La construction i386 est valide ; la suite noyau reste à **36/36** et la suite complète atteint **437/437**.

## Limites restantes

Le socket fournit à présent les phases actives SYN et SYN-ACK, ainsi que les bridges RX/TX. L’étape suivante consiste à extraire le handshake TLS actif de l’orchestrateur historique vers le registre socket, puis à relier cet état aux primitives HTTP et SSE déjà disponibles. La planification périodique DHCP et le raccordement de la commande `ai` restent également distincts.

## Références

[1]: aos1385_1392_ne2k_socket_rx_bridge.md "Pont de réception NE2000 vers sockets"
[2]: aos1373_1384_llm_socket_http_sse.md "Réception HTTP et SSE LLM sur sockets TLS"

[1] [2]
