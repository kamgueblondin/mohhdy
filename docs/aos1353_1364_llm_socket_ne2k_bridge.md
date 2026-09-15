# AOS-1353 à AOS-1364 — Adaptateur LLM/socket et pont NE2000

## Objectif

Ce macro-lot commence la migration du client LLM HTTPS hors des structures TCP privées. Il sépare la construction de requête LLM, le chiffrement TLS sur socket et la transmission matérielle en trois responsabilités composables, toutes fondées sur des buffers caller-owned.

## Contrat livré

| Composant | Responsabilité | Entrées/sorties caller-owned |
|---|---|---|
| `net_llm_socket_build_request` | Produit le JSON Ollama ou OpenAI, le POST HTTP, le record TLS AES-GCM et le segment TCP du socket établi | JSON, requête, record TLS et segment TCP |
| `net_socket_send_tls` | Construit le record AES-GCM et engage la séquence TCP après succès | Session TLS, record et segment |
| `ne2k_tcp_segment` | Encapsule un segment TCP préconstruit dans Ethernet/IPv4, recalcule le checksum TCP et soumet la trame NE2000 | Segment TCP et trame Ethernet |

Le provider OpenAI exige un Bearer non vide. Le provider Ollama ne dépend pas d’un secret distant. Le bridge NE2000 vérifie la forme du segment TCP, consulte le cache ARP caller-owned, recalcule les checksums IPv4/TCP après encapsulation et respecte les bornes de trame du périphérique.

> Aucun `kmalloc`, buffer global supplémentaire ni copie de secret persistante n’est ajouté. Le commit de séquence TLS/TCP est réalisé par l’API socket avant la remise du segment au transport matériel.

## Validation

`test_net_llm_socket` valide la construction OpenAI stream, l’en-tête Bearer, la progression de la séquence AES-GCM et le rejet transactionnel d’un Bearer manquant. `test_ne2k_tcp_segment_bridge` vérifie l’ARP, l’encapsulation Ethernet/IPv4, les ports, le payload et le checksum TCP du segment préconstruit. La compilation i386 réussit ; le runner noyau passe **36/36 tests** et la suite complète passe **431/431 tests**.

## Limites restantes

Ce lot fournit le chemin de données composable, pas encore un orchestrateur unique qui pilote DNS, SYN, handshake TLS, POST, polling HTTP et SSE depuis un descripteur socket. Le polling matériel et le déchiffrement TLS exposés par `net_socket_receive_tls` restent à raccorder au parseur HTTP/SSE dans le prochain macro-lot. Le renouvellement DHCP live reste également ouvert.

## Références

[1]: aos1345_1352_socket_tls_adapter.md "Exposition TLS caller-owned par le registre socket"
[2]: aos1333_1344_tcp_passive_foundation.md "Fondation TCP d’écoute passive"
[3]: todo.md "Backlog MOHHDY"

[1] [2] [3]
