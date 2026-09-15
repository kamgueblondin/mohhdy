# AOS-1385 à AOS-1392 — Pont de réception NE2000 vers sockets

## Objectif

Ce macro-lot relie la réception matérielle NE2000 au registre socket statique. Une trame Ethernet/IPv4/TCP extraite dans un buffer caller-owned peut désormais être injectée directement dans un socket, sans fournir au pilote une `net_tcp_connection_t` privée.

## Contrat livré

| Primitive | Responsabilité |
|---|---|
| `ne2k_socket_poll_tcp` | Lit au plus une trame NE2000, valide Ethernet/IPv4/TCP par le chemin existant et passe le segment TCP au socket désigné. |
| `net_socket_feed` | Conserve la responsabilité des transitions SYN, ACK, données et de la file RX statique. |

Le pont recalcule l’offset du segment TCP depuis le champ IHL IPv4 de la trame reçue. Le pilote ne copie ni ne conserve le payload ; le registre socket applique ses validations et ses limites RX. Une absence de contexte matériel est rejetée avant toute opération d’E/S.

> L’adaptateur ne crée aucun socket, aucune tâche et aucune allocation dynamique. Les buffers RX restent propriété de l’appelant du polling NE2000.

## Validation

Le test NE2000 couvre la garde de contexte du polling socket. Les harness local et CI ont été synchronisés pour lier `net_socket.c` et son graphe cryptographique au test NE2000 sans sources dupliquées. La compilation i386 réussit ; la suite noyau passe **36/36 tests** et la suite complète passe **435/435 tests**.

## Limites restantes

Ce macro-lot fournit l’injection de trame. L’orchestrateur LLM doit encore planifier les étapes actives DNS, SYN, handshake TLS, émission et polling, puis relier ce chemin à la commande `ai` avec une configuration de fournisseur et de secret contrôlée. La planification périodique du bail DHCP demeure séparée.

## Références

[1]: aos1373_1384_llm_socket_http_sse.md "Réception HTTP et SSE LLM sur sockets TLS"
[2]: todo.md "Backlog MOHHDY"

[1] [2]
