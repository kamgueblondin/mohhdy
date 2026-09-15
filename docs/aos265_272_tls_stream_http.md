# AOS-265 à AOS-272 — flux TCP/TLS authentifié et framing HTTP sécurisé

## Périmètre livré

Ce macro-lot complète l’orchestration caller-owned entre le transport TCP, le parsing TLS authentifié et une première couche HTTP/1.1 minimale. Il rend possible la consommation d’un message TLS serveur réparti entre plusieurs fragments TCP, puis la construction et l’ouverture de requêtes HTTP protégées par une session TLS AES-GCM déjà complète.

| Composant | Livraison |
|---|---|
| `net_tcp_tls_stream_t` | Contexte caller-owned associant accumulateurs de record TLS et de handshake TLS. |
| `net_tcp_connection_accept_tls_authenticated_fragment` | Consomme un fragment TCP, réassemble un record et un message handshake, puis appelle le dispatch RSA authentifié. |
| Fragments incomplets | Retournent `1` après consommation TCP et conservation des octets pour le fragment suivant. |
| Rejet authentifié | Restaure connexion TCP, handshake, transcript et longueurs des accumulateurs lorsqu’un message complet est invalide. |
| HTTP GET | Construit `GET <path> HTTP/1.1`, `Host` et `Connection: close` dans un buffer caller-owned. |
| HTTP sur TLS | Chiffre la requête dans un record applicatif AES-GCM/TCP et ouvre transactionnellement une réponse HTTP/1.1 chiffrée. |

## Tests de flux

Le test TCP réassemble un `ServerKeyExchange` RSA authentifié de 141 octets à partir de deux fragments TCP. Il vérifie que le transcript ne reçoit le message qu’une fois, que les séquences TCP progressent à chaque fragment reçu et qu’une signature altérée restaure l’état avant consommation logique.

Le test HTTP chiffre un GET vers `/v1/models`, vérifie son déchiffrement côté serveur, chiffre une réponse `HTTP/1.1 200 OK` et publie une vue bornée sur son body. Les réponses au framing HTTP invalide sont rejetées avec restauration de la connexion TCP et de la session AEAD.

> Le parser HTTP livré est volontairement minimal : il exige une réponse HTTP/1.1 complète dans un seul plaintext TLS et ne met pas encore en œuvre `Content-Length`, `Transfer-Encoding: chunked`, la compression, les en-têtes repliés, HTTP/2 ou la réassemblage d’un body sur plusieurs records.

## Contrat mémoire

Les buffers de record TLS, handshake TLS, transcript, plaintext HTTP, requête, réponse et espaces de travail RSA restent fournis et possédés par l’appelant. Aucune allocation dynamique n’est introduite dans le transport, TLS ou HTTP.

## Limites explicites

L’orchestrateur traite un message handshake par record complet ; une séquence de plusieurs messages handshake dans un même record reste à étendre. Le chemin de production n’enchaîne pas encore automatiquement ClientHello, Certificate, ServerKeyExchange, ServerHelloDone, flight client et post-flight à partir du pilote réseau. La chaîne X.509, les dates, le nom d’hôte, les certificats clients, `Content-Length`, les réponses streaming, les requêtes POST, l’authentification HTTP, les appels API LLM et un HTTPS de production complet restent non implémentés. Le backend X25519/bigint reste non constante-temps. La validation de soumission obtient **345/345 tests réussis**, un build i386 freestanding réussi et les smoke tests `qemu-ai-provider` et `qemu-ne2k-status` réussis.
