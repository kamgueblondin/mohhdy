# AOS-1209 à AOS-1224 — registre socket TCP statique

> **État :** implémenté et validé localement. **Suite globale : 418/418 tests verts.**

## Objectif

Ce macro-lot introduit un registre de quatre sockets TCP statiques, indépendant de la session LLM. Il fournit un cycle de vie minimal pour les connexions caller-owned : ouverture, acceptation d’un SYN-ACK, émission d’un segment de données, alimentation de la file de réception depuis un segment TCP validé, lecture bornée et fermeture.

Aucune allocation dynamique n’est utilisée. Chaque slot contient une `net_tcp_connection_t` et une file RX fixe de 1024 octets. Les segments d’émission restent fournis par l’appelant ; le registre ne possède donc pas de buffer réseau global à durée indéterminée.

## Contrat

```c
int net_socket_open(uint16_t local_port, uint16_t remote_port,
                    uint32_t local_sequence);
int net_socket_accept_syn_ack(int socket_id, const net_tcp_view_t* view);
int net_socket_send(int socket_id, const uint8_t* payload, uint16_t length,
                    uint8_t* segment, uint16_t capacity, uint16_t* out_length);
int net_socket_feed(int socket_id, const uint8_t* segment, uint16_t length);
int net_socket_receive(int socket_id, uint8_t* buffer, uint16_t capacity,
                       uint16_t* out_length);
int net_socket_close(int socket_id);
```

`net_socket_send` délègue la construction et la progression de séquence à `net_tcp_connection_build_data` et `net_tcp_connection_commit_send`. `net_socket_feed` parse le segment et ne copie le payload qu’après acceptation de la séquence et de l’ACK par la primitive TCP existante. La fermeture libère le slot sans toucher aux autres connexions.

| Aspect | Valeur |
|---|---:|
| Slots simultanés | 4 |
| File RX par slot | 1024 octets |
| Taille TX maximale | 1500 octets |
| Allocation dynamique | Aucune |
| État transport | `SYN_SENT`, `ESTABLISHED` et états TCP existants |
| Tests noyau | 34/34 |
| Suite globale | 418/418 |

## Limites explicites

Ce lot ne réalise pas encore l’envoi matériel via la NIC, la résolution DNS, l’écoute passive ni les syscalls utilisateurs `socket/connect/send/recv/close`. Le registre fournit la couche de descripteurs et de buffers nécessaire à cette exposition ultérieure, sans coupler l’API générique au client TLS/LLM.

Le prochain incrément doit ajouter des structures POD de syscall, une validation des pointeurs utilisateur conforme aux conventions existantes, puis relier les opérations au dispatch syscall. Les chemins TLS/HTTP continueront d’utiliser leur session spécialisée jusqu’à leur migration contrôlée.

**Auteur :** Manus AI
