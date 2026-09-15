# AOS-1345 à AOS-1352 — exposition TLS caller-owned par le registre socket

## Objectif

Ce macro-lot réduit le couplage du client LLM avec les structures TCP privées. Le registre socket expose désormais des primitives TLS caller-owned pour construire un record AES-GCM dans un buffer fourni par l’appelant, l’encapsuler dans le segment TCP du socket et déchiffrer un record entrant après validation de l’état `ESTABLISHED`.

## Contrat livré

| Primitive | Rôle | Politique mémoire |
|---|---|---|
| `net_socket_send_tls` | Construit un record TLS applicatif, prépare le segment TCP pending et engage la séquence après succès | Record et segment fournis par l’appelant |
| `net_socket_receive_tls` | Déchiffre un payload TLS reçu et publie sa vue de record | Plaintext et vue fournis par l’appelant |

La primitive d’émission réutilise le rollback cryptographique de `net_tcp_connection_build_tls_aes_gcm`. Elle ne commit le `pending_length` TCP qu’après construction réussie du record et du segment. La réception délègue à `net_tcp_connection_accept_tls_aes_gcm`, qui restaure la connexion et la séquence de lecture en cas d’échec.

> Aucune allocation dynamique, aucun buffer global supplémentaire et aucun secret de fournisseur ne sont introduits par ce lot.

## Validation

Le test `test_socket_tls_send_wrapper` couvre l’ouverture TCP, l’acceptation du SYN-ACK, l’initialisation d’une session AES-GCM caller-owned, l’émission d’un record applicatif et l’avancement de `write_sequence`. La suite noyau passe **35/35 tests** et la suite complète passe **428/428 tests**.

## Limites restantes

La résolution DNS, l’ARP, l’émission Ethernet NE2000 et le polling matériel restent des responsabilités de l’adaptateur réseau. Le raccordement complet du client LLM doit encore remplacer ses appels directs `ne2k_https_llm_*` par un orchestrateur utilisant le descripteur socket, ces primitives TLS et les buffers caller-owned.

Voir également [aos1209_tcp_socket_registry.md](aos1209_tcp_socket_registry.md), [aos1333_1344_tcp_passive_foundation.md](aos1333_1344_tcp_passive_foundation.md) et [todo.md](todo.md).

## Références

[1]: aos1209_tcp_socket_registry.md "Registre TCP caller-owned"
[2]: aos1333_1344_tcp_passive_foundation.md "Fondation TCP passive"
[3]: todo.md "Backlog MOHHDY"

---

Auteur : **Manus AI**
La date de validation correspond à l’état du dépôt du 20 août 2026.

[1] [2] [3]
